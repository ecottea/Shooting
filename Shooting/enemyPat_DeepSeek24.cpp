// enemyPat_Tmp.cpp
// メビウスの帯をモチーフにした弾幕（曲線＋飛び飛びの横断線）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// メビウスの帯パターン（曲線＋横断線を飛び飛びに配置）
// ------------------------------------------------------------
static void PatternMobius(sEnemyShotSet* pEnemyShotSet)
{
    const double R = 170.0;   // 主半径
    const double W = 100.0;    // 帯の半幅
    const double SPEED = 0.005;    // 回転速度

    if (pEnemyShotSet->count == 0) {
        // 効果音
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int num_t_curve = 48;   // 曲線の周方向分割数
        const int num_s_curve = 5;    // 曲線の本数（s方向の分割数）
        const int num_t_bridge = 12;  // 横断線の数（周方向に何本配置するか）
        const int num_s_bridge = 8;   // 各横断線内の点の数（隣接曲線間の分割数）

        // --- 曲線用の弾を生成（メビウスの帯の経線） ---
        for (int it = 0; it < num_t_curve; ++it) {
            double t0 = (2.0 * DX_PI * it) / num_t_curve;
            for (int is = 0; is < num_s_curve; ++is) {
                double s = -W + (2.0 * W * is) / (num_s_curve - 1);

                sEnemyShot* pShot = new sEnemyShot;
                pShot->param_d[0] = t0;   // 初期位相
                pShot->param_d[1] = s;    // 幅パラメータ
                pShot->param_i[0] = 0;    // 種類: 0=曲線

                // 色：赤(0)とシアン(3)でメビウスの表裏を表現
                pShot->kind = (s < 0.0) ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[3];

                // 初期座標
                double cos_t = cos(t0), sin_t = sin(t0);
                double half_t = t0 / 2.0;
                double r = R + s * cos(half_t);
                pShot->x = pEnemyShotSet->x + r * cos_t;
                pShot->y = pEnemyShotSet->y + r * sin_t + s * sin(half_t) * 0.5;

                pShot->speed = 0.0;
                pShot->muki = 0.0;
                pShot->count = 0;
                pShot->margin = 420.0;

                // 双方向リストに挿入
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        // --- 横断線用の弾を生成（飛び飛びのt位置で、隣接曲線間を結ぶ） ---
        for (int ib = 0; ib < num_t_bridge; ++ib) {
            double t0 = (2.0 * DX_PI * ib) / num_t_bridge;  // 横断線の初期位相

            // 隣接する曲線のペア (is と is+1) の間を結ぶ
            for (int is = 0; is < num_s_curve - 1; ++is) {
                if ((ib + 2) % 4 == is) continue;
                
                double s1 = -W + (2.0 * W * is) / (num_s_curve - 1);
                double s2 = -W + (2.0 * W * (is + 1)) / (num_s_curve - 1);
               
                // s1 から s2 までを num_s_bridge 個の点で補間（両端は曲線の点と重ならないようにする）
                for (int js = 0; js < num_s_bridge; ++js) {
                    double alpha = (double)(js + 1) / (num_s_bridge + 1); // 0<alpha<1
                    double s = s1 + alpha * (s2 - s1);

                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->param_d[0] = t0;   // 横断線の基準位相
                    pShot->param_d[1] = s;    // 幅パラメータ
                    pShot->param_i[0] = 1;    // 種類: 1=横断線

                    // 横断線の色は白(6)で統一
                    pShot->kind = img_enemyShotSmallBall[6];

                    // 初期座標
                    double cos_t = cos(t0), sin_t = sin(t0);
                    double half_t = t0 / 2.0;
                    double r = R + s * cos(half_t);
                    pShot->x = pEnemyShotSet->x + r * cos_t;
                    pShot->y = pEnemyShotSet->y + r * sin_t + s * sin(half_t) * 0.5;

                    pShot->speed = 0.0;
                    pShot->muki = 0.0;
                    pShot->count = 0;
                    pShot->margin = 420.0;

                    // 双方向リストに挿入
                    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pShot;
                }
            }
        }
    }

    // 毎フレーム：全弾の位置を更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->param_d[0] + SPEED * pEnemyShotSet->count;
        double s = pShot->param_d[1];

        double cos_t = cos(t);
        double sin_t = sin(t);
        double half_t = t / 2.0;
        double r = R + s * cos(half_t);

        pShot->x = pEnemyShotSet->x + r * cos_t;
        pShot->y = pEnemyShotSet->y + r * sin_t + s * sin(half_t) * 0.5;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Mobius_DeepSeek()
{
    static int move_dir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
        move_dir = 1;

        // メビウスの帯弾幕セットを一つだけ生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = PatternMobius;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリストに接続
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 敵の飾り移動
    enemy.x += 1.0 * move_dir;
    if (enemy.x > 360.0 || enemy.x < 120.0)
        move_dir *= -1;
}
