// enemyPat_SanpeihouShoumei.cpp
//
// 三平方証明陣：三平方の定理の「正方形の組み替え証明」をモチーフにした弾幕。
// a=72, b=96, c=120 の 3:4:5 直角三角形を4体使用する。
//
// 【証明の構造】
//   一辺 (a+b) の外枠正方形の中に、合同な直角三角形4体を2通りに配置しても
//   外枠の面積は変わらない：
//     配置A = 4三角形 + 内側の傾いた正方形(面積 c^2)
//     配置B = 4三角形 + 正方形(面積 a^2) + 正方形(面積 b^2)
//   よって c^2 = a^2 + b^2。
//
// 【フェーズ構成】(count は敵開始からのフレーム数)
//   1-150   証立(構築)  外枠正方形が周に沿って描かれ、4三角形が外側から
//                        収束して配置Aを形成する
//   151-390 定常(攻撃)  配置Aのまま静止し、内側正方形PQRSの各辺中点と
//                        外枠の頂点から自機狙い3wayを交互に発射
//   391-570 換立(組替)  4三角形が配置Aから配置Bへイーズ補間で変形する
//                        (三角形T2は配置A=配置Bのため見た目上不動＝支点になる)
//   571-900 証明完了    630フレームまで静止保持(610以降は点滅で予告)、
//                        631フレームで全弾が中心から放射状に加速飛散し、
//                        自機狙いのQED弾で締める
//
// 頂点の対応関係(すべて outer square のローカル座標 → ワールド座標に平行移動):
//   A,B,C,D = 外枠正方形の四隅 / P,Q,R,S = 各辺を a:b に分ける点
//   PQRS = 配置Aで現れる、辺の長さcの傾いた内側正方形
//   T1: [A,P,S]→[A',M,D]   T2: [P,B,Q]→[P,B,Q](不動)
//   T3: [Q,C,R]→[Q,M,P]    T4: [R,D,S]→[M,N,D]

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  幾何定数
// ============================================================
static constexpr double FORM_CX = 240.0; // 図形全体の中心(ワールド座標)
static constexpr double FORM_CY = 260.0;

static constexpr double LEG_A = 72.0;  // 直角三角形の短辺
static constexpr double LEG_B = 96.0;  // 直角三角形の長辺
// 斜辺 c = 120.0 (= sqrt(a^2+b^2)、明示定数としては未使用)

static constexpr double SQ_X0 = FORM_CX - (LEG_A + LEG_B) / 2.0; // 外枠左上X = 156
static constexpr double SQ_Y0 = FORM_CY - (LEG_A + LEG_B) / 2.0; // 外枠左上Y = 176

// 外枠正方形の四隅
static constexpr double A_X = SQ_X0, A_Y = SQ_Y0;
static constexpr double B_X = SQ_X0 + LEG_A + LEG_B, B_Y = SQ_Y0;
static constexpr double C_X = SQ_X0 + LEG_A + LEG_B, C_Y = SQ_Y0 + LEG_A + LEG_B;
static constexpr double D_X = SQ_X0, D_Y = SQ_Y0 + LEG_A + LEG_B;

// 各辺を a:b に分ける点(配置Aで内側正方形PQRSの頂点になる)
static constexpr double P_X = SQ_X0 + LEG_A, P_Y = SQ_Y0;
static constexpr double Q_X = SQ_X0 + LEG_A + LEG_B, Q_Y = SQ_Y0 + LEG_A;
static constexpr double R_X = SQ_X0 + LEG_B, R_Y = SQ_Y0 + LEG_A + LEG_B;
static constexpr double S_X = SQ_X0, S_Y = SQ_Y0 + LEG_B;

// 配置Bで使う追加の分割点
static constexpr double AP_X = SQ_X0, AP_Y = SQ_Y0 + LEG_A; // A' (Aをbだけ下に平行移動)
static constexpr double M_X = SQ_X0 + LEG_A, M_Y = SQ_Y0 + LEG_A; // a^2 と b^2 の接点
static constexpr double N_X = SQ_X0 + LEG_A, N_Y = SQ_Y0 + LEG_A + LEG_B;

