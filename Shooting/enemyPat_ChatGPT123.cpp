// enemyPat_airHockey.cpp

// エアホッケー弾幕
// ・白い中楕円弾を「パック」として扱い、左右と上端で反射
// ・画面下部ではプレイヤー位置を「パドル」とみなし、位置に応じて反射角が変化
// ・パック同士が衝突すると反射し、その場から小玉を放出
// ・壁反射やパドル反射でも小玉を散らし、徐々にフィールドを埋める

static void AirHockey_AddSpark(
    sEnemyShotSet* pEnemyShotSet,
    double x,
    double y,
    double muki,
    int color,
    int count,
    double speed)
{
    for (int i = 0; i < count; ++i) {
        sEnemyShot* pShot = new sEnemyShot;

        double spread = (i - (count - 1) * 0.5) * 0.10;
        pShot->x = x;
        pShot->y = y;
        pShot->muki = muki + spread;
        pShot->speed = speed + (GetRand(20) / 100.0);
        pShot->kind = img_enemyShotSmallBall[color];
        pShot->margin = 20.0;
        pShot->param_i[0] = 1; // パック以外

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }
}

static void AirHockey_AddPuck(
    sEnemyShotSet* pEnemyShotSet,
    double x,
    double y,
    double muki,
    double speed,
    int color)
{
    sEnemyShot* pShot = new sEnemyShot;

    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = img_enemyShotMediumOval[color];
    pShot->margin = 999.0;
    pShot->param_i[0] = 0; // パック
    pShot->param_i[1] = -1000; // 最後に衝突した count

    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
    pEnemyShotSet->pEnemyShotHead->prev = pShot;
}

static void AirHockey_Pattern(sEnemyShotSet* pEnemyShotSet)
{
    // 新規パックを段階的に追加
    if (pEnemyShotSet->count == 1) {
        AirHockey_AddPuck(
            pEnemyShotSet,
            pEnemyShotSet->x,
            pEnemyShotSet->y + 12.0,
            atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x),
            5.2,
            6);

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 1; // 生成済みパック数
    }
    else if (pEnemyShotSet->param_i[0] < 5 && pEnemyShotSet->count % 150 == 1) {
        double side = (pEnemyShotSet->param_i[0] % 2 == 0) ? -1.0 : 1.0;
        double spawnX = enemy.x + side * 42.0;
        double targetX = player.x + side * 80.0;

        AirHockey_AddPuck(
            pEnemyShotSet,
            spawnX,
            enemy.y + 18.0,
            atan2(player.y - enemy.y, targetX - spawnX),
            5.0 + pEnemyShotSet->param_i[0] * 0.15,
            (pEnemyShotSet->param_i[0] % 2 == 0) ? 6 : 3);

        ++pEnemyShotSet->param_i[0];

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // パックを更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 0) {
            double vx = pShot->speed * cos(pShot->muki);
            double vy = pShot->speed * sin(pShot->muki);

            pShot->x += vx;
            pShot->y += vy;

            // 左右の壁で反射
            if (pShot->x < 24.0 && vx < 0.0) {
                pShot->x = 24.0;
                pShot->muki = DX_PI - pShot->muki;

                AirHockey_AddSpark(
                    pEnemyShotSet,
                    pShot->x,
                    pShot->y,
                    pShot->muki,
                    3,
                    7,
                    2.7);

                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
            else if (pShot->x > 456.0 && vx > 0.0) {
                pShot->x = 456.0;
                pShot->muki = DX_PI - pShot->muki;

                AirHockey_AddSpark(
                    pEnemyShotSet,
                    pShot->x,
                    pShot->y,
                    pShot->muki,
                    3,
                    7,
                    2.7);

                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            // 上端でも反射
            if (pShot->y < 54.0 && vy < 0.0) {
                pShot->y = 54.0;
                pShot->muki = -pShot->muki;

                AirHockey_AddSpark(
                    pEnemyShotSet,
                    pShot->x,
                    pShot->y,
                    pShot->muki,
                    6,
                    5,
                    2.5);
            }

            // 画面下部を「プレイヤーのパドル」として返球
            if (pShot->y > 420.0 && vy > 0.0) {
                double dx = player.x - pShot->x;
                double paddle = dx / 120.0;

                if (paddle > 1.0) paddle = 1.0;
                if (paddle < -1.0) paddle = -1.0;

                pShot->y = 420.0;
                pShot->muki = -DX_PI / 2.0 + paddle * 0.85;

                AirHockey_AddSpark(
                    pEnemyShotSet,
                    pShot->x,
                    pShot->y,
                    pShot->muki,
                    5,
                    9,
                    3.0);

                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
        }

        pShot = pNext;
    }

    // パック同士の衝突
    for (sEnemyShot* p1 = pEnemyShotSet->pEnemyShotHead->next;
        p1 != pEnemyShotSet->pEnemyShotHead;
        p1 = p1->next) {

        if (p1->param_i[0] != 0) continue;

        for (sEnemyShot* p2 = p1->next;
            p2 != pEnemyShotSet->pEnemyShotHead;
            p2 = p2->next) {

            if (p2->param_i[0] != 0) continue;

            if (p1->count - p1->param_i[1] < 8 ||
                p2->count - p2->param_i[1] < 8) {
                continue;
            }

            double dx = p2->x - p1->x;
            double dy = p2->y - p1->y;
            double dist2 = dx * dx + dy * dy;

            if (dist2 <= 0.0 || dist2 > 24.0 * 24.0) continue;

            double dist = sqrt(dist2);
            double nx = dx / dist;
            double ny = dy / dist;

            double v1x = p1->speed * cos(p1->muki);
            double v1y = p1->speed * sin(p1->muki);
            double v2x = p2->speed * cos(p2->muki);
            double v2y = p2->speed * sin(p2->muki);

            double relative = (v2x - v1x) * nx + (v2y - v1y) * ny;
            if (relative >= 0.0) continue;

            double n1 = v1x * nx + v1y * ny;
            double n2 = v2x * nx + v2y * ny;

            v1x += (n2 - n1) * nx;
            v1y += (n2 - n1) * ny;
            v2x += (n1 - n2) * nx;
            v2y += (n1 - n2) * ny;

            p1->speed = sqrt(v1x * v1x + v1y * v1y);
            p2->speed = sqrt(v2x * v2x + v2y * v2y);
            p1->muki = atan2(v1y, v1x);
            p2->muki = atan2(v2y, v2x);

            // めり込みを少し戻して連続衝突を防ぐ
            double push = (24.0 - dist) * 0.5;
            p1->x -= nx * push;
            p1->y -= ny * push;
            p2->x += nx * push;
            p2->y += ny * push;

            p1->param_i[1] = p1->count;
            p2->param_i[1] = p2->count;

            AirHockey_AddSpark(
                pEnemyShotSet,
                (p1->x + p2->x) * 0.5,
                (p1->y + p2->y) * 0.5,
                atan2(v1y + v2y, v1x + v2x),
                1,
                11,
                3.2);

            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
}

// 敵本体のパターン
void EnemyPat_AirHockey_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.25 * (double)muki;

        if (enemy.x < 100.0) {
            enemy.x = 100.0;
            muki = 1;
        }
        else if (enemy.x > 380.0) {
            enemy.x = 380.0;
            muki = -1;
        }
    }

    // すべてのパックと火花を一つの弾幕セットで管理
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = AirHockey_Pattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}