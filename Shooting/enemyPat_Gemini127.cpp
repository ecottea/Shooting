// ----------------------------------------------------------------
// 弾幕処理関数：ガブリエルのラッパ（立体回転体）
// ----------------------------------------------------------------
static void ShotGabrielTrumpet(sEnemyShotSet* pEnemyShotSet)
{
    // 一定周期で環境効果音を再生
    if (pEnemyShotSet->count % 60 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // ============================================================
    // 1. ラッパの断面となる「リング（楕円の輪）」の定期生成
    // ============================================================
    // 8フレームごとに、ボスの位置（ラッパの奥の管）から1つずつリングを射出
    if (pEnemyShotSet->count % 30 == 0) {
        const int RING_BULLETS = 16; // 輪を構成する弾の数
        double base_rot = (pEnemyShotSet->count * 0.04); // 時間とともにラッパ全体が自転

        for (int i = 0; i < RING_BULLETS; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double angle = base_rot + (2.0 * DX_PI / RING_BULLETS) * i;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0; // 位置はパラメータによる二次曲線展開のため直接更新

            // 弾の識別パラメータ
            pEnemyShot->param_i[0] = 0;     // 0: ラッパの壁を構成するリング弾
            pEnemyShot->param_d[0] = angle; // 初期角度

            // 生成初期（奥にある状態）は「青の小玉」
            pEnemyShot->kind = img_enemyShotSmallBall[4];

            // 双方向リストへ挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ============================================================
    // 2. ラッパの中心を通る「赤色銃弾（直線貫通弾）」の生成
    // ============================================================
    // 14フレームごとに、ラッパの細い奥からプレイヤーへ向けて高速弾を吹き出させる
    if (pEnemyShotSet->count % 60 == 30) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        // 自機狙い（わずかに精度をブレさせて弾幕感を追加）
        double aim_angle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        aim_angle += (GetRand(16) - 8) * (DX_PI / 180.0); // ±8度のランダム偏差

        pEnemyShot->muki = aim_angle;
        pEnemyShot->speed = 4.2 + (GetRand(100) / 100.0); // 高速
        pEnemyShot->kind = img_enemyShotBullet[0];        // 赤色の銃弾

        pEnemyShot->param_i[0] = 1; // 1: 直線進行弾

        // 双方向リストへ挿入
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // ============================================================
    // 3. 画面上の全弾の位置更新および立体表現（変形・移動）
    // ============================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (pShot->param_i[0] == 0) {
            // 【リング構成弾の処理】
            // メインルーチンで自動インクリメントされる pShot->count を経過時間 t として利用
            double t = pShot->count / 2.0;

            // ガブリエルのラッパの曲面関数 $R(t) = R_0 + A \cdot (t / T)^{2.2}$
            // 時間経過（＝手前に進む）とともに急激に半径が広がる
            double norm_t = t / 90.0;
            if (norm_t > 1.4) norm_t = 1.4;

            double radius = 10.0 + 185.0 * pow(norm_t, 2.2);

            // 立体遠近感（3D見下ろし効果）の計算
            double y_shift = t * 2.2; // 手前に押し出されてくるY軸オフセット
            double aspect_y = 0.45;   // 縦方向に押し潰して楕円化（斜め俯瞰）

            // 時間経過による回転（らせんを描くワイヤーフレーム感）
            double current_angle = pShot->param_d[0] + t * 0.025;

            // 座標更新（ボス位置を原点とした相対立体座標）
            pShot->x = pEnemyShotSet->x + radius * cos(current_angle);
            pShot->y = pEnemyShotSet->y + y_shift + (radius * aspect_y) * sin(current_angle);

            // --------------------------------------------------------
            // 遠近感に応じた見た目（サイズ・色）の段階的変化
            // 奥（小・青） -> 中間（中・シアン） -> 手前（大・白）
            // --------------------------------------------------------
            if (t < 25) {
                pShot->kind = img_enemyShotSmallBall[4];  // 小玉（青）
            }
            else if (t < 50) {
                pShot->kind = img_enemyShotMediumBall[3]; // 中玉（シアン）
            }
            else {
                pShot->kind = img_enemyShotLargeBall[6];  // 大玉（白）
            }

        }
        else if (pShot->param_i[0] == 1) {
            // 【直線貫通弾の処理】
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ----------------------------------------------------------------
// 敵本体のパターン関数（エントリーポイント）
// ----------------------------------------------------------------
void EnemyPat_GabrielsHorn_Gemini()
{
    static int move_dir;

    // 初期化処理（1フレーム目）
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
        move_dir = 1;

        // 弾幕管理用セットの生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGabrielTrumpet;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 弾双方向リストのダミーヘッド生成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバル弾セットリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵本体のゆるやかな左右移動
        enemy.x += 0.5 * (double)move_dir;
        if (count % 160 == 80) {
            move_dir *= -1;
        }

        // 敵の移動に合わせて弾幕生成源の位置も追従
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotGabrielTrumpet) {
                pSet->x = enemy.x;
                pSet->y = enemy.y;
            }
            pSet = pSet->next;
        }
    }
}