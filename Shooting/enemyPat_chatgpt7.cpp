// enemyPat_comet.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 彗星
// ============================================================
static void ShotComet(sEnemyShotSet* pEnemyShotSet)
{
    // --------------------------------------------------------
    // 初回生成
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* core = new sEnemyShot;

        core->x = pEnemyShotSet->x;
        core->y = pEnemyShotSet->y;
        core->muki = pEnemyShotSet->muki;
        core->speed = 3.6;

        // 白い大玉＝彗星核
        core->kind = img_enemyShotLargeBall[6];

        core->prev = pEnemyShotSet->pEnemyShotHead->prev;
        core->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = core;
        pEnemyShotSet->pEnemyShotHead->prev = core;
    }

    // --------------------------------------------------------
    // 核を移動
    // --------------------------------------------------------
    sEnemyShot* core = pEnemyShotSet->pEnemyShotHead->next; // バグってるけどおもろいから放置

    if (core != pEnemyShotSet->pEnemyShotHead)
    {
        core->x += core->speed * cos(core->muki);
        core->y += core->speed * sin(core->muki);

        // ----------------------------------------------------
        // 尾を生成
        // ----------------------------------------------------
        if (pEnemyShotSet->count % 2 == 0)
        {
            double spread =
                0.05 +
                (pEnemyShotSet->count * 0.002);

            for (int i = -1; i <= 1; i++)
            {
                sEnemyShot* tail = new sEnemyShot;

                tail->x = core->x - 12.0 * cos(core->muki);
                tail->y = core->y - 12.0 * sin(core->muki);

                tail->muki =
                    core->muki +
                    DX_PI +
                    i * spread;

                tail->speed =
                    0.8 +
                    pEnemyShotSet->count * 0.01;

                // シアン系の尾
                tail->kind = img_enemyShotSmallBall[3];

                tail->prev = pEnemyShotSet->pEnemyShotHead->prev;
                tail->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = tail;
                pEnemyShotSet->pEnemyShotHead->prev = tail;
            }
        }
    }

    // --------------------------------------------------------
    // 全弾移動
    // --------------------------------------------------------
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (pEnemyShot != core)
        {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Comet_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 60.0;

        enemy.maxHp = enemy.hp = 200;

        dir = 1;
    }
    else
    {
        enemy.x += dir * 1.2;

        if (enemy.x < 100.0)
            dir = 1;

        if (enemy.x > 380.0)
            dir = -1;
    }

    // --------------------------------------------------------
    // 彗星発射
    // --------------------------------------------------------
    if (count % 300 == 30)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotComet;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

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