#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：空間充填・ペアノ螺旋
//
//  フェーズ1: 3x3 のペアノ曲線をガイド弾がなぞり、各セルで自機狙い3wayを撃つ
//  フェーズ2: 9x9 に細分化。ガイド弾が高速で曲線をなぞり、各セルから下向きの遅い弾を置く
//             曲線の折り返しセルでは横3連にして密度を上げる
//  フェーズ3: 置かれた弾が一瞬停止し、曲線の順番どおりに波のように自機へ加速する
// ============================================================

// ------------------------------------------------------------
//  定数
// ------------------------------------------------------------
// ペアノ曲線を重ねる格子領域（ゲーム画面は 480x480、ボスの下に配置）
static const double GRID_LEFT = 60.0;
static const double GRID_TOP = 50.0;
static const double GRID_W = 360.0;
static const double GRID_H = 240.0;

// 時間割（弾幕セットの count 基準。60fps）
static const int PH1_START = 30;                                // 予告のための待ち時間
static const int PH1_STEP = 33;                                // 1セルあたりのフレーム数
static const int PH1_NUM = 9;                                 // 3x3
static const int PH1_END = PH1_START + PH1_STEP * PH1_NUM;    // 約5秒
static const int PH2_START = PH1_END + 30;                      // フェーズ間の間
static const int PH2_STEP = 6/2;
static const int PH2_NUM = 81;                                // 9x9
static const int PH2_END = PH2_START + PH2_STEP * PH2_NUM;    // 約8秒
static const int PH3_HOLD = 30;                                // 全弾停止の時間（約0.5秒）
static const int PH3_WAVE = 2;                                 // 加速開始を曲線順にずらす間隔(フレーム)
static const int PATTERN_CYCLE = 1200-400;                          // 弾幕を繰り返す周期

// 弾の速度
static const double AIMED_SPEED = 2.0;      // フェーズ1の自機狙い弾
static const double WALL_DRIFT = 0.35;     // フェーズ2の壁弾がゆっくり落ちる速さ
static const double WALL_ACCEL = 0.03;     // フェーズ3の加速度
static const double WALL_MAX = 3.4;      // フェーズ3の最高速度

// 弾の役割（param_i[0]）
static const int TYPE_NONE = 0;    // 何もしない（画面外へ飛ばしたガイド弾など）
static const int TYPE_GUIDE = 1;    // ガイド弾（曲線上を移動するだけ）
static const int TYPE_AIMED = 2;    // フェーズ1の自機狙い弾（直進）
static const int TYPE_WALL = 3;    // フェーズ2〜3の壁弾
// param_i の添字
static const int P_TYPE = 0;       // 弾の役割
static const int P_IDX = 1;       // 曲線上の通し番号（加速開始の順序に使う）
static const int P_STATE = 2;       // 壁弾の状態 0:ゆっくり落下 1:停止 2:加速中

// 壁弾の色（曲線を9セルずつ区切って色分けし、辿った順序が見えるようにする）
static const int WALL_PALETTE[7] = { 0, 8, 1, 2, 3, 4, 5 };

// ------------------------------------------------------------
//  ペアノ曲線
// ------------------------------------------------------------
// level 段階のペアノ曲線の d 番目(0始まり)のセル座標(x, y)を求める。
// 格子は 3^level x 3^level。d は 0 〜 9^level - 1。
// d を3進数で表した桁 a1 a2 a3 a4 ... のうち、
//   x の各桁 = 奇数番目の桁（それまでの偶数番目の桁の和が奇数なら 2-a に反転）
//   y の各桁 = 偶数番目の桁（それまでの奇数番目の桁の和が奇数なら 2-a に反転）
// とすることで、隣り合うセルが必ずつながる曲線になる。
static void PeanoCell(int level, int d, int* px, int* py)
{
    int digit[8];
    int n2 = level * 2;
    int v = d;
    for (int i = n2 - 1; i >= 0; i--) {
        digit[i] = v % 3;
        v /= 3;
    }
    int sumOdd = 0, sumEven = 0;
    int x = 0, y = 0;
    for (int i = 0; i < level; i++) {
        int aOdd = digit[2 * i];
        int aEven = digit[2 * i + 1];
        int xd = (sumEven % 2) ? 2 - aOdd : aOdd;
        sumOdd += aOdd;
        int yd = (sumOdd % 2) ? 2 - aEven : aEven;
        sumEven += aEven;
        x = x * 3 + xd;
        y = y * 3 + yd;
    }
    *px = x;
    *py = y;
}

