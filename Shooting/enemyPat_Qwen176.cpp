// ============================================================================
// 弾幕：四段咲き・線香花火 (Senko Hanabi)
// ============================================================================
static void ShotSenkoHanabi(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int phaseCount = pEnemyShotSet->count;

    // ------------------------------------------------------------------------
    // 0. フェーズ切り替わり時の効果音再生処理
    // ------------------------------------------------------------------------
    if (phaseCount == 0) {
        // 【点火】
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    else if (phaseCount == 120) {
        // 【蕊：安定】
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    else if (phaseCount == 240) {
        // 【松葉：拡散】
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    else if (phaseCount == 360) {
        // 【散り菊：落下開始】
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // ------------------------------------------------------------------------
    // 1. 新規弾の生成処理 (フェーズごとの分岐)
    // ------------------------------------------------------------------------

    // 【フェーズ①：玉】 (0～119フレーム)
    if (phaseCount < 120) {
        if (phaseCount % 3 == 0) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(40) - 20);
            pEnemyShot->muki = GetRand(359) / 359.0 * DX_PI * 2.0;
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0;

            int color = (GetRand(1) == 0) ? 8 : 1; // 8:橙, 1:黄
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->param_d[0] = 0.85; // 急減速
            pEnemyShot->param_d[1] = 0.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 【フェーズ②：蕊】 (120～239フレーム)
    else if (phaseCount < 240) {
        if (phaseCount % 8 == 0) {
            int numBullets = 8;
            double baseAngle = (phaseCount / 8) * 0.15;

            for (int i = 0; i < numBullets; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAngle + (i * (DX_PI * 2.0 / numBullets));
                pEnemyShot->speed = 1.2;
                pEnemyShot->kind = img_enemyShotMediumBall[0]; // 0:赤

                pEnemyShot->param_d[0] = 0.98; // 微減速
                pEnemyShot->param_d[1] = 0.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    // 【フェーズ③：松葉】 (240～359フレーム)
    else if (phaseCount == 240) {
        int numBullets = 36;
        for (int i = 0; i < numBullets; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * (DX_PI * 2.0 / numBullets);
            pEnemyShot->speed = 4.5;
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // 3:シアン

            pEnemyShot->param_d[0] = 0.88; // 強く減速
            pEnemyShot->param_d[1] = 0.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 【フェーズ④：散り菊】 (360～479フレーム)
    else if (phaseCount < 480) {
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_d[1] == 0.0) {
                pEnemyShot->param_d[0] = 0.95; // 減速を緩やかに
                pEnemyShot->param_d[1] = 1.8;  // Y軸方向の重力追加量
            }
            pEnemyShot = pEnemyShot->next;
        }

        // 最後の散り際演出 (460フレーム目)
        if (phaseCount == 460) {
            for (int i = 0; i < 12; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + (GetRand(60) - 30);
                pEnemyShot->y = pEnemyShotSet->y + (GetRand(60) - 30);
                pEnemyShot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 100.0;
                pEnemyShot->speed = 1.0;
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 6:白

                pEnemyShot->param_d[0] = 0.90;
                pEnemyShot->param_d[1] = 2.5; // 強い重力

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ------------------------------------------------------------------------
    // 2. 既存弾の更新処理 (カスタム物理演算の適用)
    // ------------------------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_d[0] > 0.0) {
            pShot->speed *= pShot->param_d[0];
            if (pShot->speed < 0.1) pShot->speed = 0.1;
        }

        if (pShot->param_d[1] > 0.0) {
            pShot->y += pShot->param_d[1];
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================================
// 敵本体のパターン：線香花火 (Tmp)
// ============================================================================
void EnemyPat_Sparkler_Qwen()
{
    static int next_fall = 1;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0; // やや上めで花火を咲かせる
        enemy.maxHp = enemy.hp = 200; // 200で固定
        next_fall = 1;
    }
    else {
        // 敵本体は中央付近でゆっくり左右に揺れる
        enemy.x = 240.0 + sin(count / 90.0) * 120.0;
        enemy.y = 80.0 + cos(count / 120.0) * 20.0;
    }

    // 弾幕セットの作成は最初の1回のみ
    // あとは ShotSenkoHanabi 内部で pEnemyShotSet->count を見てフェーズ制御する
    if (count == next_fall) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSenkoHanabi;
        pEnemyShotSet->x = enemy.x + GetRand(300) - 150;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->alive = 600; // 480フレーム＋余裕を持たせる

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        next_fall = count + max(enemy.hp * 2 / 3, 50);
    }
}