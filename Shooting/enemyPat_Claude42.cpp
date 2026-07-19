// enemyPat_pyramid.cpp
//
// ピラミッドモチーフ弾幕：積層リング + 稜線収束 + 頂上崩壊バースト
//
// 【使用素材の選定】
//   ・リング弾   : img_enemyShotDiamond  (菱形弾) … 石材が組み合わさったピラミッド側面の
//                  ブロック感を表現しやすい形状。下段は橙(8)＝砂岩色、上段は黄(1)＝陽光に
//                  照らされた頂上付近、という色分けで高さ方向の遠近感を補強する。
//   ・稜線弾     : img_enemyShotBullet   (銃弾, 5.0x2.0) … 細い針状弾。img_enemyShotLaser は
//                  当たり判定が見た目に対して大きすぎる(64x4)ため、稜線に沿う直線弾には
//                  不適。判定と見た目が一致する銃弾を採用し、白(6)で稜線のシャープさを表現。
//   ・崩壊バースト: img_enemyShotMediumBall (中玉) … 頂点で弾ける火の粉のような演出に丸みの
//                  ある弾形状が合うため採用。橙(8)で「崩れ落ちる」熱を感じさせる。
//
// 【構成】
//   フェーズ1・2：ShotPyramidRing  … 底面から頂上に向けて段々小さくなる正方形リングを
//                  一定間隔で積み上げる。各リングは生成時点で対応する断面サイズを持ち、
//                  そこからゆっくり外側へ拡大し続ける（画面外判定はメインルーチン任せ）。
//                  安全通路（隙間）は段ごとに位相をずらし、直線抜けを防ぐ。
//   フェーズ3　：ShotPyramidEdge  … 底面四隅から頂点へ向かう4本の稜線弾。最上段リングの
//                  生成完了と頂点到達がほぼ同時になるよう速度を調整。
//   フェーズ4　：ShotPyramidBurst … 稜線弾とリングが頂点で合流したタイミングで全方位に
//                  弾ける崩壊演出。重力加速度を与えて自然に落下させる。
//   フェーズ5　：EnemyPat_Pyramid_Claude内でサイクル全体をangleOffsetにより少しずつ回転させ、
//                  ピラミッドが自転しているように見せながら繰り返す。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  調整用定数
// ============================================================

// ピラミッド全体の位置・サイズ
static constexpr double PYRAMID_CENTER_X = 240.0; // 画面中央X (画面は480x480)
static constexpr double PYRAMID_BASE_Y = 380.0; // 最下段リングのY座標
static constexpr double PYRAMID_TOP_LAYER_Y = 150.0; // 最上段リングのY座標
static constexpr double PYRAMID_APEX_POINT_Y = 90.0;  // 頂点(崩壊バースト発生位置)のY座標
static constexpr double PYRAMID_BASE_HALF = 190.0; // 最下段リングの半幅
static constexpr double PYRAMID_TOP_HALF = 45.0;  // 最上段リングの半幅

// 積層リング関連
static constexpr int    PYRAMID_LAYER_COUNT = 30;    // 段数
static constexpr int    PYRAMID_LAYER_INTERVAL_FRAME = 6;   // 段の生成間隔(フレーム)
static constexpr int    PYRAMID_HOLD_AFTER_LAST_LAYER_FRAME = 40;  // 最上段生成後、崩壊までの待機フレーム
static constexpr int    PYRAMID_RING_SHOT_COUNT = 28;   // リング1本あたりの配置数(隙間含む)
static constexpr double PYRAMID_RING_GAP_RATIO = 0.14; // 安全通路の割合(t空間, 0〜1)
static constexpr double PYRAMID_RING_EXPAND_RATE = 0.05; // リングの拡大速度(1フレームあたり)
static constexpr double PYRAMID_GAP_ROTATE_STEP_T = 0.09; // 段ごとの隙間位相ずらし量(t空間)

// 稜線弾関連
static constexpr int PYRAMID_EDGE_SHOT_COUNT = 4; // 四隅の数
static constexpr int PYRAMID_EDGE_ARRIVAL_FRAME =
(PYRAMID_LAYER_COUNT - 1) * PYRAMID_LAYER_INTERVAL_FRAME + PYRAMID_HOLD_AFTER_LAST_LAYER_FRAME; // 頂点到達フレーム

// 頂上崩壊バースト関連
static constexpr int    PYRAMID_BURST_SHOT_COUNT = 24;   // 全方位弾の本数
static constexpr double PYRAMID_BURST_SPEED = 2.3;  // 初速
static constexpr double PYRAMID_BURST_GRAVITY = 0.035;// 重力加速度

// サイクル(繰り返し)関連
static constexpr int    PYRAMID_BURST_HOLD_FRAME = 50; // 崩壊後の余韻フレーム
static constexpr int    PYRAMID_REST_FRAME = 50; // 次サイクルまでの休止フレーム
static constexpr int    PYRAMID_CYCLE_INTERVAL_FRAME =
PYRAMID_EDGE_ARRIVAL_FRAME + PYRAMID_BURST_HOLD_FRAME + PYRAMID_REST_FRAME; // 1サイクルの総フレーム数
static constexpr double PYRAMID_SET_ROTATE_STEP = DX_PI / 9.0; // サイクルごとの全体回転量(20度)

