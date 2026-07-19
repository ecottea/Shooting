#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotFan(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回生成
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 羽3枚
        for (int i = 0; i < 3; i++)
        {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->speed = 0;

            pEnemyShot->kind = img_enemyShotLargeBall[6];

            pEnemyShot->param_d[0] = i * 2.0 * DX_PI / 3.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 羽回転
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        double angle =
            pEnemyShot->param_d[0]
            + pEnemyShot->count * 0.22;

        double r = 34.0;
        
        if (pEnemyShot->kind == img_enemyShotLargeBall[6]) {
            pEnemyShot->x =
                pEnemyShotSet->x + cos(angle) * r;

            pEnemyShot->y =
                pEnemyShotSet->y + sin(angle) * r;
        }

        // 羽の先端から風
        if (pEnemyShot->count % 6 == 0 && pEnemyShot->kind == img_enemyShotLargeBall[6])
        {
            sEnemyShot* wind = new sEnemyShot;

            wind->x = pEnemyShot->x;
            wind->y = pEnemyShot->y;

            wind->muki = angle;

            wind->speed = 3.5;

            wind->kind = img_enemyShotSmallBall[3];

            wind->prev = pEnemyShotSet->pEnemyShotHead->prev;
            wind->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = wind;
            pEnemyShotSet->pEnemyShotHead->prev = wind;
        }

        pEnemyShot = pEnemyShot->next;
    }

    // 風だけ通常移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (pEnemyShot->speed > 0.0)
        {
            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

void EnemyPat_ElectricFan_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240;
        enemy.y = 60;

        enemy.maxHp = enemy.hp = 200;

        dir = 1;        
    }

    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFan;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 首振り
    enemy.x += dir * 0.9;

    if (enemy.x > 380)
        dir = -1;

    if (enemy.x < 100)
        dir = 1;

    // ShotSetの座標も敵に追従
    sEnemyShotSet* pEnemyShotSet = enemyShotSetHead.next;

    while (pEnemyShotSet != &enemyShotSetHead)
    {
        if (pEnemyShotSet->count <= 180) {
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
        }
        else {
            pEnemyShotSet->y++;
        }

        pEnemyShotSet = pEnemyShotSet->next;
    }
}