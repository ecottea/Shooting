// enemyPat_Tmp.cpp
// 双曲線モチーフ弾幕パターン（完全リメイク・シンプル版）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ========================
// 双曲線モチーフ弾幕：ShotHyperbola
// ========================
static void ShotHyperbola(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int NUM = 130;               // 片側13発
        const double SPREAD = 0.25;       // 広がり角度をかなり控えめに

        for (int side = 0; side < 2; ++side) {
            double sign = (side == 0) ? 1.0 : -1.0;

            for (int i = 0; i < NUM; ++i) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                // 双曲線風角度配置（中央密・外側開く）
                double t = (double)i / (NUM - 1) * 1.6 - 0.3;
                double angleOffset = sign * atan(sinh(t * 0.85)) * SPREAD;

                pEnemyShot->muki = pEnemyShotSet->muki + angleOffset;
                pEnemyShot->speed = 2.9 + (double)i * 0.06;

                // 弾種・色を大幅に絞る（青系のみ）
                int col = 4; // 青
                if (i % 3 == 0) {
                    pEnemyShot->kind = img_enemyShotDiamond[col];     // 菱形弾
                }
                else {
                    pEnemyShot->kind = img_enemyShotMediumBall[col];  // 中玉
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾移動処理（極めてシンプルに）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本進行方向のみ（双曲線曲がりは最小限に）
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 軽い双曲線風加速（外側へ少し広がる程度）
        double accel = 0.012 * pEnemyShot->count;
        pEnemyShot->x += accel * cos(pEnemyShot->muki + DX_PI / 2);
        pEnemyShot->y += accel * sin(pEnemyShot->muki + DX_PI / 2);

        pEnemyShot->count++;
        pEnemyShot = pEnemyShot->next;
    }
}

// ========================
// 敵本体パターン
// ========================
void EnemyPat_Hyperbola_Grok()
{
    static int muki = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.1 * (double)muki;
        if (enemy.x < 90.0 || enemy.x > 390.0) muki *= -1;
        enemy.y = 55.0 + 13.0 * sin(count * 0.03);
    }

    if (count % 38 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHyperbola;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // 自機方向を正確に（補正なし）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}