#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：三節棍・連環旋回
//
//  敵の周囲で3本の三節棍が回転し、関節がS字⇔Z字にしなる。
//  一直線に伸びきった瞬間に先棍が自機狙いで投擲される(先棍は即再生)。
//  最後に全ての節が分離し、自機狙いの扇状に射出される。
//
//  1本の三節棍(1つのsEnemyShotSet)は3つの節(seg 0:手元 / 1:中 / 2:先)でできている。
//    棍     ：中楕円弾 x5 (手元=橙 / 中=黄 / 先=白)  muki は棍の向き
//    鎖     ：小玉(シアン) 各関節の両側に2発ずつ
//    金具   ：中玉(赤) 各関節に1発
//    穂先   ：菱形弾(赤) 先棍の先端に1発
//    風切り ：鱗弾 先端と関節から外向きに放つ(渦巻き状の軌跡になる)
//
//  弾の状態(param_i[0])
//    ST_ATTACHED : 骨格(関節位置と角度)に追従する
//    ST_RIGID    : 節ごとに剛体として一緒に飛ぶ(棍が回転しながら飛んでいく)
//    ST_LINEAR   : 等速直線運動(風切りの弾)
//
//  乱数は使用せず、すべて経過フレームとプレイヤー位置から決定する(リプレイ安全)。
// ============================================================

// ---------- 骨格・形状 ----------
static const double kPi = DX_PI;
static const double kSegLen = 97.0;       // 関節から関節までの長さ
static const double kHandRadius = 24.0;   // 敵の中心から手元の関節までの距離
static const double kRodFirst = 26.5;     // 節の始点から最初の棍弾までの距離
static const double kRodStep = 11.0;      // 棍弾の間隔(中楕円弾の長さ21より短く、隙間なく並べる)
static const int    kRodNum = 5;          // 1節あたりの棍弾の数
static const double kChainNear = 10.0;    // 関節から鎖までの距離(内側)
static const double kChainFar = 14.0;     // 関節から鎖までの距離(外側)
static const double kTipOffset = 88.0;    // 先棍の穂先(菱形弾)の位置

// ---------- 時間進行(フレーム) ----------
static const int    kBendStart = 80;      // 構え終了(しなり開始)
static const int    kBendStagger = 20;    // 三節棍ごとのしなり開始のずらし幅
static const int    kThrowStart = 100;    // 投擲を始める時刻
static const int    kFinalTime = 480;     // 全節射出の時刻
static const int    kThrowEnd = kFinalTime - 30;
static const int    kCycle = 640;         // パターンの繰り返し周期

// ---------- 動き ----------
static const double kStartAngle = -kPi / 2.0;       // 1本目の初期角度(上向き)
static const double kOmega = 0.024;                 // 旋回の最大角速度(rad/frame)
static const double kBendOmega = 2.0 * kPi / 120.0; // しなりの角速度(2回/周期ごとに一直線)
static const double kBendMax = 1.1;                 // 関節の最大折れ角(rad)
static const double kFoldAngle = 2.8;               // 構え時の折り畳み角(rad)
static const double kThrowSpeed = 3.0;
static const double kThrowSpin = 0.06;
static const double kTrailSpeed = 1.5;

// ---------- 画面外消去の余白(弾のmargin) ----------
// 骨格に追従している間は画面外に出ても消えないようにする。
// 分離後は、画面外から飛び込んでくる分を残せる程度の余白にする。
static const double kAttachedMargin = 400.0;
static const double kRigidMargin = 250.0;
static const double kDefaultMargin = 20.0;

// ---------- 弾の状態 ----------
enum {
    ST_ATTACHED = 0,
    ST_RIGID = 1,
    ST_LINEAR = 2,
};
// param_i[0]:状態 / param_i[1]:節番号 / param_i[2]:分離した時刻
// param_d[0]:節の始点からの距離(分離後は節の中心からの距離)
// param_d[1],[2]:分離時の節の中心座標 / param_d[3],[4]:速度 / param_d[5]:分離時の棍の角度 / param_d[6]:回転速度
// ShotSetの param_d[0]:旋回角の累積 / param_d[1]:前フレームのしなり位相(符号判定用)
// ShotSetの kind:三節棍の番号(0〜2)

// 骨格(関節の座標と各節の向き)
struct sSkeleton {
    double jx[4], jy[4];
    double ang[3];
};

