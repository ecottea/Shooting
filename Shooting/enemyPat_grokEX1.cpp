#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

enum {
    COMET_HEAD = 0,
    COMET_TAIL = 1,
};

static void ShotComet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //--------------------------------------
    // 初回生成
    //--------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 3.5;

        pEnemyShot->kind = img_enemyShotLargeBall[6]; // 白

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    //--------------------------------------
    // 彗星本体移動
    //--------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    if (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        // 緩く曲がる
        pEnemyShot->muki += sin(pEnemyShotSet->count * 0.03) * 0.008;

        pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
        pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;

        //----------------------------------
        // 尾を生成
        //----------------------------------
        if (pEnemyShotSet->count % 2 == 0)
        {
            sEnemyShot* tail = new sEnemyShot;

            tail->x = pEnemyShot->x;
            tail->y = pEnemyShot->y;

            tail->muki = pEnemyShot->muki;
            tail->speed = 0.0;

            int color;

            if (pEnemyShotSet->count < 40)
                color = 6; // 白
            else if (pEnemyShotSet->count < 80)
                color = 3; // シアン
            else
                color = 4; // 青

            tail->kind = img_enemyShotSmallBall[color];

            tail->prev = pEnemyShotSet->pEnemyShotHead->prev;
            tail->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->pEnemyShotHead->prev->next = tail;
            pEnemyShotSet->pEnemyShotHead->prev = tail;
        }
    }

    //--------------------------------------
    // 尾を徐々に流す
    //--------------------------------------
    int index = 0;

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (index > 0)
        {
            double spd = 0.2 + (480 - pEnemyShot->y) * 0.01;

            pEnemyShot->x -= cos(pEnemyShot->muki) * spd;
            pEnemyShot->y -= sin(pEnemyShot->muki) * spd;
        }

        index++;
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Comet_Grok2()
{   
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 55.0;        
        enemy.maxHp = enemy.hp = 200;
    }

    if (count % 20 == 0)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotComet;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10;

        double base =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x
            );

        pEnemyShotSet->muki =
            base + (GetRand(60) - 30 - 10) * DX_PI / 180.0;

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