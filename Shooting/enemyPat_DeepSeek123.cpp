// ============================================================================
//  enemyPat_Tmp.cpp
//  弾幕：エアホッケー「リフレクト・パック」
//
//  ShotSet は1つだけ生成し、その中にパックを定期的に追加する方式。
//  上下マレット（大玉）は最も近いパックの x 座標へ向かって移動する。
//
//  大玉（赤 / 青） : マレット
//  中玉（白）       : パック
//  小玉（黄）       : マレット打撃スパーク
//  小玉（シアン）   : 反射上限時の分裂弾
// ============================================================================

// --- 弾をセット末尾に繋ぐヘルパ ---
static void AddAirHockeyShot(sEnemyShotSet* pSet, sEnemyShot* pShot)
{
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// --- 弾幕：エアホッケー「リフレクト・パック」 ---
static void ShotAirHockey(sEnemyShotSet* pEnemyShotSet)
{
    // ---- 初回：上下マレットを2つだけ生成 ----
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pM1 = new sEnemyShot;
        pM1->x = 240.0;
        pM1->y = 70.0;
        pM1->muki = 0.0;
        pM1->speed = 0.0;
        pM1->kind = img_enemyShotLargeBall[0];   // 赤
        pM1->param_i[0] = 0;                     // tag: 上マレット
        AddAirHockeyShot(pEnemyShotSet, pM1);

        sEnemyShot* pM2 = new sEnemyShot;
        pM2->x = 240.0;
        pM2->y = 440.0;
        pM2->muki = 0.0;
        pM2->speed = 0.0;
        pM2->kind = img_enemyShotLargeBall[4];   // 青
        pM2->param_i[0] = 1;                     // tag: 下マレット
        AddAirHockeyShot(pEnemyShotSet, pM2);
    }

    // ---- パックを定期的に追加 ----
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 60 == 0) {
        sEnemyShot* pNew = new sEnemyShot;
        pNew->x = 240.0;
        pNew->y = 90.0;
        {
            double baseAngle = atan2(player.y - pNew->y, player.x - pNew->x);
            pNew->muki = baseAngle + (GetRand(60) - 30) / 180.0 * DX_PI;
        }
        pNew->speed = 3.0;
        pNew->kind = img_enemyShotMediumBall[6];   // 白
        pNew->param_i[0] = 2;   // tag: パック
        pNew->param_i[1] = 0;   // 反射回数
        pNew->param_i[2] = 3;   // 反射上限
        AddAirHockeyShot(pEnemyShotSet, pNew);

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // ---- マレット現在位置を取得 ----
    double mX[2] = { 240.0, 240.0 };
    double mY[2] = { 70.0, 410.0 };
    {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) { mX[0] = p->x; mY[0] = p->y; }
            else if (p->param_i[0] == 1) { mX[1] = p->x; mY[1] = p->y; }
            p = p->next;
        }
    }

    // ---- 各マレットの目標x（最も近いパック）を決定 ----
    double targetX[2] = { mX[0], mX[1] };
    double bestDist[2] = { 1e18, 1e18 };
    {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 2) {
                for (int m = 0; m < 2; m++) {
                    double dx = p->x - mX[m];
                    double dy = p->y - mY[m];
                    double d = dx * dx + dy * dy;
                    if (d < bestDist[m]) { bestDist[m] = d; targetX[m] = p->x; }
                }
            }
            p = p->next;
        }
    }

    // ---- マレットを目標xへ移動 ----
    {
        const double MALLET_STEP = 3.0;
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 0 || p->param_i[0] == 1) {
                int m = (p->param_i[0] == 0) ? 0 : 1;
                double diff = targetX[m] - p->x;
                if (diff > MALLET_STEP) diff = MALLET_STEP;
                else if (diff < -MALLET_STEP) diff = -MALLET_STEP;
                p->x += diff;
                if (p->x < 60.0) p->x = 60.0;
                if (p->x > 420.0) p->x = 420.0;
                mX[m] = p->x;
                mY[m] = p->y;
            }
            p = p->next;
        }
    }

    // ---- 弾更新 ----
    const double WALL_L = 20.0;
    const double WALL_R = 460.0;
    const double WALL_T = 20.0;
    const double WALL_B = 460.0;
    const double HIT_DIST = 14.0;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int tag = pShot->param_i[0];

        if (tag == 2) {
            // ---- パック移動 ----
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // マレット衝突
            bool hitMallet = false;
            for (int m = 0; m < 2; m++) {
                double dx = pShot->x - mX[m];
                double dy = pShot->y - mY[m];
                if (dx * dx + dy * dy < HIT_DIST * HIT_DIST) {
                    double len = sqrt(dx * dx + dy * dy);
                    double nx, ny;
                    if (len < 0.001) { nx = 0.0; ny = (m == 0) ? 1.0 : -1.0; }
                    else { nx = dx / len; ny = dy / len; }

                    double vx = pShot->speed * cos(pShot->muki);
                    double vy = pShot->speed * sin(pShot->muki);
                    double dot = vx * nx + vy * ny;
                    vx -= 2.0 * dot * nx;
                    vy -= 2.0 * dot * ny;
                    pShot->muki = atan2(vy, vx);

                    pShot->x = mX[m] + nx * (HIT_DIST + 1.0);
                    pShot->y = mY[m] + ny * (HIT_DIST + 1.0);

                    pShot->param_i[1]++;
                    pShot->speed *= 1.10;
                    if (pShot->speed > 8.0) pShot->speed = 8.0;

                    // スパーク（黄・小玉）を扇状に3発
                    for (int s = -2; s <= 2; s++) {
                        sEnemyShot* sp = new sEnemyShot;
                        sp->x = pShot->x;
                        sp->y = pShot->y;
                        sp->muki = pShot->muki + s * (15.0 / 180.0 * DX_PI);
                        sp->speed = 2.5;
                        sp->kind = img_enemyShotSmallBall[1];
                        sp->param_i[0] = 3;
                        AddAirHockeyShot(pEnemyShotSet, sp);
                    }

                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                    hitMallet = true;
                    break;
                }
            }

            // 壁反射
            if (!hitMallet) {
                double vx = cos(pShot->muki);
                double vy = sin(pShot->muki);
                if (pShot->x < WALL_L && vx < 0.0) { pShot->x = WALL_L; pShot->muki = DX_PI - pShot->muki; }
                if (pShot->x > WALL_R && vx > 0.0) { pShot->x = WALL_R; pShot->muki = DX_PI - pShot->muki; }
                if (pShot->y < WALL_T && vy < 0.0) { pShot->y = WALL_T; pShot->muki = -pShot->muki; }
                if (pShot->y > WALL_B && vy > 0.0) { pShot->y = WALL_B; pShot->muki = -pShot->muki; }
            }

            // 反射上限で分裂消滅（シアン小玉を2発）
            if (pShot->param_i[1] >= pShot->param_i[2]) {
                for (int s = 0; s < 18; s++) {
                    sEnemyShot* sp = new sEnemyShot;
                    sp->x = pShot->x;
                    sp->y = pShot->y;
                    sp->muki = pShot->muki + s * (2.0 * DX_PI / 18);
                    sp->speed = pShot->speed * 0.8;
                    sp->kind = img_enemyShotSmallBall[3];
                    sp->param_i[0] = 4;
                    AddAirHockeyShot(pEnemyShotSet, sp);
                }
                pShot->y = 9999.0;
                pShot->param_i[0] = -1;
            }
        }
        else if (tag == 3 || tag == 4) {
            // ---- スパーク / 分裂弾の移動 ----
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// --- 敵本体のパターン ---
void EnemyPat_AirHockey_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // エアホッケー用の ShotSet を1つだけ生成する
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAirHockey;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }
}