// ============================================================
//  斬符「残月無明」
//
//  ボスが弾で出来た刀を抜き、斬撃の軌跡を弾幕として残していく。
//   第1幕 抜刀   : 白・青の小玉がボスへ収束する(予告)
//   第2幕 斬撃   : 袈裟斬り → 逆袈裟。軌跡が放射状の弾の壁として残り、外へ解き放たれる
//                  (逆袈裟は袈裟斬りと角度を半ステップずらしてあり、隙間の位置が入れ替わる)
//   第3幕 突き   : 刀全体が1本の弾列として自機へ突進。通った軌跡の白い小玉が左右に割れる
//   第4幕 交差斬り: ボスが左右に移動して斜めに2連斬り。残像の網が交点から全方位へ散華する
//
//  刀の構成(すべて既存弾の組み合わせ)
//   切先 : 菱形弾          刀身 : 銃弾(中心線) + 鱗弾(峰側に1列、1つおき)
//   鍔   : 大玉1 + 中玉4   柄   : 中楕円弾3     柄頭 : 小玉
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数(調整したい値はここ)
// ============================================================
// 弾の色インデックス
enum {
    KCOL_RED = 0, KCOL_YELLOW = 1, KCOL_GREEN = 2, KCOL_CYAN = 3, KCOL_BLUE = 4,
    KCOL_MAGENTA = 5, KCOL_WHITE = 6, KCOL_BLACK = 7, KCOL_ORANGE = 8
};

// 弾の状態(sEnemyShot::param_i[0])
enum {
    KSTATE_NONE = 0,
    KSTATE_TEMP,    // 短命弾(構え・柄・鍔)。param_i[1] = 消滅するShotSetのcount
    KSTATE_TRAIL,   // 斬撃の残像。param_i[2] = 解放を開始するShotSetのcount、param_i[3] = 解放済みフラグ
    KSTATE_THRUST,  // 突きの刀(全弾が同じ加速で直進)
    KSTATE_SPLIT,   // 突きの軌跡の小玉。param_i[2] = 動き出すShotSetのcount
    KSTATE_FLY      // 通常飛翔弾。param_i[1] = 消滅するShotSetのcount(0なら消さない)
};

// 刀の配色
struct KatanaPalette {
    int blade;  // 刀身(銃弾)
    int spine;  // 峰(鱗弾)
    int tip;    // 切先(菱形弾)
};
static const KatanaPalette KATANA_PALETTES[3] = {
    { KCOL_WHITE, KCOL_CYAN,   KCOL_WHITE },  // 0: 白銀(袈裟斬り・逆袈裟・突き)
    { KCOL_RED,   KCOL_ORANGE, KCOL_RED   },  // 1: 紅(交差斬りA)
    { KCOL_BLUE,  KCOL_CYAN,   KCOL_BLUE  },  // 2: 蒼(交差斬りB)
};

// 刀の寸法(ボス中心からの距離)
static constexpr double KATANA_BLADE_INNER = 60.0;   // 刀身の根元
static constexpr double KATANA_BLADE_PITCH = 12.0;   // 刀身の弾の間隔
static constexpr int    KATANA_BLADE_COUNT = 21;     // 刀身の弾の数(先端 = 60 + 12*20 = 300)
static constexpr double KATANA_SPINE_OFFSET = 5.0;    // 峰の鱗弾の横ずれ
static constexpr double KATANA_GUARD_R = 62.0;   // 鍔の位置

// 1周期のタイムライン(フレーム)
static constexpr int KATANA_CYCLE = 600;
static constexpr int KATANA_T_CHARGE = 0;     // 第1幕 予告
static constexpr int KATANA_T_SLASH1 = 40;    // 第2幕 袈裟斬り
static constexpr int KATANA_T_SLASH2 = 100;   //       逆袈裟
static constexpr int KATANA_T_THRUST = 200;   // 第3幕 突き
static constexpr int KATANA_T_CROSS_SE = 280;   // 第4幕 予告音
static constexpr int KATANA_T_MOVE_A = 278;   //       位置Aへ移動開始
static constexpr int KATANA_T_CROSS_A = 300;   //       交差斬りA
static constexpr int KATANA_T_MOVE_B = 342;   //       位置Bへ移動開始
static constexpr int KATANA_T_CROSS_B = 372;   //       交差斬りB
static constexpr int KATANA_T_MOVE_HOME = 410;   //       定位置へ戻る
static constexpr int KATANA_T_BURST = 430;   //       散華(A・B同時に解放)

