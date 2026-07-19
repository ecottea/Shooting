// EnemyPat_Arrow_ChatGPT.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace
{

    //------------------------------------------------------------
    // 矢印1本生成
    //------------------------------------------------------------
    static void SpawnArrow(
        sEnemyShotSet* pSet,
        double dir,
        bool feint)
    {
        const double ux = cos(dir);
        const double uy = sin(dir);
        const double px = -uy;
        const double py = ux;

        // 軸
        for (int i = -10; i <= 3; i++)
        {
            sEnemyShot* p = new sEnemyShot;

            p->x = pSet->x - ux * 42.0 + ux * i * 12.0;
            p->y = pSet->y - uy * 42.0 + uy * i * 12.0;

            p->muki = dir;
            p->speed = 0.45;

            p->kind = img_enemyShotBullet[1];

            p->param_d[0] = dir;
            p->param_i[0] = 50;
            p->param_i[1] = feint ? 1 : 0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }

        // 矢じり
        for (int s = -10; s <= 10; s++)
        {
            sEnemyShot* p = new sEnemyShot;

            p->x =
                pSet->x +
                ux * 42.0 +
                px * s * 12.0 -
                ux * fabs((double)s) * 10.0;

            p->y =
                pSet->y +
                uy * 42.0 +
                py * s * 12.0 -
                uy * fabs((double)s) * 10.0;

            p->muki = dir;
            p->speed = 0.45;

            p->kind = img_enemyShotBullet[8];

            p->param_d[0] = dir;
            p->param_i[0] = 50;
            p->param_i[1] = feint ? 1 : 0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    //------------------------------------------------------------
    // 矢印弾幕
    //------------------------------------------------------------
    static void ShotArrow(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0)
        {
            if (CheckSoundMem(sound_enemyShot_light))
                StopSoundMem(sound_enemyShot_light);

            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            SpawnArrow(
                pSet,
                pSet->muki,
                pSet->kind != 0);
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;

        while (pShot != pSet->pEnemyShotHead)
        {
            if (pShot->count == pShot->param_i[0])
            {
                double dir = pShot->param_d[0];

                if (pShot->param_i[1])
                    dir += DX_PI / 4.0;

                pShot->muki = dir;
                pShot->speed = 4.8;

                if (pShot == pSet->pEnemyShotHead->next)
                {
                    if (CheckSoundMem(sound_enemyShot_heavy))
                        StopSoundMem(sound_enemyShot_heavy);

                    PlaySoundMem(
                        sound_enemyShot_heavy,
                        DX_PLAYTYPE_BACK);
                }
            }

            pShot->x +=
                cos(pShot->muki) *
                pShot->speed;

            pShot->y +=
                sin(pShot->muki) *
                pShot->speed;

            pShot = pShot->next;
        }
    }

} // namespace

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_Arrow_ChatGPT()
{
    static int moveDir;
    static int arrowIndex;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 145.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        moveDir = 1;
        arrowIndex = 0;
    }
    else
    {
        enemy.x += moveDir * 0.9;

        if (enemy.x > 380.0)
            moveDir = -1;

        if (enemy.x < 100.0)
            moveDir = 1;
    }

    if (count % 20 == 0)
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotArrow;

        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        // →
        switch (arrowIndex & 3)
        {
        case 0:
            pSet->muki = 0.0;
            break;

        case 1:
            pSet->muki = DX_PI * 0.5;
            break;

        case 2:
            pSet->muki = DX_PI;
            break;

        default:
            pSet->muki = DX_PI * 1.5;
            break;
        }

        // 4回に1回だけ45°フェイント
        pSet->kind = ((arrowIndex % 4) == 3);

        ++arrowIndex;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}