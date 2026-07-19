// EnemyPat_Volcano.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ----------------------------------------------------------
// 火山弾幕
// ----------------------------------------------------------
static void ShotVolcano(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ======================================================
    // 初回生成
    // ======================================================
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // マグマ塊を打ち上げ
        for (int i = 0; i < 6; i++)
        {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki =
                -DX_PI / 2.0 +
                (GetRand(150) - 75) * DX_PI / 180.0;

            pEnemyShot->speed =
                3.5 + GetRand(100) / 100.0;

            pEnemyShot->kind = img_enemyShotLargeBall[8]; // 橙

            pEnemyShot->param_i[0] = 0; // 親弾

            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ======================================================
    // 更新
    // ======================================================
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        sEnemyShot* nextShot = pEnemyShot->next;

        // --------------------------------------------------
        // 親弾（マグマ塊）
        // --------------------------------------------------
        if (pEnemyShot->param_i[0] == 0)
        {
            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;

            // 上昇しながら減速
            pEnemyShot->speed *= 0.985;

            // 爆発
            if (pEnemyShot->count == 60)
            {
                if (CheckSoundMem(sound_enemyShot_medium) == 1)
                    StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                const int N = 18;

                for (int j = 0; j < N; j++)
                {
                    sEnemyShot* pChild = new sEnemyShot;

                    double ang =
                        DX_PI * 2.0 * j / N +
                        (GetRand(20) - 10) * DX_PI / 180.0;

                    double sp =
                        1.8 + GetRand(120) / 100.0 + 2;

                    pChild->x = pEnemyShot->x;
                    pChild->y = pEnemyShot->y;

                    pChild->muki = ang;
                    pChild->speed = sp;

                    pChild->param_i[0] = 1; // 子弾

                    pChild->param_d[0] = sp;
                    pChild->param_d[1] =
                        0.03 + GetRand(10) / 1000.0;

                    int color = GetRand(1);

                    if (color == 0)
                        pChild->kind = img_enemyShotMediumBall[1]; // 黄
                    else
                        pChild->kind = img_enemyShotMediumBall[8]; // 橙

                    pChild->margin = 480;

                    pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pChild->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
                    pEnemyShotSet->pEnemyShotHead->prev = pChild;
                }

                // 親弾消滅
                pEnemyShot->x = 99999;
            }
        }
        // --------------------------------------------------
        // 子弾（溶岩片）
        // --------------------------------------------------
        else
        {
            double gravity = pEnemyShot->param_d[1];

            double vx =
                cos(pEnemyShot->muki) *
                pEnemyShot->speed;

            double vy =
                sin(pEnemyShot->muki) *
                pEnemyShot->speed;

            vy += gravity * pEnemyShot->count;

            pEnemyShot->x += vx;
            pEnemyShot->y += vy;

            double angle = atan2(vy, vx);

            pEnemyShot->muki = angle;

            if (pEnemyShot->count > 60)
            {
                pEnemyShot->kind =
                    img_enemyShotSmallBall[0]; // 赤熱後に赤色化
            }

            pEnemyShot->margin = 480;
        }

        pEnemyShot = nextShot;
    }
}

// ----------------------------------------------------------
// 敵本体
// ----------------------------------------------------------
void EnemyPat_Volcano_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 160.0;

        enemy.maxHp = 200;
        enemy.hp = enemy.maxHp;

        dir = 1;
    }
    else
    {
        enemy.x += 0.7 * dir;

        if (enemy.x < 120) dir = 1;
        if (enemy.x > 360) dir = -1;
    }

    // 定期的に噴火
    if (count % 90 == 0)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotVolcano;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        pEnemyShotSet->muki = -DX_PI / 2.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}