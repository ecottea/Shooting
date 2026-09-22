// enemyPat_Tmp.cpp
// 弾幕「氷晶法陣」 ── コッホ雪片をモチーフにした弾幕
//
// <展開の流れ> (1サイクル = 660フレーム。弾幕セットは内部で無限ループする)
//   [1] 六角形の出現  (tt   0~ 29) 頂点=小玉(シアン) / 辺=中玉(青)
//   [2] 静止・予告    (tt  30~ 74) ゆっくり回転
//   [3] コッホ展開1   (tt  75~254) 各辺を3分割し中央を外側へ押し出す → 六芒星
//   [4] 静止          (tt 255~299)
//   [5] コッホ展開2   (tt 300~539) さらに再帰分割 → 雪片が完成
//   [6] 完成形の回転  (tt 540~629) 高速回転で脅威度を上げる
//   [7] 一斉射出      (tt 630    ) 辺弾=速い放射弾 / 頂点弾=一部が自機狙い加速弾
//   [8] 余韻          (tt 631~659) → [1] へ戻る(回転位相を変えて再開)
//
// <使用素材>
//   弾: 小玉(頂点/シアン3) 中玉(辺/青4) 中玉(成長中の頂点/白6)
//   音: sound_enemyCharge(展開予告) / sound_enemyShot_light(展開1開始)
//       sound_enemyShot_medium(展開2開始) / sound_enemyShot_extreme(一斉射出)
//
// ※ count のインクリメントと画面外の弾の消去はメインルーチンが行うため、
//   このファイルでは行わない。
// ※ 弾幕セットは count==1 のときに1つだけ生成する。マルチセット同時稼働は
//   想定していない(静的な辺バッファを共有するため)。

// ============================================================
// 定数
// ============================================================
static const double HEX_R = 200.0;   // レベル0(六角形)の半径
static const double CRYST_X = 240.0;   // 結晶中心
static const double CRYST_Y = 210.0;
static const int    PERIOD = 660;     // 1サイクルのフレーム数
static const int    T_L1 = 75;      // コッホ展開1 開始
static const int    T_L2 = 300;     // コッホ展開2 開始
static const int    T_FIRE = 630;     // 一斉射出
static const int    COL_V = 3;       // 頂点用小玉: シアン
static const int    COL_E = 4;       // 辺用中玉: 青
static const int    COL_G = 6;       // 成長中の頂点: 白

// コッホ分割後の「辺」リスト(結晶中心を原点とする体標準座標)
static double s_kochAx[24], s_kochAy[24];
static double s_kochBx[24], s_kochBy[24];
static int    s_kochNum = 0;

// ============================================================
// 補助関数
// ============================================================

// 辺A→Bを3分割し、外側に盛り上がるコッホ頂点Qを求める(体標準座標)
static void KochDivide(double ax, double ay, double bx, double by,
    double* p1x, double* p1y,
    double* qx, double* qy,
    double* p2x, double* p2y)
{
    *p1x = ax + (bx - ax) / 3.0;
    *p1y = ay + (by - ay) / 3.0;
    *p2x = ax + (bx - ax) * 2.0 / 3.0;
    *p2y = ay + (by - ay) * 2.0 / 3.0;
    double dx = *p2x - *p1x, dy = *p2y - *p1y;
    // 分割区間を-60度回転して外側へ(頂点列が角度の増える順なため外向きになる)
    *qx = *p1x + (dx * 0.5 + dy * 0.86602540378);
    *qy = *p1y + (-dx * 0.86602540378 + dy * 0.5);
}

// 角度を [-π, π] に正規化
static double NormAngle(double a)
{
    while (a > DX_PI) a -= DX_PI * 2.0;
    while (a < -DX_PI) a += DX_PI * 2.0;
    return a;
}

// 効果音再生(重なって積み込まれないよう一度止めてから鳴らす)
static void PlayEnemySound(int sound)
{
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);
}

