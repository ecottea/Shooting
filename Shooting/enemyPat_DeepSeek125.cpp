// ============================================================
// 弾幕：南京玉すだれ「玉簾七変化」
// ------------------------------------------------------------
// 小弾＝竹の珠、中弾＝節、大弾＝手元／房 として、
// 下向きの玉簾が伸び縮みしながら揺れ、
// 橋 → 帆掛け舟 → しだれ柳 へと変化する。
// 中弾の節からは、弾列に垂直な3way弾が放出される。
//
// 弾の制御について:
//   param_i[0] : ノード種別 (0=小, 1=中, 2=大)
//   param_i[1] : 制御種別   (0=制御弾, 1=自由弾)
//   param_d[0] : 根元からの位置 t (0.0=根元, 1.0=先端)
// ============================================================

enum {
    NODE_SMALL = 0,   // 珠（小弾）
    NODE_MEDIUM = 1,   // 節（中弾）
    NODE_LARGE = 2,   // 手元（大弾）
};

enum {
    BULLET_CONTROLLED = 0, // 玉簾のノード（座標を毎フレーム制御）
    BULLET_FREE = 1, // 放出された自由弾（speed/muki で移動）
};

static const double CURTAIN_BASE_LEN = 150.0*2;
static const int    T_GROW = 40;    // 出現
static const int    T_SWING = 120;   // 伸張＆揺れ
static const int    T_MORPH = 180;   // 七変化（3形状×60F）
static const int    T_FOLD = 60;    // 畳み

