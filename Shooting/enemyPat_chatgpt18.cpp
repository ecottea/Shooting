// enemyPat_SpiderWeb.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// クモの巣弾幕
// ============================================================
static void ShotSpiderWeb(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --------------------------------------------------------
    // 初期生成
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int spokeCount = 12;

        // 中心核
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->speed = 0.0;
        pEnemyShot->muki = 0.0;
        pEnemyShot->kind = img_enemyShotLargeBall[0];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 放射状の糸
        for (int i = 0; i < spokeCount; i++)
        {
            double ang = pEnemyShotSet->muki + DX_PI * 2.0 * i / spokeCount;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + cos(ang) * 24;
            pEnemyShot->y = pEnemyShotSet->y + sin(ang) * 24;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 0.9;
            pEnemyShot->kind = img_enemyShotLargeBall[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --------------------------------------------------------
    // 巣の輪を増殖
    // --------------------------------------------------------
    if (pEnemyShotSet->count > 0 &&
        pEnemyShotSet->count % 40 == 0)
    {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int spokeCount = 12;
        double radius = 24.0 + pEnemyShotSet->count * 0.9;

        for (int i = 0; i < spokeCount; i++)
        {
            double ang = pEnemyShotSet->muki + DX_PI * 2.0 * i / spokeCount;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + cos(ang) * radius;
            pEnemyShot->y = pEnemyShotSet->y + sin(ang) * radius;
            pEnemyShot->muki = ang + DX_PI / 2.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[3];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --------------------------------------------------------
    // 弾更新
    // --------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        // 放射糸だけ外へ伸びる
        if (pEnemyShot->kind == img_enemyShotLargeBall[6])
        {
            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;

            // 少しずつ加速して巣を広げる
            //if (pEnemyShot->speed < 2.0)
            //    pEnemyShot->speed += 0.01;
        }

        if (pEnemyShot->count >= 900) pEnemyShot->x = 9999;

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_SpiderWeb_ChatGPT()
{
    static int moveDir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 60.0;

        enemy.maxHp = 300;
        enemy.hp = 300;

        moveDir = 1;
    }
    else
    {
        enemy.x += moveDir * 1.2;

        if (enemy.x < 100.0) moveDir = 1;
        if (enemy.x > 380.0) moveDir = -1;
    }

    // クモの巣を設置
    if (count % 90 == 0)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiderWeb;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = GetRand(359) / 360. * DX_PI;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}