// d 番目のセルの中心の画面座標
static void PeanoPos(int level, int d, double* gx, double* gy)
{
    int n = 1;
    for (int i = 0; i < level; i++) n *= 3;
    int x, y;
    PeanoCell(level, d, &x, &y);
    *gx = GRID_LEFT + (x + 0.5) * GRID_W / n;
    *gy = GRID_TOP + (y + 0.5) * GRID_H / n;
}

// フェーズ開始から tt フレーム後のガイド弾の位置（セル間を直線補間）
static void GuidePos(int level, int step, int num, int tt, double* gx, double* gy)
{
    int k = tt / step;
    if (k > num - 1) k = num - 1;
    PeanoPos(level, k, gx, gy);
    if (k < num - 1) {
        double nx, ny;
        double frac = (tt % step) / (double)step;
        PeanoPos(level, k + 1, &nx, &ny);
        *gx += (nx - *gx) * frac;
        *gy += (ny - *gy) * frac;
    }
}

// d 番目のセルで曲線が折り返す（進行方向が変わる）か
static bool IsTurn(int level, int d, int num)
{
    if (d <= 0 || d >= num - 1) return false;
    int x0, y0, x1, y1, x2, y2;
    PeanoCell(level, d - 1, &x0, &y0);
    PeanoCell(level, d, &x1, &y1);
    PeanoCell(level, d + 1, &x2, &y2);
    return (x1 - x0 != x2 - x1) || (y1 - y0 != y2 - y1);
}

// ------------------------------------------------------------
//  弾の生成補助
// ------------------------------------------------------------
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y,
    double muki, double speed, int kind, int type, int idx)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[P_TYPE] = type;
    pEnemyShot->param_i[P_IDX] = idx;
    pEnemyShot->param_i[P_STATE] = 0;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ガイド弾を探す（画面外の弾はメインルーチンが消すため、ポインタは保持せず毎回探す）
static sEnemyShot* FindGuide(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[P_TYPE] == TYPE_GUIDE) return pShot;
        pShot = pShot->next;
    }
    return nullptr;
}

// ガイド弾を画面外へ飛ばして役目を終えさせる（消去はメインルーチンに任せる）
static void RetireGuide(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pGuide = FindGuide(pEnemyShotSet);
    if (pGuide) {
        pGuide->param_i[P_TYPE] = TYPE_NONE;
        pGuide->x = -200.0;
        pGuide->y = -200.0;
    }
}

static void PlayShotSound(int sound)
{
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);
}

// ------------------------------------------------------------
//  壁弾の挙動（フェーズ2〜3）
// ------------------------------------------------------------
static void UpdateWallShot(sEnemyShot* pShot, int t)
{
    int state = pShot->param_i[P_STATE];
    // フェーズ2終了：全弾が一瞬停止
    if (state == 0 && t >= PH2_END) {
        state = 1;
        pShot->speed = 0.0;
    }
    // 曲線の通し番号に応じて、順番に自機へ向けて動き出す
    if (state == 1 && t >= PH2_END + PH3_HOLD + pShot->param_i[P_IDX] * PH3_WAVE) {
        state = 2;
        pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
        pShot->speed = 0.0;
    }
    if (state == 2) {
        pShot->speed += WALL_ACCEL;
        if (pShot->speed > WALL_MAX) pShot->speed = WALL_MAX;
    }
    pShot->param_i[P_STATE] = state;
    pShot->x += pShot->speed * cos(pShot->muki);
    pShot->y += pShot->speed * sin(pShot->muki);
}

