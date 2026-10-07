#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕「三態輪廻」 ― 固体 → 液体 → 気体 → 再凝固
//
//   1ShotSet(= 1サイクル分)が全弾を保持し、弾ごとの状態遷移で相転移を表現する。
//   フェーズをまたいでも弾が0にならないので、ShotSetが途中で消えることはない。
//
//   t = pEnemyShotSet->count (ShotSet生成からのフレーム数)
//     0〜 : ①固体  六角格子(菱形弾=格子点、小玉=結合)がゆっくり降下し熱振動が増大
//   240〜 : ②融解  上から順に格子が崩れ、2つの渦の流れ場に沿う液体になる
//   480〜 : ③沸騰  大玉の気泡が上昇して破裂、小玉の蒸気を放出。液体も順次蒸発
//   780〜 : ④気体  全弾が高速直進＋壁反射(最大2回)。短レーザーも混ざる
//   960〜 : ⑤凝華  全弾が急停止して青い菱形弾(結晶)になり、1020〜に自機狙いで加速
//
//   格子には自機のいる位置に「欠陥レーン」(弾の無い縦の通路)を空けてある。
// ============================================================

// ---------- タイムライン ----------
static const int kCycleLen = 1300-200;  // 1サイクルのフレーム数
static const int kPreWait = 60;    // サイクル開始〜格子生成までの予告時間
static const int kTMelt = 240;   // 融解開始
static const int kTBoil = 480;   // 沸騰開始(気泡の発生開始)
static const int kTBubbleEnd = 720;   // 気泡の発生終了
static const int kTGas = 780;   // 気体フェーズ(全弾が蒸発済みになる)
static const int kTLaserEnd = 930;   // 短レーザー発射終了＆凝華の予告音
static const int kTFreeze = 960;   // 凝華(急停止)
static const int kTLaunch = 1020;  // 結晶弾の発射開始

// ---------- 弾の状態 (param_i[0]) ----------
enum {
    kStSolid = 0,   // 格子として振動中
    kStLiquid,      // 液体(流れ場に追従)
    kStBubble,      // 気泡(大玉)
    kStGas,         // 気体(直進＋壁反射)
    kStFrozen,      // 凝華中(減速→静止)
    kStLaunch,      // 結晶弾の加速中
    kStLaser        // 短レーザー(直進のみ)
};

// ---------- 弾の形 (param_i[1]) ----------
enum {
    kShSmall = 0,   // 小玉
    kShMedium,      // 中玉
    kShLarge,       // 大玉
    kShOval,        // 中楕円弾
    kShDiamond,     // 菱形弾
    kShLaser        // 短レーザー
};

// ---------- 弾ごとのパラメータの使い道 ----------
//  param_i[0] : 状態(kSt*)
//  param_i[1] : 形(kSh*)
//  param_i[2] : 次の状態遷移時刻(固体=融解時刻 / 液体=蒸発時刻 / 気泡=破裂時刻 / 凝華=発射時刻)
//  param_i[3] : 壁での反射回数
//  param_i[4] : 気体になった時刻
//  param_i[5] : 格子での役割(0=格子点、1=結合)
//  param_d[0],[1] : 格子上の基準位置(降下前)
//  param_d[2],[3] : 熱振動の位相 / 気泡の揺れの位相
//  param_d[4],[5] : 速度ベクトル(vx, vy)
//  param_d[6] : 液体の流れへの追従率
//  param_d[7] : 気体の目標速さ

// 弾の形と色から画像ハンドルを得る
static int ShotImage(int shape, int color)
{
    switch (shape) {
    case kShSmall:   return img_enemyShotSmallBall[color];
    case kShMedium:  return img_enemyShotMediumBall[color];
    case kShLarge:   return img_enemyShotLargeBall[color];
    case kShOval:    return img_enemyShotMediumOval[color];
    case kShDiamond: return img_enemyShotDiamond[color];
    default:         return img_enemyShotLaser[color];
    }
}

// 弾の形と色を変更する(相転移で見た目を切り替える)
static void SetShape(sEnemyShot* p, int shape, int color)
{
    p->param_i[1] = shape;
    p->kind = ShotImage(shape, color);
}

