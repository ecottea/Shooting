#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <vector>
#include <algorithm>

// ============================================================
//  弾幕：柱サボテン「サグアロ・ニードルレイン」
//
//  ・幹/腕=中玉(緑)、縦筋=小玉(黄)、針=菱形弾(白/黄)、花=大玉(マゼンタ)+小玉(橙)で
//    サボテンの形を作り、剛体として画面を降下させる
//  ・1本のサボテン = 1つの sEnemyShotSet(弾が残っている間は消えない)
//
//  サボテン1本のタイムライン(ShotSet の count 基準)
//    0〜36    成長(下から上へ生える。この間は静止)
//    36〜     ゆっくり降下(kFallSpeed)
//    60       開花(大玉+花びら12枚)
//    76/88/100 花粉リング(28発 × 3段)。花びらは76で外へ飛ぶ
//    84〜     針が下から上へ順に離れて四方へ加速(1本あたり針2本1組)
//    216      縦筋の小玉が白い銃弾に変化(予告音)
//    228〜    銃弾が自機狙いの7way×3列で発射(8f間隔)
// ============================================================

// ---------- 調整用定数 ----------
static const int    kCycle = 360;     // 1サイクルの長さ(f)
static const double kGroundY = 165.0;   // サボテンの根元のY(成長時)
static const int    kGrowEnd = 36;      // 成長が終わるフレーム
static const double kFallSpeed = 1.2;     // 降下速度
static const double kFlowerDy = -140.0;  // 花の中心(根元からのオフセット)
static const int    kBloomT = 60;      // 開花
static const int    kPetalT = 76;      // 花びらが飛ぶ/1段目の花粉リング
static const int    kRingNum = 28;      // 花粉リングの弾数
static const int    kSpineStart = 84;      // 針の発射開始
static const double kSpineDelay = 0.85;    // 針の発射遅れ(高さ1pxあたりのf数)
static const int    kArmT = 216;     // 縦筋→銃弾に変化
static const int    kLaunchT = 228;     // 銃弾の発射開始
static const int    kLaunchStep = 8;       // 列ごとの発射間隔(f)

// 弾の役割(param_i[0])
enum {
    kRoleStem,   // 幹・腕・花の中心(剛体として動くだけ)
    kRoleSpine,  // 針(時間が来たら離れる)
    kRoleRib,    // 縦筋(後で銃弾に変化して自機狙い)
    kRolePetal,  // 花びら(時間が来たら外へ飛ぶ)
    kRoleFree    // 離れた後の弾(花粉リングなど)
};
// 弾の状態(param_i[1])
enum { kStateAttached = 0, kStateArmed = 1, kStateFree = 2 };
// 弾のパラメータの使い方
//   param_i[0]=役割 / [1]=状態 / [2]=離れる時刻(離れた後は離れ始めた時刻)
//   param_i[3]=加速にかけるフレーム数 / [4]=縦筋の列 / [5]=縦筋の扇の位置
//   param_d[0],[1]=剛体オフセット(dx,dy) / [2]=最終速度 / [3]=初速

// ---------- サボテンの部品表 ----------
struct CactusPart {
    int    role;
    double dx, dy;   // 根元からのオフセット(ミラーなし基準)
    double muki;     // 針の向き(ミラーなし基準)
    int    color;
    int    rel;      // 針:発射遅れ / 縦筋:通し番号
    int    tSpawn;   // 生える時刻
};
static std::vector<CactusPart> gCactusParts;

