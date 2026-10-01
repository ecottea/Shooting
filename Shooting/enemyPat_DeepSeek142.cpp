#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ペアノ曲線弾幕 「ペアノ・タペストリー ～空間充填の弾糸～」
//  ------------------------------------------------------------
//  ボスは1本の長い弾列を放ち、その先頭がペアノ曲線の第n次近似に
//  沿って画面を這う。弾は等間隔で経路を追従するため、画面に再帰的
//  な折り返しを持つ「動く線」が現れる。線そのものが弾壁で、線と
//  線の間の空白が避け道になる。
//
//  ※ 経路データはファイルスコープの静的配列に保持しているため、
//     このパターンは同時に1セットのみ有効。多重発動させたい場合は
//     経路データをセットごとに持つよう改造すること。
// ============================================================

#define PEANO_MAX_PATH  2000   // 経路ウェイポイント最大数
#define PEANO_SUBSTEPS  2      // 1回の前進で記録する中間点の数
#define PEANO_ORDER     2      // ペアノ曲線の次数（2 → 9x9 = 81セル）

static double g_peanoPathX[PEANO_MAX_PATH];
static double g_peanoPathY[PEANO_MAX_PATH];
static int    g_peanoPathCount = 0;

// ------------------------------------------------------------
//  タートルグラフィックス用ヘルパ
// ------------------------------------------------------------
static void MoveF(double& x, double& y, double& angle, int& idx)
{
    double stepLen = 1.0 / PEANO_SUBSTEPS;
    for (int s = 0; s < PEANO_SUBSTEPS; s++) {
        x += stepLen * cos(angle);
        y += stepLen * sin(angle);
        if (idx < PEANO_MAX_PATH) {
            g_peanoPathX[idx] = x;
            g_peanoPathY[idx] = y;
            idx++;
        }
    }
}

static void TurnL(double& angle) { angle += DX_PI / 2.0; }
static void TurnR(double& angle) { angle -= DX_PI / 2.0; }

// ------------------------------------------------------------
//  ペアノ曲線のL-system再帰展開
//    X → XFYFX+F+YFXFY-F-XFYFX
//    Y → YFXFY-F-XFXFY+F+YFXFY
//  F:前進  +:左回転90°  -:右回転90°
// ------------------------------------------------------------
static void PeanoRec(int order, bool isX,
    double& x, double& y, double& angle, int& idx)
{
    if (order == 0) return;

    if (isX) {
        PeanoRec(order - 1, true, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, false, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, true, x, y, angle, idx);
        TurnL(angle);
        MoveF(x, y, angle, idx);
        TurnL(angle);
        PeanoRec(order - 1, false, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, true, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, false, x, y, angle, idx);
        TurnR(angle);
        MoveF(x, y, angle, idx);
        TurnR(angle);
        PeanoRec(order - 1, true, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, false, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, true, x, y, angle, idx);
    }
    else {
        PeanoRec(order - 1, false, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, true, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, false, x, y, angle, idx);
        TurnR(angle);
        MoveF(x, y, angle, idx);
        TurnR(angle);
        PeanoRec(order - 1, true, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, false, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, true, x, y, angle, idx);
        TurnL(angle);
        MoveF(x, y, angle, idx);
        TurnL(angle);
        PeanoRec(order - 1, false, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, true, x, y, angle, idx);
        MoveF(x, y, angle, idx);
        PeanoRec(order - 1, false, x, y, angle, idx);
    }
}

// ------------------------------------------------------------
//  ペアノ経路を生成し、中心(cx,cy)・スケールscaleで配置
// ------------------------------------------------------------
static void BuildPeanoPath(int order, double cx, double cy, double scale)
{
    g_peanoPathCount = 0;
    double x = 0.0, y = 0.0, angle = 0.0;

    // 始点を記録
    if (g_peanoPathCount < PEANO_MAX_PATH) {
        g_peanoPathX[g_peanoPathCount] = x;
        g_peanoPathY[g_peanoPathCount] = y;
        g_peanoPathCount++;
    }

    PeanoRec(order, true, x, y, angle, g_peanoPathCount);

    // 3^order グリッドの中心へオフセットし、scale倍
    int gridSize = 1;
    for (int i = 0; i < order; i++) gridSize *= 3;
    double offset = (gridSize - 1) * 0.5;

    for (int i = 0; i < g_peanoPathCount; i++) {
        g_peanoPathX[i] = cx + (g_peanoPathX[i] - offset) * scale;
        g_peanoPathY[i] = cy + (g_peanoPathY[i] - offset) * scale;
    }
}

// ------------------------------------------------------------
//  弾幕パターン：ペアノ・タペストリー
// ------------------------------------------------------------
static void ShotPeano(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 予告・発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 経路ウェイポイント上に弾を配置
        for (int i = 0; i < g_peanoPathCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x + g_peanoPathX[i];
            pEnemyShot->y = pEnemyShotSet->y + g_peanoPathY[i];
            pEnemyShot->speed = 0.0;

            // 経路の接線から垂直方向を弾の向きに設定
            int next = (i + 4 < g_peanoPathCount) ? i + 4 : (g_peanoPathCount - 1);
            int prev = (i > 4) ? i - 4 : 0;
            double dx = g_peanoPathX[next] - g_peanoPathX[prev];
            double dy = g_peanoPathY[next] - g_peanoPathY[prev];
            pEnemyShot->muki = atan2(dx, -dy);

            // 再帰深度に応じて色を変化（ここでは位置で代用）
            int colorIdx = (i / 8) % 9;
            pEnemyShot->kind = img_enemyShotMediumBall[colorIdx];

            // param_d[0] を展開待ちタイマーとして使用
            pEnemyShot->param_d[0] = 0.0;

            // リストへ挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_d[0] += 1.0;

        // 一定時間静止した後、経路に垂直な方向へ加速しながら流れ出す
        if (pShot->param_d[0] > 60.0) {
            pShot->speed += 0.03;
            if (pShot->speed > 2.5) pShot->speed = 2.5;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_PeanoCurve_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;

        // ペアノ経路を一度だけ生成（画面中央付近に配置）
        BuildPeanoPath(PEANO_ORDER, 0.0, 0.0, 40.0);
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 240フレームごとに弾幕セットを発射
    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPeano;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 260.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}