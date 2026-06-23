// enemyPat_Hourglass.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//----------------------------------------------------------
// param_i[0]
//   0 : リング収束
//   1 : 落下砂
//   2 : 中央砂流
//
// param_i[1]
//   0 : 上リング
//   1 : 下リング
//
// param_d[0] angle
// param_d[1] radius
// param_d[2] centerX
// param_d[3] centerY
//----------------------------------------------------------

static void ShotHourglass(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //------------------------------------------------------
    // 初期生成
    //------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);

        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int BULLET_NUM = 24;

        for (int ring = 0; ring < 2; ring++)
        {
            double centerY =
                pEnemyShotSet->y +
                (ring == 0 ? -70.0 : 70.0);

            for (int i = 0; i < BULLET_NUM; i++)
            {
                sEnemyShot* p = new sEnemyShot;

                double ang =
                    DX_PI * 2.0 * i / BULLET_NUM;

                p->param_i[0] = 0;
                p->param_i[1] = ring;

                p->param_d[0] = ang;
                p->param_d[1] = 55.0;
                p->param_d[2] = pEnemyShotSet->x;
                p->param_d[3] = centerY;

                p->x =
                    p->param_d[2] +
                    cos(ang) * p->param_d[1];

                p->y =
                    p->param_d[3] +
                    sin(ang) * p->param_d[1];

                p->speed = 0.0;

                p->kind =
                    (ring == 0)
                    ? img_enemyShotSmallBall[1]  // 黄
                    : img_enemyShotSmallBall[8]; // 橙

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    //------------------------------------------------------
    // 中央砂流
    //------------------------------------------------------
    if (pEnemyShotSet->count % 3 == 0 && pEnemyShotSet->count <= 300)
    {
        sEnemyShot* p = new sEnemyShot;

        p->param_i[0] = 2;

        p->x =
            pEnemyShotSet->x +
            (GetRand(8) - 4);

        p->y =
            pEnemyShotSet->y;

        p->speed =
            2.2 + GetRand(15) * 0.05;

        p->kind = img_enemyShotSmallBall[6]; // 白

        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;
    }

    //------------------------------------------------------
    // 更新
    //------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        //--------------------------------------------------
        // リング収束
        //--------------------------------------------------
        if (pEnemyShot->param_i[0] == 0)
        {
            pEnemyShot->param_d[1] *= 0.985;

            pEnemyShot->param_d[2] +=
                (pEnemyShotSet->x -
                    pEnemyShot->param_d[2]) * 0.03;

            pEnemyShot->param_d[3] +=
                (pEnemyShotSet->y -
                    pEnemyShot->param_d[3]) * 0.03;

            pEnemyShot->param_d[0] += 0.02;

            pEnemyShot->x =
                pEnemyShot->param_d[2] +
                cos(pEnemyShot->param_d[0]) *
                pEnemyShot->param_d[1];

            pEnemyShot->y =
                pEnemyShot->param_d[3] +
                sin(pEnemyShot->param_d[0]) *
                pEnemyShot->param_d[1];

            if (pEnemyShot->param_d[1] < 5.0)
            {
                pEnemyShot->param_i[0] = 1;

                double spread =
                    (pEnemyShot->x -
                        pEnemyShotSet->x) * 0.035;

                pEnemyShot->param_d[0] = spread;
                pEnemyShot->speed =
                    1.0 + fabs(spread);
            }
        }
        //--------------------------------------------------
        // 崩れて落下
        //--------------------------------------------------
        else if (pEnemyShot->param_i[0] == 1)
        {
            pEnemyShot->speed += 0.012;

            pEnemyShot->x +=
                pEnemyShot->param_d[0];

            pEnemyShot->y +=
                pEnemyShot->speed;

            pEnemyShot->x +=
                sin(
                    pEnemyShot->count * 0.08 +
                    pEnemyShot->param_d[0] * 30.0
                ) * 0.25;
        }
        //--------------------------------------------------
        // 中央砂流
        //--------------------------------------------------
        else
        {
            pEnemyShot->y += pEnemyShot->speed;

            pEnemyShot->x +=
                sin(
                    pEnemyShot->count * 0.15
                ) * 0.15;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

//----------------------------------------------------------
// 敵本体
//----------------------------------------------------------
void EnemyPat_Hourglass_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 160.0;

        enemy.maxHp = enemy.hp = 200;

        dir = 1;
    }
    else
    {
        enemy.x += dir * 0.8;

        if (enemy.x > 340.0)
            dir = -1;

        if (enemy.x < 140.0)
            dir = 1;
    }

    //------------------------------------------------------
    // 砂時計生成
    //------------------------------------------------------
    if (count % 60 == 30)
    {
        sEnemyShotSet* pEnemyShotSet =
            new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc =
            ShotHourglass;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 00.0;

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