static void PlaySe(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

static sEnemyShot* CactusAddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

static void CactusAddPart(int role, double dx, double dy, double muki, int color, int rel)
{
    CactusPart p;
    p.role = role;
    p.dx = dx;
    p.dy = dy;
    p.muki = muki;
    p.color = color;
    p.rel = rel;
    p.tSpawn = (int)(-dy * kGrowEnd / 130.0); // 低い所から先に生える
    if (p.tSpawn < 0) p.tSpawn = 0;
    gCactusParts.push_back(p);
}

// 針1か所 = 2本1組(わずかに角度と位置をずらしてV字に見せる)
static void CactusAddSpineSite(double sx, double sy, double muki, int color)
{
    for (int k = -1; k <= 1; k += 2) {
        double m = muki + k * 0.14;
        double ox = -sin(muki) * 2.0 * k;
        double oy = cos(muki) * 2.0 * k;
        int rel = (int)(-sy * kSpineDelay);
        if (rel < 0) rel = 0;
        CactusAddPart(kRoleSpine, sx + ox, sy + oy, m, color, rel);
    }
}

static bool CactusPartLess(const CactusPart& a, const CactusPart& b)
{
    return a.tSpawn < b.tSpawn;
}

static void BuildCactusLayout()
{
    if (!gCactusParts.empty()) return;

    int siteNo = 0;
    // 針の色は3か所に1か所を黄、他は白
#define CACTUS_SITE(sx, sy, m) CactusAddSpineSite((sx), (sy), (m), (siteNo++ % 3 == 2) ? 1 : 6)

// 幹(中玉・緑の2列、高さ120)
    for (int row = 0; row <= 15; row++) {
        double dy = -8.0 * row;
        CactusAddPart(kRoleStem, -5.0, dy, 0.0, 2, 0);
        CactusAddPart(kRoleStem, +5.0, dy, 0.0, 2, 0);
    }
    // 縦筋(小玉・黄)21個。後で銃弾に変化する
    for (int h = 0; h <= 20; h++) {
        CactusAddPart(kRoleRib, 0.0, -6.0 * h, 0.0, 1, h);
    }
    // 左腕(長め): 横4 + 縦6
    for (int i = 0; i < 4; i++) CactusAddPart(kRoleStem, -16.0 - 8.0 * i, -48.0, 0.0, 2, 0);
    for (int j = 0; j < 6; j++) CactusAddPart(kRoleStem, -40.0, -56.0 - 8.0 * j, 0.0, 2, 0);
    // 右腕(短め・高め): 横4 + 縦4
    for (int i = 0; i < 4; i++) CactusAddPart(kRoleStem, 16.0 + 8.0 * i, -72.0, 0.0, 2, 0);
    for (int j = 0; j < 4; j++) CactusAddPart(kRoleStem, 40.0, -80.0 - 8.0 * j, 0.0, 2, 0);

    // 幹の針(腕の付け根の段は避ける)
    for (int r = 1; r <= 14; r++) {
        double dy = -8.0 * r;
        double tilt = ((r % 3) - 1) * 0.45;
        if (r < 4 || r > 7) CACTUS_SITE(-15.0, dy, DX_PI - tilt);  // 左側
        if (r < 7 || r > 10) CACTUS_SITE(15.0, dy, tilt);          // 右側
    }
    // 左腕の針: 下向き、外向き、先端の扇
    for (int i = 0; i < 4; i++) CACTUS_SITE(-16.0 - 8.0 * i, -48.0 + 11.0, DX_PI / 2 + ((i % 3) - 1) * 0.35);
    for (int j = 0; j < 6; j++) CACTUS_SITE(-51.0, -56.0 - 8.0 * j, DX_PI + ((j % 3) - 1) * 0.4);
    for (int m = -1; m <= 1; m++) CACTUS_SITE(-40.0 + 3.0 * m, -107.0, -DX_PI / 2 + m * 0.5);
    // 右腕の針
    for (int i = 0; i < 4; i++) CACTUS_SITE(16.0 + 8.0 * i, -72.0 + 11.0, DX_PI / 2 + ((i % 3) - 1) * 0.35);
    for (int j = 0; j < 4; j++) CACTUS_SITE(51.0, -80.0 - 8.0 * j, ((j % 3) - 1) * 0.4);
    for (int m = -1; m <= 1; m++) CACTUS_SITE(40.0 + 3.0 * m, -115.0, -DX_PI / 2 + m * 0.5);

#undef CACTUS_SITE

    // 低い所から生えるように並べ替え(同時刻の描画順は生成順を維持)
    std::stable_sort(gCactusParts.begin(), gCactusParts.end(), CactusPartLess);
}

// 部品を1個生やす(ミラー時は左右反転)
static void CactusSpawnPart(sEnemyShotSet* pSet, const CactusPart& p)
{
    const bool mirror = (pSet->param_i[1] != 0);
    const double dx = mirror ? -p.dx : p.dx;
    double muki = mirror ? (DX_PI - p.muki) : p.muki;
    int kind;
    switch (p.role) {
    case kRoleStem:
        kind = img_enemyShotMediumBall[p.color];
        muki = DX_PI / 2;   // 降下方向
        break;
    case kRoleRib:
        kind = img_enemyShotSmallBall[p.color];
        muki = DX_PI / 2;   // 降下方向
        break;
    default:
        kind = img_enemyShotDiamond[p.color];  // 針は外向き(刺さって見える向き)
        break;
    }
    sEnemyShot* pShot = CactusAddShot(pSet, pSet->x + dx, pSet->y + p.dy, muki, 0.0, kind);
    pShot->param_i[0] = p.role;
    pShot->param_i[1] = kStateAttached;
    pShot->param_d[0] = dx;
    pShot->param_d[1] = p.dy;
    if (p.role == kRoleSpine) {
        pShot->param_i[2] = kSpineStart + p.rel;      // 離れる時刻
    }
    else if (p.role == kRoleRib) {
        pShot->param_i[4] = p.rel / 7;                // 列(0〜2)
        pShot->param_i[5] = p.rel % 7;                // 扇の位置(0〜6)
    }
}

// 花粉リング
static void CactusSpawnRing(sEnemyShotSet* pSet, int color, double speed)
{
    const double cx = pSet->x;
    const double cy = pSet->y + kFlowerDy;
    const double base = GetRand(99) / 100.0 * (2.0 * DX_PI / kRingNum);
    for (int i = 0; i < kRingNum; i++) {
        double a = base + 2.0 * DX_PI * i / kRingNum;
        sEnemyShot* pShot = CactusAddShot(pSet, cx, cy, a, speed, img_enemyShotSmallBall[color]);
        pShot->param_i[0] = kRoleFree;
        pShot->param_i[1] = kStateFree;
        pShot->param_i[2] = pSet->count;
        pShot->param_i[3] = 1;
        pShot->param_d[2] = speed;
        pShot->param_d[3] = speed;
    }
}

// 弾を剛体から切り離して自由飛行にする
static void CactusRelease(sEnemyShot* pShot, int t, double v0, double v1, int ramp)
{
    pShot->param_i[1] = kStateFree;
    pShot->param_i[2] = t;
    pShot->param_i[3] = ramp;
    pShot->param_d[3] = v0;
    pShot->param_d[2] = v1;
}

// 弾幕：サボテン1本
static void ShotCactus(sEnemyShotSet* pSet)
{
    const int t = pSet->count;

    if (t == 0) {
        BuildCactusLayout();
        pSet->param_i[0] = 0;   // 次に生やす部品の番号
    }
    // 成長が終わったら降下
    if (t >= kGrowEnd) pSet->y += kFallSpeed;

    // 成長: 低い部品から順に生える
    while (pSet->param_i[0] < (int)gCactusParts.size() && gCactusParts[pSet->param_i[0]].tSpawn <= t) {
        CactusSpawnPart(pSet, gCactusParts[pSet->param_i[0]]);
        pSet->param_i[0]++;
    }

    // 開花: 大玉(マゼンタ)+花びら(小玉・橙)12枚
    if (t == kBloomT) {
        PlaySe(sound_enemyShot_medium);
        sEnemyShot* pBud = CactusAddShot(pSet, pSet->x, pSet->y + kFlowerDy, DX_PI / 2, 0.0, img_enemyShotLargeBall[5]);
        pBud->param_i[0] = kRoleStem;
        pBud->param_i[1] = kStateAttached;
        pBud->param_d[0] = 0.0;
        pBud->param_d[1] = kFlowerDy;
        for (int k = 0; k < 12; k++) {
            double a = 2.0 * DX_PI * k / 12.0;
            double dx = 30.0 * cos(a);
            double dy = kFlowerDy + 30.0 * sin(a);
            sEnemyShot* pPetal = CactusAddShot(pSet, pSet->x + dx, pSet->y + dy, a, 0.0, img_enemyShotSmallBall[8]);
            pPetal->param_i[0] = kRolePetal;
            pPetal->param_i[1] = kStateAttached;
            pPetal->param_d[0] = dx;
            pPetal->param_d[1] = dy;
        }
    }
    // 花粉リング3段
    if (t == kPetalT) { PlaySe(sound_enemyShot_medium); CactusSpawnRing(pSet, 1, 2.2); }
    if (t == kPetalT + 12) { PlaySe(sound_enemyShot_medium); CactusSpawnRing(pSet, 8, 2.8); }
    if (t == kPetalT + 24) { PlaySe(sound_enemyShot_heavy);  CactusSpawnRing(pSet, 5, 3.4); }

    // 針の連射音 / 銃弾の予告音と発射音
    if (t >= kSpineStart && t <= kSpineStart + 100 && (t - kSpineStart) % 5 == 0) PlaySe(sound_enemyShot_light);
    if (t == kArmT) PlaySe(sound_enemyCharge);
    if (t == kLaunchT || t == kLaunchT + kLaunchStep || t == kLaunchT + kLaunchStep * 2) PlaySe(sound_enemyShot_heavy);

    // 各弾の更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        const int role = pShot->param_i[0];
        const int state = pShot->param_i[1];

        if (state != kStateFree) {
            // 剛体として根元に追従
            pShot->x = pSet->x + pShot->param_d[0];
            pShot->y = pSet->y + pShot->param_d[1];
            pShot->speed = (t >= kGrowEnd) ? kFallSpeed : 0.0;

            if (role == kRoleSpine) {
                if (t >= pShot->param_i[2]) {
                    // 針が離れる: 少しばらけて外向きに加速
                    pShot->muki += (GetRand(100) - 50) / 50.0 * 0.10;
                    CactusRelease(pShot, t, 1.0, 5.0 + GetRand(60) / 100.0-1, 24);
                }
            }
            else if (role == kRolePetal) {
                if (t >= kPetalT) {
                    // 花びらは中心から外向きに飛ぶ(muki は配置時に外向き)
                    CactusRelease(pShot, t, 0.6, 2.6, 20);
                }
            }
            else if (role == kRoleRib) {
                if (state == kStateAttached && t >= kArmT) {
                    // 縦筋が白い銃弾に変化し、自機の方を向く
                    pShot->kind = img_enemyShotBullet[6];
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->param_i[1] = kStateArmed;
                    pShot->param_i[2] = kLaunchT + pShot->param_i[4] * kLaunchStep; // 列ごとの発射時刻
                }
                else if (state == kStateArmed) {
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x); // 発射直前まで狙い続ける
                    if (t >= pShot->param_i[2]) {
                        const int g = pShot->param_i[4];
                        const int fan = pShot->param_i[5];
                        pShot->muki += (fan - 3) * 0.12;   // 7way
                        CactusRelease(pShot, t, 1.0, 3.0 + g * 0.9, 12);
                    }
                }
            }
        }
        else {
            // 自由飛行: 初速から最終速度へ加速(向きに沿って直進)
            const int age = t - pShot->param_i[2];
            double r = (double)age / (double)pShot->param_i[3];
            if (r > 1.0) r = 1.0;
            const double v = pShot->param_d[3] + (pShot->param_d[2] - pShot->param_d[3]) * r;
            pShot->speed = v;
            pShot->x += v * cos(pShot->muki);
            pShot->y += v * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// サボテンを1本生やす
static void CreateCactus(double x, int mirror)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotCactus;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = kGroundY;
    pEnemyShotSet->muki = DX_PI / 2;
    pEnemyShotSet->kind = 0;
    pEnemyShotSet->param_i[1] = mirror;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_Cactus_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    else {
        // 敵はゆっくり左右に揺れる
        enemy.x = 240.0 + 40.0 * sin(count * DX_PI / 240.0);
    }

    const int phase = (count - 1) % kCycle;
    if (phase == 0) {
        PlaySe(sound_enemyCharge);       // 予告音
    }
    if (phase == 20) {
        CreateCactus(240.0, 1);          // 中央のサボテンが先に生える
    }
    if (phase == 40) {
        CreateCactus(95.0-20, 0);           // 左右は20f遅れ
        CreateCactus(385.0+20, 0);
    }
}