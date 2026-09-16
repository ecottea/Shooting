// EnemyPat_miComet_MetaAI.cpp
// 桜(マゼンタ) + 彗星(シアン) の斑鳩風弾幕

static const int BARRIER_NUM = 16;
static const double BARRIER_RADIUS = 38.0;
static const double BARRIER_BALL_R = 2.5*2;

//-------------------------
// ヘルパ
//-------------------------
static bool GetBulletRadii(int kind, double& rx, double& ry) {
    for (int c = 0; c < 9; ++c) {
        if (kind == img_enemyShotSmallBall[c]) { rx = 2.5; ry = 2.5; return true; }
        if (kind == img_enemyShotMediumBall[c]) { rx = 7.0; ry = 7.0; return true; }
        if (kind == img_enemyShotLargeBall[c]) { rx = 20.0; ry = 20.0; return true; }
        if (kind == img_enemyShotBullet[c]) { rx = 5.0; ry = 2.0; return true; }
        if (kind == img_enemyShotScale[c]) { rx = 4.0; ry = 3.0; return true; }
        if (kind == img_enemyShotDiamond[c]) { rx = 4.5; ry = 2.5; return true; }
        if (kind == img_enemyShotMediumOval[c]) { rx = 10.5; ry = 7.0; return true; }
        if (kind == img_enemyShotLaser[c]) { rx = 64.0; ry = 4.0; return true; }
    }
    return false;
}
static bool IsMagentaBullet(int kind) {
    return kind == img_enemyShotSmallBall[5] ||
        kind == img_enemyShotMediumBall[5] ||
        kind == img_enemyShotLargeBall[5] ||
        kind == img_enemyShotBullet[5] ||
        kind == img_enemyShotScale[5] ||
        kind == img_enemyShotDiamond[5] ||
        kind == img_enemyShotMediumOval[5] ||
        kind == img_enemyShotLaser[5];
}
static bool IsCyanBullet(int kind) {
    return kind == img_enemyShotSmallBall[3] ||
        kind == img_enemyShotMediumBall[3] ||
        kind == img_enemyShotLargeBall[3] ||
        kind == img_enemyShotBullet[3] ||
        kind == img_enemyShotScale[3] ||
        kind == img_enemyShotDiamond[3] ||
        kind == img_enemyShotMediumOval[3] ||
        kind == img_enemyShotLaser[3];
}
static void PlayCharge() {
    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
}
static void PlayExtreme() {
    if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
}

