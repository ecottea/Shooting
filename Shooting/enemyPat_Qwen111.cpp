// enemyPat_Tmp.cpp
// ケントの花とリンゴの収穫 パターン

static void ShotKentApple(sEnemyShotSet* pEnemyShotSet)
{
    int c = pEnemyShotSet->count;

    // フェーズ1: 茎の成長 (count 0~59)
    if (c < 60) {
        if (c % 4 == 0) {
            sEnemyShot* pShot = new sEnemyShot;
            // パターン開始位置から上に伸びる
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y + 10.0 - (c / 4) * 6.0;
            pShot->muki = -DX_PI / 2.0; // 上向き
            pShot->speed = 1.0;
            pShot->kind = img_enemyShotMediumBall[2]; // 緑の中玉

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
    // フェーズ2: 花の開花 (count == 60)
    else if (c == 60) {
        // 効果音: 花が開く軽い音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double flowerX = pEnemyShotSet->x;
        double flowerY = pEnemyShotSet->y + 10.0 - (60 / 4) * 6.0; // 茎の頂点付近

        // 花の中心 (黄の大玉)
        sEnemyShot* pCenter = new sEnemyShot;
        pCenter->x = flowerX;
        pCenter->y = flowerY;
        pCenter->muki = DX_PI / 2.0; // 下向き
        pCenter->speed = 0.5;
        pCenter->kind = img_enemyShotLargeBall[1]; // 黄の大玉
        pCenter->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pCenter->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pCenter;
        pEnemyShotSet->pEnemyShotHead->prev = pCenter;

        // 花びら (白の小玉) を鐘形に発射
        for (int i = 0; i < 7; i++) {
            sEnemyShot* pPetal = new sEnemyShot;
            pPetal->x = flowerX;
            pPetal->y = flowerY;
            // 下向き(DX_PI/2)を中心に左右に広がる
            pPetal->muki = DX_PI / 2.0 + (i - 3) * 0.25;
            pPetal->speed = 1.2;
            pPetal->kind = img_enemyShotSmallBall[6]; // 白の小玉
            pPetal->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pPetal->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pPetal;
            pEnemyShotSet->pEnemyShotHead->prev = pPetal;
        }
    }
    // フェーズ3: リンゴの成熟 (count 61~120)
    else if (c > 60 && c <= 120) {
        if (c == 61) {
            double flowerY = pEnemyShotSet->y + 10.0 - (60 / 4) * 6.0;
            // リンゴ (赤の大玉) を花の直下に配置
            sEnemyShot* pApple = new sEnemyShot;
            pApple->x = pEnemyShotSet->x;
            pApple->y = flowerY + 20.0;
            pApple->muki = 0;
            pApple->speed = 0;
            pApple->kind = img_enemyShotLargeBall[0]; // 赤の大玉
            pApple->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pApple->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pApple;
            pEnemyShotSet->pEnemyShotHead->prev = pApple;
        }

        // 既存のリンゴ弾を探して振動させる
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->kind == img_enemyShotLargeBall[0]) {
                // 振動表現: x座標をsin波で揺らす。振幅は時間とともに増加
                double amplitude = (c - 60) * 0.05;
                pShot->x = pEnemyShotSet->x + sin((c - 60) * 0.3) * amplitude;
            }
            pShot = pShot->next;
        }
    }
    // フェーズ4: リンゴの落下と散華 (count > 120)
    else {
        if (c == 121) {
            // 効果音: リンゴが落ちる重たい音
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->kind == img_enemyShotLargeBall[0]) {
                // リンゴの落下 (重力加速)
                pShot->speed += 0.2;
                pShot->y += pShot->speed;

                // 落下時に枝の破片 (極小粒子) を散らす
                if (c % 2 == 0 && pShot->y < 460) {
                    sEnemyShot* pDebris = new sEnemyShot;
                    pDebris->x = pShot->x;
                    pDebris->y = pShot->y;
                    // GetRand(360) は 0~360 の 361 種類を返す
                    pDebris->muki = GetRand(360) * DX_PI / 180.0;
                    pDebris->speed = 1.0 + GetRand(100) / 100.0;
                    pDebris->kind = img_enemyShotSmallBall[8]; // 橙の小玉 (茶色系的な表現)
                    pDebris->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pDebris->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pDebris;
                    pEnemyShotSet->pEnemyShotHead->prev = pDebris;
                }
            }
            else if (pShot->kind == img_enemyShotSmallBall[6]) {
                // 花びらの揺れ (フェーズ5相当)
                pShot->x += cos(pShot->muki) * pShot->speed;
                pShot->y += sin(pShot->muki) * pShot->speed;
                // 正弦波で揺らす (リンゴ落下の反動を表現)
                pShot->x += sin(c * 0.15) * 1.2;
            }
            else if (pShot->kind == img_enemyShotLargeBall[1]) {
                // 花の中心はゆっくり回転しながら下降
                pShot->y += pShot->speed;
                pShot->x += sin(c * 0.1) * 0.5;
            }
            else {
                // その他の弾 (茎など) は通常移動
                pShot->x += cos(pShot->muki) * pShot->speed;
                pShot->y += sin(pShot->muki) * pShot->speed;
            }
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_FlowerOfKent_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        // 茎が伸びるスペースを確保するため、y座標は少し上めに設定
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // 敵はゆっくりと左右に移動
        enemy.x += 0.5 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    // count 60 でパターン開始 (敵が少し移動して安定したタイミング)
    if (count % 90 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKentApple;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}