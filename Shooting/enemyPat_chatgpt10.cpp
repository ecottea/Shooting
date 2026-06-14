// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotDragon(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    // ------------------------------------
    // 初期化（竜の頭）
    // ------------------------------------
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki;
        pShot->speed = 2.4;
        pShot->kind = img_enemyShotLargeBall[1]; // 黄

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // ------------------------------------
    // 竜の頭
    // ------------------------------------
    pShot = pEnemyShotSet->pEnemyShotHead->next;

    if (pShot != pEnemyShotSet->pEnemyShotHead) {

        // 蛇行
        pShot->muki += cos(pEnemyShotSet->count / 18.0) * 0.025;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // --------------------------------
        // 胴体生成（鱗）
        // --------------------------------
        if (pEnemyShotSet->count % 3 == 0 && pEnemyShotSet->count < 180) {

            sEnemyShot* pBody = new sEnemyShot;

            pBody->x = pShot->x - cos(pShot->muki) * 18.0;
            pBody->y = pShot->y - sin(pShot->muki) * 18.0;

            pBody->muki = pShot->muki;
            pBody->speed = 0.0;

            int color = (pEnemyShotSet->count / 24) % 7;
            pBody->kind = img_enemyShotScale[color];

            pBody->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBody->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBody;
            pEnemyShotSet->pEnemyShotHead->prev = pBody;
        }

        // --------------------------------
        // 崩壊
        // --------------------------------
        if (pEnemyShotSet->count == 180) {

            sEnemyShot* pCur = pShot->next;

            while (pCur != pEnemyShotSet->pEnemyShotHead) {

                double ang = GetRand(179) / 180. * DX_PI;

                pCur->muki = ang;
                pCur->speed = 2.8;

                pCur = pCur->next;
            }
        }
    }

    // ------------------------------------
    // 移動
    // ------------------------------------
    pShot = pEnemyShotSet->pEnemyShotHead->next;

    bool first = true;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (!first) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        first = false;
        pShot = pShot->next;
    }
}


// 敵本体
void EnemyPat_Dragon_ChatGPT()
{
    static int moveDir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;

        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
    }
    else {

        enemy.x += moveDir * 1.2;

        if (enemy.x < 120.0) moveDir = 1;
        if (enemy.x > 360.0) moveDir = -1;
    }

    // 竜召喚
    if (count % 90 == 0) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDragon;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 16.0;

        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}