//-------------------------
// 桜: マゼンタ - 鱗弾を花弁状に
//-------------------------
static void ShotSakuraMagenta(sEnemyShotSet* pSet) {
    // param_d[0]: 全体回転
    pSet->param_d[0] += 0.8 * DX_PI / 180.0;

    if (pSet->count % 9 == 0) {
        double base = pSet->param_d[0];
        for (int petal = 0; petal < 5; ++petal) {
            double dir = base + petal * 72.0 * DX_PI / 180.0;
            dir += (GetRand(20) - 10) * DX_PI / 180.0 * 0.3; // 微ランダム
            for (int layer = 0; layer < 3; ++layer) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = dir + (layer - 1) * 8.0 * DX_PI / 180.0;
                pShot->speed = 1.4 + layer * 0.35 + GetRand(20) / 100.0;
                if (layer == 0) pShot->kind = img_enemyShotScale[5];
                else if (layer == 1) pShot->kind = img_enemyShotDiamond[5];
                else pShot->kind = img_enemyShotMediumBall[5];
                pShot->param_d[0] = GetRand(100) / 100.0 * DX_PI * 2.0; // 揺れ位相
                pShot->param_d[1] = GetRand(10) / 10.0;
                pShot->margin = 20.0;
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
        if (GetRand(3) == 0) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x; pShot->y = pSet->y;
            pShot->muki = pSet->muki + (GetRand(40) - 20) * DX_PI / 180.0;
            pShot->speed = 1.0 + GetRand(50) / 100.0;
            pShot->kind = img_enemyShotMediumOval[5];
            pShot->param_d[0] = GetRand(100) / 100.0 * DX_PI * 2.0;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    // 移動: 花びららしい揺れ
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double sway = sin(p->count * 0.07 + p->param_d[0]) * 0.018;
        p->muki += sway;
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

//-------------------------
// 彗星: シアン - 頭+尾の彗星
//-------------------------
static void ShotCometCyan(sEnemyShotSet* pSet) {
    if (pSet->count % 11 == 0) {
        double aim = atan2(player.y - pSet->y, player.x - pSet->x);
        aim += (GetRand(30) - 15) * DX_PI / 180.0;
        for (int side = -1; side <= 1; side += 2) {
            double dir = aim + side * 12.0 * DX_PI / 180.0;
            for (int tail = 0; tail < 6; ++tail) {
                sEnemyShot* pShot = new sEnemyShot;
                double off = tail * 12.0;
                pShot->x = pSet->x - off * cos(dir);
                pShot->y = pSet->y - off * sin(dir);
                pShot->muki = dir;
                pShot->speed = 3.2 + GetRand(60) / 100.0;
                if (tail == 0) pShot->kind = img_enemyShotMediumBall[3];
                else if (tail % 2 == 0) pShot->kind = img_enemyShotBullet[3];
                else pShot->kind = img_enemyShotDiamond[3];
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
        if (GetRand(2) == 0) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x; pShot->y = pSet->y;
            pShot->muki = aim + (GetRand(20) - 10) * DX_PI / 180.0;
            pShot->speed = 4.5;
            pShot->kind = img_enemyShotLaser[3];
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->speed += 0.008; // 微加速で彗星らしく
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

//-------------------------
// バリア: 自機追従+吸収
//-------------------------
static void ShotBarrier(sEnemyShotSet* pSet) {
    // param_i[0]: 0=マゼンタ 1=シアン
    // param_d[0]: 回転オフセット
    pSet->param_d[0] += 0.015;
    double rot = pSet->param_d[0];

    int idx = 0;
    sEnemyShot* pBall = pSet->pEnemyShotHead->next;
    while (pBall != pSet->pEnemyShotHead) {
        double ang = 2.0 * DX_PI * idx / BARRIER_NUM + rot;
        pBall->x = player.x + BARRIER_RADIUS * cos(ang);
        pBall->y = player.y + BARRIER_RADIUS * sin(ang);
        pBall->margin = 999.0;
        pBall = pBall->next;
        ++idx;
    }

    int curColor = pSet->param_i[0]; // 0 mag 1 cyan

    // 吸収判定
    sEnemyShotSet* pOther = enemyShotSetHead.next;
    while (pOther != &enemyShotSetHead) {
        if (pOther == pSet) { pOther = pOther->next; continue; }
        sEnemyShot* pShot = pOther->pEnemyShotHead->next;
        while (pShot != pOther->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;

            bool target = false;
            if (curColor == 0) target = IsMagentaBullet(pShot->kind);
            else target = IsCyanBullet(pShot->kind);

            // 小玉[5][3]自体は敵弾としては使っていないので吸収対象外にしてもよいが、色判定に含めておく
            if (target) {
                double rx, ry;
                if (GetBulletRadii(pShot->kind, rx, ry)) {
                    double erxBase = rx;
                    double eryBase = ry;
                    // 全バリア玉と衝突チェック
                    sEnemyShot* pB = pSet->pEnemyShotHead->next;
                    bool hit = false;
                    while (pB != pSet->pEnemyShotHead) {
                        double dx = pB->x - pShot->x;
                        double dy = pB->y - pShot->y;
                        double dist2 = dx * dx + dy * dy;
                        if (dist2 > 80.0 * 80.0) { pB = pB->next; continue; }
                        double c = cos(pShot->muki);
                        double s = sin(pShot->muki);
                        double lx = dx * c + dy * s;
                        double ly = -dx * s + dy * c;
                        double erx = erxBase + BARRIER_BALL_R;
                        double ery = eryBase + BARRIER_BALL_R;
                        double v = (lx * lx) / (erx * erx) + (ly * ly) / (ery * ery);
                        if (v <= 1.0) { hit = true; break; }
                        pB = pB->next;
                    }
                    if (hit) {
                        pShot->prev->next = pShot->next;
                        pShot->next->prev = pShot->prev;
                        delete pShot;
                    }
                }
            }
            pShot = pNext;
        }
        pOther = pOther->next;
    }
}

//-------------------------
// 本体
//-------------------------
void EnemyPat_miComet_MetaAI() {
    static int muki = 1;
    static sEnemyShotSet* pSakuraSet = nullptr;
    static sEnemyShotSet* pCometSet = nullptr;
    static sEnemyShotSet* pBarrierSet = nullptr;

    if (count == 1) {
        enemy.x = 120.0; enemy.y = 80.0;
        enemy.x2 = 360.0; enemy.y2 = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // 桜セット
        pSakuraSet = new sEnemyShotSet;
        pSakuraSet->count = 0;
        pSakuraSet->patternFunc = ShotSakuraMagenta;
        pSakuraSet->x = enemy.x; pSakuraSet->y = enemy.y;
        pSakuraSet->muki = 0;
        pSakuraSet->param_d[0] = 0;
        pSakuraSet->pEnemyShotHead = new sEnemyShot;
        pSakuraSet->pEnemyShotHead->prev = pSakuraSet->pEnemyShotHead;
        pSakuraSet->pEnemyShotHead->next = pSakuraSet->pEnemyShotHead;
        pSakuraSet->prev = enemyShotSetHead.prev;
        pSakuraSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSakuraSet;
        enemyShotSetHead.prev = pSakuraSet;

        // 彗星セット
        pCometSet = new sEnemyShotSet;
        pCometSet->count = 0;
        pCometSet->patternFunc = ShotCometCyan;
        pCometSet->x = enemy.x2; pCometSet->y = enemy.y2;
        pCometSet->muki = 0;
        pCometSet->pEnemyShotHead = new sEnemyShot;
        pCometSet->pEnemyShotHead->prev = pCometSet->pEnemyShotHead;
        pCometSet->pEnemyShotHead->next = pCometSet->pEnemyShotHead;
        pCometSet->prev = enemyShotSetHead.prev;
        pCometSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pCometSet;
        enemyShotSetHead.prev = pCometSet;

        pBarrierSet = nullptr;
        PlayCharge(); // 0秒後
    }
    else {
        enemy.x += 0.7 * (double)muki;
        enemy.x2 += 0.7 * (double)muki;
        if (count % 180 == 90) muki *= -1;

        if (pSakuraSet) {
            pSakuraSet->x = enemy.x; pSakuraSet->y = enemy.y;
            pSakuraSet->muki = atan2(player.y - pSakuraSet->y, player.x - pSakuraSet->x);
        }
        if (pCometSet) {
            pCometSet->x = enemy.x2; pCometSet->y = enemy.y2;
            pCometSet->muki = atan2(player.y - pCometSet->y, player.x - pCometSet->x);
        }
    }

    // 1秒後: バリア生成(マゼンタ)
    if (count == 60) {
        pBarrierSet = new sEnemyShotSet;
        pBarrierSet->count = 0;
        pBarrierSet->patternFunc = ShotBarrier;
        pBarrierSet->param_i[0] = 0; // magenta
        pBarrierSet->param_d[0] = 0;
        pBarrierSet->pEnemyShotHead = new sEnemyShot;
        pBarrierSet->pEnemyShotHead->prev = pBarrierSet->pEnemyShotHead;
        pBarrierSet->pEnemyShotHead->next = pBarrierSet->pEnemyShotHead;

        for (int i = 0; i < BARRIER_NUM; ++i) {
            sEnemyShot* pShot = new sEnemyShot;
            double ang = 2.0 * DX_PI * i / BARRIER_NUM;
            pShot->x = player.x + BARRIER_RADIUS * cos(ang);
            pShot->y = player.y + BARRIER_RADIUS * sin(ang);
            pShot->kind = img_enemyShotSmallBall[5];
            pShot->margin = 999.0;
            pShot->prev = pBarrierSet->pEnemyShotHead->prev;
            pShot->next = pBarrierSet->pEnemyShotHead;
            pBarrierSet->pEnemyShotHead->prev->next = pShot;
            pBarrierSet->pEnemyShotHead->prev = pShot;
        }
        pBarrierSet->prev = enemyShotSetHead.prev;
        pBarrierSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBarrierSet;
        enemyShotSetHead.prev = pBarrierSet;

        PlayExtreme();
    }

    // 3秒おきの予告
    if (count >= 180 && count % 180 == 0) {
        PlayCharge();
    }
    // 3秒おきの色変化
    if (count >= 240 && count % 180 == 60) {
        if (pBarrierSet) {
            int newColor = 1 - pBarrierSet->param_i[0];
            pBarrierSet->param_i[0] = newColor;
            sEnemyShot* pB = pBarrierSet->pEnemyShotHead->next;
            while (pB != pBarrierSet->pEnemyShotHead) {
                pB->kind = (newColor == 0) ? img_enemyShotSmallBall[5] : img_enemyShotSmallBall[3];
                pB->margin = 999.0;
                pB = pB->next;
            }
        }
        PlayExtreme();
    }
}