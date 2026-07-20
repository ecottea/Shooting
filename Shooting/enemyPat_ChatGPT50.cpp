// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace
{
    constexpr int FISH_COUNT = 49;

    struct FishPos
    {
        double x;
        double y;
        bool eye;
    };

    // 魚のシルエット（ローカル座標）
    static const FishPos gFish[FISH_COUNT] =
    {
        {-70,  0,false},
        {-60,-12,false},
        {-60, 12,false},
        {-50,-22,false},
        {-50, 22,false},
        {-40,-30,false},
        {-40,-15,false},
        {-40,  0,false},
        {-40, 15,false},
        {-40, 30,false},

        {-25,-36,false},
        {-25,-20,false},
        {-25, -5,false},
        {-25, 10,false},
        {-25, 25,false},
        {-25, 40,false},

        { -5,-42,false},
        { -5,-26,false},
        { -5,-10,false},
        { -5,  6,false},
        { -5, 22,false},
        { -5, 38,false},

        { 18,-40,false},
        { 18,-24,false},
        { 18, -8,false},
        { 18,  8,false},
        { 18, 24,false},
        { 18, 40,false},

        { 42,-34,false},
        { 42,-18,false},
        { 42, -2,true },   // スイミー（目）
        { 42, 14,false},
        { 42, 30,false},

        { 60,-26,false},
        { 60,-10,false},
        { 60,  6,false},
        { 60, 22,false},

        { 76,-16,false},
        { 76,  0,false},
        { 76, 16,false},

        {-92,  0,false},
        {-105,-12,false},
        {-105, 12,false},
        {-120,-24,false},
        {-120, 24,false},
        {-135,-36,false},
        {-135, 36,false},
        {-150,  0,false},
    };
}

//------------------------------------------------------------
// スイミー弾幕
//------------------------------------------------------------
static void ShotSwimmy(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    sEnemyShot* pShot;

    //========================================================
    // 初期生成
    //========================================================
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < FISH_COUNT; i++)
        {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double ang = DX_PI * 2.0 * i / FISH_COUNT;

            pEnemyShot->muki = ang;
            pEnemyShot->speed = 2.4;

            if (gFish[i].eye)
                pEnemyShot->kind = img_enemyShotSmallBall[7];   // 黒
            else
                pEnemyShot->kind = img_enemyShotSmallBall[3];   // シアン

            // 目標位置
            pEnemyShot->param_d[0] = gFish[i].x;
            pEnemyShot->param_d[1] = gFish[i].y;

            // 崩壊時の飛び散る角度
            pEnemyShot->param_d[2] = ang;

            pEnemyShot->margin = 200;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    //--------------------------------------------------------
    // フェーズ0：群れが散らばる
    //--------------------------------------------------------
    if (pEnemyShotSet->count < 45)
    {
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead)
        {
            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;

            pShot = pShot->next;
        }
    }

    //--------------------------------------------------------
    // フェーズ1：魚の形へ整列
    //--------------------------------------------------------
    else if (pEnemyShotSet->count < 135)
    {
        double t = (pEnemyShotSet->count - 45) / 90.0;
        if (t > 1.0) t = 1.0;

        // 少しイージング
        t = t * t * (3.0 - 2.0 * t);

        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead)
        {
            double tx = pEnemyShotSet->x + pShot->param_d[0];
            double ty = pEnemyShotSet->y + pShot->param_d[1];

            pShot->x += (tx - pShot->x) * (0.05 + 0.25 * t);
            pShot->y += (ty - pShot->y) * (0.05 + 0.25 * t);

            pShot = pShot->next;
        }
    }

    //--------------------------------------------------------
    // フェーズ2：魚が泳ぐ
    //--------------------------------------------------------
    else if (pEnemyShotSet->count < 255)
    {
        double swimX = sin((pEnemyShotSet->count - 135) * 0.03) * 70.0;
        double swimY = sin((pEnemyShotSet->count - 135) * 0.06) * 18.0;

        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead)
        {
            double tx = pEnemyShotSet->x + swimX + pShot->param_d[0];
            double ty = pEnemyShotSet->y + swimY + pShot->param_d[1];

            pShot->x += (tx - pShot->x) * 0.15;
            pShot->y += (ty - pShot->y) * 0.15;

            pShot = pShot->next;
        }
    }

    //--------------------------------------------------------
    // フェーズ3：群れが崩壊
    //--------------------------------------------------------
    else
    {
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead)
        {
            // 一度だけ速度を設定
            if (pShot->count == 255)
            {
                pShot->muki = pShot->param_d[2];
                pShot->speed = 3.8;
            }

            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;

            pShot = pShot->next;
        }
    }
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_Swimmy_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.hp = enemy.maxHp = 200;

        dir = 1;
    }
    else
    {
        enemy.x += dir * 0.8;

        if (enemy.x > 360.0) dir = -1;
        if (enemy.x < 120.0) dir = 1;
    }

    // 約5秒ごとにスイミー弾幕を展開
    if (count == 1)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSwimmy;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (count % 300 == 1 && count != 1) for (int i = -1; i <= 1; i += 2) for (int j = -1; j <= 1; j += 2)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSwimmy;

        pEnemyShotSet->x = enemy.x + i * 150.0 + GetRand(50) - 25;
        pEnemyShotSet->y = enemy.y + 10.0 + j * 100.0 + GetRand(50) - 25;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
