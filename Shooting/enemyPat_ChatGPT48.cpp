// enemyPat_tmp.cpp
// 第一回
// 潮吹き弾幕
// ・EnemyPat_Spout_ChatGPT()
// ・水柱(短レーザー)を生成
// ・第二回で水しぶきへ分裂を追加

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotBlow(sEnemyShotSet* pEnemyShotSet)
{
    //==================================================
    // 初回生成（水柱）
    //==================================================
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;

        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;

        // 真下
        pShot->muki = DX_PI / 2.0;
        pShot->speed = 5.2;

        // シアン短レーザー
        pShot->kind = img_enemyShotLaser[3];

        // 第二回で使用
        pShot->param_i[0] = 0;

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    //==================================================
    // 更新
    //==================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 0)
        {
            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;
        }

        //------------------------------------------------
        // 水柱が一定時間後に破裂
        //------------------------------------------------
        if (pShot->param_i[0] == 0 && pShot->count >= 36)
        {
            if (CheckSoundMem(sound_enemyShot_light))
                StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            const int NUM = 50;

            for (int i = 0; i < NUM; i++)
            {
                sEnemyShot* pDrop = new sEnemyShot;

                pDrop->x = pShot->x;
                pDrop->y = pShot->y;

                // 上向き半球（約180度）
                double ang =
                    -DX_PI
                    + DX_PI * i / (NUM - 1);

                pDrop->muki = ang;

                // 少しばらつきを付ける
                pDrop->muki += (GetRand(20) - 10) * DX_PI / 180.0;

                pDrop->speed = 2.2 + GetRand(600) / 100.0;

                pDrop->kind = img_enemyShotSmallBall[3];

                // 1 = 水滴
                pDrop->param_i[0] = 1;

                pDrop->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pDrop->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pDrop;
                pEnemyShotSet->pEnemyShotHead->prev = pDrop;
            }

            // 水柱を削除
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }

        //------------------------------------------------
        // 水滴
        //------------------------------------------------
        if (pShot->param_i[0] == 1)
        {
            // 最初は空気抵抗で減速
            if (pShot->count < 20)
            {
                pShot->speed *= 0.96;
            }
            // その後は重力で落下
            else
            {
                const double gx = 0.0;
                const double gy = 0.10;

                double vx = cos(pShot->muki) * pShot->speed;
                double vy = sin(pShot->muki) * pShot->speed;

                vx += gx;
                vy += gy;

                pShot->speed = sqrt(vx * vx + vy * vy);
                pShot->muki = atan2(vy, vx);
            }

            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;
        }

        pShot = pNext;
    }
}

//======================================================
// 敵本体
//======================================================
void EnemyPat_Spout_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 40.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        dir = 1;
    }
    else
    {
        enemy.x += dir * 1.0;

        if (enemy.x > 360.0)
            dir = -1;
        if (enemy.x < 120.0)
            dir = 1;
    }

    // 約0.8秒ごとに潮吹き
    if (count % 48 == 1)
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotBlow;

        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;

        pSet->muki = DX_PI / 2.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}