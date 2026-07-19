// EnemyPat_Bomberman_ChatGPT.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//--------------------------------------
// ボンバーマン風
// 爆弾→一定時間後に十字爆風
//--------------------------------------

static void ShotBomber(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //----------------------------------
    // 初期化
    //----------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        pEnemyShot->speed = 3.5 + GetRand(3);
        pEnemyShot->muki = pEnemyShotSet->muki;

        // 黒い大玉＝爆弾
        pEnemyShot->kind = img_enemyShotLargeBall[7];

        // param_i[0]
        // 0:爆弾
        // 1:爆風
        pEnemyShot->param_i[0] = 0;

        pEnemyShot->margin = 200.0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    //----------------------------------
    // 更新
    //----------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (pEnemyShot->param_i[0] == 0)
        {
            //----------------------------------
            // 爆弾
            //----------------------------------

            // 点滅
            if (pEnemyShot->count > 40)
            {
                if ((pEnemyShot->count / 4) & 1)
                    pEnemyShot->kind = img_enemyShotLargeBall[8];
                else
                    pEnemyShot->kind = img_enemyShotLargeBall[7];
            }

            //----------------------------------
            // 爆発
            //----------------------------------
            if (pEnemyShot->count == 60)
            {
                if (CheckSoundMem(sound_enemyShot_extreme) == 1)
                    StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                static const double dir[4] =
                {
                    0.0,
                    DX_PI / 2,
                    DX_PI,
                    DX_PI * 3 / 2
                };

                for (int d = 0; d < 4; d++)
                {
                    for (int i = 1; i <= 8; i++)
                    {
                        sEnemyShot* b = new sEnemyShot;

                        b->x = pEnemyShot->x;
                        b->y = pEnemyShot->y;

                        b->speed = 0.0;
                        b->muki = dir[d];

                        b->kind = img_enemyShotMediumBall[8];

                        b->param_i[0] = 1;
                        b->param_i[1] = i;

                        b->param_d[0] = pEnemyShot->x;
                        b->param_d[1] = pEnemyShot->y;

                        b->margin = 200.0;

                        b->prev =
                            pEnemyShotSet->pEnemyShotHead->prev;
                        b->next =
                            pEnemyShotSet->pEnemyShotHead;

                        pEnemyShotSet->pEnemyShotHead
                            ->prev->next = b;
                        pEnemyShotSet->pEnemyShotHead
                            ->prev = b;
                    }
                }

                // 爆弾消滅
                pEnemyShot->kind = -1;
                pEnemyShot->x = -1000;
                pEnemyShot->y = -1000;
            }
        }
        else
        {
            //----------------------------------
            // 爆風
            //----------------------------------

            int step = pEnemyShot->param_i[1];

            int t = pEnemyShot->count;
            if (t > step * 4)
                t = step * 4;

            double len = 16.0 * (t / 4.0);

            pEnemyShot->x =
                pEnemyShot->param_d[0]
                + cos(pEnemyShot->muki) * len;

            pEnemyShot->y =
                pEnemyShot->param_d[1]
                + sin(pEnemyShot->muki) * len;

            // 先端だけ大きい
            if (step == 8)
                pEnemyShot->kind =
                img_enemyShotLargeBall[8];
            else
                pEnemyShot->kind =
                img_enemyShotMediumBall[8];

            // 爆風は一定時間で消える
            if (pEnemyShot->count > 48)
            {
                pEnemyShot->x = -1000.0;
                pEnemyShot->y = -1000.0;
            }
        }

        pEnemyShot = pEnemyShot->next;
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

//--------------------------------------
// 敵本体
//--------------------------------------
void EnemyPat_Bomberman_ChatGPT()
{
    static int muki;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 40.0;

        enemy.maxHp = enemy.hp = 200;

        muki = 1;
    }
    else
    {
        enemy.x += 0.8 * muki;

        if (enemy.x < 80.0)
            muki = 1;
        if (enemy.x > 400.0)
            muki = -1;
    }

    //----------------------------------
    // 爆弾設置
    //----------------------------------
    if (count % 45 == 0)
    {
        for (int i = -1; i <= 1; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotBomber;

            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 40.0;

            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) + i * DX_PI / 6;
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
    }
}