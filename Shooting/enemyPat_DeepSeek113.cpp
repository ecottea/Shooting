// enemyPat_Tmp.cpp

// 桜と彗星をモチーフにした斑鳩のような弾幕
// ボス1: 桜（マゼンタ） / ボス2: 彗星（シアン）
// 自機周囲にシールド小玉を配置し、3秒おきに色を反転して同色弾を吸収する

#define COLOR_MAGENTA 5
#define COLOR_CYAN 3
#define FPS 60
#define SHIELD_RADIUS 30.0
#define SHIELD_BALL_COUNT 16

// 楕円当たり判定
static bool IsHitEllipse(double x1, double y1, double rx1, double ry1,
    double x2, double y2, double rx2, double ry2)
{
    double dx = x1 - x2;
    double dy = y1 - y2;
    double rx = rx1 + rx2;
    double ry = ry1 + ry2;
    if (rx <= 0.0 || ry <= 0.0) return false;
    return (dx * dx) / (rx * rx) + (dy * dy) / (ry * ry) <= 1.0;
}

// シールド小玉の更新
static void ShieldUpdate(sEnemyShotSet* pEnemyShotSet)
{
    // 初回生成
    if (pEnemyShotSet->count == 0) {
        int n = SHIELD_BALL_COUNT;
        for (int i = 0; i < n; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = player.x + cos(2 * DX_PI * i / n) * SHIELD_RADIUS;
            p->y = player.y + sin(2 * DX_PI * i / n) * SHIELD_RADIUS;
            p->muki = 0;
            p->speed = 0;
            p->kind = img_enemyShotSmallBall[COLOR_MAGENTA];
            p->margin = 999.0;
            p->param_i[0] = COLOR_MAGENTA; // 色
            p->param_i[1] = 1;             // シールド弾フラグ
            p->param_d[0] = 2.5;           // 半径x
            p->param_d[1] = 2.5;           // 半径y
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 3秒おきに色を反転（1秒後、4秒後、7秒後…）
    if (pEnemyShotSet->count > 0 && count % (3 * FPS) == FPS) {
        int newColor = (pEnemyShotSet->param_i[0] == COLOR_MAGENTA) ? COLOR_CYAN : COLOR_MAGENTA;
        pEnemyShotSet->param_i[0] = newColor;
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            p->kind = img_enemyShotSmallBall[newColor];
            p->param_i[0] = newColor;
            p = p->next;
        }
    }

    // 自機追従
    int n = SHIELD_BALL_COUNT;
    int i = 0;
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        double angle = 2 * DX_PI * i / n;
        p->x = player.x + cos(angle) * SHIELD_RADIUS;
        p->y = player.y + sin(angle) * SHIELD_RADIUS;
        p = p->next;
        i++;
    }
}

