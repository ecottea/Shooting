// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 吹雪の雪片
// ============================================================
static void ShotBlizzard(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射
    if (pEnemyShotSet->count == 0)
    {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 18; i++)
        {
            pEnemyShot = new sEnemyShot;

            // 横に広く散らす
            pEnemyShot->x = pEnemyShotSet->x + GetRand(440) - 220;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;

            // 下向きを基本に少し傾ける
            pEnemyShot->muki =
                DX_PI / 2.0 +
                (GetRand(40) - 20) * DX_PI / 180.0;

            pEnemyShot->speed =
                1.2 +
                GetRand(120) / 100.0;

            int color;
            switch (GetRand(2))
            {
            case 0: color = 6; break; // 白
            case 1: color = 3; break; // シアン
            default: color = 4; break; // 青
            }

            switch (GetRand(1))
            {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;

            default:
                pEnemyShot->kind = img_enemyShotDiamond[color];
                break;
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        // ----------------------------------------------------
        // 吹雪の横風
        // ----------------------------------------------------
        double wind =
            1.4 *
            sin(
                pEnemyShot->count * 0.05 +
                pEnemyShotSet->count * 0.03
            );

        // 一定時間ごとに突風
        if ((pEnemyShotSet->count % 180) >= 90)
        {
            wind *= 2.5;
        }

        pEnemyShot->x +=
            pEnemyShot->speed * cos(pEnemyShot->muki)
            + wind;

        pEnemyShot->y +=
            pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Blizzard_ChatGPT()
{
    static int moveDir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 50.0;

        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
    }
    else
    {
        enemy.x += moveDir * 1.1;

        if (enemy.x > 380.0) moveDir = -1;
        if (enemy.x < 100.0) moveDir = 1;
    }

    // --------------------------------------------------------
    // 吹雪を連続発生
    // --------------------------------------------------------
    if (count % 50 == 30)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBlizzard;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y - 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;

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