// 速度ベクトルを設定し、speed と muki(進行方向)も揃える
static void SetVelocity(sEnemyShot* p, double vx, double vy)
{
    p->param_d[4] = vx;
    p->param_d[5] = vy;
    p->speed = sqrt(vx * vx + vy * vy);
    if (p->speed > 1e-6) p->muki = atan2(vy, vx);
}

// 弾を1発追加する(リストの末尾に連結)
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, int shape, int color)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = 0.0;
    SetShape(p, shape, color);
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 効果音を鳴らす(再生中なら止めてから鳴らし直す)
static void PlaySE(int sound)
{
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);
}

// 蒸気・気体の弾の色(赤・橙・黄)
static int GasColor()
{
    static const int colors[3] = { 0, 8, 1 };
    return colors[GetRand(2)];
}

// 液体の流れ場: 逆回転する2つの渦(中央は下向きの噴流になる)
static void FlowField(double x, double y, double* pfx, double* pfy)
{
    static const double cx[2] = { 130.0, 350.0 };
    static const double cy[2] = { 300.0, 300.0 };
    static const double sg[2] = { 1.0, -1.0 };
    const double strength = 110.0;
    const double coreSq = 60.0 * 60.0;
    double fx = 0.0, fy = 0.0;
    for (int i = 0; i < 2; i++) {
        double dx = x - cx[i];
        double dy = y - cy[i];
        double d2 = dx * dx + dy * dy + coreSq;
        fx += sg[i] * strength * (-dy) / d2;
        fy += sg[i] * strength * dx / d2;
    }
    *pfx = fx;
    *pfy = fy;
}

// ============================================================
// ① 固体: 六角格子の生成
// ============================================================
static void AddLatticeShot(sEnemyShotSet* pSet, double hx, double hy, double muki, int shape, int color, int role)
{
    sEnemyShot* p = AddShot(pSet, hx, hy, muki, shape, color);
    p->speed = 0.8;
    p->param_i[0] = kStSolid;
    p->param_i[5] = role;
    // 融解時刻: 上の弾から順に溶ける(融解前線が上から下へ約70フレームで進む)＋ばらつき
    p->param_i[2] = kTMelt + (int)((hy - 60.0) / 190.0 * 70.0) + GetRand(12);
    p->param_d[0] = hx;
    p->param_d[1] = hy;
    p->param_d[2] = GetRand(628) / 100.0;
    p->param_d[3] = GetRand(628) / 100.0;
}

static void SpawnLattice(sEnemyShotSet* pSet)
{
    const double spacing = 22.0;
    const double rowH = spacing * 0.8660254;  // 六角格子の行間隔
    const double x0 = 24.0;
    const double y0 = 60.0;
    const int cols = 20;
    const int rows = 10;
    const double laneHalf = 30.0;             // 自機位置に空ける欠陥レーンの半幅

    // 格子点(c, r)が存在するか(欠陥レーンの内側は存在しない)
    auto nodeX = [&](int c, int r) { return x0 + c * spacing + (r & 1) * spacing * 0.5; };
    auto exists = [&](int c, int r) {
        if (c < 0 || c >= cols || r < 0 || r >= rows) return false;
        return fabs(nodeX(c, r) - player.x) >= laneHalf;
    };

    for (int r = 0; r < rows; r++) {
        double y = y0 + r * rowH;
        for (int c = 0; c < cols; c++) {
            if (!exists(c, r)) continue;
            double x = nodeX(c, r);

            // 格子点: 菱形弾(青)。3本の格子軸のどれかに向ける
            AddLatticeShot(pSet, x, y, DX_PI / 3.0 * ((c + r) % 3), kShDiamond, 4, 0);

            // 結合: 隣の格子点との中点に小玉(シアン)。向きは結合方向
            if (exists(c + 1, r)) {
                AddLatticeShot(pSet, x + spacing * 0.5, y, 0.0, kShSmall, 3, 1);
            }
            int dlc = (r & 1) ? c : c - 1;       // 下左の隣
            int drc = (r & 1) ? c + 1 : c;       // 下右の隣
            if (exists(dlc, r + 1)) {
                AddLatticeShot(pSet, x - spacing * 0.25, y + rowH * 0.5, DX_PI * 2.0 / 3.0, kShSmall, 3, 1);
            }
            if (exists(drc, r + 1)) {
                AddLatticeShot(pSet, x + spacing * 0.25, y + rowH * 0.5, DX_PI / 3.0, kShSmall, 3, 1);
            }
        }
    }
}

