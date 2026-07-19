// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//============================================================
// DNA 二重らせん弾幕
// 第一回
//============================================================

static void ShotDNA(sEnemyShotSet* pEnemyShotSet)
{
    //--------------------------------------------------------
    // 初回生成
    //--------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const double amp = 42.0;
        const int NUM = 30;

        for (int i = 0; i < NUM; i++)
        {
            double t = i * 0.42;

            //------------------------------------
            // 左右2本のDNA鎖
            //------------------------------------
            for (int side = 0; side < 2; side++)
            {
                sEnemyShot* pShot = new sEnemyShot;

                double phase = t + (side ? DX_PI : 0.0);

                pShot->param_d[0] = pEnemyShotSet->x;      // 中心X
                pShot->param_d[1] = phase;                 // 位相
                pShot->param_d[2] = amp;                   // 振幅
                pShot->param_d[3] = 1.4;                   // 落下速度

                pShot->x = pEnemyShotSet->x + amp * sin(phase);
                pShot->y = pEnemyShotSet->y - i * 14.0;

                pShot->speed = 0.0;
                pShot->muki = 0.0;

                // 0=DNA本体
                pShot->param_i[0] = 0;

                // 左右識別
                pShot->param_i[1] = side;

                // 青・マゼンタ
                pShot->kind =
                    (side == 0)
                    ? img_enemyShotMediumBall[4]
                    : img_enemyShotMediumBall[5];

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    //--------------------------------------------------------
    // 移動
    //--------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    double open =
        1.0 + 0.55 * (sin(pEnemyShotSet->count * 0.020) + 1.0);
    
    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (pShot->param_i[0] == 0)
        {
            // DNA全体が周期的にほどけて再結合する            
            pShot->param_d[1] += 0.055;

            double cx = pShot->param_d[0];
            double phase = pShot->param_d[1];
            double amp = pShot->param_d[2] * open;

            pShot->x = cx + amp * sin(phase);
            pShot->y += pShot->param_d[3];
        }

        pShot = pShot->next;
    }

    //--------------------------------------------------------
    // 塩基対生成
    //--------------------------------------------------------
    if (pEnemyShotSet->count % 1 == 0)
    {
        const double phase = pEnemyShotSet->count * 0.055;
        const double amp = 42.0 * open;
        const double y = pEnemyShotSet->y + pEnemyShotSet->count * 1.4;

        const double x1 = pEnemyShotSet->x + amp * sin(phase);
        const double x2 = pEnemyShotSet->x + amp * sin(phase + DX_PI);

        const int NUM = 7;

        for (int i = 0; i < NUM; i++)
        {
            sEnemyShot* pShot = new sEnemyShot;

            double t = (double)i / (NUM - 1);

            pShot->x = x1 + (x2 - x1) * t;
            pShot->y = y;

            // 1=塩基対
            pShot->param_i[0] = 1;

            // 生成時の高さ
            pShot->param_d[0] = y;

            pShot->kind = img_enemyShotSmallBall[6];   // 白

            pShot->speed = 0.0;
            pShot->muki = 0.0;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    //--------------------------------------------------------
    // 塩基対移動
    //--------------------------------------------------------
    pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (pShot->param_i[0] == 1)
        {
            pShot->y += 1.4;
            if (pShot->count > 1) {
                pShot->margin = -9999;
            }
        }

        pShot = pShot->next;
    }
}


//============================================================
// 敵本体
//============================================================
void EnemyPat_DNA_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        dir = 1;
    }
    else
    {
        enemy.x += dir * 0.9;

        if (enemy.x > 360.0) dir = -1;
        if (enemy.x < 120.0) dir = 1;
    }

    //--------------------------------------------------------
    // DNA生成
    //--------------------------------------------------------
    if (count % 90 == 1)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDNA;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;

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
