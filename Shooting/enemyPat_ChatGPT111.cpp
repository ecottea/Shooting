// enemyPat_Tmp.cpp
// 弾幕：ケントの花と落ちるリンゴ

// 花弁を形作る弾群
static void ShotKentFlower(sEnemyShotSet* pEnemyShotSet)
{
    constexpr int PETAL_COUNT = 18;
    constexpr int PETAL_POINT_COUNT = 8*2;
    constexpr double PI2 = DX_PI * 2.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int petal = 0; petal < PETAL_COUNT; ++petal) {
            for (int point = 0; point < PETAL_POINT_COUNT; ++point) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                double u = (double)point / (double)(PETAL_POINT_COUNT - 1);
                double baseAngle = PI2 * (double)petal / (double)PETAL_COUNT;
                double centerAngle = baseAngle + 0.10 * sin(u * DX_PI);
                double radius = 18.0 + 118.0 * u*2;
                double bend = 25.0 * sin(u * DX_PI);

                pEnemyShot->x = pEnemyShotSet->x + cos(centerAngle) * radius - sin(centerAngle) * bend;
                pEnemyShot->y = pEnemyShotSet->y + sin(centerAngle) * radius + cos(centerAngle) * bend;
                pEnemyShot->muki = centerAngle + DX_PI * 0.5;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = (point % 3 == 0) ? img_enemyShotMediumOval[2] : img_enemyShotScale[1];
                pEnemyShot->margin = 999.0;

                pEnemyShot->param_i[0] = petal;
                pEnemyShot->param_i[1] = point;
                pEnemyShot->param_d[0] = baseAngle;
                pEnemyShot->param_d[1] = (double)point / (double)(PETAL_POINT_COUNT - 1);
                pEnemyShot->param_d[2] = 1.0 + 0.06 * (double)(petal % 2);

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    double t = (double)pEnemyShotSet->count;
    double rotation = t * 0.010;
    double bloom = 0.90 + 0.10 * sin(t * 0.018);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double u = pShot->param_d[1];
        double angle = pShot->param_d[0] + rotation * pShot->param_d[2] + 0.12 * sin(u * DX_PI + t * 0.020);
        double radius = (18.0 + 118.0 * u*2) * bloom;
        double bend = (24.0 + 8.0 * sin(t * 0.014 + pShot->param_i[0] * 0.4)) * sin(u * DX_PI);

        pShot->x = pEnemyShotSet->x + cos(angle) * radius - sin(angle) * bend;
        pShot->y = pEnemyShotSet->y + sin(angle) * radius + cos(angle) * bend;
        pShot->muki = angle + DX_PI * 0.5;

        pShot = pShot->next;
    }
}

// 落下するリンゴと、その着弾時の花への衝撃波
static void ShotFallingApple(sEnemyShotSet* pEnemyShotSet)
{
    constexpr int BURST_COUNT = 24;
    constexpr double PI2 = DX_PI * 2.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = -32.0;
        pEnemyShot->muki = DX_PI * 0.5;
        pEnemyShot->speed = 2.25;
        pEnemyShot->kind = img_enemyShotLargeBall[0];
        pEnemyShot->margin = 999.0;
        pEnemyShot->param_i[0] = 0; // 0:落下、1:爆発後に退場

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 0) {
            double t = (double)pShot->count;
            pShot->x = pEnemyShotSet->x + 24.0 * sin(t * 0.040 + pEnemyShotSet->param_d[0]);
            pShot->y = -32.0 + t * 2.25;
            pShot->muki = DX_PI * 0.5;

            // 花の中心へ落ちた瞬間、リンゴが花弁を散らすように放射状の弾を発生。
            if (pShot->count == 96) {
                if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                for (int i = 0; i < BURST_COUNT; ++i) {
                    sEnemyShot* pBurst = new sEnemyShot;
                    double angle = PI2 * (double)i / (double)BURST_COUNT;

                    pBurst->x = pShot->x;
                    pBurst->y = pShot->y;
                    pBurst->muki = angle;
                    pBurst->speed = 2.2 + 0.15 * (double)(i % 3);
                    pBurst->kind = (i % 2 == 0) ? img_enemyShotSmallBall[0] : img_enemyShotScale[0];
                    pBurst->param_i[0] = 2;

                    pBurst->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pBurst->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pBurst;
                    pEnemyShotSet->pEnemyShotHead->prev = pBurst;
                }

                // 爆発後はリンゴ本体を下へ流して退場させる。
                pShot->param_i[0] = 1;
                pShot->muki = DX_PI * 0.5;
                pShot->speed = 4.5;
            }
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_FlowerOfKent_ChatGPT()
{
    static int muki;
    static int apple_cycle;
    static double flower_x;
    static double flower_y;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        apple_cycle = 0;
        flower_x = 240.0;
        flower_y = 215.0+80;
    }
    else {
        enemy.x += 0.55 * (double)muki;
        if (enemy.x < 95.0 || enemy.x > 385.0) {
            muki *= -1;
        }
    }

    // 花の中心は敵の緩やかな動きと連動させる。
    flower_x = 240.0 + 38.0 * sin((double)count * 0.008);
    flower_y = 215.0+80 + 24.0 * sin((double)count * 0.013);

    // 花弁は長く残して画面中央の主役にする。
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKentFlower;
        pEnemyShotSet->x = flower_x;
        pEnemyShotSet->y = flower_y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->param_d[0] = 0.0;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // リンゴは周期的に上から落下。左右の出現位置をずらして花との重なり方を変える。
    if (count >= 80 && count % 110 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFallingApple;
        pEnemyShotSet->x = 105.0 + (double)((apple_cycle * 79) % 270);
        pEnemyShotSet->y = -20.0;
        pEnemyShotSet->muki = DX_PI * 0.5;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_d[0] = (double)(apple_cycle % 5) * 0.7;
        ++apple_cycle;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 花の中心位置を既存セットへ反映。
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        sEnemyShotSet* pNext = pSet->next;
        if (pSet->patternFunc == ShotKentFlower) {
            pSet->x = flower_x;
            pSet->y = flower_y;
        }
        pSet = pNext;
    }
}