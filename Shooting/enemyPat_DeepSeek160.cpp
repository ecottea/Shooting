#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：『双曲七角形牢獄 ― Poincaré {7,3}』  ― 多重化版 ―
//   ・画面をポアンカレ円板とみなす
//   ・弾は測地線（円弧）に沿って移動
//   ・分裂を3回繰り返し {7,3} タイル状に多層伝播
//   ・全体にメビウス変換 M_a(z)=(z+a)/(1+conj(a)z) を毎フレーム適用
// ============================================================

static const double PI_ = DX_PI;
static const double POI_CX = 240.0;   // 円板中心X（画面中心）
static const double POI_CY = 240.0;   // 円板中心Y
static const double POI_R = 240.0;   // 円板半径（画面に内接）

// メビウス変換 M_a(z) = (z + a) / (1 + conj(a) z)
static inline void MobiusApply(double u, double v, double ar, double ai,
    double* ou, double* ov)
{
    double nr = u + ar;
    double ni = v + ai;
    double dr = 1.0 + ar * u + ai * v;
    double di = ar * v - ai * u;
    double den = dr * dr + di * di;
    if (den < 1e-12) { *ou = u; *ov = v; return; }
    *ou = (nr * dr + ni * di) / den;
    *ov = (ni * dr - nr * di) / den;
}

// 測地線 a → b 上の点 (t ∈ [0,1]) をポアンカレ座標で返す
static void GeodesicPoint(double ar_, double ai_,
    double br, double bi,
    double t, double* ou, double* ov)
{
    double nr = br - ar_;
    double ni = bi - ai_;
    double dr = 1.0 - ar_ * br - ai_ * bi;
    double di = ai_ * br - ar_ * bi;
    double den = dr * dr + di * di;
    if (den < 1e-12) { *ou = ar_; *ov = ai_; return; }
    double wbr = (nr * dr + ni * di) / den;
    double wbi = (ni * dr - nr * di) / den;

    double wr = t * wbr;
    double wi = t * wbi;

    double ur = wr + ar_;
    double ui = wi + ai_;
    double vr = 1.0 + ar_ * wr + ai_ * wi;
    double vi = ar_ * wi - ai_ * wr;
    double vden = vr * vr + vi * vi;
    if (vden < 1e-12) { *ou = wr; *ov = wi; return; }
    *ou = (ur * vr + ui * vi) / vden;
    *ov = (ui * vr - ur * vi) / vden;
}

// 残り分裂数に応じた弾グラフィック
static int PickShotKind(int remainSplits)
{
    switch (remainSplits) {
    case 3:  return img_enemyShotLargeBall[4];   // 青の大玉
    case 2:  return img_enemyShotMediumBall[5];  // マゼンタ中玉
    case 1:  return img_enemyShotDiamond[1];     // 黄色の菱形弾
    default: return img_enemyShotSmallBall[6];   // 白い小玉
    }
}

// ------------------------------------------------------------
// 弾幕パターン本体
// ------------------------------------------------------------
static void ShotHyperbolicPrison(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ---- 初回：ボス位置から21方向へ測地線発射 ----
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double bu = (pEnemyShotSet->x - POI_CX) / POI_R;
        double bv = (pEnemyShotSet->y - POI_CY) / POI_R;

        // shot_count によりリング半径を少しずつ変えて視覚的な多層化
        double baseR = 0.55 + 0.03 * (double)(pEnemyShotSet->kind % 3);

        for (int i = 0; i < 21; i++) {
            pEnemyShot = new sEnemyShot;

            // 21方向に等間隔に発射（7角形対称を3重重ね合わせ）
            double ang = (double)i / 21.0 * 2.0 * PI_ - PI_ * 0.5;
            double tu = baseR * cos(ang);
            double tv = baseR * sin(ang);

            pEnemyShot->param_d[0] = bu;
            pEnemyShot->param_d[1] = bv;
            pEnemyShot->param_d[2] = bu;
            pEnemyShot->param_d[3] = bv;
            pEnemyShot->param_d[4] = tu;
            pEnemyShot->param_d[5] = tv;
            pEnemyShot->param_d[6] = 0.0;
            pEnemyShot->param_d[7] = 0.010 + 0.002 * (double)((i + pEnemyShotSet->kind) % 3);
            pEnemyShot->param_i[0] = 3;   // 残り分裂回数

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = PickShotKind(3);

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- メビウス変換パラメータ（グローバル count で全セット同期） ----
    double phase = (double)count / 900.0 * 2.0 * PI_;
    double A = 0.55;
    double ar = A * cos(phase);
    double ai = A * sin(phase);

    // ---- 各弾の更新 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        pShot->param_d[6] += pShot->param_d[7];

        if (pShot->param_d[6] >= 1.0) {
            int splits = pShot->param_i[0];

            if (splits > 0) {
                double cu = pShot->param_d[4];
                double cv = pShot->param_d[5];

                double cang = atan2(cv, cu);
                double r = sqrt(cu * cu + cv * cv);
                double nextR = r + 0.13;
                if (nextR > 0.94) nextR = 0.94;

                double dA = PI_ / 7.0;   // ±π/7 で七角形対称を保つ

                for (int s = 0; s < 2; s++) {
                    sEnemyShot* pChild = new sEnemyShot;
                    double na = cang + (s == 0 ? -dA : dA);
                    double ntu = nextR * cos(na);
                    double ntv = nextR * sin(na);

                    pChild->param_d[0] = cu;
                    pChild->param_d[1] = cv;
                    pChild->param_d[2] = cu;
                    pChild->param_d[3] = cv;
                    pChild->param_d[4] = ntu;
                    pChild->param_d[5] = ntv;
                    pChild->param_d[6] = 0.0;
                    pChild->param_d[7] = pShot->param_d[7] * 1.15;
                    pChild->param_i[0] = splits - 1;

                    pChild->x = pShot->x;
                    pChild->y = pShot->y;
                    pChild->muki = 0.0;
                    pChild->speed = 0.0;
                    pChild->kind = PickShotKind(splits - 1);

                    pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pChild->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
                    pEnemyShotSet->pEnemyShotHead->prev = pChild;
                }
            }

            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        else {
            double cu, cv;
            GeodesicPoint(pShot->param_d[2], pShot->param_d[3],
                pShot->param_d[4], pShot->param_d[5],
                pShot->param_d[6], &cu, &cv);
            pShot->param_d[0] = cu;
            pShot->param_d[1] = cv;

            double du, dv;
            MobiusApply(cu, cv, ar, ai, &du, &dv);

            pShot->x = POI_CX + du * POI_R;
            pShot->y = POI_CY + dv * POI_R;
        }

        pShot = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_GeometryTAS_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 36フレームごとに1波。21方向×3回分裂で1波あたり最大 21+42+84+168 = 315 発
    if (count % 36 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHyperbolicPrison;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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