// ============================================================
//  補助関数
// ============================================================

// 中心(0,0)・半幅1の正方形の周上の点を、angleOffsetだけ回転させて返す。
// t は 0〜1 未満で周を一巡するパラメータ(0, 0.25, 0.5, 0.75 が四隅に対応)。
static void SquarePerimeterPoint(double angleOffset, double t, double* outX, double* outY)
{
    double tt = t * 4.0;
    int    side = (int)tt;
    double frac = tt - side;

    double lx, ly;
    switch (side) {
    case 0: lx = -1.0 + frac * 2.0; ly = -1.0;              break; // 上辺: 左→右
    case 1: lx = 1.0;              ly = -1.0 + frac * 2.0; break; // 右辺: 上→下
    case 2: lx = 1.0 - frac * 2.0; ly = 1.0;              break; // 下辺: 右→左
    default:lx = -1.0;              ly = 1.0 - frac * 2.0; break; // 左辺: 下→上
    }

    double c = cos(angleOffset);
    double s = sin(angleOffset);
    *outX = lx * c - ly * s;
    *outY = lx * s + ly * c;
}

// 新しい弾セットを生成し、リストへ連結して返す(param_i/param_dは呼び出し側で設定する)。
static sEnemyShotSet* CreateEnemyShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->kind = 0;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// 弾セット内に新しい弾を1つ生成してリストへ連結する(座標等は呼び出し側で設定する)。
static sEnemyShot* CreateEnemyShotInSet(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->margin = 999;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ============================================================
//  弾幕：積層リング(フェーズ1・2)
//
//  param_d[0] = halfSize(このリングの断面サイズ)
//  param_d[1] = angleOffset(全体回転角)
//  param_d[2] = gapCenterT(安全通路の中心位置, t空間)
//  param_d[3] = growthRate(拡大速度)
//  param_i[0] = 弾の種類(色違い)
// ============================================================
static void ShotPyramidRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double halfSize = pEnemyShotSet->param_d[0];
        double angleOffset = pEnemyShotSet->param_d[1];
        double gapCenterT = pEnemyShotSet->param_d[2];
        int    shotKind = pEnemyShotSet->param_i[0];

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < PYRAMID_RING_SHOT_COUNT; i++) {
            double t = (double)i / PYRAMID_RING_SHOT_COUNT;

            // 安全通路(隙間)判定: gapCenterTを中心とした範囲をスキップする
            double diff = t - gapCenterT;
            diff -= floor(diff + 0.5); // -0.5〜0.5に正規化
            if (fabs(diff) < PYRAMID_RING_GAP_RATIO * 0.5) continue;

            double dirX, dirY;
            SquarePerimeterPoint(angleOffset, t, &dirX, &dirY);

            sEnemyShot* pEnemyShot = CreateEnemyShotInSet(pEnemyShotSet);
            pEnemyShot->x = pEnemyShotSet->x + dirX * halfSize;
            pEnemyShot->y = pEnemyShotSet->y + dirY * halfSize;
            pEnemyShot->kind = shotKind;
            pEnemyShot->param_d[0] = dirX; // 単位正方形上の方向(拡大の基準)
            pEnemyShot->param_d[1] = dirY;
        }
    }

    double halfSize0 = pEnemyShotSet->param_d[0];
    double growthRate = pEnemyShotSet->param_d[3];

    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 生成時からの経過フレーム(pEnemyShot->count、弾ごとの値)でサイズを計算する
        double currentSize = halfSize0 + growthRate * pEnemyShot->count;
        pEnemyShot->x = pEnemyShotSet->x + pEnemyShot->param_d[0] * currentSize;
        pEnemyShot->y = pEnemyShotSet->y + pEnemyShot->param_d[1] * currentSize;

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  弾幕：稜線弾(フェーズ3)
//
//  param_d[0] = halfSize(最下段の半幅、四隅の位置決定用)
//  param_d[1] = angleOffset(全体回転角)
//  param_d[2] = arrivalFrame(頂点到達までのフレーム数)
//  param_d[3] = apexX, param_d[4] = apexY(頂点座標)
// ============================================================
static void ShotPyramidEdge(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double halfSize = pEnemyShotSet->param_d[0];
        double angleOffset = pEnemyShotSet->param_d[1];
        double arrivalFrame = pEnemyShotSet->param_d[2];
        double apexX = pEnemyShotSet->param_d[3];
        double apexY = pEnemyShotSet->param_d[4];

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < PYRAMID_EDGE_SHOT_COUNT; i++) {
            double t = i * 0.25; // 0, 0.25, 0.5, 0.75 → 正方形の四隅に対応

            double dirX, dirY;
            SquarePerimeterPoint(angleOffset, t, &dirX, &dirY);

            double startX = pEnemyShotSet->x + dirX * halfSize;
            double startY = pEnemyShotSet->y + dirY * halfSize;

            double dx = apexX - startX;
            double dy = apexY - startY;
            double dist = sqrt(dx * dx + dy * dy);

            sEnemyShot* pEnemyShot = CreateEnemyShotInSet(pEnemyShotSet);
            pEnemyShot->x = startX;
            pEnemyShot->y = startY;
            pEnemyShot->muki = atan2(dy, dx);
            pEnemyShot->speed = dist / arrivalFrame; // 全リング積み上げ完了とほぼ同時に頂点到達
            pEnemyShot->kind = img_enemyShotBullet[6]; // 白色の銃弾(針状) : 稜線を表現
        }
    }

    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  弾幕：頂上崩壊バースト(フェーズ4)
