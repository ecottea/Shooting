// enemyPat_popcorn.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void AddShot(
    sEnemyShotSet* pSet,
    double x,
    double y,
    double muki,
    double speed,
    int kind)
{
    sEnemyShot* pShot = new sEnemyShot;

    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ------------------------------------------------------------
// ポップコーン弾幕
// 小さな黄色い粒がゆっくり動き、時間差で弾ける。
// ------------------------------------------------------------
static void ShotPopcorn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //----------------------------------------------------------
    // 初期生成
    //----------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 18; i++)
        {
            pEnemyShot = new sEnemyShot;

            double ang = GetRand(359) * DX_PI / 180.0;
            double r = GetRand(28);

            pEnemyShot->x = pEnemyShotSet->x + cos(ang) * r;
            pEnemyShot->y = pEnemyShotSet->y + sin(ang) * r;

            // ゆっくり漂う
            pEnemyShot->muki = GetRand(359) * DX_PI / 180.0;
            pEnemyShot->speed = 0.35 + GetRand(30) / 100.0;

            // 黄色い粒
            pEnemyShot->kind = img_enemyShotSmallBall[1];

            // 破裂までの待ち時間
            pEnemyShot->param_i[0] = 45 + GetRand(90);

            // ジャンボポップコーン
            pEnemyShot->param_i[1] = (GetRand(19) == 0);

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    //----------------------------------------------------------
    // 更新
    //----------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        sEnemyShot* pNext = pEnemyShot->next;

        // ゆっくり移動
        pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
        pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;

        // 少し減速
        pEnemyShot->speed *= 0.992;

        // 弾ける直前は細かく震える
        if (pEnemyShot->count > pEnemyShot->param_i[0] - 20)
        {
            pEnemyShot->x += (GetRand(4) - 2) * 0.4;
            pEnemyShot->y += (GetRand(4) - 2) * 0.4;
        }

        //------------------------------------------------------
        // ポップ！
        //------------------------------------------------------
        if (pEnemyShot->count >= pEnemyShot->param_i[0])
        {
            if (CheckSoundMem(sound_enemyShot_medium) == 1)
                StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            int num;

            if (pEnemyShot->param_i[1])
            {
                // ジャンボポップコーン
                num = 24;
            }
            else
            {
                // 通常
                num = 6 + GetRand(4);   // 6～10発
            }

            double base = GetRand(359) * DX_PI / 180.0;

            for (int i = 0; i < num; i++)
            {
                double ang;
                double spd;

                if (pEnemyShot->param_i[1])
                {
                    ang = base + DX_PI * 2.0 * i / num;
                    ang += (GetRand(20) - 10) * DX_PI / 180.0;
                }
                else
                {
                    ang = base + DX_PI * 2.0 * i / num;
                    ang += (GetRand(40) - 20) * DX_PI / 180.0;
                }

                spd = 2.0 + GetRand(180) / 100.0;

                AddShot(
                    pEnemyShotSet,
                    pEnemyShot->x,
                    pEnemyShot->y,
                    ang,
                    spd,
                    img_enemyShotSmallBall[6]   // 白
                );
                pEnemyShotSet->pEnemyShotHead->prev->param_i[0] = 999999999;
            }

            // 元のコーン粒を削除
            pEnemyShot->prev->next = pEnemyShot->next;
            pEnemyShot->next->prev = pEnemyShot->prev;
            delete pEnemyShot;
        }

        pEnemyShot = pNext;
    }
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_Popcorn_ChatGPT()
{
    static int dir;
    static int shotCount;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 50.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        dir = 1;
        shotCount = 0;
    }
    else
    {
        enemy.x += dir * 1.1;

        if (enemy.x < 90.0)
            dir = 1;
        if (enemy.x > 390.0)
            dir = -1;

        // ゆっくり上下に揺れる
        enemy.y = 50.0 + sin(count * DX_PI / 90.0) * 12.0;
    }

    // 約1.2秒ごとに新しいポップコーン群を生成
    if (count % 72 == 0)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPopcorn;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = atan2(
            player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

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