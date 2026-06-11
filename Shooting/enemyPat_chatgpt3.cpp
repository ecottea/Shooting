// enemyPat13.cpp
#include "DxLib.h"
#include "gv.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotRotatingFlower(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int N = 24;

        for (int i = 0; i < N; i++)
        {
            pEnemyShot = new sEnemyShot;

            double ang =
                pEnemyShotSet->muki +
                PI * 2.0 * i / N;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = ang;
            pEnemyShot->speed = 1.8;

            int color = pEnemyShotSet->kind % 6;

            pEnemyShot->kind =
                img_enemyShotMediumBall[color];

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

    double rot =
        0.012 +
        pEnemyShotSet->count * 0.00008;

    pEnemyShot =
        pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        pEnemyShot->muki += rot * 0.3;

        pEnemyShot->x +=
            cos(pEnemyShot->muki) *
            pEnemyShot->speed;

        pEnemyShot->y +=
            sin(pEnemyShot->muki) *
            pEnemyShot->speed;

        pEnemyShot = pEnemyShot->next;
    }
}

void EnemyPat_Beautiful_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240;
        enemy.y = 70;

        enemy.maxHp = 150;
        enemy.hp = enemy.maxHp;

        dir = 1;
    }

    enemy.x += dir * 1.2;

    if (enemy.x < 120)
        dir = 1;

    if (enemy.x > 360)
        dir = -1;

    if (count % 18 == 0)
    {
        sEnemyShotSet* pEnemyShotSet =
            new sEnemyShotSet;

        pEnemyShotSet->count = 0;

        pEnemyShotSet->patternFunc =
            ShotRotatingFlower;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->muki =
            atan2(
                player.y - enemy.y,
                player.x - enemy.x);

        pEnemyShotSet->kind =
            count / 18;

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