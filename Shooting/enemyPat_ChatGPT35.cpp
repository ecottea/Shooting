// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotTwinWinder(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //============================================================
    // 発射
    //============================================================
    if (pEnemyShotSet->count <= 180)
    {
        if ((pEnemyShotSet->count % 2) == 0)
        {
            if (CheckSoundMem(sound_enemyShot_noize) == 1)
                StopSoundMem(sound_enemyShot_noize);
            PlaySoundMem(sound_enemyShot_noize, DX_PLAYTYPE_BACK);
        }

        double t = pEnemyShotSet->count;

        double amp;
        double dirOffset;

        if (t < 90.0)
        {
            // ワインダー
            amp = 34.0;
            dirOffset = 0.0;
        }
        else
        {
            // 収束→分裂
            double r = (t - 90.0) / 90.0;

            if (r < 0.5)
            {
                double k = r / 0.5;
                amp = 34.0 * (1.0 - k);
                dirOffset = 0.0;
            }
            else
            {
                double k = (r - 0.5) / 0.5;
                amp = 34.0 * k;
                dirOffset = 25.0 / 180.0 * DX_PI * k;
            }
        }

        double ofs =
            sin(t * 0.23) * amp;

        for (int side = -1; side <= 1; side += 2)
        {
            pEnemyShot = new sEnemyShot;

            double base =
                pEnemyShotSet->muki +
                side * DX_PI / 2.0;

            pEnemyShot->x =
                pEnemyShotSet->x +
                cos(base) * ofs * side;

            pEnemyShot->y =
                pEnemyShotSet->y +
                sin(base) * ofs * side;

            pEnemyShot->muki =
                pEnemyShotSet->muki +
                side * dirOffset;

            pEnemyShot->speed = 3.4;

            pEnemyShot->kind =
                img_enemyShotMediumBall[
                    (side < 0) ? 4 : 1
                ];

            pEnemyShot->prev =
                pEnemyShotSet->pEnemyShotHead->prev;

            pEnemyShot->next =
                pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->pEnemyShotHead->prev->next =
                pEnemyShot;

            pEnemyShotSet->pEnemyShotHead->prev =
                pEnemyShot;
        }
    }

    //============================================================
    // 更新
    //============================================================
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        pEnemyShot->x +=
            cos(pEnemyShot->muki) * pEnemyShot->speed;

        pEnemyShot->y +=
            sin(pEnemyShot->muki) * pEnemyShot->speed;

        pEnemyShot = pEnemyShot->next;
    }
}

//============================================================
// 敵本体
//============================================================

void EnemyPat_Winder_ChatGPT()
{
    static int muki;

    if (count == 1)
    {
        enemy.x = 240;
        enemy.y = 40;

        enemy.maxHp = 200;
        enemy.hp = 200;

        muki = 1;
    }
    else
    {
        enemy.x += muki * 0.8;

        if (enemy.x < 120) muki = 1;
        if (enemy.x > 360) muki = -1;
    }

    //============================================================
    // ワインダー生成
    //============================================================
    if (count % 240 == 1)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTwinWinder;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = 15;

        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    //============================================================
    // 次のサイクル開始前に向きを更新
    //============================================================
    if (count % 240 == 180)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTwinWinder;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = 15;

        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

        pEnemyShotSet->kind = 1;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