// 固体の更新: ゆっくり降下しつつ、時間とともに大きくなる熱振動
static void UpdateSolid(sEnemyShot* p, int t)
{
    double ratio = t / (double)kTMelt;
    double amp = 0.3 + 3.6 * ratio * ratio;
    if (amp > 6.0) amp = 6.0;
    p->x = p->param_d[0] + amp * sin(t * 0.55 + p->param_d[2]);
    p->y = p->param_d[1] + 0.8 * t + amp * sin(t * 0.47 + p->param_d[3]);
    // muki は格子軸のまま維持する
}

// ============================================================
// ② 液体
// ============================================================
static void MeltShot(sEnemyShot* p)
{
    p->param_i[0] = kStLiquid;
    p->param_i[2] = 500 + GetRand(280);                    // 蒸発時刻(500〜780)
    p->param_d[6] = 0.03 + GetRand(30) / 1000.0;           // 流れへの追従率(粘性のばらつき)
    if (p->param_i[5] == 0) {
        SetShape(p, kShOval, 4);                           // 格子点 → 中楕円弾(青)
    }
    else if (GetRand(7) == 0) {
        SetShape(p, kShMedium, 2);                         // 結合の一部 → 中玉(緑)
    }
    else {
        SetShape(p, kShSmall, GetRand(1) ? 3 : 2);         // 結合 → 小玉(シアン/緑)
    }
    // 降下中の速度を引き継ぐ
    SetVelocity(p, (GetRand(60) - 30) / 100.0, 0.8 + (GetRand(20) - 10) / 100.0);
}

static void UpdateLiquid(sEnemyShot* p)
{
    double fx, fy;
    FlowField(p->x, p->y, &fx, &fy);

    // 自機に近いほど流れが遅くなり、自機の周りに液体が溜まる
    double dx = p->x - player.x;
    double dy = p->y - player.y;
    double dist = sqrt(dx * dx + dy * dy);
    double factor = 0.35 + 0.65 * (dist >= 100.0 ? 1.0 : dist / 100.0);

    double relax = p->param_d[6];
    double vx = p->param_d[4];
    double vy = p->param_d[5];
    vx += (fx * factor - vx) * relax;
    vy += (fy * factor + 0.15 - vy) * relax;               // +0.15 は重力による下向きのドリフト
    p->x += vx;
    p->y += vy;
    SetVelocity(p, vx, vy);                                // 楕円弾が流線方向を向く
}

// ============================================================
// ③ 沸騰: 気泡
// ============================================================
static void SpawnBubble(sEnemyShotSet* pSet, int t)
{
    double bx = 240.0, by = 300.0;
    for (int k = 0; k < 6; k++) {
        bx = 40.0 + GetRand(400);
        by = 220.0 + GetRand(200);
        double dx = bx - player.x;
        double dy = by - player.y;
        if (dx * dx + dy * dy > 100.0 * 100.0) break;      // 自機の真上には出さない
    }
    sEnemyShot* p = AddShot(pSet, bx, by, -DX_PI / 2.0, kShLarge, 6);
    p->param_i[0] = kStBubble;
    p->param_i[2] = t + 70 + GetRand(50);                  // 破裂時刻
    p->param_d[2] = GetRand(628) / 100.0;
    SetVelocity(p, 0.0, -(0.5 + GetRand(50) / 100.0));     // ゆっくり上昇
}

static void UpdateBubble(sEnemyShot* p)
{
    double vx = 0.4 * sin(p->count * 0.08 + p->param_d[2]); // 左右に揺れながら昇る
    double vy = p->param_d[5];
    p->x += vx;
    p->y += vy;
    SetVelocity(p, vx, vy);
}

// 弾を気体にする(速度・色・形は呼び出し側で決めた ang, spd を使う)
static void ToGas(sEnemyShot* p, int t, double ang, double spd)
{
    p->param_i[0] = kStGas;
    p->param_i[3] = 0;
    p->param_i[4] = t;
    p->param_d[7] = spd;
    SetVelocity(p, spd * cos(ang), spd * sin(ang));
}

