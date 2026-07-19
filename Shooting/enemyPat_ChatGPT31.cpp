// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotBoomerangRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //============================================================
    // 初回：リング生成
    //============================================================
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int WAY = 36;

        for (int i = 0; i < WAY; i++)
        {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = DX_PI * 2.0 * i / WAY;
            pEnemyShot->speed = 5.5;

            pEnemyShot->kind = img_enemyShotMediumBall[i % 9];

            // 状態
            // 0: 外へ飛ぶ
            // 1: 停止
            // 2: 戻る
            // 3: 分裂済
            pEnemyShot->param_i[0] = 0;

            // 個体番号
            pEnemyShot->param_i[1] = i;

            // 戻り角補正
            pEnemyShot->param_d[0] =
                sin(i * 0.6) * (15.0 / 180.0 * DX_PI);

            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    //============================================================
    // 更新
    //============================================================
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        //--------------------------------------------------------
        // 外へ飛ぶ
        //--------------------------------------------------------
        if (pEnemyShot->param_i[0] == 0)
        {
            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;

            if (pEnemyShot->count >= 60)
            {
                pEnemyShot->speed = 0.0;
                pEnemyShot->param_i[0] = 1;
            }
        }

        //--------------------------------------------------------
        // 停止
        //--------------------------------------------------------
        else if (pEnemyShot->param_i[0] == 1)
        {
            if (pEnemyShot->count >= 90)
            {
                double ang =
                    atan2(
                        enemy.y - pEnemyShot->y,
                        enemy.x - pEnemyShot->x);

                ang += pEnemyShot->param_d[0];

                pEnemyShot->muki = ang;
                pEnemyShot->speed = 4.0;
                pEnemyShot->param_i[0] = 2;
            }
        }

        //--------------------------------------------------------
        // 敵へ戻る
        //--------------------------------------------------------
        else if (pEnemyShot->param_i[0] == 2)
        {
            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;

            double dx = enemy.x - pEnemyShot->x;
            double dy = enemy.y - pEnemyShot->y;

            if (dx * dx + dy * dy < 12.0 * 12.0)
            {
                pEnemyShot->param_i[0] = 3;

                // 分裂は次回
            }
        }

        pEnemyShot = pEnemyShot->next;
    }
}

//============================================================
// 敵本体
//============================================================
void EnemyPat_Reverse_ChatGPT()
{
    static int dir;
    static int shotCount;

    if (count == 1)
    {
        enemy.x = 240;
        enemy.y = 190;

        enemy.maxHp = enemy.hp = 200;

        dir = 1;
        shotCount = 0;
    }
    else
    {
        enemy.x += dir * 1.0;

        if (count % 120 == 60)
            dir = -dir;
        if (count % 180 == 30)
        {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotBoomerangRing;

            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 12.0;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = shotCount++;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    //============================================================
    // 戻った弾の分裂処理
    //============================================================
    sEnemyShotSet* pSet = enemyShotSetHead.next;

    while (pSet != &enemyShotSetHead)
    {
        if (pSet->patternFunc == ShotBoomerangRing)
        {
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;

            while (pShot != pSet->pEnemyShotHead)
            {
                if (pShot->param_i[0] == 3 && pShot->kind >= img_enemyShotMediumBall[0])
                {
                    pShot->param_i[0] = 4;

                    if (CheckSoundMem(sound_enemyShot_light) == 1)
                        StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                    const int WAY = 16;

                    for (int i = 0; i < WAY; i++)
                    {
                        sEnemyShot* pNew = new sEnemyShot;

                        pNew->x = enemy.x;
                        pNew->y = enemy.y + 10.0;

                        pNew->muki = DX_PI * 2.0 * i / WAY;
                        pNew->speed = 2.8;

                        pNew->kind = img_enemyShotSmallBall[i % 9];

                        pNew->margin = 480;

                        pNew->prev = pSet->pEnemyShotHead->prev;
                        pNew->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = pNew;
                        pSet->pEnemyShotHead->prev = pNew;
                    }

                    // 戻った親弾は消す
                    sEnemyShot* erase = pShot;
                    pShot = pShot->next;

                    erase->prev->next = erase->next;
                    erase->next->prev = erase->prev;
                    delete erase;

                    continue;
                }

                pShot = pShot->next;
            }
        }

        pSet = pSet->next;
    }
}