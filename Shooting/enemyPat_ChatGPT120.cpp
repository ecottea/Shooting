// 弾幕：仲間割れ大戦
static void ShotFight(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // pEnemyShotSet->param_i[0]
    // 0: ボス1が発射
    // 1: ボス2が発射
    const int shooter = pEnemyShotSet->param_i[0];

    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 一度に複数方向へ撃ち、互いの攻撃が交差するようにする
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;

            double targetX = shooter == 0 ? enemy.x2 : enemy.x;
            double targetY = shooter == 0 ? enemy.y2 : enemy.y;

            double baseMuki = atan2(targetY - pEnemyShotSet->y,
                targetX - pEnemyShotSet->x);

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 正面を中心に広がるように発射
            pEnemyShot->muki =
                baseMuki + (i - 3) * 0.10;

            pEnemyShot->speed = (180 + GetRand(120)) / 100.0;

            // 発射者を記録
            pEnemyShot->param_i[0] = shooter;

            // 弾種を少し混ぜる
            switch (GetRand(5)) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[shooter == 0 ? 0 : 4];
                break;

            case 1:
                pEnemyShot->kind = img_enemyShotBullet[shooter == 0 ? 0 : 4];
                break;

            case 2:
                pEnemyShot->kind = img_enemyShotScale[shooter == 0 ? 0 : 4];
                break;

            case 3:
                pEnemyShot->kind = img_enemyShotDiamond[shooter == 0 ? 0 : 4];
                break;

            default:
                pEnemyShot->kind = img_enemyShotMediumOval[shooter == 0 ? 0 : 4];
                break;
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 現在の相手の位置を取得
        double targetX = pShot->param_i[0] == 0 ? enemy.x2 : enemy.x;
        double targetY = pShot->param_i[0] == 0 ? enemy.y2 : enemy.y;

        // 相手へ向かう角度
        double targetMuki = atan2(targetY - pShot->y,
            targetX - pShot->x);

        // 角度差を -PI ～ PI に収める
        double diff = targetMuki - pShot->muki;
        while (diff > DX_PI)
            diff -= DX_PI * 2.0;
        while (diff < -DX_PI)
            diff += DX_PI * 2.0;

        // ゆっくり相手を追尾
        if (diff > 0.009)
            diff = 0.009;
        else if (diff < -0.009)
            diff = -0.009;

        pShot->muki += diff;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}


// 敵本体のパターン
void EnemyPat_FallOut_ChatGPT()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 120.0;
        enemy.y = 80.0;
        enemy.x2 = 360.0;
        enemy.y2 = 80.0;

        enemy.maxHp = enemy.hp = 200;
    }

    // ボス1
    enemy.x =
        240.0 +
        cos(count * 0.012) * 170.0 +
        cos(count * 0.021) * 50.0;

    enemy.y =
        240.0 +
        sin(count * 0.017) * 150.0;

    // ボス2は別の軌道で移動
    enemy.x2 =
        240.0 +
        cos(count * 0.015 + DX_PI) * 170.0 +
        sin(count * 0.023) * 45.0;

    enemy.y2 =
        240.0 +
        sin(count * 0.011 + DX_PI) * 150.0;

    // ボス1とボス2が交互に発射
    if (count % 8 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFight;

        if ((count / 8) % 2 == 0) {
            // ボス1
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->param_i[0] = 0;
        }
        else {
            // ボス2
            pEnemyShotSet->x = enemy.x2;
            pEnemyShotSet->y = enemy.y2 + 10.0;
            pEnemyShotSet->param_i[0] = 1;
        }

        double targetX =
            pEnemyShotSet->param_i[0] == 0 ? enemy.x2 : enemy.x;
        double targetY =
            pEnemyShotSet->param_i[0] == 0 ? enemy.y2 : enemy.y;

        pEnemyShotSet->muki =
            atan2(targetY - pEnemyShotSet->y,
                targetX - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}