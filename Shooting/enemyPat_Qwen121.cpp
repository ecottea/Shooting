// ============================================================
// カルマン渦をモチーフにした弾幕パターン
// ファイル名: EnemyPat_KarmanVortex_Qwen.cpp (相当)
// ============================================================

// ----------------------------------------------------------------
// 弾幕パターン関数：基底流（ベースとなる下向きの流れ）
// ----------------------------------------------------------------
static void ShotBaseFlow(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 軽い効果音で流れを表現
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = DX_PI / 2.0; // 真下向き
        pShot->speed = 2.5 + (double)GetRand(50) / 50.0; // 2.5 ~ 3.0 のばらつき

        // シアン(3) の小玉
        pShot->kind = img_enemyShotSmallBall[3];

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // 通常の直進移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ----------------------------------------------------------------
// 弾幕パターン関数：カルマン渦（回転しながら流される弾の塊）
// ----------------------------------------------------------------
static void ShotKarmanVortex(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 渦の発生を重めの効果音で予告・演出
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int vortex_dir = pEnemyShotSet->param_i[0];   // 1: 右渦(反時計回り), -1: 左渦(時計回り)
        double center_x = pEnemyShotSet->param_d[0];  // 渦の中心X
        double center_y = pEnemyShotSet->param_d[1];  // 渦の中心Y (初期値)
        double v_base = 2.2;                          // 基底流による下方向の移動速度
        double omega = 0.07;                          // 回転の角速度

        int num_shots = 24; // 1つの渦を構成する弾の数
        for (int i = 0; i < num_shots; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // 半径をばらつかせて、自然な渦の濃淡を表現 (20.0 ~ 60.0)
            double r = 20.0 + (double)GetRand(400) / 10.0;
            // 円周上に均等配置
            double theta = (double)i / num_shots * 2.0 * DX_PI;

            // 弾ごとのパラメータに物理状態を保存
            pShot->param_d[0] = r;                                  // 半径
            pShot->param_d[1] = theta;                              // 現在の角度
            pShot->param_d[2] = center_x;                           // 中心X (固定)
            pShot->param_d[3] = center_y;                           // 中心Y (初期値)
            pShot->param_d[4] = v_base;                             // 流下速度
            pShot->param_d[5] = omega * (double)vortex_dir;         // 角速度 (符号で回転方向を制御)

            // 初期位置設定
            pShot->x = center_x + r * cos(theta);
            pShot->y = center_y + r * sin(theta);

            // 弾の種類と色: 左渦(-1)は青(4)、右渦(1)は赤(0) の中玉
            int color_idx = (vortex_dir == 1) ? 0 : 4;
            pShot->kind = img_enemyShotMediumBall[color_idx];

            // リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 毎フレームの更新: 極座標計算により「回転＋流下」を再現
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 角度を更新 (回転)
        pShot->param_d[1] += pShot->param_d[5];

        // 中心Yは基底流によって時間(count)とともに下がる
        double current_center_y = pShot->param_d[3] + pShot->count * pShot->param_d[4];

        // 極座標からデカルト座標へ変換して位置を更新
        pShot->x = pShot->param_d[2] + pShot->param_d[0] * cos(pShot->param_d[1]);
        pShot->y = current_center_y + pShot->param_d[0] * sin(pShot->param_d[1]);

        pShot = pShot->next;
    }
}

// ----------------------------------------------------------------
// 敵本体のパターン制御
// ----------------------------------------------------------------
void EnemyPat_KarmanVortex_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0; // 障害物として機能させるため、やや下めに配置
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // ボス本体が左右に滑らかに蛇行移動
        // これにより、渦の発生位置も変化し、よりリアルな「剥離」の動きになる
        enemy.x = 240.0 + 80.0 * sin(count * 0.015);
    }

    // --------------------------------------------------------
    // 1. 基底流の生成 (常にボスの左右から下へ流す)
    // --------------------------------------------------------
    if (count % 12 == 1) {
        // 左側の流れ
        sEnemyShotSet* pSetL = new sEnemyShotSet;
        pSetL->count = 0;
        pSetL->patternFunc = ShotBaseFlow;
        pSetL->x = enemy.x - 70.0;
        pSetL->y = enemy.y + 20.0;
        pSetL->kind = 0;
        pSetL->pEnemyShotHead = new sEnemyShot;
        pSetL->pEnemyShotHead->prev = pSetL->pEnemyShotHead;
        pSetL->pEnemyShotHead->next = pSetL->pEnemyShotHead;
        pSetL->prev = enemyShotSetHead.prev;
        pSetL->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetL;
        enemyShotSetHead.prev = pSetL;

        // 右側の流れ
        sEnemyShotSet* pSetR = new sEnemyShotSet;
        pSetR->count = 0;
        pSetR->patternFunc = ShotBaseFlow;
        pSetR->x = enemy.x + 70.0;
        pSetR->y = enemy.y + 20.0;
        pSetR->kind = 0;
        pSetR->pEnemyShotHead = new sEnemyShot;
        pSetR->pEnemyShotHead->prev = pSetR->pEnemyShotHead;
        pSetR->pEnemyShotHead->next = pSetR->pEnemyShotHead;
        pSetR->prev = enemyShotSetHead.prev;
        pSetR->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetR;
        enemyShotSetHead.prev = pSetR;
    }

    // --------------------------------------------------------
    // 2. カルマン渦の生成 (左右交互に発生させる)
    // --------------------------------------------------------

    // 左渦（時計回り）の発生タイミング
    if (count % 120 == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotKarmanVortex;
        pSet->param_i[0] = -1;                // -1: 左渦(時計回り)
        pSet->param_d[0] = enemy.x - 50.0;    // 渦の中心X
        pSet->param_d[1] = enemy.y + 50.0;    // 渦の中心Y

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // 右渦（反時計回り）の発生タイミング
    else if (count % 120 == 90) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotKarmanVortex;
        pSet->param_i[0] = 1;                 // 1: 右渦(反時計回り)
        pSet->param_d[0] = enemy.x + 50.0;    // 渦の中心X
        pSet->param_d[1] = enemy.y + 50.0;    // 渦の中心Y

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}