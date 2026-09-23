// enemyPat_TamasudareShichihenge.cpp
// 玉簾七変化（たますだれしちへんげ）
// 南京玉すだれ（大道芸）をモチーフにした無限ループ4フェーズパターン。
//
//  垂下フェーズ   : 紅白の玉ですだれが行ごとに時間差を付けて垂れ下がる
//  見立て変化フェーズ: すだれの点群を 釣り竿→橋→旗 の順にイージング補間で
//                     見立て変化させながら、各形状の要所から自機狙い弾を放つ
//  決めフェーズ   : 鳥居（左右の柱＋両端が反り上がる笠木）へ変形し、
//                     予告音の後に自機狙い5wayと笠木ラインの弾幕を同時発射
//  千切れ落ちフェーズ: 決め形状の位置から放射状加速＋重力落下で飛散し、
//                     垂下フェーズへ戻ってループする
//
// すだれ本体は1サイクルにつき1つの sEnemyShotSet として生成し、
// 各弾は自身の param_i[0]/[1] に行・列番号を持たせ、pShot->count から
// 絶対座標を式で計算する（速度積分は行わない）。
// 見立ての要所から放つ自機狙い弾は別レイヤーとして都度生成する。

// ------------------------------------------------------------
// 定数
// ------------------------------------------------------------
static const int ROWS = 16; // すだれの長さ方向の分割数
static const int COLS = 13; // すだれの幅方向の分割数（中心が真ん中の列）

static const int PHASE1_END = 90;                                  // 垂下フェーズ終了
static const int SHAPE_MORPH = 45;                                  // 各見立て形状への変形時間
static const int SHAPE_HOLD = 75;                                  // 各見立て形状の保持時間
static const int SHAPE_BLOCK = SHAPE_MORPH + SHAPE_HOLD;            // 120
static const int PHASE2_END = PHASE1_END + SHAPE_BLOCK * 3;        // 450（釣り竿・橋・旗の3形状）
static const int TORII_MORPH = 55;
static const int TORII_HOLD = 45;
static const int PHASE3_END = PHASE2_END + TORII_MORPH + TORII_HOLD; // 550
static const int SCATTER_DURATION = 90;
static const int LOOP_FRAMES = PHASE3_END + SCATTER_DURATION;       // 640

static const int TORII_TELEGRAPH_FRAME = PHASE2_END + TORII_MORPH;        // 505（予告音）
static const int TORII_BURST_FRAME = TORII_TELEGRAPH_FRAME + 15;      // 520（決め技発射）

// ------------------------------------------------------------
// 共通ユーティリティ
// ------------------------------------------------------------
struct sPoint { double x, y; };

static double EaseInOut(double t)
{
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t * t * (3.0 - 2.0 * t); // smoothstep
}

static sPoint Lerp(const sPoint& a, const sPoint& b, double t)
{
    sPoint p;
    p.x = a.x + (b.x - a.x) * t;
    p.y = a.y + (b.y - a.y) * t;
    return p;
}

// ------------------------------------------------------------
// 見立て形状の座標式
// t_r : 0.0〜1.0（すだれの長さ方向の位置）
// t_c : -1.0〜1.0（すだれの幅方向の位置、0が中心）
// ------------------------------------------------------------

// 素のすだれ（左右にゆらゆらと揺れる）
static sPoint ShapeCurtain(double t_r, double t_c, double bx, double by, int localCount)
{
    sPoint p;
    double sway = 8.0 * sin(localCount * 0.05 + t_r * 1.2);
    p.x = bx + t_c * 55.0 + sway;
    p.y = by + 40.0 + t_r * 220.0;
    return p;
}

// 見立て1：釣り竿（先端が鉤状にくるりと巻く）
static sPoint ShapeRod(double t_r, double t_c, double bx, double by)
{
    double x0 = bx - 40.0, y0 = by + 20.0;
    double x1 = bx + 180.0, y1 = by + 170.0;
    double x = x0 + (x1 - x0) * t_r;
    double y = y0 + (y1 - y0) * t_r;

    if (t_r > 0.85) {
        double local = (t_r - 0.85) / 0.15;
        double angle = local * DX_PI * 1.5;
        x += 20.0 * sin(angle);
        y += 20.0 * (1.0 - cos(angle));
    }

    double perp = atan2(y1 - y0, x1 - x0) + DX_PI / 2.0;
    x += t_c * 4.0 * cos(perp);
    y += t_c * 4.0 * sin(perp);

    sPoint p; p.x = x; p.y = y;
    return p;
}