// ボスの位置
static constexpr double KATANA_HOME_X = 240.0;
static constexpr double KATANA_HOME_Y = 50.0;
static constexpr double KATANA_ARM_A_X = 110.0;
static constexpr double KATANA_ARM_B_X = 370.0;
static constexpr double KATANA_MOVE_RATE = 0.18;
// 交差斬りの交点(散華の中心)
static constexpr double KATANA_CROSS_X = 240.0;
static constexpr double KATANA_CROSS_Y = 230.0;

// 残像の解放(添字0: 袈裟斬り・逆袈裟 = 放射状に外へ、 1: 交差斬り = 交点から全方位へ散華)
static constexpr double KATANA_REL_V0[2] = { 0.5, 0.8 };
static constexpr double KATANA_REL_ACC[2] = { 0.07, 0.09 };
static constexpr double KATANA_REL_VMAX[2] = { 4.2, 3.8 };
static constexpr int    KATANA_REL_STAGGER = 1;       // 放射状解放のとき、1列ごとにずらすフレーム数

// 予告(収束リング)
static constexpr int    KATANA_CHARGE_FRAMES = 40;

// 突き
static constexpr int    KATANA_THRUST_WINDUP = 34;   // 構え(追尾→固定)の長さ
static constexpr int    KATANA_THRUST_LOCK = 10;   // 発射の何フレーム前に狙いを固定するか
static constexpr double KATANA_THRUST_TURN = 0.045;// 構え中の追尾旋回速度(rad/frame)
static constexpr double KATANA_THRUST_V0 = 1.0;
static constexpr double KATANA_THRUST_ACC = 0.6;
static constexpr double KATANA_THRUST_VMAX = 9.0;
static constexpr int    KATANA_THRUST_TRAIL_FRAMES = 60;   // 軌跡の小玉を置き続けるフレーム数
static constexpr int    KATANA_THRUST_TRAIL_INTERVAL = 2;    // 何フレームおきに置くか
static constexpr int    KATANA_SPLIT_WAIT = 22;   // 小玉が割れ始めるまでの待ち
static constexpr double KATANA_SPLIT_V0 = 0.2;
static constexpr double KATANA_SPLIT_ACC = 0.08;
static constexpr double KATANA_SPLIT_VMAX = 3.4;
static constexpr double KATANA_SPLIT_LEAN = 0.30;  // 2個目の弾の斜め具合(rad)

// ============================================================
//  共通ヘルパー
// ============================================================
static inline double Deg2Rad(double deg)
{
    return deg * DX_PI / 180.0;
}

static double NormalizeAngle(double a)
{
    while (a > DX_PI)  a -= 2.0 * DX_PI;
    while (a < -DX_PI) a += 2.0 * DX_PI;
    return a;
}