// 弾を追加する
//   (bx,by) : 結晶中心原点の体標準座標での位置
//   mode    : 0=結晶(回転に同期して静止) / 1=成長中 / 2=発射済み
//   role    : 0=辺 / 1=頂点(射出時に一部が自機狙いになる)
//   (tx,ty) : mode1 用の目標座標(体標準座標)
//   growT   : mode1 用の移動フレーム数
//   child   : 到達時に頂点小弾を発生させるか
static sEnemyShot* AddKochShot(sEnemyShotSet* pSet, int kind,
    double bx, double by, int mode, int role,
    double tx, double ty, int growT, int child)
{
    sEnemyShot* p = new sEnemyShot;
    p->kind = kind;
    p->param_i[0] = mode;
    p->param_i[1] = role;
    p->param_i[2] = growT;
    p->param_i[3] = child;

    p->param_d[0] = sqrt(bx * bx + by * by);   // 中心からの半径
    p->param_d[1] = atan2(by, bx);             // 体標準での角度
    p->param_d[2] = 0.0;                       // 射出時の加速度
    p->margin = 120;

    if (mode == 1 && growT > 0) {
        p->param_d[5] = (tx - bx) / growT;     // 体標準座標での移動速度
        p->param_d[6] = (ty - by) / growT;
        p->param_d[7] = bx;
        p->param_d[8] = by;
    }

    // 現在の回転量を適用して画面座標へ
    double th = pSet->param_d[0];
    p->x = pSet->x + bx * cos(th) - by * sin(th);
    p->y = pSet->y + bx * sin(th) + by * cos(th);

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 1本の辺のコッホ展開で生じる新しい頂点(1/3,2/3点)と成長弾をスポーン
static void SpawnEdgeKoch(sEnemyShotSet* pSet, int e, int growT)
{
    double p1x, p1y, qx, qy, p2x, p2y;
    KochDivide(s_kochAx[e], s_kochAy[e], s_kochBx[e], s_kochBy[e],
        &p1x, &p1y, &qx, &qy, &p2x, &p2y);

    AddKochShot(pSet, img_enemyShotSmallBall[COL_V], p1x, p1y, 0, 1, 0, 0, 0, 0);
    AddKochShot(pSet, img_enemyShotSmallBall[COL_V], p2x, p2y, 0, 1, 0, 0, 0, 0);

    // 成長弾: 分割点の中間から頂点Qへ押し出され、到達時に頂点小弾を残す
    double mx = (p1x + qx) * 0.5, my = (p1y + qy) * 0.5;
    AddKochShot(pSet, img_enemyShotMediumBall[COL_G], mx, my, 1, 1, qx, qy, growT, 1);
}

// 辺を4分割した各小辺の中点に辺用中玉をスポーン (which: 0~3)
static void SpawnEdgeMid(sEnemyShotSet* pSet, int e, int which)
{
    double p1x, p1y, qx, qy, p2x, p2y;
    KochDivide(s_kochAx[e], s_kochAy[e], s_kochBx[e], s_kochBy[e],
        &p1x, &p1y, &qx, &qy, &p2x, &p2y);

    double mx, my;
    switch (which) {
    case 0:  mx = (s_kochAx[e] + p1x) * 0.5; my = (s_kochAy[e] + p1y) * 0.5; break;
    case 1:  mx = (p1x + qx) * 0.5;          my = (p1y + qy) * 0.5;          break;
    case 2:  mx = (qx + p2x) * 0.5;          my = (qy + p2y) * 0.5;          break;
    default: mx = (p2x + s_kochBx[e]) * 0.5; my = (p2y + s_kochBy[e]) * 0.5; break;
    }
    AddKochShot(pSet, img_enemyShotMediumBall[COL_E], mx, my, 0, 0, 0, 0, 0, 0);
}

// ============================================================
// 弾幕パターン本体
// ============================================================
static void ShotKochSnow(sEnemyShotSet* pSet)
{
    int tt = pSet->count % PERIOD;
    double cx = pSet->x, cy = pSet->y;

    // ---------- 周期の区切りごとの処理 ----------
    if (tt == 0) {
        // 回転量をリセットし、雪片の位相と回転方向を決め直す
        pSet->param_d[0] = 0.0;                      // 回転量θ(サイクル内の累積)
        pSet->param_d[1] = GetRand(628) / 100.0;     // 基準位相φ
        pSet->param_i[0] = GetRand(2) * 2 - 1;       // 回転方向

        // レベル1(六角形)の辺を体標準座標で作成
        s_kochNum = 6;
        for (int i = 0; i < 6; i++) {
            double a0 = pSet->param_d[1] + i * DX_PI / 3.0;
            double a1 = pSet->param_d[1] + (i + 1) * DX_PI / 3.0;
            s_kochAx[i] = HEX_R * cos(a0);  s_kochAy[i] = HEX_R * sin(a0);
            s_kochBx[i] = HEX_R * cos(a1);  s_kochBy[i] = HEX_R * sin(a1);
        }
        PlayEnemySound(sound_enemyCharge);
    }
    if (tt == T_L1) {
        PlayEnemySound(sound_enemyShot_light);
    }
    if (tt == T_L2) {
        PlayEnemySound(sound_enemyShot_medium);
        // レベル2: 各辺をさらにコッホ分割して 6*4=24 辺へ
        double tx[24], ty[24], ux[24], uy[24];
        int n = 0;
        for (int i = 0; i < s_kochNum; i++) {
            double p1x, p1y, qx, qy, p2x, p2y;
            KochDivide(s_kochAx[i], s_kochAy[i], s_kochBx[i], s_kochBy[i],
                &p1x, &p1y, &qx, &qy, &p2x, &p2y);
            tx[n] = s_kochAx[i]; ty[n] = s_kochAy[i]; ux[n] = p1x; uy[n] = p1y; n++;
            tx[n] = p1x;         ty[n] = p1y;         ux[n] = qx;  uy[n] = qy;  n++;
            tx[n] = qx;          ty[n] = qy;          ux[n] = p2x; uy[n] = p2y; n++;
            tx[n] = p2x;         ty[n] = p2y;         ux[n] = s_kochBx[i]; uy[n] = s_kochBy[i]; n++;
        }
        for (int i = 0; i < n; i++) {
            s_kochAx[i] = tx[i]; s_kochAy[i] = ty[i];
            s_kochBx[i] = ux[i]; s_kochBy[i] = uy[i];
        }
        s_kochNum = n;
    }

    // ---------- 回転(射出後は停止) ----------
    double spinSpd;
    if (tt < 30) spinSpd = 0.002;    // 六角形の出現中
    else if (tt < T_L1) spinSpd = 0.006;    // 予告
    else if (tt < 255) spinSpd = 0.002;    // 展開1
    else if (tt < T_L2) spinSpd = 0.006;    // 予告
    else if (tt < 540) spinSpd = 0.0015;   // 展開2
    else if (tt < T_FIRE) spinSpd = 0.010;  // 完成形の脅威の回転
    else                spinSpd = 0.0;      // 射出後
    pSet->param_d[0] += spinSpd * pSet->param_i[0];

    // ---------- スポーン ----------
    if (tt < 30) {
        // [1] 六角形: 頂点6個 → 辺上24個の順に1フレーム1個ずつ出現
        if (tt < 6) {
            double a = pSet->param_d[1] + tt * DX_PI / 3.0;
            AddKochShot(pSet, img_enemyShotSmallBall[COL_V],
                HEX_R * cos(a), HEX_R * sin(a), 0, 1, 0, 0, 0, 0);
        }
        else {
            int idx = tt - 6;
            int e = idx / 4, s = idx % 4;
            double f = (s + 1) / 5.0;
            double bx = s_kochAx[e] + (s_kochBx[e] - s_kochAx[e]) * f;
            double by = s_kochAy[e] + (s_kochBy[e] - s_kochAy[e]) * f;
            AddKochShot(pSet, img_enemyShotMediumBall[COL_E], bx, by, 0, 0, 0, 0, 0, 0);
        }
    }
    else if (tt >= T_L1 && tt < 255) {
        // [3] コッホ展開1: 6辺を順番に成長させる(1辺30フレーム)
        int e = (tt - T_L1) / 30;
        int k = (tt - T_L1) % 30;
        if (k == 0)  SpawnEdgeKoch(pSet, e, 24);
        else if (k == 8)  SpawnEdgeMid(pSet, e, 0);
        else if (k == 14) SpawnEdgeMid(pSet, e, 1);
        else if (k == 20) SpawnEdgeMid(pSet, e, 2);
        else if (k == 26) SpawnEdgeMid(pSet, e, 3);
    }
    else if (tt >= T_L2 && tt < 540) {
        // [5] コッホ展開2: 24辺を順番に成長させる(1辺10フレーム)
        int e = (tt - T_L2) / 10;
        int k = (tt - T_L2) % 10;
        if (k == 0) SpawnEdgeKoch(pSet, e, 8);
        else if (k == 2) SpawnEdgeMid(pSet, e, 0);
        else if (k == 4) SpawnEdgeMid(pSet, e, 1);
        else if (k == 6) SpawnEdgeMid(pSet, e, 2);
        else if (k == 8) SpawnEdgeMid(pSet, e, 3);
    }
    else if (tt == T_FIRE) {
        // [7] 一斉射出
        PlayEnemySound(sound_enemyShot_extreme);
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] != 2) {
                double ang = atan2(pShot->y - cy, pShot->x - cx);
                if (pShot->param_i[1] == 1 && GetRand(100) < 40) {
                    // 頂点弾の一部: 自機を狙う加速弾
                    double aim = atan2(player.y - pShot->y, player.x - pShot->x);
                    double d = NormAngle(aim - ang);
                    if (d > 0.25) d = 0.25;
                    if (d < -0.25) d = -0.25;
                    pShot->muki = ang + d;
                    pShot->speed = 2.0;
                    pShot->param_d[2] = 0.03;
                }
                else {
                    // 辺弾と残りの頂点弾: ばらけをもたせた放射弾(氷壁サージ)
                    pShot->muki = ang + (GetRand(100) - 50) / 100.0 * 0.12;
                    pShot->speed = (pShot->param_i[1] == 1 ? 3.2 : 4.5)
                        + GetRand(150) / 100.0;
                    pShot->param_d[2] = 0.0;
                }
                pShot->param_i[0] = 2;
            }
            pShot = pShot->next;
        }
    }

    // ---------- 弾の移動 ----------
    double th = pSet->param_d[0];
    double cth = cos(th), sth = sin(th);
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case 0: // 結晶: 体標準角度+回転量で剛体回転
        {
            double a = pShot->param_d[1] + th;
            pShot->x = cx + pShot->param_d[0] * cos(a);
            pShot->y = cy + pShot->param_d[0] * sin(a);
            break;
        }
        case 1: // 成長中: 体標準座標上を等速移動
        {
            pShot->param_d[7] += pShot->param_d[5];
            pShot->param_d[8] += pShot->param_d[6];
            pShot->x = cx + pShot->param_d[7] * cth - pShot->param_d[8] * sth;
            pShot->y = cy + pShot->param_d[7] * sth + pShot->param_d[8] * cth;
            pShot->param_i[2]--;
            if (pShot->param_i[2] <= 0) {
                double bx = pShot->param_d[7], by = pShot->param_d[8];
                pShot->param_i[0] = 0;
                pShot->param_d[0] = sqrt(bx * bx + by * by);
                pShot->param_d[1] = atan2(by, bx);
                if (pShot->param_i[3] == 1) {
                    pShot->param_i[3] = 0;
                    AddKochShot(pSet, img_enemyShotSmallBall[COL_V],
                        bx, by, 0, 1, 0, 0, 0, 0);
                }
            }
            break;
        }
        case 2: // 射出済み: 直進(頂点弾は加速)
        {
            pShot->speed += pShot->param_d[2];
            if (pShot->speed > 3.0) pShot->speed = 3.0;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_KochSnowflake_Zai()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;

        // 弾幕セットは1つだけ生成し、以降はセット内部で周期ループする
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKochSnow;
        pEnemyShotSet->x = CRYST_X;
        pEnemyShotSet->y = CRYST_Y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_d[0] = 0.0;                  // 回転量θ
        pEnemyShotSet->param_d[1] = GetRand(628) / 100.0; // 基準位相φ
        pEnemyShotSet->param_i[0] = GetRand(2) * 2 - 1;   // 回転方向

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // ゆっくり左右にパトロール(結晶はセット生成時の位置に固定される)
        enemy.x += 0.98 * (double)muki;
        if (enemy.x > 340.0) muki = -1;
        if (enemy.x < 140.0) muki = 1;
    }
}