//
//  各弾の param_d[0]=vx, param_d[1]=vy に速度を持たせ、重力加速度を与える。
// ============================================================
static void ShotPyramidBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < PYRAMID_BURST_SHOT_COUNT; i++) {
            double angle = DX_PI * 2.0 * i / PYRAMID_BURST_SHOT_COUNT;

            sEnemyShot* pEnemyShot = CreateEnemyShotInSet(pEnemyShotSet);
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙色の中玉: 崩れ落ちる炎のイメージ
            pEnemyShot->param_d[0] = PYRAMID_BURST_SPEED * cos(angle); // vx
            pEnemyShot->param_d[1] = PYRAMID_BURST_SPEED * sin(angle); // vy
        }
    }

    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->param_d[1] += PYRAMID_BURST_GRAVITY; // 重力加速
        pEnemyShot->x += pEnemyShot->param_d[0];
        pEnemyShot->y += pEnemyShot->param_d[1];

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Pyramid_Claude()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = PYRAMID_CENTER_X;
        enemy.y = PYRAMID_APEX_POINT_Y - 20.0; // 頂点のさらに上に浮遊
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        enemy.x += 0.4 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }

    // count==1開始を基準にした、サイクル内での経過フレーム(0始まり)
    int    phase = (count - 1) % PYRAMID_CYCLE_INTERVAL_FRAME;
    int    cycleIndex = (count - 1) / PYRAMID_CYCLE_INTERVAL_FRAME;
    double angleOffset = cycleIndex * PYRAMID_SET_ROTATE_STEP; // フェーズ5: サイクルごとに全体回転

    // フェーズ1・2: 底面から頂上に向けて段々小さくなるリングを積み上げる
    for (int i = 0; i < PYRAMID_LAYER_COUNT; i++) {
        if (phase == i * PYRAMID_LAYER_INTERVAL_FRAME) {
            double ratio = (double)i / (PYRAMID_LAYER_COUNT - 1);
            double y = PYRAMID_BASE_Y + (PYRAMID_TOP_LAYER_Y - PYRAMID_BASE_Y) * ratio;
            double halfSize = PYRAMID_BASE_HALF + (PYRAMID_TOP_HALF - PYRAMID_BASE_HALF) * ratio;
            double gapCenterT = fmod(0.125 + i * PYRAMID_GAP_ROTATE_STEP_T, 1.0); // 段ごとに隙間の位置をずらす

            sEnemyShotSet* pEnemyShotSet = CreateEnemyShotSet(ShotPyramidRing, PYRAMID_CENTER_X, y);
            pEnemyShotSet->param_d[0] = halfSize;
            pEnemyShotSet->param_d[1] = angleOffset;
            pEnemyShotSet->param_d[2] = gapCenterT;
            pEnemyShotSet->param_d[3] = PYRAMID_RING_EXPAND_RATE;
            pEnemyShotSet->param_i[0] = (i < PYRAMID_LAYER_COUNT / 2)
                ? img_enemyShotDiamond[8]  // 下段: 橙色(砂岩をイメージ)
                : img_enemyShotDiamond[1]; // 上段: 黄色(陽光をイメージ)
        }
    }

    // フェーズ3: 底面四隅から頂点へ向かう稜線弾
    if (phase == 0) {
        sEnemyShotSet* pEnemyShotSet = CreateEnemyShotSet(ShotPyramidEdge, PYRAMID_CENTER_X, PYRAMID_BASE_Y);
        pEnemyShotSet->param_d[0] = PYRAMID_BASE_HALF;
        pEnemyShotSet->param_d[1] = angleOffset;
        pEnemyShotSet->param_d[2] = (double)PYRAMID_EDGE_ARRIVAL_FRAME;
        pEnemyShotSet->param_d[3] = PYRAMID_CENTER_X;
        pEnemyShotSet->param_d[4] = PYRAMID_APEX_POINT_Y;
    }

    // フェーズ4: 稜線弾とリングが頂点で合流するタイミングで崩壊バースト
    if (phase == PYRAMID_EDGE_ARRIVAL_FRAME) {
        CreateEnemyShotSet(ShotPyramidBurst, PYRAMID_CENTER_X, PYRAMID_APEX_POINT_Y);
    }
}