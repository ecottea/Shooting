// enemyPat_Mobius.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotMobius(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // =====================================================
    // 初期生成
    // =====================================================
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int U_DIV = 24;
        const int V_DIV = 5;

        for (int vIdx = 0; vIdx < V_DIV; vIdx++)
        {
            double v = -40.0 + vIdx * 20.0;

            for (int uIdx = 0; uIdx < U_DIV; uIdx++)
            {
                sEnemyShot* pShot = new sEnemyShot;

                pShot->speed = 0.0;
                pShot->muki = 0.0;

                pShot->kind = img_enemyShotDiamond[vIdx + 2];

                pShot->param_i[0] = uIdx;
                pShot->param_i[1] = vIdx;

                pShot->param_d[0] = (DX_PI * 2.0 * uIdx) / U_DIV;
                pShot->param_d[1] = v;

                pShot->margin = 480;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // =====================================================
    // メビウス帯更新
    // =====================================================

    const double R = 110.0;

    double globalRot = pEnemyShotSet->count * 0.015;
    double advance = pEnemyShotSet->count * 1.1;

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        double u0 = pEnemyShot->param_d[0];
        double v = pEnemyShot->param_d[1];

        double u = u0 + globalRot;

        // -----------------------------------------
        // メビウス帯
        // -----------------------------------------
        double x3 =
            (R + v * cos(u * 0.5))
            * cos(u);

        double y3 =
            (R + v * cos(u * 0.5))
            * sin(u);

        double z3 =
            v * sin(u * 0.5);

        // -----------------------------------------
        // 擬似3D回転
        // -----------------------------------------
        double rotX = pEnemyShotSet->count * 0.010;

        double cy = cos(rotX);
        double sy = sin(rotX);

        double yy = y3 * cy - z3 * sy;
        double zz = y3 * sy + z3 * cy;

        // -----------------------------------------
        // 崩壊フェーズ
        // -----------------------------------------
        if (pEnemyShotSet->count > 240)
        {
            double t =
                (pEnemyShotSet->count - 240) / 120.0;

            if (t > 1.0)
                t = 1.0;

            double dir =
                atan2(yy, x3);

            x3 += cos(dir) * 180.0 * t;
            yy += sin(dir) * 180.0 * t;
        }

        // -----------------------------------------
        // プレイヤー方向へ流す
        // -----------------------------------------
        double cx = pEnemyShotSet->x;
        double cy2 = pEnemyShotSet->y + advance;

        pEnemyShot->x = cx + x3;

        pEnemyShot->y =
            cy2
            + yy
            + zz * 0.45;

        // 崩壊完了後は通常弾化
        if (pEnemyShotSet->count == 360)
        {
            double dx =
                pEnemyShot->x - cx;

            double dy =
                pEnemyShot->y - cy2;

            pEnemyShot->muki =
                atan2(dy, dx);

            pEnemyShot->speed = 2.5;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// =========================================================
// 敵本体
// =========================================================

void EnemyPat_Mobius_ChatGPT()
{
    static int movePhase;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 40.0;

        enemy.hp = 200;
        enemy.maxHp = 200;

        movePhase = 0;
    }

    // -----------------------------------------
    // 緩やかな∞軌道
    // -----------------------------------------
    enemy.x =
        240.0
        + cos(count * 0.015) * 120.0;

    enemy.y =
        70.0
        + sin(count * 0.030) * 25.0;

    // -----------------------------------------
    // メビウス帯生成
    // -----------------------------------------
    if (count % 280 == 1)
    {
        sEnemyShotSet* pEnemyShotSet =
            new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc =
            ShotMobius;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead =
            new sEnemyShot;

        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev =
            enemyShotSetHead.prev;

        pEnemyShotSet->next =
            &enemyShotSetHead;

        enemyShotSetHead.prev->next =
            pEnemyShotSet;

        enemyShotSetHead.prev =
            pEnemyShotSet;
    }
}