// 弾幕処理：炭酸怒濤：メントス・フォンテナー
static void ShotMentosCola(sEnemyShotSet* pEnemyShotSet)
{
    // ----------------------------------------------------
    // 1. 新規弾の生成処理
    // ----------------------------------------------------

    // Phase 1: メントス投下（予兆：count 1 ~ 50）
    if (pEnemyShotSet->count == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count >= 1 && pEnemyShotSet->count <= 50) {
        // 10フレームごとに敵頭上から白の中玉（メントス）を落とす
        if (pEnemyShotSet->count % 10 == 0) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
            pEnemyShot->y = pEnemyShotSet->y - 120.0; // 敵の少し上から出現
            pEnemyShot->muki = DX_PI / 2.0;            // 下向き
            pEnemyShot->speed = 3.0 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白・中玉
            pEnemyShot->param_i[0] = 3; // 通常弾

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // Phase 2 & 3: コーラ大噴射・泡・重力飛沫（count 60 ~ 240）
    if (pEnemyShotSet->count == 60) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count >= 60 && pEnemyShotSet->count <= 240) {

        // --- A. コーラ柱（真上へ向かう大玉・中楕円弾の高速連射）---
        for (int i = 0; i < 2 * 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x + GetRand(24) - 12;
            pEnemyShot->y = pEnemyShotSet->y;
            // 真上（-90度）を中心にわずかなブレを持たせる
            pEnemyShot->muki = -DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 7.5 + GetRand(250) / 100.0;

            // 橙(8)と赤(0)の大玉・楕円弾を混ぜてコーラ液を表現
            if (GetRand(1) == 0) {
                pEnemyShot->kind = img_enemyShotLargeBall[(GetRand(1) == 0) ? 8 : 0];
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumOval[8];
            }
            pEnemyShot->param_i[0] = 2; // 通常弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- B. 炭酸泡（白・シアンの小玉が広角へ拡散）---
        if (pEnemyShotSet->count % 1 == 0) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x + GetRand(30) - 15;
            pEnemyShot->y = pEnemyShotSet->y;
            // 上方向を中心に左右へ広く展開 (-160度 ~ -20度)
            pEnemyShot->muki = -DX_PI / 2.0 + (GetRand(280) - 140) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.0 + GetRand(250) / 100.0;
            // 白(6) または シアン(3) の小玉
            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[6] : img_enemyShotSmallBall[3];
            pEnemyShot->param_i[0] = 0; // 通常弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- C. 弾け飛ぶ飛沫（重力で放物線を描いて落下する銃弾）---
        if (pEnemyShotSet->count % 1 == 0) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
            pEnemyShot->y = pEnemyShotSet->y;

            // 斜め上方向に打ち出す
            double angle = -DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI;
            double sp = 4.0 + GetRand(300) / 100.0;

            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotBullet[8] : img_enemyShotBullet[0]; // 橙(8) / 赤(0)
            pEnemyShot->param_i[0] = 1; // 1: 重力影響フラグ
            pEnemyShot->param_d[0] = sp * cos(angle); // X方向速度 vx
            pEnemyShot->param_d[1] = sp * sin(angle); // Y方向速度 vy (負の値で上向き)
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----------------------------------------------------
    // 2. 既存弾の移動処理
    // ----------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) { // 飛沫弾（重力落下計算）
            pShot->param_d[1] += 0.08; // Y方向速度に重力加速度を加算
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            // 進行方向に合わせて画像の向きを更新
            pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
        }
        else { // 通常弾（等速直線移動）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->param_i[0] == 3 && pEnemyShotSet->count >= 60) pShot->margin = -9999;
        }

        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_MentosCola_Gemini()
{
    static int muki;

    if (count == 1) {
        // メントスコーラが上に大きく噴き上がるため、敵の位置を画面中央やや下寄りに配置
        enemy.x = 240.0;
        enemy.y = 220.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆるやかに左右へゆれて噴流の軸をずらす
        enemy.x += 0.6 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 280フレーム周期（約4.6秒）ごとにメントスコーラ弾幕を発動
    if (count % 280 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMentosCola;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // 双方向リストのダミーヘッド初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体リストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}