// ============================================================
// レーヴァテイン 弾幕パターンの制御関数
// ============================================================
static void ShotLaevateinn(sEnemyShotSet* pEnemyShotSet)
{
    // レーザーを構成する短レーザーの数と配置間隔
    const int LASER_NUM = 20;
    const double LASER_INTERVAL = 32.0; // 64サイズのレーザーを半分重ねて綺麗に繋げる

    double current_angle = pEnemyShotSet->param_d[0];
    int attack_flag = pEnemyShotSet->param_i[0];

    // 現在リストに存在している短レーザーのインデックスを記録
    bool laser_exists[32] = { false };

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 【種別0: 短レーザー】
            int idx = pShot->param_i[1];
            if (idx >= 0 && idx < LASER_NUM) {
                laser_exists[idx] = true;
                // ボス(レーザーの根元)の位置と角度から、各部位の座標を再計算
                pShot->x = pEnemyShotSet->x + cos(current_angle) * (idx * LASER_INTERVAL);
                pShot->y = pEnemyShotSet->y + sin(current_angle) * (idx * LASER_INTERVAL);
                pShot->muki = current_angle;
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 【種別1: 赤菱形弾】
            // 初速0から、設定された終端速度(param_d[0])まで徐々に加速
            if (pShot->speed < pShot->param_d[0]) {
                pShot->speed += 0.05;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // 画面外に出て消去されるなどして不足している短レーザーがあれば補充する
    for (int i = 0; i < LASER_NUM; i++) {
        if (!laser_exists[i]) {
            sEnemyShot* pNew = new sEnemyShot;
            pNew->param_i[0] = 0; // 短レーザー
            pNew->param_i[1] = i; // 部位インデックス
            pNew->kind = img_enemyShotLaser[0]; // 赤色短レーザー

            pNew->x = pEnemyShotSet->x + cos(current_angle) * (i * LASER_INTERVAL);
            pNew->y = pEnemyShotSet->y + sin(current_angle) * (i * LASER_INTERVAL);
            pNew->muki = current_angle;

            // リストの末尾（headの直前）に追加
            pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNew->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
            pEnemyShotSet->pEnemyShotHead->prev = pNew;
        }
    }

    // 攻撃中、5フレームに1回のペースで赤菱形弾を射出
    if (attack_flag == 1 && pEnemyShotSet->count % 5 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double prev_x = pEnemyShotSet->param_d[1];
        double prev_y = pEnemyShotSet->param_d[2];
        double prev_angle = pEnemyShotSet->param_d[3];

        // 剣の向きに対して垂直な2つのベクトル
        double nx1 = -sin(current_angle);
        double ny1 = cos(current_angle);
        double nx2 = sin(current_angle);
        double ny2 = -cos(current_angle);

        // レーザーを等間隔に内分する点から射出（根元は近すぎるので i=1 から）
        for (int i = 1; i < LASER_NUM; i += 2) {
            double r = i * LASER_INTERVAL;

            // 現在の点の座標
            double cx = pEnemyShotSet->x + cos(current_angle) * r;
            double cy = pEnemyShotSet->y + sin(current_angle) * r;
            // 1フレーム前の点の座標
            double px = prev_x + cos(prev_angle) * r;
            double py = prev_y + sin(prev_angle) * r;

            // 剣のその部位の移動ベクトル
            double vx = cx - px;
            double vy = cy - py;

            // ほとんど動いていない場合は撃たない
            if (vx * vx + vy * vy < 0.1) continue;

            // 移動方向との内積を計算
            double dot1 = vx * nx1 + vy * ny1;
            double dot2 = vx * nx2 + vy * ny2;

            // 内積が正（進行方向）になる垂直ベクトルを選ぶ
            double shot_muki = current_angle + DX_PI / 2.0;
            if (dot2 > dot1) {
                shot_muki = current_angle - DX_PI / 2.0;
            }

            // 少しだけ射出角度にランダムなばらつきを持たせる
            shot_muki += (GetRand(20) - 10) / 180.0 * (DX_PI / 2.0);

            sEnemyShot* pNew = new sEnemyShot;
            pNew->x = cx;
            pNew->y = cy;
            pNew->muki = shot_muki;
            pNew->speed = 0.0; // 初速度は0
            pNew->param_d[0] = 2.0 + GetRand(150) / 100.0; // 終端速度(2.0〜3.5)を記憶
            pNew->param_i[0] = 1; // 赤菱形弾
            pNew->kind = img_enemyShotDiamond[0];

            pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNew->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
            pEnemyShotSet->pEnemyShotHead->prev = pNew;
        }
    }

    // 次フレームの移動ベクトル計算用に現在の状態を保存
    pEnemyShotSet->param_d[1] = pEnemyShotSet->x;
    pEnemyShotSet->param_d[2] = pEnemyShotSet->y;
    pEnemyShotSet->param_d[3] = current_angle;
}


// ============================================================
// 敵本体の挙動 (禁忌「レーヴァテイン」)
// ============================================================
void EnemyPat_Levatain_Gemini()
{
    static int phase = 0;
    static int phase_count = 0;
    static double start_x, start_y;
    static double target_x, target_y;
    static double start_angle, target_angle;
    static int attack_flag = 0;

    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;

        phase = 0;
        phase_count = 0;

        // レーザー全体と菱形弾を管理するSetを1つだけ生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotLaevateinn;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->param_d[0] = -DX_PI / 4.0; // 現在の剣の角度(最初は右上)
        pSet->param_i[0] = 0;            // 攻撃フラグ(0:待機, 1:攻撃中)

        pSet->param_d[1] = enemy.x;
        pSet->param_d[2] = enemy.y;
        pSet->param_d[3] = pSet->param_d[0];

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 管理用Setを取得
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    if (pSet == &enemyShotSetHead) return; // Setが消去されていた場合の安全対策

    int t = phase_count;

    // 周期パターンの制御
    switch (phase) {
    case 0: // 【パターン1準備】予告音を鳴らして剣を振りかぶる
        if (t == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            start_x = enemy.x; start_y = enemy.y;
            target_x = 120.0; target_y = 80.0;
            start_angle = pSet->param_d[0];
            target_angle = -DX_PI / 4.0; // 右上へ
            attack_flag = 0;
        }
        if (t <= 60) { // 60フレームかけて移動
            double r = (double)t / 60.0;
            enemy.x = start_x + (target_x - start_x) * r;
            enemy.y = start_y + (target_y - start_y) * r;
            pSet->param_d[0] = start_angle + (target_angle - start_angle) * r;
        }
        if (t >= 80) { phase = 1; phase_count = -1; }
        break;

    case 1: // 【パターン1攻撃】遅めの平行移動 ＋ 速めの回転移動の合成（振り下ろし）
        if (t == 0) {
            start_x = enemy.x; start_y = enemy.y;
            target_x = 360.0; target_y = 80.0; // 遅めの移動
            start_angle = pSet->param_d[0];
            target_angle = start_angle + DX_PI * 1.5; // 270度大きく回転
            attack_flag = 1;
        }
        if (t <= 120) {
            double r = (double)t / 120.0;
            enemy.x = start_x + (target_x - start_x) * r;
            enemy.y = start_y + (target_y - start_y) * r;
            pSet->param_d[0] = start_angle + (target_angle - start_angle) * r;
        }
        if (t >= 160) { phase = 2; phase_count = -1; }
        break;

    case 2: // 【パターン2準備】剣を斜めに構え直す
        if (t == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            start_x = enemy.x; start_y = enemy.y;
            target_x = 360.0; target_y = 80.0;
            start_angle = pSet->param_d[0];
            target_angle = DX_PI * 0.75; // 左上〜左下の中間あたり
            attack_flag = 0;
        }
        if (t <= 60) {
            double r = (double)t / 60.0;
            enemy.x = start_x + (target_x - start_x) * r;
            enemy.y = start_y + (target_y - start_y) * r;
            pSet->param_d[0] = start_angle + (target_angle - start_angle) * r;
        }
        if (t >= 80) { phase = 3; phase_count = -1; }
        break;

    case 3: // 【パターン2攻撃】速めの平行移動（薙ぎ払い）
        if (t == 0) {
            start_x = enemy.x; start_y = enemy.y;
            target_x = 120.0; target_y = 200.0; // 斜め左下へ素早く移動
            start_angle = pSet->param_d[0];
            target_angle = start_angle; // 角度は維持（回転しない）
            attack_flag = 1;
        }
        if (t <= 60) { // 60フレームで速く移動
            double r = (double)t / 60.0;
            enemy.x = start_x + (target_x - start_x) * r;
            enemy.y = start_y + (target_y - start_y) * r;
            pSet->param_d[0] = start_angle + (target_angle - start_angle) * r;
        }
        if (t >= 100) { phase = 0; phase_count = -1; } // 1周期終了、最初に戻る
        break;
    }

    // Setへボス(剣の根元)の状態を毎フレーム反映させる
    pSet->param_i[0] = attack_flag;
    pSet->x = enemy.x;
    pSet->y = enemy.y;

    phase_count++;
}