// 気泡の破裂: その場で小玉に変わり、周囲に蒸気の小玉を二重のリング状に放出
static void BurstBubble(sEnemyShotSet* pSet, sEnemyShot* p, int t)
{
    PlaySE(sound_enemyShot_light);
    const int n = 22 + GetRand(8)-20;
    const double base = GetRand(628) / 100.0;
    for (int i = 0; i < n; i++) {
        double ang = base + 2.0 * DX_PI * i / n;
        double spd = 1.5 + (i % 2) * 0.9 + GetRand(30) / 100.0;
        sEnemyShot* q = AddShot(pSet, p->x, p->y, ang, kShSmall, GasColor());
        ToGas(q, t, ang, spd);
    }
    SetShape(p, kShSmall, GasColor());
    ToGas(p, t, base, 1.8);
}

// ============================================================
// ③④ 気体
// ============================================================
// 液体の蒸発: 進んでいた向きにばらつきを加えて気体の弾にする
static void EvaporateShot(sEnemyShot* p, int t)
{
    double vx = p->param_d[4];
    double vy = p->param_d[5];
    double ang;
    if (vx * vx + vy * vy > 0.01) ang = atan2(vy, vx) + (GetRand(100) - 50) / 100.0;
    else                          ang = GetRand(628) / 100.0;
    SetShape(p, kShSmall, GasColor());
    ToGas(p, t, ang, 1.4 + GetRand(120) / 100.0);
}

static void UpdateGas(sEnemyShot* p, int t)
{
    double vx = p->param_d[4];
    double vy = p->param_d[5];

    // 気体になった直後(約120フレーム)だけ流れ場の影響を受け、次第に直進性が高くなる
    double w = 1.0 - (t - p->param_i[4]) / 120.0;
    if (w > 0.0) {
        double fx, fy;
        FlowField(p->x, p->y, &fx, &fy);
        vx += (fx - vx) * 0.04 * w;
        vy += (fy - vy) * 0.04 * w;
    }

    // 目標の速さへ徐々に近づける(気体フェーズでは加熱されてさらに速くなる)
    double target = p->param_d[7] * (t >= kTGas ? 1.3 : 1.0);
    double s = sqrt(vx * vx + vy * vy);
    if (s > 1e-6) {
        double ns = s + (target - s) * 0.06;
        vx *= ns / s;
        vy *= ns / s;
    }

    p->x += vx;
    p->y += vy;

    // 容器の壁での反射(最大2回。それ以降は画面外へ出て行く)
    if (p->param_i[3] < 2) {
        if (p->x < 6.0) { p->x = 12.0 - p->x;  vx = fabs(vx);  p->param_i[3]++; }
        else if (p->x > 474.0) { p->x = 948.0 - p->x; vx = -fabs(vx); p->param_i[3]++; }
    }
    if (p->param_i[3] < 2) {
        if (p->y < 6.0) { p->y = 12.0 - p->y;  vy = fabs(vy);  p->param_i[3]++; }
        //else if (p->y > 474.0) { p->y = 948.0 - p->y; vy = -fabs(vy); p->param_i[3]++; }
    }
    SetVelocity(p, vx, vy);
}

// 高エネルギー粒子: 画面の縁から自機付近へ飛ぶ短レーザー
static void SpawnLaserVolley(sEnemyShotSet* pSet)
{
    for (int i = 0; i < 4; i++) {
        double lx, ly;
        switch (GetRand(2)) {
        case 0:  lx = 30.0 + GetRand(420); ly = 10.0;                break;  // 上辺
        case 1:  lx = 10.0;                ly = 40.0 + GetRand(300); break;  // 左辺
        default: lx = 470.0;               ly = 40.0 + GetRand(300); break;  // 右辺
        }
        double ang = atan2(player.y + (GetRand(80) - 40) - ly, player.x + (GetRand(80) - 40) - lx);
        sEnemyShot* p = AddShot(pSet, lx, ly, ang, kShLaser, 6);
        p->param_i[0] = kStLaser;
        SetVelocity(p, 3.6 * cos(ang), 3.6 * sin(ang));
    }
}

