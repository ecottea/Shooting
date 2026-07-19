// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotShotgun(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //=========================================================
    // 発射
    //=========================================================
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 5.5;
        pEnemyShot->kind = img_enemyShotBullet[6];

        // param_i[0]
        // 0 : スラッグ
        // 1 : 散弾
        pEnemyShot->param_i[0] = 0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        //-----------------------------------------------------
        // スラッグ
        //-----------------------------------------------------
        if (pEnemyShot->param_i[0] == 0)
        {
            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;

            // 空中炸裂
            if (pEnemyShot->count == 20)
            {
                if (CheckSoundMem(sound_enemyShot_medium) == 1)
                    StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                const int NUM = 25;

                for (int i = 0; i < NUM; i++)
                {
                    sEnemyShot* p = new sEnemyShot;

                    p->x = pEnemyShot->x;
                    p->y = pEnemyShot->y;

                    double t = (double)i / (NUM - 1);

                    // 約120度
                    p->muki =
                        pEnemyShot->muki
                        - DX_PI / 3.0
                        + DX_PI * 2.0 / 3.0 * t;

                    // 少しばらつかせる
                    p->muki += (GetRand(100) - 50) / 180.0 * DX_PI / 18.0;

                    p->speed = 5.5 + GetRand(100) / 100.0;

                    p->kind = img_enemyShotSmallBall[1];

                    p->param_i[0] = 1;

                    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    p->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = p;
                    pEnemyShotSet->pEnemyShotHead->prev = p;
                }

                sEnemyShot* next = pEnemyShot->next;

                pEnemyShot->prev->next = pEnemyShot->next;
                pEnemyShot->next->prev = pEnemyShot->prev;
                delete pEnemyShot;

                pEnemyShot = next;
                continue;
            }
        }
        //-----------------------------------------------------
        // 散弾
        //-----------------------------------------------------
        else
        {
            if (pEnemyShot->count < 60)
            {
                pEnemyShot->speed *= 0.95;
            }
            else if (pEnemyShot->count == 60)
            {
                // 停止しかけた散弾を再加速
                pEnemyShot->speed = 6.0;

                // 色を変えて再発射感を演出
                pEnemyShot->kind = img_enemyShotSmallBall[8];

                if (CheckSoundMem(sound_enemyShot_light) == 1)
                    StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

//=========================================================
// 敵本体
//=========================================================
void EnemyPat_Shotgun_ChatGPT()
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
        enemy.x += dir * 1.0;

        if (enemy.x > 380.0) dir = -1;
        if (enemy.x < 100.0) dir = 1;

        enemy.y = 60.0 + sin(count / 45.0) * 18.0;
    }

    // 約2秒ごとにショットガン発射
    if (count % 35 == 0)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShotgun;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki =
            atan2(player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

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

    if (player.y < 300 && count % 5 == 0) { // ※ペナ弾追加
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShotgun;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki =
            atan2(player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 1;

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