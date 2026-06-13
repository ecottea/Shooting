// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static inline int sgn(double x)
{
    return (x > 0.0) - (x < 0.0);
}

// ------------------------------------------------------------
// 雷弾
// ------------------------------------------------------------
static void ShotLightning(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --------------------------------------------------------
    // 発射
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 3.5;
        pEnemyShot->kind = img_enemyShotLargeBall[3];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // --------------------------------------------------------
    // 分裂（1回のみ）
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 18 &&
        pEnemyShotSet->kind == 0)
    {
        pEnemyShotSet->kind = 1;

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
        {
            sEnemyShot* current = pEnemyShot;
            pEnemyShot = pEnemyShot->next;

            for (int dir = -10; dir <= 10; dir += 1)
            {
                sEnemyShot* pNew = new sEnemyShot;

                pNew->x = current->x;
                pNew->y = current->y;

                pNew->muki =
                    current->muki +
                    dir * (5.0 / 180.0 * DX_PI);

                pNew->speed = current->speed;

                pNew->kind = img_enemyShotSmallBall[6];

                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;

                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }
    }

    // --------------------------------------------------------
    // ジグザグ方向
    // --------------------------------------------------------
    int zigDir =
        sgn(
            sin(
                pEnemyShotSet->count * 0.10
            )
        );

    int prevDir =
        sgn(
            sin(
                (pEnemyShotSet->count - 1) * 0.10
            )
        );

    bool turned = (zigDir != prevDir);

    // --------------------------------------------------------
    // 更新
    // --------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (turned)
        {
            pEnemyShot->speed = 6;
        }
        else
        {
            pEnemyShot->speed *= 0.9;
        }

        double zigzag =
            zigDir *
            (12.0 / 180.0 * DX_PI);

        pEnemyShot->x +=
            pEnemyShot->speed *
            cos(pEnemyShot->muki + zigzag);

        pEnemyShot->y +=
            pEnemyShot->speed *
            sin(pEnemyShot->muki + zigzag);

        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_Thunder_ChatGPT()
{
    static int moveDir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 50.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        moveDir = 1;
    }
    else
    {
        enemy.x += moveDir * 1.4;

        if (enemy.x < 120.0)
            moveDir = 1;

        if (enemy.x > 360.0)
            moveDir = -1;
    }

    // 雷発射
    if (count % 45 == 0)
    {
        sEnemyShotSet* pEnemyShotSet =
            new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->patternFunc =
            ShotLightning;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

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