static void PlaySE(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ShotSetを作ってリストに繋ぐ
static sEnemyShotSet* CreateShotSet(double x, double y, double muki, int kind, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// ShotSetに弾を1つ追加する
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->margin = 120;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

static inline void SetShotState(sEnemyShot* pShot, int state, int a, int b)
{
    pShot->param_i[0] = state;
    pShot->param_i[1] = a;
    pShot->param_i[2] = b;
}

// 弾をリストから外して解放する(呼び出し側で先にnextを退避しておくこと)
static void RemoveShot(sEnemyShot* pShot)
{
    pShot->prev->next = pShot->next;
    pShot->next->prev = pShot->prev;
    delete pShot;
}

static inline void MoveShot(sEnemyShot* pShot, double speed)
{
    pShot->speed = speed;
    pShot->x += speed * cos(pShot->muki);
    pShot->y += speed * sin(pShot->muki);
}

// ============================================================
//  刀の描画(弾の配置)
// ============================================================
// 刀身1列分。theta方向にボス中心から切先まで並べる。
// sideSign: 峰を置く側(+1なら、thetaが増える向きに振るとき進行方向の反対側)
static void SpawnBladeRow(sEnemyShotSet* pSet, double theta, double sideSign,
    const KatanaPalette& pal, int state, int a, int b)
{
    const double ox = pSet->x, oy = pSet->y;
    const double cs = cos(theta), sn = sin(theta);
    const double sx = sideSign * sn;    // 峰側への単位ベクトル
    const double sy = -sideSign * cs;

    for (int i = 0; i < KATANA_BLADE_COUNT; i++) {
        const double r = KATANA_BLADE_INNER + KATANA_BLADE_PITCH * i;
        sEnemyShot* pShot = AddShot(pSet, ox + r * cs, oy + r * sn, theta, 0.0,
            img_enemyShotBullet[pal.blade]);
        SetShotState(pShot, state, a, b);
        if (i % 2 == 0) {
            pShot = AddShot(pSet, ox + r * cs + sx * KATANA_SPINE_OFFSET,
                oy + r * sn + sy * KATANA_SPINE_OFFSET, theta, 0.0,
                img_enemyShotScale[pal.spine]);
            SetShotState(pShot, state, a, b);
        }
    }
    // 切先
    const double rt = KATANA_BLADE_INNER + KATANA_BLADE_PITCH * KATANA_BLADE_COUNT;
    sEnemyShot* pTip = AddShot(pSet, ox + rt * cs, oy + rt * sn, theta, 0.0,
        img_enemyShotDiamond[pal.tip]);
    SetShotState(pTip, state, a, b);
}

// 柄・鍔
static void SpawnHilt(sEnemyShotSet* pSet, double theta, int state, int a, int b)
{
    const double ox = pSet->x, oy = pSet->y;
    const double cs = cos(theta), sn = sin(theta);
    const double lx = -sn, ly = cs;     // 刀と直交する向き
    sEnemyShot* pShot;

    // 柄頭(赤の小玉)
    pShot = AddShot(pSet, ox + 10.0 * cs, oy + 10.0 * sn, theta, 0.0, img_enemyShotSmallBall[KCOL_RED]);
    SetShotState(pShot, state, a, b);

    // 柄(黒の中楕円弾 x3)
    static const double gripR[3] = { 22.0, 34.0, 46.0 };
    for (int i = 0; i < 3; i++) {
        pShot = AddShot(pSet, ox + gripR[i] * cs, oy + gripR[i] * sn, theta, 0.0,
            img_enemyShotMediumOval[KCOL_BLACK]);
        SetShotState(pShot, state, a, b);
    }

    // 鍔(中央に黄の大玉、左右に橙の中玉を2つずつ)
    const double gx = ox + KATANA_GUARD_R * cs;
    const double gy = oy + KATANA_GUARD_R * sn;
    pShot = AddShot(pSet, gx, gy, theta, 0.0, img_enemyShotLargeBall[KCOL_YELLOW]);
    SetShotState(pShot, state, a, b);
    static const double guardLat[4] = { -38.0, -24.0, 24.0, 38.0 };
    for (int i = 0; i < 4; i++) {
        pShot = AddShot(pSet, gx + lx * guardLat[i], gy + ly * guardLat[i], theta, 0.0,
            img_enemyShotMediumBall[KCOL_ORANGE]);
        SetShotState(pShot, state, a, b);
    }
}

// ============================================================
//  第1幕: 予告(収束リング)
// ============================================================
static void ShotCharge(sEnemyShotSet* pSet)
{
    const int c = pSet->count;

    if (c == 0) {
        static const double ringR[2] = { 90.0, 130.0 };
        const int n = 36;
        for (int ring = 0; ring < 2; ring++) {
            const int color = (ring == 0) ? KCOL_WHITE : KCOL_BLUE;
            for (int i = 0; i < n; i++) {
                const double ang = 2.0 * DX_PI * i / n + ring * DX_PI / n;
                sEnemyShot* pShot = AddShot(pSet,
                    pSet->x + ringR[ring] * cos(ang), pSet->y + ringR[ring] * sin(ang),
                    ang + DX_PI, ringR[ring] / KATANA_CHARGE_FRAMES,    // 中心へ向かう
                    img_enemyShotSmallBall[color]);
                SetShotState(pShot, KSTATE_FLY, KATANA_CHARGE_FRAMES, 0);
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        if (pShot->param_i[1] > 0 && c >= pShot->param_i[1]) {
            RemoveShot(pShot);
        }
        else {
            MoveShot(pShot, pShot->speed);
        }
        pShot = pNext;
    }
}

// ============================================================
//  第2幕・第4幕: 斬撃
//   param_i[0] 振りのステップ数   param_i[1] 構えのフレーム数
//   param_i[2] 残像の解放フレーム param_i[3] 0:放射状に解放 1:散華(交点から全方位)
//   param_d[0] 開始角  param_d[1] 1ステップの角度(符号付き)
//   param_d[2], param_d[3] 散華の中心座標   kind: 配色(KATANA_PALETTES)
// ============================================================
static void ShotSlash(sEnemyShotSet* pSet)
{
    const int c = pSet->count;
    const int steps = pSet->param_i[0];
    const int windup = pSet->param_i[1];
    const int release = pSet->param_i[2];
    const int mode = pSet->param_i[3];
    const double theta0 = pSet->param_d[0];
    const double dTheta = pSet->param_d[1];
    const double burstX = pSet->param_d[2];
    const double burstY = pSet->param_d[3];
    const KatanaPalette& pal = KATANA_PALETTES[pSet->kind];
    const double sideSign = (dTheta >= 0.0) ? 1.0 : -1.0;

    // --- 既存弾の更新 ---
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        switch (pShot->param_i[0]) {
        case KSTATE_TEMP:
            if (c >= pShot->param_i[1]) RemoveShot(pShot);
            break;
        case KSTATE_TRAIL: {
            const int age = c - pShot->param_i[2];
            if (age < 0) break;                     // 解放待ち(その場に静止)
            if (pShot->param_i[3] == 0) {           // 解放の瞬間に一度だけ向きを決める
                pShot->param_i[3] = 1;
                if (mode == 1) {
                    // 散華: 交点から見て外向きへ
                    pShot->muki = atan2(pShot->y - burstY, pShot->x - burstX);
                }
            }
            double v = KATANA_REL_V0[mode] + KATANA_REL_ACC[mode] * age;
            if (v > KATANA_REL_VMAX[mode]) v = KATANA_REL_VMAX[mode];
            MoveShot(pShot, v);
            break;
        }
        default:
            break;
        }
        pShot = pNext;
    }

    // --- 構え ---
    if (c == 0) {
        SpawnHilt(pSet, theta0, KSTATE_TEMP, windup, 0);
        SpawnBladeRow(pSet, theta0, sideSign, pal, KSTATE_TEMP, windup, 0);
    }

    // --- 振り(1フレームに1ステップ。刀の通った跡に刀身1列分を残像として残す) ---
    const int k = c - windup;
    if (k >= 0 && k < steps) {
        const double theta = theta0 + dTheta * k;
        const int relFrame = (mode == 0) ? (release + k * KATANA_REL_STAGGER) : release;
        SpawnHilt(pSet, theta, KSTATE_TEMP, c + 1, 0);     // 柄と鍔は今のフレームだけ見せる
        SpawnBladeRow(pSet, theta, sideSign, pal, KSTATE_TRAIL, k, relFrame);
        if (k == 0)          PlaySE(sound_enemyShot_heavy);
        else if (k % 6 == 0) PlaySE(sound_enemyShot_light);
    }

    // --- 解放の合図 ---
    if (c == release) {
        PlaySE(mode == 0 ? sound_enemyShot_medium : sound_enemyShot_heavy);
    }
}

// ============================================================
//  第3幕: 突き
//   param_i[0] 構えのフレーム数   param_d[0] 刀の向き   param_d[1] 刀の進んだ距離
// ============================================================
static inline double ThrustSpeed(int age)
{
    double v = KATANA_THRUST_V0 + KATANA_THRUST_ACC * age;
    return (v > KATANA_THRUST_VMAX) ? KATANA_THRUST_VMAX : v;
}

static void ShotThrust(sEnemyShotSet* pSet)
{
    const int c = pSet->count;
    const int windup = pSet->param_i[0];
    const KatanaPalette& pal = KATANA_PALETTES[0];

    // --- 既存弾の更新 ---
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        switch (pShot->param_i[0]) {
        case KSTATE_TEMP:
            if (c >= pShot->param_i[1]) RemoveShot(pShot);
            break;
        case KSTATE_THRUST:
            MoveShot(pShot, ThrustSpeed(c - windup));
            break;
        case KSTATE_SPLIT: {
            const int age = c - pShot->param_i[2];
            if (age < 0) break;                     // 割れ始めるまで静止
            double v = KATANA_SPLIT_V0 + KATANA_SPLIT_ACC * age;
            if (v > KATANA_SPLIT_VMAX) v = KATANA_SPLIT_VMAX;
            MoveShot(pShot, v);
            break;
        }
        default:
            break;
        }
        pShot = pNext;
    }

    // --- 構え(自機を追尾し、発射の少し前に狙いを固定) ---
    if (c < windup) {
        if (c < windup - KATANA_THRUST_LOCK) {
            double diff = NormalizeAngle(atan2(player.y - pSet->y, player.x - pSet->x) - pSet->param_d[0]);
            if (diff > KATANA_THRUST_TURN)       diff = KATANA_THRUST_TURN;
            else if (diff < -KATANA_THRUST_TURN) diff = -KATANA_THRUST_TURN;
            pSet->param_d[0] += diff;
        }
        const double theta = pSet->param_d[0];
        SpawnHilt(pSet, theta, KSTATE_TEMP, c + 1, 0);
        SpawnBladeRow(pSet, theta, 1.0, pal, KSTATE_TEMP, c + 1, 0);
    }
    // --- 発射: 刀全体を弾列としてそのまま突進させる ---
    else if (c == windup) {
        const double theta = pSet->param_d[0];
        SpawnHilt(pSet, theta, KSTATE_THRUST, 0, 0);
        SpawnBladeRow(pSet, theta, 1.0, pal, KSTATE_THRUST, 0, 0);
        PlaySE(sound_enemyShot_heavy);
    }

    // --- 軌跡: 鍔の通った位置に白の小玉を置き、少し待ってから左右に割る ---
    if (c >= windup && c < windup + KATANA_THRUST_TRAIL_FRAMES) {
        if (c > windup) pSet->param_d[1] += ThrustSpeed(c - windup);
        if ((c - windup) % KATANA_THRUST_TRAIL_INTERVAL == 0) {
            const double theta = pSet->param_d[0];
            const double gx = pSet->x + (KATANA_GUARD_R + pSet->param_d[1]) * cos(theta);
            const double gy = pSet->y + (KATANA_GUARD_R + pSet->param_d[1]) * sin(theta);
            const int idx = (c - windup) / KATANA_THRUST_TRAIL_INTERVAL;
            const double lean = (idx % 2 == 0) ? KATANA_SPLIT_LEAN : -KATANA_SPLIT_LEAN;
            for (int side = -1; side <= 1; side += 2) {
                const double split = theta + side * DX_PI / 2.0;
                // 真横へ割れる弾
                sEnemyShot* p1 = AddShot(pSet, gx, gy, split, 0.0, img_enemyShotSmallBall[KCOL_WHITE]);
                SetShotState(p1, KSTATE_SPLIT, 0, c + KATANA_SPLIT_WAIT);
                // やや後ろ向きに割れる弾(少し遅れて動き出す)
                sEnemyShot* p2 = AddShot(pSet, gx, gy, split + side * lean, 0.0, img_enemyShotSmallBall[KCOL_WHITE]);
                SetShotState(p2, KSTATE_SPLIT, 0, c + KATANA_SPLIT_WAIT + 8);
            }
        }
    }
}

// ============================================================
//  第4幕: 散華の輪(交点から広がる)
// ============================================================
static void ShotBurst(sEnemyShotSet* pSet)
{
    const int c = pSet->count;

    if (c == 0) {
        PlaySE(sound_enemyShot_extreme);
        const double base = GetRand(359) / 180.0 * DX_PI;   // リプレイ再現性あり
        const int nMid = 24, nSmall = 36;
        for (int i = 0; i < nMid; i++) {
            const double ang = base + 2.0 * DX_PI * i / nMid;
            sEnemyShot* p = AddShot(pSet, pSet->x, pSet->y, ang, 1.8, img_enemyShotMediumBall[KCOL_YELLOW]);
            SetShotState(p, KSTATE_FLY, 0, 0);
        }
        for (int i = 0; i < nSmall; i++) {
            const double ang = base + DX_PI / nSmall + 2.0 * DX_PI * i / nSmall;
            sEnemyShot* p = AddShot(pSet, pSet->x, pSet->y, ang, 2.6, img_enemyShotSmallBall[KCOL_WHITE]);
            SetShotState(p, KSTATE_FLY, 0, 0);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        MoveShot(pShot, pShot->speed);
        pShot = pShot->next;
    }
}

// ============================================================
//  生成用ヘルパー
// ============================================================
static void CreateSlash(double x, double y, int palette,
    double theta0, double dTheta, int steps, int windup,
    int release, int mode, double burstX, double burstY)
{
    sEnemyShotSet* pSet = CreateShotSet(x, y, theta0, palette, ShotSlash);
    pSet->param_i[0] = steps;
    pSet->param_i[1] = windup;
    pSet->param_i[2] = release;
    pSet->param_i[3] = mode;
    pSet->param_d[0] = theta0;
    pSet->param_d[1] = dTheta;
    pSet->param_d[2] = burstX;
    pSet->param_d[3] = burstY;
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Sword_Claude()
{
    static double s_aim;    // 袈裟斬りと逆袈裟で共有する基準角(隙間の格子を揃えるため)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = KATANA_HOME_X;
        enemy.y = KATANA_HOME_Y;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        s_aim = DX_PI / 2.0;
    }

    const int t = (count - 1) % KATANA_CYCLE;

    // --- ボスの移動 ---
    double tx = KATANA_HOME_X;
    if (t >= KATANA_T_MOVE_A && t < KATANA_T_MOVE_B)         tx = KATANA_ARM_A_X;
    else if (t >= KATANA_T_MOVE_B && t < KATANA_T_MOVE_HOME) tx = KATANA_ARM_B_X;
    enemy.x += (tx - enemy.x) * KATANA_MOVE_RATE;
    enemy.y += (KATANA_HOME_Y - enemy.y) * KATANA_MOVE_RATE;

    // --- 第1幕: 抜刀の予告 ---
    if (t == KATANA_T_CHARGE) {
        PlaySE(sound_enemyCharge);
        CreateShotSet(enemy.x, enemy.y, 0.0, 0, ShotCharge);
    }

    // --- 第2幕: 袈裟斬り(角度が増える向きに約104度) ---
    if (t == KATANA_T_SLASH1) {
        s_aim = atan2(player.y - enemy.y, player.x - enemy.x);
        const int steps = 27, windup = 20, hold = 30;
        CreateSlash(enemy.x, enemy.y, 0,
            s_aim - Deg2Rad(52.0), Deg2Rad(4.0), steps, windup,
            windup + steps + hold, 0, 0.0, 0.0);
    }
    // --- 第2幕: 逆袈裟(反対向き。袈裟斬りと半ステップずれた角度に残像が残る) ---
    if (t == KATANA_T_SLASH2) {
        const int steps = 26, windup = 14, hold = 24;
        CreateSlash(enemy.x, enemy.y, 0,
            s_aim + Deg2Rad(50.0), -Deg2Rad(4.0), steps, windup,
            windup + steps + hold, 0, 0.0, 0.0);
    }

    // --- 第3幕: 突き ---
    if (t == KATANA_T_THRUST) {
        PlaySE(sound_enemyCharge);
        const double aim = atan2(player.y - enemy.y, player.x - enemy.x);
        sEnemyShotSet* pSet = CreateShotSet(enemy.x, enemy.y, aim, 0, ShotThrust);
        pSet->param_i[0] = KATANA_THRUST_WINDUP;
        pSet->param_d[0] = aim;
        pSet->param_d[1] = 0.0;
    }

    // --- 第4幕: 交差斬り ---
    if (t == KATANA_T_CROSS_SE) {
        PlaySE(sound_enemyCharge);
    }
    if (t == KATANA_T_CROSS_A) {
        // 左上から交点へ向けて、扇状に約69度(紅)
        const double center = atan2(KATANA_CROSS_Y - enemy.y, KATANA_CROSS_X - enemy.x);
        const int steps = 24, windup = 18;
        CreateSlash(enemy.x, enemy.y, 1,
            center - Deg2Rad(34.5), Deg2Rad(3.0*2), steps, windup,
            KATANA_T_BURST - KATANA_T_CROSS_A, 1, KATANA_CROSS_X, KATANA_CROSS_Y);
    }
    if (t == KATANA_T_CROSS_B) {
        // 右上から交点へ向けて、反対向きに約69度(蒼)
        const double center = atan2(KATANA_CROSS_Y - enemy.y, KATANA_CROSS_X - enemy.x);
        const int steps = 24, windup = 14;
        CreateSlash(enemy.x, enemy.y, 2,
            center + Deg2Rad(34.5), -Deg2Rad(3.0*2), steps, windup,
            KATANA_T_BURST - KATANA_T_CROSS_B, 1, KATANA_CROSS_X, KATANA_CROSS_Y);
    }
    // --- 散華: 残像の網が一斉に交点から外へ弾ける(同時に交点から輪を放つ) ---
    if (t == KATANA_T_BURST) {
        CreateShotSet(KATANA_CROSS_X, KATANA_CROSS_Y, 0.0, 0, ShotBurst);
    }
}