// ============================================================
//  補助関数
// ============================================================
static void PlaySE(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 弾を1発生成して、ShotSetのリストの末尾に連結する
static sEnemyShot* AddShot(sEnemyShotSet* pSet, int kind, int state, int seg, double offset)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = pSet->x;
    pShot->y = pSet->y;
    pShot->muki = 0.0;
    pShot->speed = 0.0;
    pShot->kind = kind;
    pShot->margin = kAttachedMargin;
    pShot->param_i[0] = state;
    pShot->param_i[1] = seg;
    pShot->param_d[0] = offset;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 1つの節(棍・鎖・金具・穂先)を生成する
static void CreateSegment(sEnemyShotSet* pSet, int seg)
{
    // 弾の種類と色: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
    static const int rodColor[3] = { 8, 1, 6 };

    // 手前側の鎖(中棍・先棍のみ)
    if (seg >= 1) {
        AddShot(pSet, img_enemyShotSmallBall[3], ST_ATTACHED, seg, kChainNear);
        AddShot(pSet, img_enemyShotSmallBall[3], ST_ATTACHED, seg, kChainFar);
    }
    // 棍
    for (int i = 0; i < kRodNum; i++) {
        AddShot(pSet, img_enemyShotMediumOval[rodColor[seg]], ST_ATTACHED, seg, kRodFirst + kRodStep * i);
    }
    if (seg <= 1) {
        // 奥側の鎖と金具(手元棍・中棍のみ)
        AddShot(pSet, img_enemyShotSmallBall[3], ST_ATTACHED, seg, kSegLen - kChainFar);
        AddShot(pSet, img_enemyShotSmallBall[3], ST_ATTACHED, seg, kSegLen - kChainNear);
        AddShot(pSet, img_enemyShotMediumBall[0], ST_ATTACHED, seg, kSegLen);
    }
    else {
        // 穂先
        AddShot(pSet, img_enemyShotDiamond[0], ST_ATTACHED, seg, kTipOffset);
    }
}

// 旋回の角速度(0から加速 → 一定 → 反転 → 逆回転)
static double OmegaAt(int t)
{
    if (t < 80) {
        double r = t / 80.0;
        return kOmega * r * r;
    }
    if (t < 240) return kOmega;
    if (t < 300) return kOmega * cos(kPi * (t - 240) / 60.0);
    return -kOmega;
}

// 関節の折れ角の振幅(構え中は折り畳み → 徐々に開く)
static double BendAmp(int t)
{
    if (t >= kBendStart) return kBendMax;
    double r = (double)t / kBendStart;
    return kFoldAngle + (kBendMax - kFoldAngle) * (r * (2.0 - r));
}

// しなりの位相(1.0で最大に折れる、0で一直線)
static double BendPhase(int t, int w)
{
    int t0 = kBendStart + w * kBendStagger;
    if (t < t0) return 1.0;
    return cos(kBendOmega * (t - t0));
}

// 弾を骨格から分離して、節の剛体として飛ばす
static void ReleaseShot(sEnemyShot* pShot, int t, double cx, double cy, double vx, double vy, double alpha0, double spin)
{
    pShot->param_d[0] -= kSegLen * 0.5; // 節の中心を基準にした距離へ
    pShot->param_i[0] = ST_RIGID;
    pShot->param_i[2] = t;
    pShot->param_d[1] = cx;
    pShot->param_d[2] = cy;
    pShot->param_d[3] = vx;
    pShot->param_d[4] = vy;
    pShot->param_d[5] = alpha0;
    pShot->param_d[6] = spin;
    pShot->margin = kRigidMargin;
}

// 指定した節を自機狙いで投擲する
static void ThrowSegment(sEnemyShotSet* pSet, int seg, const sSkeleton& sk, int w, int t)
{
    double cx = sk.jx[seg] + cos(sk.ang[seg]) * kSegLen * 0.5;
    double cy = sk.jy[seg] + sin(sk.ang[seg]) * kSegLen * 0.5;
    double aim = atan2(player.y - cy, player.x - cx);
    double vx = kThrowSpeed * cos(aim);
    double vy = kThrowSpeed * sin(aim);
    double spin = (w == 1) ? -kThrowSpin : kThrowSpin;

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == ST_ATTACHED && pShot->param_i[1] == seg) {
            ReleaseShot(pShot, t, cx, cy, vx, vy, sk.ang[seg], spin);
        }
        pShot = pShot->next;
    }
}

// 全ての節を分離して、自機狙いの扇状に射出する
static void ReleaseAll(sEnemyShotSet* pSet, const sSkeleton& sk, int w, int t)
{
    double cx[3], cy[3], vx[3], vy[3], spin[3];
    for (int seg = 0; seg < 3; seg++) {
        cx[seg] = sk.jx[seg] + cos(sk.ang[seg]) * kSegLen * 0.5;
        cy[seg] = sk.jy[seg] + sin(sk.ang[seg]) * kSegLen * 0.5;
        double aim = atan2(player.y - cy[seg], player.x - cx[seg]) + (seg - 1) * 0.30 + (w - 1) * 0.09;
        double speed = 2.2 + 0.5 * seg;
        vx[seg] = speed * cos(aim);
        vy[seg] = speed * sin(aim);
        spin[seg] = ((seg + w) % 2 == 0) ? 0.05 : -0.05;
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == ST_ATTACHED) {
            int seg = pShot->param_i[1];
            ReleaseShot(pShot, t, cx[seg], cy[seg], vx[seg], vy[seg], sk.ang[seg], spin[seg]);
        }
        pShot = pShot->next;
    }
}