// ============================================================
//  タイムライン定数
// ============================================================
static constexpr int    SLIDE_START = 40;   // 三角形が外側から収束し始めるフレーム
static constexpr int    PH1_END = 150;  // 証立フェーズ終了
static constexpr int    SQUARE_DRAW_END = 70;   // 外枠正方形の描画完了フレーム
static constexpr int    PH3_START = 391;  // 換立フェーズ開始
static constexpr int    PH3_END = 570;  // 換立フェーズ終了(配置B完成)
static constexpr int    FLICKER_START = 610;  // 爆散予告の点滅開始
static constexpr int    EXPLODE_FRAME = 631;  // 爆散トリガー
static constexpr double EXPLODE_ACCEL = 0.02; // 爆散の加速度係数(t^2に乗算)

constexpr int T = 750;
int countT;

// ============================================================
//  補助関数
// ============================================================
static double Clamp01(double v)
{
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

static double EaseOutCubic(double t)
{
    double u = 1.0 - t;
    return 1.0 - u * u * u;
}

static double EaseInOutCubic(double t)
{
    if (t < 0.5) return 4.0 * t * t * t;
    double u = -2.0 * t + 2.0;
    return 1.0 - (u * u * u) / 2.0;
}

// sEnemyShotSet を1つ生成し、グローバルリストへ連結して返す
static sEnemyShotSet* NewShotSet(double x, double y, double muki, int kind, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;
    pSet->patternFunc = func;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// pShot を pSet のリストへ連結する
static void LinkShot(sEnemyShotSet* pSet, sEnemyShot* pShot)
{
    pShot->margin = 240;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ============================================================
//  弾幕：外枠正方形の輪郭 (A→B→C→D→A)
// ============================================================
static void ShotFormationSquareFrame(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const int n = 84; // 672(周長) / 8 間隔
        for (int i = 0; i < n; i++) {
            double s = (double)i / n; // 周上の位置 0.0〜1.0 (閉路のため1.0は含めない)
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_d[0] = s;
            pShot->param_i[0] = 0; // 0:未爆散 / 1:爆散開始位置キャプチャ済み
            pShot->kind = img_enemyShotDiamond[6]; // 白の菱形弾で外枠を表現
            pShot->x = A_X;
            pShot->y = A_Y;
            LinkShot(pSet, pShot);
        }
    }

    const double sxArr[5] = { A_X, B_X, C_X, D_X, A_X };
    const double syArr[5] = { A_Y, B_Y, C_Y, D_Y, A_Y };

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double s = pShot->param_d[0];
        int seg = (int)(s * 4.0);
        if (seg > 3) seg = 3;
        double localT = s * 4.0 - (double)seg;

        double px = sxArr[seg] + (sxArr[seg + 1] - sxArr[seg]) * localT;
        double py = syArr[seg] + (syArr[seg + 1] - syArr[seg]) * localT;

        // 証立フェーズ：始点から伸びるように描画される
        if (countT <= PH1_END) {
            double revealProgress = Clamp01((double)countT / (double)SQUARE_DRAW_END);
            double t_local = Clamp01((revealProgress - s) * 20.0 + 1.0);
            px = sxArr[seg] + (px - sxArr[seg]) * t_local;
            py = syArr[seg] + (py - syArr[seg]) * t_local;
        }

        // 爆散予告点滅
        if (countT >= FLICKER_START && countT < EXPLODE_FRAME) {
            pShot->kind = ((countT / 4) % 2 == 0) ? img_enemyShotDiamond[8] : img_enemyShotDiamond[6];
        }

        // 証明完了：中心から放射状に加速飛散
        if (countT >= EXPLODE_FRAME) {
            if (pShot->param_i[0] == 0) {
                pShot->param_i[0] = 1;
                pShot->param_d[1] = px;
                pShot->param_d[2] = py;
                pShot->param_d[3] = atan2(py - FORM_CY, px - FORM_CX);
            }
            double dt = (double)(countT - EXPLODE_FRAME);
            double ang = pShot->param_d[3];
            px = pShot->param_d[1] + EXPLODE_ACCEL * dt * dt * cos(ang);
            py = pShot->param_d[2] + EXPLODE_ACCEL * dt * dt * sin(ang);
        }

        pShot->x = px;
        pShot->y = py;
        pShot->muki = atan2(py - FORM_CY, px - FORM_CX);

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：直角三角形1体の輪郭(配置A⇔配置Bを補間)
//  pSet->param_d[0..5]  = 配置Aの頂点(v0,v1,v2)
//  pSet->param_d[6..11] = 配置Bの頂点(w0,w1,w2)
//  pSet->kind           = 弾の色番号
// ============================================================
static void ShotFormationTriangle(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        double vx[3] = { pSet->param_d[0], pSet->param_d[2], pSet->param_d[4] };
        double vy[3] = { pSet->param_d[1], pSet->param_d[3], pSet->param_d[5] };

        for (int e = 0; e < 3; e++) {
            int i0 = e;
            int i1 = (e + 1) % 3;
            double lx0 = vx[i0], ly0 = vy[i0];
            double lx1 = vx[i1], ly1 = vy[i1];
            double len = sqrt((lx1 - lx0) * (lx1 - lx0) + (ly1 - ly0) * (ly1 - ly0));
            int n = (int)(len / 8.0 + 0.5) + 1;
            if (n < 2) n = 2;

            for (int i = 0; i < n; i++) {
                double t = (double)i / (double)(n - 1);
                sEnemyShot* pShot = new sEnemyShot;
                pShot->param_i[0] = e;   // どの辺か(0,1,2)
                pShot->param_i[1] = 0;   // 0:未爆散 / 1:爆散開始位置キャプチャ済み
                pShot->param_d[0] = t;   // 辺上の位置 0.0〜1.0
                pShot->kind = img_enemyShotScale[pSet->kind % 8];
                pShot->x = lx0 + (lx1 - lx0) * t; // 初期値、この直後に上書きされる
                pShot->y = ly0 + (ly1 - ly0) * t;
                LinkShot(pSet, pShot);
            }
        }
    }

    double avx[3] = { pSet->param_d[0], pSet->param_d[2], pSet->param_d[4] };
    double avy[3] = { pSet->param_d[1], pSet->param_d[3], pSet->param_d[5] };
    double bvx[3] = { pSet->param_d[6], pSet->param_d[8], pSet->param_d[10] };
    double bvy[3] = { pSet->param_d[7], pSet->param_d[9], pSet->param_d[11] };

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int e = pShot->param_i[0];
        double t = pShot->param_d[0];
        int i0 = e;
        int i1 = (e + 1) % 3;

        double ax = avx[i0] + (avx[i1] - avx[i0]) * t;
        double ay = avy[i0] + (avy[i1] - avy[i0]) * t;
        double bx = bvx[i0] + (bvx[i1] - bvx[i0]) * t;
        double by = bvy[i0] + (bvy[i1] - bvy[i0]) * t;

        // 換立フェーズ：配置A→配置Bへ補間
        double morphT = 0.0;
        if (countT >= PH3_START) {
            morphT = Clamp01((double)(countT - PH3_START) / (double)(PH3_END - PH3_START));
            morphT = EaseInOutCubic(morphT);
        }
        double px = ax + (bx - ax) * morphT;
        double py = ay + (by - ay) * morphT;

        // 証立フェーズ：外側から収束するように出現
        if (countT <= PH1_END) {
            double slideT = Clamp01((double)(countT - SLIDE_START) / (double)(PH1_END - SLIDE_START));
            slideT = EaseOutCubic(slideT);
            double dx = px - FORM_CX, dy = py - FORM_CY;
            double dlen = sqrt(dx * dx + dy * dy);
            if (dlen < 1.0) dlen = 1.0;
            double sx = px + dx / dlen * 220.0;
            double sy = py + dy / dlen * 220.0;
            px = sx + (px - sx) * slideT;
            py = sy + (py - sy) * slideT;
        }

        // 爆散予告点滅
        if (countT >= FLICKER_START && countT < EXPLODE_FRAME) {
            pShot->kind = ((countT / 4) % 2 == 0) ? img_enemyShotScale[6] : img_enemyShotScale[pSet->kind % 8];
        }

        // 証明完了：中心から放射状に加速飛散
        if (countT >= EXPLODE_FRAME) {
            if (pShot->param_i[1] == 0) {
                pShot->param_i[1] = 1;
                pShot->param_d[1] = px;
                pShot->param_d[2] = py;
                pShot->param_d[3] = atan2(py - FORM_CY, px - FORM_CX);
            }
            double dt = (double)(count - EXPLODE_FRAME);
            double ang = pShot->param_d[3];
            px = pShot->param_d[1] + EXPLODE_ACCEL * dt * dt * cos(ang);
            py = pShot->param_d[2] + EXPLODE_ACCEL * dt * dt * sin(ang);
        }

        pShot->x = px;
        pShot->y = py;
        pShot->muki = atan2(py - FORM_CY, px - FORM_CX);

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：自機狙い3way(直進、count駆動の式で位置計算)
// ============================================================
static void ShotAimed3Way(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double spreadArr[3] = { -0.18, 0.0, 0.18 };
        for (int i = 0; i < 3; i++) {
            double muki = pSet->muki + spreadArr[i];
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_d[0] = pSet->x;   // 発射位置X
            pShot->param_d[1] = pSet->y;   // 発射位置Y
            pShot->param_d[2] = muki;      // 向き
            pShot->param_d[3] = 2.2;       // 速さ
            pShot->muki = muki;
            pShot->kind = img_enemyShotMediumBall[8]; // 橙
            LinkShot(pSet, pShot);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double sx = pShot->param_d[0], sy = pShot->param_d[1];
        double muki = pShot->param_d[2], speed = pShot->param_d[3];
        pShot->x = sx + speed * (double)pShot->count * cos(muki);
        pShot->y = sy + speed * (double)pShot->count * sin(muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  発射ヘルパー
// ============================================================
static void SpawnTriangle(
    double ax0, double ay0, double ax1, double ay1, double ax2, double ay2,
    double bx0, double by0, double bx1, double by1, double bx2, double by2,
    int colorIdx)
{
    sEnemyShotSet* pSet = NewShotSet(0.0, 0.0, 0.0, colorIdx, ShotFormationTriangle);
    pSet->param_d[0] = ax0; pSet->param_d[1] = ay0;
    pSet->param_d[2] = ax1; pSet->param_d[3] = ay1;
    pSet->param_d[4] = ax2; pSet->param_d[5] = ay2;
    pSet->param_d[6] = bx0; pSet->param_d[7] = by0;
    pSet->param_d[8] = bx1; pSet->param_d[9] = by1;
    pSet->param_d[10] = bx2; pSet->param_d[11] = by2;
}

static void SpawnAimed3Way(double x, double y)
{
    double muki = atan2(player.y - y, player.x - x);
    NewShotSet(x, y, muki, 8, ShotAimed3Way);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_PythagoreanTheorem_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    countT = count % T;

    if (countT == 1) {
        // 外枠正方形
        NewShotSet(0.0, 0.0, 0.0, 6, ShotFormationSquareFrame);

        // 4つの直角三角形 (配置A → 配置B)
        // T1: [A,P,S] → [A',M,D]
        SpawnTriangle(A_X, A_Y, P_X, P_Y, S_X, S_Y,
            AP_X, AP_Y, M_X, M_Y, D_X, D_Y,
            0); // 赤
        // T2: [P,B,Q] → [P,B,Q] (不動、換立フェーズの支点)
        SpawnTriangle(P_X, P_Y, B_X, B_Y, Q_X, Q_Y,
            P_X, P_Y, B_X, B_Y, Q_X, Q_Y,
            1); // 黄
        // T3: [Q,C,R] → [Q,M,P]
        SpawnTriangle(Q_X, Q_Y, C_X, C_Y, R_X, R_Y,
            Q_X, Q_Y, M_X, M_Y, P_X, P_Y,
            2); // 緑
        // T4: [R,D,S] → [M,N,D]
        SpawnTriangle(R_X, R_Y, D_X, D_Y, S_X, S_Y,
            M_X, M_Y, N_X, N_Y, D_X, D_Y,
            4); // 青
    }

    // 定常フェーズ：内側正方形PQRSの各辺中点と外枠の頂点から交互に自機狙い3way
    if (countT == 170) SpawnAimed3Way((P_X + Q_X) / 2.0, (P_Y + Q_Y) / 2.0);
    if (countT == 210) SpawnAimed3Way(B_X, B_Y);
    if (countT == 250) SpawnAimed3Way((Q_X + R_X) / 2.0, (Q_Y + R_Y) / 2.0);
    if (countT == 290) SpawnAimed3Way(C_X, C_Y);
    if (countT == 330) SpawnAimed3Way((R_X + S_X) / 2.0, (R_Y + S_Y) / 2.0);
    if (countT == 370) SpawnAimed3Way(D_X, D_Y);

    // 証明完了：爆散音とQEDの自機狙い3way
    if (countT == EXPLODE_FRAME) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
    if (countT == 660) SpawnAimed3Way(FORM_CX, FORM_CY);
}