// 見立て2：橋（虹のようなアーチ、欄干の厚みを列方向で表現）
static sPoint ShapeBridge(double t_r, double t_c, double bx, double by)
{
    sPoint p;
    p.x = bx - 160.0 + t_r * 320.0;
    p.y = by + 160.0 - 110.0 * sin(DX_PI * t_r) + t_c * 8.0;
    return p;
}

// 見立て3：旗（竿＋はためく旗布）
static sPoint ShapeFlag(double t_r, double t_c, double bx, double by, int localCount)
{
    sPoint p;
    if (t_r <= 0.35) {
        double local = t_r / 0.35;
        p.x = bx - 120.0 + t_c * 2.0;
        p.y = by - 60.0 + local * 220.0;
    }
    else {
        double local = (t_r - 0.35) / 0.65;
        p.x = bx - 120.0 + local * 150.0;
        p.y = by - 60.0 + t_c * 20.0 + 8.0 * sin(local * DX_PI * 3.0 + localCount * 0.15);
    }
    return p;
}

// 決め形状：鳥居（左右の柱＋両端が反り上がる笠木）
static sPoint ShapeTorii(double t_r, double t_c, double bx, double by)
{
    sPoint p;
    if (t_r <= 0.75) {
        double local = t_r / 0.75;
        double pillarX = (t_c < 0.0) ? (bx - 100.0) : (bx + 100.0);
        p.x = pillarX + t_c * 6.0;
        p.y = by - 50.0 + local * 210.0;
    }
    else {
        double kasagi = (t_c + 1.0) / 2.0; // 0〜1
        p.x = bx + t_c * 130.0;
        p.y = by - 70.0 - 15.0 * pow(2.0 * kasagi - 1.0, 2.0); // 両端が反り上がる
    }
    return p;
}

// ------------------------------------------------------------
// Layer A：すだれ本体（1サイクルにつき1回だけ生成し、以降は式で変形させ続ける）
// ------------------------------------------------------------
static void ShotTamasudareBody(sEnemyShotSet* pEnemyShotSet)
{
    double bx = pEnemyShotSet->param_d[0];
    double by = pEnemyShotSet->param_d[1];

    if (pEnemyShotSet->count == 0) {
        // 使える効果音: sound_enemyShot_light/medium/heavy/extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->param_i[0] = r;
                pEnemyShot->param_i[1] = c;
                // 玉すだれらしく紅白の小玉（2.5x2.5）を交互に並べる
                pEnemyShot->kind = (c % 2 == 0) ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[6];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int r = pShot->param_i[0];
        int c = pShot->param_i[1];
        double t_r = (double)r / (double)(ROWS - 1);
        double t_c = ((double)c - (COLS - 1) / 2.0) / ((COLS - 1) / 2.0);
        int lc = pShot->count; // 生成からの経過フレーム＝サイクル内経過フレーム

        sPoint pos;

        if (lc < PHASE1_END) {
            // 垂下：行ごとにわずかな時間差を付けて下りてくる
            double p = EaseInOut((lc - t_r * 10.0) / 40.0);
            sPoint start = { bx + t_c * 55.0, by + 40.0 };
            sPoint target = ShapeCurtain(t_r, t_c, bx, by, lc);
            pos = Lerp(start, target, p);
        }
        else if (lc < PHASE2_END) {
            int local2 = lc - PHASE1_END;
            int blockIdx = local2 / SHAPE_BLOCK;   // 0:竿 1:橋 2:旗
            int blockLoc = local2 % SHAPE_BLOCK;

            sPoint fromShape, toShape;
            switch (blockIdx) {
            case 0:
                fromShape = ShapeCurtain(t_r, t_c, bx, by, lc);
                toShape = ShapeRod(t_r, t_c, bx, by);
                break;
            case 1:
                fromShape = ShapeRod(t_r, t_c, bx, by);
                toShape = ShapeBridge(t_r, t_c, bx, by);
                break;
            default:
                fromShape = ShapeBridge(t_r, t_c, bx, by);
                toShape = ShapeFlag(t_r, t_c, bx, by, lc);
                break;
            }

            if (blockLoc < SHAPE_MORPH) {
                double t = EaseInOut((double)blockLoc / (double)SHAPE_MORPH);
                pos = Lerp(fromShape, toShape, t);
            }
            else {
                pos = toShape;
            }
        }
        else if (lc < PHASE3_END) {
            int local3 = lc - PHASE2_END;
            sPoint flagShape = ShapeFlag(t_r, t_c, bx, by, lc);
            sPoint toriiShape = ShapeTorii(t_r, t_c, bx, by);
            if (local3 < TORII_MORPH) {
                double t = EaseInOut((double)local3 / (double)TORII_MORPH);
                pos = Lerp(flagShape, toriiShape, t);
            }
            else {
                pos = toriiShape; // 保持＋予告
            }
        }
        else {
            // 千切れ落ち：鳥居の位置から放射状に加速飛散しつつ重力で落ちる
            int local4 = lc - PHASE3_END;
            sPoint freezePos = ShapeTorii(t_r, t_c, bx, by);
            double angle = atan2(freezePos.y - by, freezePos.x - bx);
            double dist = 1.2 * local4 + 0.05 * local4 * local4;
            pos.x = freezePos.x + dist * cos(angle);
            pos.y = freezePos.y + dist * sin(angle) + 0.03 * local4 * local4;
        }

        pShot->x = pos.x;
        pShot->y = pos.y;
        pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x); // 見た目の向き

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// Layer B：見立ての要所から放つ自機狙い弾（直進・式駆動）
// ------------------------------------------------------------
static void ShotStraight(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double x0 = pShot->param_d[0];
        double y0 = pShot->param_d[1];
        pShot->x = x0 + pShot->speed * cos(pShot->muki) * pShot->count;
        pShot->y = y0 + pShot->speed * sin(pShot->muki) * pShot->count;
        pShot = pShot->next;
    }
}

