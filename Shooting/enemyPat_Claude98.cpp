// enemyPat_MurasakiOugiDanmakuKekkai.cpp
// 紫奥義「弾幕結界」
//
// 仕様まとめ:
//   1. 画面中央に青と紫の大玉をk個ずつ生成。画面中央を中心に角速度aで回転しながら
//      半径を広げ、画面幅の半分(240)に達したら半径拡大を停止する。
//   2. 半径固定後、各大玉から対応する色の鱗弾を「Tフレームの間」発射する。
//      毎フレーム、内側へ速さvで1個・外側へ低速で3個(扇状)を発射する。
//   3. 発射リズムはt1フレーム発射→t2フレーム休止を繰り返す。
//   4. 発射方向は画面中央から見た大玉の方向(=大玉の公転角)を基準に、
//      射撃開始からの経過フレーム数に応じて -th0 → +th0 へ線形に振れる。
//   5. 内側鱗弾は最初l0進んだところで静止し、静止距離は経過フレーム数に応じて
//      わずかにlずつ伸びていく(=結界の壁が少しずつ厚みを増していくイメージ)。
//   6. Tフレーム経過したら発射をやめ、静止させていた鱗弾を再び速さvで動かし始める。
//   7. ここまでを1周期とし、周期ごとにk,a,v,t1,t2,th0,l0,l,Tを変化させて無限ループする。
//
// 紫の色は色一覧(0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙)に存在しないため、
// 最も近いマゼンタ(5)で代用している。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  周期パラメータ
// ============================================================
struct sKekkaiCycleParam {
    int    k;    // 青・紫それぞれの大玉の数(大玉総数は2k個)
    double a;    // 大玉の公転角速度[rad/frame]
    double v;    // 内側鱗弾の速さ
    int    t1;   // 連続発射フレーム数
    int    t2;   // 休止フレーム数
    double th0;  // 発射方向の最大ズレ量[rad](-th0～+th0)
    double l0;   // 内側鱗弾の初期停止距離
    double l;    // 停止距離の増加量(1フレームあたり)
    int    T;    // 発射継続フレーム数
};

static const sKekkaiCycleParam g_kekkaiCycle[] = {
    // k,  a,      v,   t1, t2,  th0,   l0,   l,    T
    {  8,  0.015,  4.0,  3,  2,  0.35, 15.0, 0.6, 100 },
    {  6, -0.025,  5.0,  2,  1,  0.60, 10.0, 0.9,  90 },
    {  9,  0.020,  6.0,  3,  2,  0.45, 20.0, 0.5, 130 },
};
static const int kKekkaiCycleNum = sizeof(g_kekkaiCycle) / sizeof(g_kekkaiCycle[0]);

// ============================================================
//  固定定数(周期によらず共通)
// ============================================================
static const double kCenterX = 240.0; // 画面中央x(画面は480x480)
static const double kCenterY = 240.0; // 画面中央y
static const double kHalfScreenWidth = 240.0; // 画面幅の半分
static const double kRadiusGrowSpeed = 2.0;   // 大玉の半径拡大速度[px/frame]
static const int    kExpandFrames = 120;   // 半径拡大に要するフレーム数(=240/2.0)
static const double kOutwardSpeed = 2.5;   // 外側鱗弾の速さ(低速固定)
static const double kOutwardSpread = 0.14;  // 外側3個の扇の開き角[rad]
static const int    kReleaseClearFrames = 150; // 解放後、鱗弾・大玉が画面外へ抜けきる猶予フレーム数

// 大玉/鱗弾の役割タグ(param_i[0]で識別)
enum {
    KEKKAI_TYPE_BIGBALL = 0, // 中心の大玉本体
    KEKKAI_TYPE_INWARD = 1, // 内側へ撃つ鱗弾(l0+l*fireTで一旦静止し、解放後に再度動く)
    KEKKAI_TYPE_OUTWARD = 2, // 外側へ撃つ鱗弾(常に低速で流れ続ける)
};

