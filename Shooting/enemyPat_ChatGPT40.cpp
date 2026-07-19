// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotDoublePendulum(sEnemyShotSet* pEnemyShotSet)
{
    // -----------------------------
    // 初回：親振り子生成
    // -----------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (!CheckSoundMem(sound_enemyShot_heavy))
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int NUM = 9;

        for (int i = 0; i < NUM; i++)
        {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            // 親振り子位相
            double phase = DX_PI * 2.0 * i / NUM;

            pShot->param_d[0] = phase;       // 親角
            pShot->param_d[1] = phase * 1.7; // 子角
            pShot->param_d[2] = 0.020;       // 親角速度
            pShot->param_d[3] = 0.065;       // 子角速度

            pShot->param_d[4] = 90.0;        // 親半径
            pShot->param_d[5] = 42.0;        // 子半径

            pShot->kind = img_enemyShotLargeBall[4];   // 青大玉

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    //---------------------------------------------------------
    // 更新
    //---------------------------------------------------------

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (pShot->kind == img_enemyShotLargeBall[4]) {
            //-------------------------------------------------
            // 親振り子
            //-------------------------------------------------

            double parentAngle = pShot->param_d[0];
            double childAngle = pShot->param_d[1];

            double parentSpeed = pShot->param_d[2];
            double childSpeed = pShot->param_d[3];

            parentAngle += parentSpeed;

            // わずかなカオス化
            childSpeed += sin((double)pShot->count * 0.011 + parentAngle) * 0.00012;
            childAngle += childSpeed;

            pShot->param_d[0] = parentAngle;
            pShot->param_d[1] = childAngle;
            pShot->param_d[3] = childSpeed;

            double r1 = pShot->param_d[4];

            double x1 = pEnemyShotSet->x + cos(parentAngle) * r1;
            double y1 = pEnemyShotSet->y + sin(parentAngle) * r1;

            pShot->x = x1;
            pShot->y = y1;
        
            //-------------------------------------------------
            // 子振り子放射
            //-------------------------------------------------
       
            if (pShot->count % 12 == 0)
            {
                sEnemyShot* pChild = new sEnemyShot;

                double r2 = pShot->param_d[5];

                double x2 = x1 + cos(childAngle) * r2;
                double y2 = y1 + sin(childAngle) * r2;

                pChild->x = x2;
                pChild->y = y2;

                pChild->muki =
                    atan2(
                        sin(parentAngle) + 0.8 * sin(childAngle),
                        cos(parentAngle) + 0.8 * cos(childAngle));

                pChild->speed = 2.6;
                pChild->kind = img_enemyShotSmallBall[6];   // 白小玉

                pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pChild->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
                pEnemyShotSet->pEnemyShotHead->prev = pChild;
            }
        }

        // 次の弾へ
        pShot = pShot->next;
    }

    //---------------------------------------------------------
    // 子弾の移動
    //---------------------------------------------------------
    pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        // 小玉のみ移動
        if (pShot->kind == img_enemyShotSmallBall[6])
        {
            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;
        }

        pShot = pShot->next;
    }
}


//---------------------------------------------------------
// 敵本体
//---------------------------------------------------------
void EnemyPat_DoublePendulum_ChatGPT()
{
    static int moveDir;
    static int shotTimer;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 80.0;

        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
        shotTimer = 0;
    }
    else
    {
        enemy.x += moveDir * 0.8;

        if (enemy.x < 120.0)
            moveDir = 1;

        if (enemy.x > 360.0)
            moveDir = -1;
    }

    //---------------------------------------------------------
    // 二重振り子生成
    //---------------------------------------------------------
    if (count % 120 == 0)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDoublePendulum;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;

        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shotTimer++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}