// x0s/y0s/mukis はそれぞれ長さ n の配列。まとめて1つの sEnemyShotSet として生成する。
static void SpawnStraightGroup(const double* x0s, const double* y0s, const double* mukis,
    int n, double speed, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotStraight;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    for (int i = 0; i < n; i++) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = x0s[i];
        pShot->y = y0s[i];
        pShot->muki = mukis[i];
        pShot->speed = speed;
        pShot->kind = kind;
        pShot->param_d[0] = x0s[i];
        pShot->param_d[1] = y0s[i];

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_NankinTamasudare_Claude()
{
    static double bx0, by0; // このサイクルのすだれ原点
    static int muki;

    if (count == 1) {
        // ゲーム画面は480x480
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        enemy.x += 0.4 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }

    int cycleLocal = (count - 1) % LOOP_FRAMES;

    // --- サイクル開始：すだれ本体を生成 ---
    if (cycleLocal == 0) {
        bx0 = enemy.x;
        by0 = enemy.y;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTamasudareBody;
        pSet->param_d[0] = bx0;
        pSet->param_d[1] = by0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --- 見立て1：釣り竿の穂先から自機狙いの7wayを連続発射 ---
    {
        const int RodHoldStart = PHASE1_END + SHAPE_MORPH;                 // 135
        const int RodHoldEnd = RodHoldStart + SHAPE_HOLD;                 // 210
        const int RodFireInterval = 6;
        const int RodFireN = 7;
        if (cycleLocal >= RodHoldStart && cycleLocal < RodHoldEnd &&
            (cycleLocal - RodHoldStart) % RodFireInterval == 0) {
            sPoint tip = ShapeRod(1.0, 0.0, bx0, by0);
            double baseAngle = atan2(player.y - tip.y, player.x - tip.x);
            double x0s[RodFireN], y0s[RodFireN], mukis[RodFireN];
            double spread = 0.35;
            for (int i = 0; i < RodFireN; i++) {
                x0s[i] = tip.x;
                y0s[i] = tip.y;
                mukis[i] = baseAngle - spread + spread * 2.0 * i / (RodFireN - 1);
            }

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            SpawnStraightGroup(x0s, y0s, mukis, RodFireN, 2.6, img_enemyShotBullet[0]);
        }
    }

    // --- 見立て2：橋のアーチ全体から自機狙い弾が流れるように連続発射 ---
    {
        const int BridgeHoldStart = PHASE1_END + SHAPE_BLOCK + SHAPE_MORPH;     // 255
        const int BridgeHoldEnd = BridgeHoldStart + SHAPE_HOLD;               // 330
        const int BridgeFireInterval = 6;
        const int BridgeFireN = 7;
        if (cycleLocal >= BridgeHoldStart && cycleLocal < BridgeHoldEnd &&
            (cycleLocal - BridgeHoldStart) % BridgeFireInterval == 0) {
            int pulseIdx = (cycleLocal - BridgeHoldStart) / BridgeFireInterval;
            double phaseShift = fmod(pulseIdx * 0.15, 1.0);
            double x0s[BridgeFireN], y0s[BridgeFireN], mukis[BridgeFireN];
            for (int i = 0; i < BridgeFireN; i++) {
                double t_r = fmod((double)i / (BridgeFireN - 1) + phaseShift, 1.0);
                sPoint p = ShapeBridge(t_r, 0.0, bx0, by0);
                x0s[i] = p.x;
                y0s[i] = p.y;
                mukis[i] = atan2(player.y - p.y, player.x - p.x);
            }

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            SpawnStraightGroup(x0s, y0s, mukis, BridgeFireN, 2.4, img_enemyShotBullet[3]);
        }
    }

    // --- 見立て3：旗がはためく波に合わせて左右2箇所から自機狙い3wayを連続発射 ---
    {
        const int FlagHoldStart = PHASE1_END + SHAPE_BLOCK * 2 + SHAPE_MORPH;   // 375
        const int FlagHoldEnd = FlagHoldStart + SHAPE_HOLD;                    // 450
        const int FlagFireInterval = 4;
        if (cycleLocal >= FlagHoldStart && cycleLocal < FlagHoldEnd &&
            (cycleLocal - FlagHoldStart) % FlagFireInterval == 0) {
            int rippleIdx = (cycleLocal - FlagHoldStart) / FlagFireInterval;
            double t_c1 = -1.0 + 2.0 * ((rippleIdx % COLS) / (double)(COLS - 1));
            double t_c2 = -t_c1; // 左右対称の点も同時に使う

            sPoint p1 = ShapeFlag(1.0, t_c1, bx0, by0, cycleLocal);
            sPoint p2 = ShapeFlag(1.0, t_c2, bx0, by0, cycleLocal);

            double x0s[6], y0s[6], mukis[6];
            sPoint pts[2] = { p1, p2 };
            for (int g = 0; g < 2; g++) {
                double baseAngle = atan2(player.y - pts[g].y, player.x - pts[g].x);
                for (int i = 0; i < 3; i++) {
                    int idx = g * 3 + i;
                    x0s[idx] = pts[g].x;
                    y0s[idx] = pts[g].y;
                    mukis[idx] = baseAngle - 0.16 + 0.16 * i;
                }
            }

            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            SpawnStraightGroup(x0s, y0s, mukis, 6, 2.8, img_enemyShotScale[8]);
        }
    }

    // --- 決め：鳥居完成の予告 ---
    if (cycleLocal == TORII_TELEGRAPH_FRAME) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- 決め：鳥居から自機狙い9way＋笠木全幅の弾幕を連続発射する最大火力の締めくくり ---
    {
        const int ToriiFireStart = TORII_BURST_FRAME;         // 520
        const int ToriiFireEnd = PHASE3_END;                 // 550
        const int ToriiFireInterval = 6;
        const int ToriiFireN = 9;
        const int WALL_N = 13;
        if (cycleLocal >= ToriiFireStart && cycleLocal < ToriiFireEnd &&
            (cycleLocal - ToriiFireStart) % ToriiFireInterval == 0) {
            sPoint center = ShapeTorii(1.0, 0.0, bx0, by0);
            double baseAngle = atan2(player.y - center.y, player.x - center.x);
            double x0sN[ToriiFireN], y0sN[ToriiFireN], mukisN[ToriiFireN];
            double spread = 0.5;
            for (int i = 0; i < ToriiFireN; i++) {
                x0sN[i] = center.x;
                y0sN[i] = center.y;
                mukisN[i] = baseAngle - spread + spread * 2.0 * i / (ToriiFireN - 1);
            }

            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            SpawnStraightGroup(x0sN, y0sN, mukisN, ToriiFireN, 2.2, img_enemyShotDiamond[0]);

            double xw[WALL_N], yw[WALL_N], mw[WALL_N];
            for (int i = 0; i < WALL_N; i++) {
                double t_c = -1.0 + 2.0 * i / (double)(WALL_N - 1);
                sPoint p = ShapeTorii(1.0, t_c, bx0, by0);
                xw[i] = p.x;
                yw[i] = p.y;
                mw[i] = DX_PI / 2.0; // 笠木から真下へ
            }
            SpawnStraightGroup(xw, yw, mw, WALL_N, 1.8, img_enemyShotMediumOval[6]);
        }
    }
}
