// enemyPat_kentoNoHana.cpp
//
// 弾幕：ケントの花（ニュートンのリンゴ）
//
// 「ケントの花」は西洋リンゴの一品種（Flower of Kent）。この品種のリンゴが
// 落下するのを見たことがニュートンの万有引力発見のきっかけになったという
// 逸話で知られる。本パターンでは、専用素材を使わず既存の敵弾（中楕円弾・
// 小玉・中玉・大玉・銃弾）を組み合わせて、
//   1) 花（5弁の花弁＋雄しべ＋雌しべ＋葉）が中心から咲き開く
//   2) しばらく静止した後、花全体が外向きにふわりと弾けながら重力に従って
//      落下していく
//   3) その花の中心には、初速ゼロの大玉（赤）が1個混ざっており、これだけ
//      は横に散らばらず真下へ自由落下する＝ニュートンのリンゴそのもの
// という2段構成で表現する。
//
// エンジンの規約に従い、各弾の座標は pShot->count のみから解析式で毎フレーム
// 再計算する（速度の積分・累積は行わない）。

// ------------------------------------------------------------
//  パラメータ
// ------------------------------------------------------------
static const int    FLOWER_SPAWN_INTERVAL = 16*3;    // 何フレームおきに新しい花を1輪発生させるか
static const int    FLOWER_PETAL_COUNT = 5;      // 花弁の枚数
static const int    FLOWER_STAMEN_COUNT = 10*3;     // 雄しべ（リング状の小玉）の数
static const int    FLOWER_BLOOM_FRAMES = 18*3;     // 開花アニメーションにかけるフレーム数
static const int    FLOWER_HOLD_FRAMES = 55;     // 開花後、静止保持するフレーム数（この後落下開始）
static const double FLOWER_GRAVITY = 0.05/2;   // 落下時の重力加速度
static const double FLOWER_UPWARD_POP = 0.6*2;    // 弾ける瞬間の上向きの初速（リンゴ以外）
static const double FLOWER_TOSS_SPEED_BASE = 0.5*3;   // 外向きに弾ける速さの基準値（+0.0〜0.4のばらつきを加算）
static const double FLOWER_TUMBLE_OMEGA = 0.01;   // 花全体がゆっくり回転する角速度

// ------------------------------------------------------------
//  弾を1個生成し、花の中心からの相対座標(bx0, by0)と、
//  弾け落ちる際の初速(vx0, vy0)を param_d に記録する。
//  以降の座標計算はすべてこの値と pShot->count から解析的に行う。
// ------------------------------------------------------------
static void SpawnFlowerShot(sEnemyShotSet* pEnemyShotSet, double bx0, double by0, int kind, double vx0, double vy0)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->x = pEnemyShotSet->x;
    pEnemyShot->y = pEnemyShotSet->y;
    pEnemyShot->muki = 0.0;
    pEnemyShot->speed = 0.0; // このパターンでは未使用（座標は式から直接求める）
    pEnemyShot->kind = kind;

    pEnemyShot->param_d[0] = bx0;              // 満開時の、花中心からの相対x
    pEnemyShot->param_d[1] = by0;              // 満開時の、花中心からの相対y
    pEnemyShot->param_d[2] = pEnemyShotSet->x; // 花が発生した時点の中心x（自由落下の基準点）
    pEnemyShot->param_d[3] = pEnemyShotSet->y; // 花が発生した時点の中心y
    pEnemyShot->param_d[4] = vx0;              // 落下開始時の水平初速
    pEnemyShot->param_d[5] = vy0;              // 落下開始時の垂直初速
    pEnemyShot->margin = 240;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 花弁・雄しべ・雌しべ・葉用：自身の相対座標の向きにそのまま弾け飛ぶ
// （中心そのものにある弾＝雌しべは、真上にわずかにポップするだけ）
static void SpawnFlowerBullet(sEnemyShotSet* pEnemyShotSet, double bx0, double by0, int kind)
{
    double dist = sqrt(bx0 * bx0 + by0 * by0);
    double vx0 = 0.0;
    double vy0 = -FLOWER_UPWARD_POP;

    if (dist > 0.01) {
        double ang = atan2(by0, bx0);
        // GetRand(x) は 0〜x の x+1 種類。GetRand(40)/100.0 で 0.00〜0.40 のばらつき（リプレイ再現性あり）
        double speed = FLOWER_TOSS_SPEED_BASE + GetRand(40) / 100.0;
        vx0 = speed * cos(ang);
        vy0 = speed * sin(ang) - FLOWER_UPWARD_POP;
    }

    SpawnFlowerShot(pEnemyShotSet, bx0, by0, kind, vx0, vy0);
}