// 風切りの弾(敵の中心から見て外向きに飛ぶ)
static void SpawnTrail(sEnemyShotSet* pSet, int w, double x, double y)
{
    static const int trailColor[3] = { 4, 2, 5 };
    double dir = atan2(y - pSet->y, x - pSet->x);
    sEnemyShot* pShot = AddShot(pSet, img_enemyShotScale[trailColor[w % 3]], ST_LINEAR, 0, 0.0);
    pShot->x = x;
    pShot->y = y;
    pShot->muki = dir;
    pShot->margin = kDefaultMargin;
    pShot->param_d[3] = kTrailSpeed * cos(dir);
    pShot->param_d[4] = kTrailSpeed * sin(dir);
}

// ============================================================
//  三節棍1本ぶんの弾幕(ShotSetのpatternFunc)
// ============================================================
static void ShotSansekkon(sEnemyShotSet* pSet)
{
    const int t = pSet->count;
    const int w = pSet->kind;

    if (t == 0) {
        pSet->param_d[0] = 0.0;
        pSet->param_d[1] = 1.0;
        for (int seg = 0; seg < 3; seg++) CreateSegment(pSet, seg);
        if (w == 0) PlaySE(sound_enemyCharge); // 予告音
    }

    // ---- 骨格の計算 ----
    pSet->param_d[0] += OmegaAt(t);
    const double theta0 = kStartAngle + w * (2.0 * kPi / 3.0) + pSet->param_d[0];
    const double s = BendPhase(t, w);
    const double bend = BendAmp(t) * s;

    sSkeleton sk;
    sk.ang[0] = theta0;
    sk.ang[1] = theta0 + bend;
    sk.ang[2] = theta0; // 先棍は手元棍と平行に戻る(S字/Z字になる)
    sk.jx[0] = pSet->x + kHandRadius * cos(theta0);
    sk.jy[0] = pSet->y + kHandRadius * sin(theta0);
    for (int k = 0; k < 3; k++) {
        sk.jx[k + 1] = sk.jx[k] + kSegLen * cos(sk.ang[k]);
        sk.jy[k + 1] = sk.jy[k] + kSegLen * sin(sk.ang[k]);
    }

    // ---- 一直線に伸びきった瞬間に先棍を投擲し、すぐ再生する ----
    const double prevS = pSet->param_d[1];
    const bool straight = (prevS > 0.0 && s <= 0.0) || (prevS < 0.0 && s >= 0.0);
    pSet->param_d[1] = s;
    if (straight && t >= kThrowStart && t < kThrowEnd) {
        PlaySE(sound_enemyShot_medium);
        ThrowSegment(pSet, 2, sk, w, t);
        CreateSegment(pSet, 2);
    }

    // ---- 全節射出 ----
    if (t == kFinalTime) {
        PlaySE(sound_enemyShot_heavy);
        ReleaseAll(pSet, sk, w, t);
    }

    // ---- 風切り(先端は3フレームごと、関節は6フレームごと) ----
    if (t >= kBendStart && t < kFinalTime) {
        if (t % 3 == 0) SpawnTrail(pSet, w, sk.jx[3], sk.jy[3]);
        if (t % 6 == 1) SpawnTrail(pSet, w, sk.jx[1], sk.jy[1]);
        if (t % 6 == 4) SpawnTrail(pSet, w, sk.jx[2], sk.jy[2]);
    }

    // ---- 全弾の位置更新 ----
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case ST_ATTACHED: {
            int seg = pShot->param_i[1];
            double off = pShot->param_d[0];
            pShot->x = sk.jx[seg] + off * cos(sk.ang[seg]);
            pShot->y = sk.jy[seg] + off * sin(sk.ang[seg]);
            pShot->muki = sk.ang[seg]; // 棍の向きに合わせる
            break;
        }
        case ST_RIGID: {
            double dt = (double)(t - pShot->param_i[2]);
            double alpha = pShot->param_d[5] + pShot->param_d[6] * dt;
            double cx = pShot->param_d[1] + pShot->param_d[3] * dt;
            double cy = pShot->param_d[2] + pShot->param_d[4] * dt;
            double off = pShot->param_d[0];
            pShot->x = cx + off * cos(alpha);
            pShot->y = cy + off * sin(alpha);
            pShot->muki = alpha; // 回転する棍の向き(進行方向とは別)
            break;
        }
        case ST_LINEAR:
            pShot->x += pShot->param_d[3];
            pShot->y += pShot->param_d[4];
            break;
        }
        pShot = pShot->next;
    }
}

// 三節棍1本ぶんのShotSetを生成する
static void SpawnWeapon(int w)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotSansekkon;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = w;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_ThreeSectionStaff_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    // 敵は動かず、一定周期で三節棍を3本同時に出す
    if (count % kCycle == 1) {
        for (int w = 0; w < 3; w++) SpawnWeapon(w);
    }
}