// ------------------------------------------------------------
// ヘルパ：制御弾（玉簾のノード）を追加
// ------------------------------------------------------------
static void AddNode(sEnemyShotSet* set, double t, int nodeType, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = set->x;
    p->y = set->y;
    p->muki = 0.0;
    p->speed = 0.0;
    p->kind = kind;
    p->param_i[0] = nodeType;
    p->param_i[1] = BULLET_CONTROLLED;
    p->param_d[0] = t;
    p->margin = 240;
    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
// ヘルパ：自由弾を追加
// ------------------------------------------------------------
static void AddFreeShot(sEnemyShotSet* set, double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[1] = BULLET_FREE;
    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
// 形状ごとの座標計算
//   shape: 0=直線, 1=橋, 2=帆掛け舟, 3=しだれ柳
// ------------------------------------------------------------
static void CalcShapePos(int shape, double t, double frame, double baseAngle,
    double rootX, double rootY, double len,
    double* outX, double* outY)
{
    const double PI = DX_PI;
    double dist = len * t;
    double px, py;

    switch (shape) {
    case 1: {
        // 橋：上に凸の円弧
        double arc = 0.9;
        double ang = baseAngle + arc * (t - 0.5);
        px = rootX + cos(ang) * dist;
        py = rootY + sin(ang) * dist - sin(arc * 0.5) * len * 0.35;
        break;
    }
    case 2: {
        // 帆掛け舟：三角形
        double tri = (t < 0.5) ? t * 2.0 : (1.0 - t) * 2.0;
        double perp = baseAngle + PI * 0.5;
        px = rootX + cos(baseAngle) * dist + cos(perp) * (1.0 - tri) * len * 0.30;
        py = rootY + sin(baseAngle) * dist + sin(perp) * (1.0 - tri) * len * 0.30;
        break;
    }
    case 3: {
        // しだれ柳：sin波で左右に揺れる
        double perp = baseAngle + PI * 0.5;
        double sway = sin(t * PI * 3.0 + frame * 0.12) * 28.0;
        px = rootX + cos(baseAngle) * dist + cos(perp) * sway;
        py = rootY + sin(baseAngle) * dist + sin(perp) * sway;
        break;
    }
    default: {
        // 直線
        px = rootX + cos(baseAngle) * dist;
        py = rootY + sin(baseAngle) * dist;
        break;
    }
    }
    *outX = px;
    *outY = py;
}

// ------------------------------------------------------------
// 玉簾パターン本体
// ------------------------------------------------------------
static void ShotTamabudare(sEnemyShotSet* pEnemyShotSet)
{
    const double PI = DX_PI;

    // 敵の位置に追従（玉簾は敵からぶら下がる）
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;

    pEnemyShotSet->param_i[1]++;               // 状態内フレーム
    int f = pEnemyShotSet->param_i[1];
    int state = pEnemyShotSet->param_i[0];

    // ---------- 状態遷移 ----------
    if (state == 0 && f >= T_GROW) {
        pEnemyShotSet->param_i[0] = 1;
        pEnemyShotSet->param_i[1] = 0;
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        f = 0; state = 1;
    }
    else if (state == 1 && f >= T_SWING) {
        pEnemyShotSet->param_i[0] = 2;
        pEnemyShotSet->param_i[1] = 0;
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        f = 0; state = 2;
    }
    else if (state == 2 && f >= T_MORPH) {
        pEnemyShotSet->param_i[0] = 3;
        pEnemyShotSet->param_i[1] = 0;
        f = 0; state = 3;
    }
    else if (state == 3 && f == T_FOLD) {
        // 最終バースト：全方位に中弾、残りの制御弾は外向きに飛散
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16; i++) {
            AddFreeShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                i * 2.0 * PI / 16.0,
                2.5,
                img_enemyShotMediumBall[i % 8]);
        }

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[1] == BULLET_CONTROLLED) {
                double dx = p->x - pEnemyShotSet->x;
                double dy = p->y - pEnemyShotSet->y;
                p->muki = atan2(dy, dx);
                p->speed = 2.0 + (double)GetRand(150) / 100.0;
                p->param_i[1] = BULLET_FREE;
            }
            p = p->next;
        }
    }

    // ---------- 形状パラメータ ----------
    int shape = 0; // 0=直線, 1=橋, 2=帆掛け舟, 3=しだれ柳
    if (state == 2) {
        int sub = f / 60;
        shape = (sub == 0) ? 1 : (sub == 1) ? 2 : 3;
    }
    else if (state >= 3) {
        shape = 3;
    }

    double swingAngle = 0.0;
    if (state >= 1 && state <= 2) {
        swingAngle = sin(f * 0.055) * 0.6; // ±約34度
    }
    double baseAngle = pEnemyShotSet->muki + swingAngle;

    double growLen = CURTAIN_BASE_LEN;
    if (state == 0) {
        growLen = CURTAIN_BASE_LEN * (0.25 + 0.75 * (f / (double)T_GROW));
    }
    else if (state == 1) {
        growLen = CURTAIN_BASE_LEN + 40.0 * sin(f * 0.045); // 脈動
    }
    else if (state == 3) {
        growLen = CURTAIN_BASE_LEN * (1.0 - f / (double)T_FOLD);
        if (growLen < 0.0) growLen = 0.0;
    }

    // ---------- 各弾の更新 ----------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[1] == BULLET_FREE) {
            // 自由弾：通常移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 制御弾：形状に沿った位置へ
            double t = pShot->param_d[0];
            int    nodeType = pShot->param_i[0];
            double px, py;
            CalcShapePos(shape, t, (double)f, baseAngle,
                pEnemyShotSet->x, pEnemyShotSet->y, growLen,
                &px, &py);
            pShot->x = px;
            pShot->y = py;

            // 中弾ノードから垂直3wayを放出
            // （伸張後半と七変化の間）
            bool canEmit = (state == 1 && f > 60) || (state == 2);
            if (canEmit && nodeType == NODE_MEDIUM) {
                if (pShot->count > 0 && pShot->count % 45 == 0) {
                    double perp = baseAngle + PI * 0.5;
                    for (int j = -1; j <= 1; j++) {
                        AddFreeShot(pEnemyShotSet, px, py,
                            perp + j * 0.45,
                            2.2,
                            img_enemyShotSmallBall[(j + 3) % 8]);
                    }
                }
            }
        }
        pShot = pShot->next;
    }

    // ---------- 出現中：ノードを追加 ----------
    if (state == 0) {
        int spawnIndex = f / 2;
        if (f % 2 == 0 && spawnIndex < 20) {
            double t = spawnIndex / 19.0;
            int nodeType;
            int kind;
            if (spawnIndex == 0) {
                nodeType = NODE_LARGE;
                kind = img_enemyShotLargeBall[4];     // 青：手元
            }
            else if (spawnIndex % 4 == 0) {
                nodeType = NODE_MEDIUM;
                kind = img_enemyShotMediumBall[1];    // 黄：節
            }
            else {
                nodeType = NODE_SMALL;
                kind = img_enemyShotSmallBall[3];     // シアン：珠
            }
            AddNode(pEnemyShotSet, t, nodeType, kind);
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_NankinTamasudare_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 玉簾を周期的に出現させる
    if (count % 360 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTamabudare;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI * 0.5; // 下向き
        pEnemyShotSet->kind = shot_count++;

        // 独自状態
        pEnemyShotSet->param_i[0] = 0;  // state = 出現
        pEnemyShotSet->param_i[1] = 0;  // 状態内フレーム
        pEnemyShotSet->param_d[0] = 0.0;
        pEnemyShotSet->param_d[1] = 0.0;

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}