// ------------------------------------------------------------
//  弾幕：ケントの花
// ------------------------------------------------------------
static void ShotFlowerOfKent(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // --- 花弁（5枚）---
        // 1枚につき：中楕円弾（白）4個で花弁本体、小玉2色4個で縁取り
        //   付け根側は薄紅（マゼンタ）、先端側は白 → 本物の花のグラデーションを再現
        for (int p = 0; p < FLOWER_PETAL_COUNT; p++) {
            double axis = p * (2.0 * DX_PI / FLOWER_PETAL_COUNT);

            const double ovalR[4] = { 12.0, 22.0, 22.0, 32.0 };
            const double ovalA[4] = { 0.0, -0.22, 0.22, 0.0 };
            for (int i = 0; i < 4; i++) {
                double ang = axis + ovalA[i];
                SpawnFlowerBullet(pEnemyShotSet, ovalR[i] * cos(ang), ovalR[i] * sin(ang), img_enemyShotMediumOval[6]);
            }

            const double dotR[4] = { 8.0, 8.0, 30.0, 30.0 };
            const double dotA[4] = { -0.15, 0.15, -0.30, 0.30 };
            const int    dotColor[4] = { 5, 5, 6, 6 }; // マゼンタ, マゼンタ, 白, 白
            for (int i = 0; i < 4; i++) {
                double ang = axis + dotA[i];
                SpawnFlowerBullet(pEnemyShotSet, dotR[i] * cos(ang), dotR[i] * sin(ang), img_enemyShotSmallBall[dotColor[i]]);
            }
        }

        // --- 雄しべ（小玉・黄、リング状） ---
        for (int k = 0; k < FLOWER_STAMEN_COUNT; k++) {
            double ang = k * (2.0 * DX_PI / FLOWER_STAMEN_COUNT);
            SpawnFlowerBullet(pEnemyShotSet, 9.0 * cos(ang), 9.0 * sin(ang), img_enemyShotSmallBall[1]);
        }

        // --- 雌しべ（中玉・黄、花の中心そのもの） ---
        SpawnFlowerBullet(pEnemyShotSet, 0.0, 0.0, img_enemyShotMediumBall[1]);

        // --- 葉（銃弾・緑、花弁の隙間に2枚） ---
        double leafAxisA = 0.5 * (2.0 * DX_PI / FLOWER_PETAL_COUNT);
        double leafAxisB = 2.5 * (2.0 * DX_PI / FLOWER_PETAL_COUNT);
        SpawnFlowerBullet(pEnemyShotSet, 14.0 * cos(leafAxisA), 14.0 * sin(leafAxisA), img_enemyShotBullet[2]);
        SpawnFlowerBullet(pEnemyShotSet, 14.0 * cos(leafAxisB), 14.0 * sin(leafAxisB), img_enemyShotBullet[2]);

        // --- ニュートンのリンゴ（大玉・赤、初速ゼロで中心のすぐ上に1個） ---
        // 他の弾と違い外向きに弾けさせず、重力だけで真下に自由落下させる
        SpawnFlowerShot(pEnemyShotSet, 0.0, -5.0, img_enemyShotLargeBall[0], 0.0, 0.0);
    }

    // 毎フレーム、この花に属する全弾の座標を pShot->count から再計算する
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int t = pShot->count;

        // 開花（0〜BLOOM_FRAMESで中心から広がる）＋ 常時ゆっくり回転
        double bloomScale = (t < FLOWER_BLOOM_FRAMES) ? (double)t / FLOWER_BLOOM_FRAMES : 1.0;
        double rotAngle = FLOWER_TUMBLE_OMEGA * t;
        double bx = pShot->param_d[0] * bloomScale;
        double by = pShot->param_d[1] * bloomScale;
        double rx = bx * cos(rotAngle) - by * sin(rotAngle);
        double ry = bx * sin(rotAngle) + by * cos(rotAngle);

        double cx, cy, vx, vy;
        if (t <= FLOWER_HOLD_FRAMES) {
            // 保持中：花の中心は静止
            cx = pShot->param_d[2];
            cy = pShot->param_d[3];
            vx = 0.0;
            vy = 0.0;
        }
        else {
            // 落下中：解析式による放物運動（速度の積分は行わない）
            double t2 = (double)(t - FLOWER_HOLD_FRAMES);
            vx = pShot->param_d[4];
            vy = pShot->param_d[5] + FLOWER_GRAVITY * t2;
            cx = pShot->param_d[2] + pShot->param_d[4] * t2;
            cy = pShot->param_d[3] + pShot->param_d[5] * t2 + 0.5 * FLOWER_GRAVITY * t2 * t2;
        }

        pShot->x = cx + rx;
        pShot->y = cy + ry;

        if (t <= FLOWER_HOLD_FRAMES) {
            pShot->muki = (fabs(rx) > 0.001 || fabs(ry) > 0.001) ? atan2(ry, rx) : 0.0;
        }
        else {
            pShot->muki = atan2(vy, vx);
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_FlowerOfKent_Claude()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    if (count % FLOWER_SPAWN_INTERVAL == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFlowerOfKent;
        // GetRand(x) は 0〜x の x+1 種類。GetRand(160)-80 で -80〜+80 に散らして木々のように配置
        pEnemyShotSet->x = enemy.x + (GetRand(160) - 80);
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 未使用（各弾の向きは個別に計算する）
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