// 桜弾幕（マゼンタ）
static void ShotCherry(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double baseMuki = pEnemyShotSet->muki;
        int petalCount = 5;
        for (int i = 0; i < petalCount; i++) {
            double muki = baseMuki + (2 * DX_PI * i / petalCount);
            // 花びら（中玉）
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = muki;
            p->speed = 1.5;
            p->kind = img_enemyShotMediumBall[COLOR_MAGENTA];
            p->param_i[0] = COLOR_MAGENTA;
            p->param_d[0] = 7.0;
            p->param_d[1] = 7.0;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;

            // 雄しべ（鱗弾）
            for (int j = 0; j < 3; j++) {
                sEnemyShot* ps = new sEnemyShot;
                ps->x = pEnemyShotSet->x;
                ps->y = pEnemyShotSet->y;
                ps->muki = muki + (j - 1) * 0.2;
                ps->speed = 2.0 + j * 0.1;
                ps->kind = img_enemyShotScale[COLOR_MAGENTA];
                ps->param_i[0] = COLOR_MAGENTA;
                ps->param_d[0] = 4.0;
                ps->param_d[1] = 3.0;
                ps->prev = pEnemyShotSet->pEnemyShotHead->prev;
                ps->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = ps;
                pEnemyShotSet->pEnemyShotHead->prev = ps;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 彗星弾幕（シアン）
static void ShotComet(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double baseMuki = pEnemyShotSet->muki;
        int cometCount = 3;
        for (int i = 0; i < cometCount; i++) {
            double muki = baseMuki + (i - 1) * (15.0 / 180.0 * DX_PI);
            // 頭（中楕円弾）
            sEnemyShot* pHead = new sEnemyShot;
            pHead->x = pEnemyShotSet->x;
            pHead->y = pEnemyShotSet->y;
            pHead->muki = muki;
            pHead->speed = 4.0;
            pHead->kind = img_enemyShotMediumOval[COLOR_CYAN];
            pHead->param_i[0] = COLOR_CYAN;
            pHead->param_d[0] = 10.5;
            pHead->param_d[1] = 7.0;
            pHead->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pHead;
            pEnemyShotSet->pEnemyShotHead->prev = pHead;

            // 尾（鱗弾）
            for (int j = 0; j < 10; j++) {
                sEnemyShot* pTail = new sEnemyShot;
                pTail->x = pEnemyShotSet->x;
                pTail->y = pEnemyShotSet->y;
                pTail->muki = muki;
                pTail->speed = 1.0 + j * 0.25;
                pTail->kind = img_enemyShotScale[COLOR_CYAN];
                pTail->param_i[0] = COLOR_CYAN;
                pTail->param_d[0] = 4.0;
                pTail->param_d[1] = 3.0;
                pTail->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pTail->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pTail;
                pEnemyShotSet->pEnemyShotHead->prev = pTail;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_miComet_DeepSeek()
{
    static int muki1, muki2;

    if (count == 1) {
        enemy.x = 60.0;
        enemy.y = 90.0;
        enemy.x2 = 420.0;
        enemy.y2 = 90.0;
        enemy.maxHp = enemy.hp = 200;
        muki1 = 1;
        muki2 = -1;
    }

    // 左右のボスが中央へ寄ったり離れたりしつつ、上下にゆっくり揺動
    enemy.x += 0.6 * muki1;
    if (enemy.x < 40) muki1 = 1;
    if (enemy.x > 240) muki1 = -1;
    enemy.y = 60.0 + 40.0 * sin(count * 0.02);

    enemy.x2 += 0.6 * muki2;
    if (enemy.x2 > 440) muki2 = -1;
    if (enemy.x2 < 240) muki2 = 1;
    enemy.y2 = 60.0 + 40.0 * cos(count * 0.02);

    // 予告音と効果音（3秒周期）
    if (count % (3 * FPS) == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count % (3 * FPS) == FPS) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 初回（1秒後）にシールド生成
        if (count == FPS) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShieldUpdate;
            pSet->x = 0; pSet->y = 0;
            pSet->param_i[0] = COLOR_MAGENTA;
            pSet->param_i[1] = 1; // シールドセットフラグ
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }

    // ボス1 桜弾幕
    if (count % 30 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotCherry;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ボス2 彗星弾幕
    if (count % 40 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotComet;
        pSet->x = enemy.x2;
        pSet->y = enemy.y2 + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // シールドによる同色弾の吸収
    sEnemyShotSet* pShieldSet = nullptr;
    for (sEnemyShotSet* pSet = enemyShotSetHead.next; pSet != &enemyShotSetHead; pSet = pSet->next) {
        if (pSet->param_i[1] == 1) {
            pShieldSet = pSet;
            break;
        }
    }
    if (pShieldSet) {
        sEnemyShot* pShield = pShieldSet->pEnemyShotHead->next;
        while (pShield != pShieldSet->pEnemyShotHead) {
            int shieldColor = pShield->param_i[0];
            double sx = pShield->x;
            double sy = pShield->y;
            double srx = pShield->param_d[0];
            double sry = pShield->param_d[1];

            sEnemyShotSet* pSet = enemyShotSetHead.next;
            while (pSet != &enemyShotSetHead) {
                if (pSet == pShieldSet) {
                    pSet = pSet->next;
                    continue;
                }
                sEnemyShot* pShot = pSet->pEnemyShotHead->next;
                while (pShot != pSet->pEnemyShotHead) {
                    sEnemyShot* pNext = pShot->next;
                    if (pShot->param_i[0] == shieldColor) {
                        double rx = pShot->param_d[0];
                        double ry = pShot->param_d[1];
                        if (IsHitEllipse(sx, sy, srx, sry, pShot->x, pShot->y, rx, ry)) {
                            pShot->prev->next = pShot->next;
                            pShot->next->prev = pShot->prev;
                            delete pShot;
                        }
                    }
                    pShot = pNext;
                }
                pSet = pSet->next;
            }
            pShield = pShield->next;
        }
    }
}