// ============================================================
// ⑤ 凝華: 急停止して青い結晶になり、自機狙いで飛び出す
// ============================================================
static void FreezeShot(sEnemyShot* p)
{
    p->param_i[0] = kStFrozen;
    p->param_i[2] = kTLaunch + GetRand(14);                // 発射時刻に少しばらつきを持たせる
    SetShape(p, kShDiamond, GetRand(1) ? 3 : 4);           // 菱形弾(シアン/青)
    p->muki = DX_PI / 3.0 * GetRand(2);                    // 結晶の軸方向(0°/60°/120°)
}

static void UpdateFrozen(sEnemyShot* p)
{
    p->param_d[4] *= 0.82;                                 // 急減速(muki は結晶軸のまま)
    p->param_d[5] *= 0.82;
    p->x += p->param_d[4];
    p->y += p->param_d[5];
    p->speed = sqrt(p->param_d[4] * p->param_d[4] + p->param_d[5] * p->param_d[5]);
}

static void LaunchShot(sEnemyShot* p)
{
    p->param_i[0] = kStLaunch;
    double tx = player.x + (GetRand(40) - 20);
    double ty = player.y + (GetRand(40) - 20);
    p->muki = atan2(ty - p->y, tx - p->x);                 // 発射方向に菱形弾を向ける
    p->speed = 0.3;
}

static void UpdateLaunch(sEnemyShot* p)
{
    p->speed += 0.06;
    if (p->speed > 4.2) p->speed = 4.2;
    p->x += p->speed * cos(p->muki);
    p->y += p->speed * sin(p->muki);
}

// ============================================================
// 弾1発ごとの状態遷移
// ============================================================
static void UpdateShot(sEnemyShotSet* pSet, sEnemyShot* p, int t)
{
    switch (p->param_i[0]) {
    case kStSolid:
        if (t >= p->param_i[2]) { MeltShot(p); UpdateLiquid(p); }
        else { UpdateSolid(p, t); }
        break;
    case kStLiquid:
        if (t >= p->param_i[2]) { EvaporateShot(p, t); UpdateGas(p, t); }
        else { UpdateLiquid(p); }
        break;
    case kStBubble:
        if (t >= p->param_i[2] || p->y < 30.0) BurstBubble(pSet, p, t);
        else                                   UpdateBubble(p);
        break;
    case kStGas:
        if (t >= kTFreeze) { FreezeShot(p); UpdateFrozen(p); }
        else { UpdateGas(p, t); }
        break;
    case kStFrozen:
        if (t >= p->param_i[2]) { LaunchShot(p); UpdateLaunch(p); }
        else { UpdateFrozen(p); }
        break;
    case kStLaunch:
        UpdateLaunch(p);
        break;
    case kStLaser:
        p->x += p->param_d[4];
        p->y += p->param_d[5];
        break;
    }
}

// 弾幕：三態輪廻(1ShotSetで1サイクル分を管理)
static void ShotPhaseTransition(sEnemyShotSet* pEnemyShotSet)
{
    const int t = pEnemyShotSet->count;

    // ---- フェーズごとのイベント ----
    if (t == 0) {
        PlaySE(sound_enemyShot_heavy);
        SpawnLattice(pEnemyShotSet);
    }
    if (t == kTMelt)     PlaySE(sound_enemyShot_medium);
    if (t == kTBoil)     PlaySE(sound_enemyShot_medium);
    if (t == kTGas)      PlaySE(sound_enemyShot_heavy);
    if (t == kTLaserEnd) PlaySE(sound_enemyCharge);        // 凝華の予告音
    if (t == kTLaunch)   PlaySE(sound_enemyShot_extreme);

    if (t >= kTBoil && t < kTBubbleEnd && (t - kTBoil) % 12 == 0) {
        SpawnBubble(pEnemyShotSet, t);
    }
    if (t >= kTGas && t < kTLaserEnd && (t - kTGas) % 30 == 0) {
        SpawnLaserVolley(pEnemyShotSet);
    }

    // ---- 全弾の更新(破裂で追加された弾も同じフレームで処理される) ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        UpdateShot(pEnemyShotSet, pShot, t);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_PhaseTransition_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    // ゆっくり左右に揺れる
    enemy.x = 240.0 + 80.0 * sin(count * 0.01);

    const int cyc = (count - 1) % kCycleLen;
    if (cyc == 0) {
        PlaySE(sound_enemyCharge); // 予告音
    }
    if (cyc == kPreWait) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPhaseTransition;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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