// 弾幕：紫奥義「弾幕結界」
static void ShotKekkaiBarrier(sEnemyShotSet* pEnemyShotSet)
{
    const sKekkaiCycleParam& p = g_kekkaiCycle[pEnemyShotSet->param_i[0]];
    const int totalBalls = p.k * 2;
    const int fireEndFrame = kExpandFrames + p.T; // この大玉基準フレーム以降は解放

    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < totalBalls; i++) {
            sEnemyShot* pBall = new sEnemyShot;

            int colorIdx = i % 2; // 交互に青・紫を配置してk個ずつにする

            pBall->param_i[0] = KEKKAI_TYPE_BIGBALL;
            pBall->param_i[1] = colorIdx;               // 0:青 1:紫(マゼンタで代用)
            pBall->param_d[0] = 2.0 * DX_PI * i / totalBalls; // 円周上の初期配置角

            // 弾の種類と半径一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
            // 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            pBall->kind = (colorIdx == 0) ? img_enemyShotLargeBall[4] : img_enemyShotLargeBall[5];
            pBall->x = kCenterX;
            pBall->y = kCenterY;
            pBall->muki = pBall->param_d[0];

            pBall->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBall->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBall;
            pEnemyShotSet->pEnemyShotHead->prev = pBall;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 弾を追加してもループが壊れないよう先に保持しておく

        if (pShot->param_i[0] == KEKKAI_TYPE_BIGBALL) {
            int tBall = pShot->count;
            double angle = pShot->param_d[0] + p.a * tBall; // 画面中央から見た方向(公転)
            double radius;

            if (tBall < kExpandFrames) {
                // フェーズ1: 中心から回転しながら半径が広がっていく
                radius = kRadiusGrowSpeed * tBall;
            }
            else if (tBall < fireEndFrame) {
                // フェーズ2: 半径固定。結界を維持しながら鱗弾を発射する
                radius = kHalfScreenWidth;

                int fireT = tBall - kExpandFrames; // 発射開始からの経過フレーム数
                int period = p.t1 + p.t2;
                bool active = (fireT % period) < p.t1; // t1フレーム発射→t2フレーム休止

                if (active && pEnemyShotSet->count % 2 == 0) {
                    // 発射方向のズレ: -th0 → +th0 へ経過フレームに応じて線形に変化
                    double sweep = (p.T > 1)
                        ? (-p.th0 + 2.0 * p.th0 * fireT / (double)(p.T - 1))
                        : 0.0;

                    double bx = kCenterX + radius * cos(angle);
                    double by = kCenterY + radius * sin(angle);
                    int    colorIdx = pShot->param_i[1];
                    int    scaleKind = (colorIdx == 0) ? img_enemyShotScale[4] : img_enemyShotScale[5];

                    // 内側へ速さvで1個
                    {
                        sEnemyShot* pIn = new sEnemyShot;
                        pIn->param_i[0] = KEKKAI_TYPE_INWARD;
                        pIn->param_i[1] = tBall; // 生成時点の大玉基準フレーム(解放判定に使う)
                        pIn->param_d[0] = bx;                    // 発射位置x
                        pIn->param_d[1] = by;                    // 発射位置y
                        pIn->param_d[2] = angle + DX_PI + sweep; // 内側方向
                        pIn->param_d[3] = p.l0 + p.l * fireT;    // 静止距離(結界の厚みが少しずつ増す)
                        pIn->kind = scaleKind;
                        pIn->x = bx;
                        pIn->y = by;
                        pIn->muki = pIn->param_d[2];

                        pIn->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pIn->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pIn;
                        pEnemyShotSet->pEnemyShotHead->prev = pIn;
                    }

                    // 外側へ低速で3個(扇状に開く)
                    double outBase = angle + sweep;
                    for (int s = -1; s <= 1; s++) {
                        sEnemyShot* pOut = new sEnemyShot;
                        pOut->param_i[0] = KEKKAI_TYPE_OUTWARD;
                        pOut->param_d[0] = bx;
                        pOut->param_d[1] = by;
                        pOut->param_d[2] = outBase + kOutwardSpread * s;
                        pOut->kind = scaleKind;
                        pOut->x = bx;
                        pOut->y = by;
                        pOut->muki = pOut->param_d[2];

                        pOut->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pOut->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pOut;
                        pEnemyShotSet->pEnemyShotHead->prev = pOut;
                    }
                }
            }
            else {
                // フェーズ3: 発射終了。大玉自身も外側へ加速しながら飛散し、結界を解く
                int releaseT = tBall - fireEndFrame;
                radius = kHalfScreenWidth + 0.02 * releaseT * releaseT;
            }

            pShot->x = kCenterX + radius * cos(angle);
            pShot->y = kCenterY + radius * sin(angle);
            pShot->muki = angle;
        }
        else if (pShot->param_i[0] == KEKKAI_TYPE_INWARD) {
            int spawnFrame = pShot->param_i[1];         // 生成時点の大玉基準フレーム
            int nowFrame = spawnFrame + pShot->count; // 現在の大玉基準フレーム
            double freezeDist = pShot->param_d[3];
            double dist;

            if (nowFrame < fireEndFrame) {
                // 静止距離に達するまで内側へ進み、達したら静止する
                double moved = p.v * pShot->count;
                dist = (moved < freezeDist) ? moved : freezeDist;
            }
            else {
                // 解放後: 静止していた位置から再び速さvで内側へ進み続ける
                int afterRelease = nowFrame - fireEndFrame;
                dist = freezeDist + p.v * afterRelease;
            }

            pShot->x = pShot->param_d[0] + dist * cos(pShot->param_d[2]);
            pShot->y = pShot->param_d[1] + dist * sin(pShot->param_d[2]);
        }
        else { // KEKKAI_TYPE_OUTWARD
            double dist = kOutwardSpeed * pShot->count;
            pShot->x = pShot->param_d[0] + dist * cos(pShot->param_d[2]);
            pShot->y = pShot->param_d[1] + dist * sin(pShot->param_d[2]);
        }

        pShot = pNext;
    }
}

// 1周期に必要なフレーム数(半径拡大+発射+解放後の後始末猶予)
static int KekkaiCycleDuration(const sKekkaiCycleParam& p)
{
    return kExpandFrames + p.T + kReleaseClearFrames;
}

// 新しい周期の弾幕結界を1セット生成する
static void SpawnKekkaiCycle(int cycleIndex)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotKekkaiBarrier;
    pEnemyShotSet->x = kCenterX;
    pEnemyShotSet->y = kCenterY;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->param_i[0] = cycleIndex;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_DanmakuKekkai_Claude()
{
    static int cycleIndex;
    static int nextSpawnFrame;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        cycleIndex = 0;
        SpawnKekkaiCycle(cycleIndex);
        nextSpawnFrame = count + KekkaiCycleDuration(g_kekkaiCycle[cycleIndex]);
    }

    if (count == nextSpawnFrame) {
        cycleIndex = (cycleIndex + 1) % kKekkaiCycleNum;
        SpawnKekkaiCycle(cycleIndex);
        nextSpawnFrame = count + KekkaiCycleDuration(g_kekkaiCycle[cycleIndex]);
    }
}