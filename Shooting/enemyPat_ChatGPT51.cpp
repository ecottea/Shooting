// enemyPat_tmp.cpp
// イライラ棒弾幕 第一回
// 「壁」を構成する短レーザーを生成する

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//------------------------------------------------------------
// 壁生成
//------------------------------------------------------------
static void ShotMazeWall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const bool vertical = (pEnemyShotSet->kind & 1) != 0;
        const int color = pEnemyShotSet->kind % 9;

        for (int i = -4; i <= 4; i++)
        {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            if (vertical)
            {
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y + i * 16.0;
                pEnemyShot->muki = DX_PI / 2.0;
            }
            else
            {
                pEnemyShot->x = pEnemyShotSet->x + i * 16.0;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = 0.0;
            }

            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[color];

            // 壁全体の移動速度
            pEnemyShot->param_d[0] = pEnemyShotSet->param_d[0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 壁全体をゆっくり蛇行
    double dx = pEnemyShotSet->param_d[0] *
        sin(pEnemyShotSet->count * 0.015);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        pShot->x += dx;
        
        if (pShot->count >= 600) pShot->margin = -9999;

        pShot = pShot->next;
    }
}

//------------------------------------------------------------
// 電流
//------------------------------------------------------------
static void ShotElectric(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int NUM = 7;

        for (int i = 0; i < NUM; i++)
        {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + (i - (NUM - 1) / 2.0) * 32.0;

            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->speed = 5.5;
            pEnemyShot->kind = img_enemyShotMediumBall[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_Irairabou_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.hp = enemy.maxHp = 200;

        dir = 1;
    }
    else
    {
        enemy.x += dir * 0.8;

        if (enemy.x < 140.0)
            dir = 1;
        if (enemy.x > 340.0)
            dir = -1;
    }

    // 縦壁
    if (count % 40 == 1)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMazeWall;
        pEnemyShotSet->kind = 1;

        while (true) {
            pEnemyShotSet->x = 80.0 + GetRand(320);
            if (abs(pEnemyShotSet->x - player.x) > 50) break;
        }
        pEnemyShotSet->y = 80.0 + GetRand(360);

        pEnemyShotSet->param_d[0] = (GetRand(1) == 0) ? 0.25 : -0.25;
        pEnemyShotSet->param_d[0] *= 2;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 横壁
    if (count % 40 == 21)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMazeWall;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->x = 80.0 + GetRand(320);        
        while (true) {
            pEnemyShotSet->y = 80.0 + GetRand(400);
            if (abs(pEnemyShotSet->y - player.y) > 20) break;
        }

        pEnemyShotSet->param_d[0] = (GetRand(1) == 0) ? 0.20 : -0.20;
        pEnemyShotSet->param_d[0] *= 2;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    //--------------------------------------------------------
    // 電流
    //--------------------------------------------------------
    //if (count % 90 == 1)
    //{
    //    if (CheckSoundMem(sound_enemyCharge))
    //        StopSoundMem(sound_enemyCharge);

    //    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    //}

    if (count % 90 == 25)
    {
        for (int i = 0; i < 5; i+=2)
        {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotElectric;

            if (GetRand(1) == 0)
            {
                pEnemyShotSet->x = -20.0;
                pEnemyShotSet->muki = 0.0;
            }
            else
            {
                pEnemyShotSet->x = 500.0;
                pEnemyShotSet->muki = DX_PI;
            }

            pEnemyShotSet->y = 70.0 + i * 75.0 + GetRand(10);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}