// ------------------------------------------------------------
//  弾幕：空間充填・ペアノ螺旋
// ------------------------------------------------------------
static void ShotPeanoSpiral(sEnemyShotSet* pEnemyShotSet)
{
    const int t = pEnemyShotSet->count;

    // --- 予告音 ---
    if (t == 0 || t == PH1_END || t == PH2_END) {
        PlayShotSound(sound_enemyCharge);
    }

    // --- フェーズ1：3x3 ---
    if (t == PH1_START) {
        AddShot(pEnemyShotSet, 0.0, 0.0, 0.0, 0.0, img_enemyShotLargeBall[6], TYPE_GUIDE, 0);
    }
    if (t >= PH1_START && t < PH1_END) {
        int tt = t - PH1_START;
        double gx, gy;
        GuidePos(1, PH1_STEP, PH1_NUM, tt, &gx, &gy);
        sEnemyShot* pGuide = FindGuide(pEnemyShotSet);
        if (pGuide) {
            pGuide->x = gx;
            pGuide->y = gy;
        }
        // セルに到達した瞬間に自機狙い3way
        if (tt % PH1_STEP == 0) {
            int k = tt / PH1_STEP;
            double aim = atan2(player.y - gy, player.x - gx);
            for (int j = -1-4; j <= 1+4; j++) {
                AddShot(pEnemyShotSet, gx, gy, aim + j * 0.25/2, AIMED_SPEED,
                    img_enemyShotMediumBall[1], TYPE_AIMED, k);
            }
            PlayShotSound(sound_enemyShot_medium);
        }
    }
    if (t == PH1_END) {
        RetireGuide(pEnemyShotSet);
    }

    // --- フェーズ2：9x9 ---
    if (t == PH2_START) {
        AddShot(pEnemyShotSet, 0.0, 0.0, 0.0, 0.0, img_enemyShotSmallBall[6], TYPE_GUIDE, 0);
    }
    if (t >= PH2_START && t < PH2_END) {
        int tt = t - PH2_START;
        double gx, gy;
        GuidePos(2, PH2_STEP, PH2_NUM, tt, &gx, &gy);
        sEnemyShot* pGuide = FindGuide(pEnemyShotSet);
        if (pGuide) {
            pGuide->x = gx;
            pGuide->y = gy;
        }
        // セルに到達した瞬間に下向きの遅い弾を置く（折り返しセルは横3連で密度を上げる）
        if (tt % PH2_STEP == 0) {
            int k = tt / PH2_STEP;
            int color = WALL_PALETTE[(k / 9) % 7];
            if (IsTurn(2, k, PH2_NUM)) {
                for (int j = -1; j <= 1; j++) {
                    AddShot(pEnemyShotSet, gx + j * 9.0, gy, DX_PI / 2.0, WALL_DRIFT,
                        img_enemyShotSmallBall[color], TYPE_WALL, k);
                }
            }
            else {
                AddShot(pEnemyShotSet, gx, gy, DX_PI / 2.0, WALL_DRIFT,
                    img_enemyShotSmallBall[color], TYPE_WALL, k);
            }
            PlayShotSound(sound_enemyShot_light);
        }
    }
    if (t == PH2_END) {
        RetireGuide(pEnemyShotSet);
    }

    // --- フェーズ3：収束開始の合図 ---
    if (t == PH2_END + PH3_HOLD) {
        PlayShotSound(sound_enemyShot_heavy);
    }

    // --- 全弾の移動 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[P_TYPE]) {
        case TYPE_AIMED:
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        case TYPE_WALL:
            UpdateWallShot(pShot, t);
            break;
        default:    // ガイド弾は位置を上で直接設定しているので何もしない
            break;
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_PeanoCurve_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    // 一定周期で弾幕を発動（count == 1 で最初の1回目）
    if ((count - 1) % PATTERN_CYCLE == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPeanoSpiral;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
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