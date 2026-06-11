// enemyPat_handmade3.cpp
// ƒpƒ^[ƒ“3Fã•”XÀ•W’Ç”ö{”j—ô’ei‰~Œ`U’ej
#include "DxLib.h"
#include "gv.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

// ”j—ôŒã‚Ì‰~Œ`U’ei©—R—‰ºj
static void ShotBurstCircle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    double accel;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 48; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = PI * 2.0 / 48.0 * (double)i;
            pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShotSet->kind];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        accel = (pEnemyShotSet->count < 200) ? pow(1.01, (double)pEnemyShotSet->count) : 7.31601785;
        pEnemyShot->x += 1.7 * cos(pEnemyShot->muki);
        pEnemyShot->y += accel + 1.7 * sin(pEnemyShot->muki) - 1.5;
        pEnemyShot = pEnemyShot->next;
    }
}

// ”j—ô‚·‚é’¼i’e
static void ShotBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot, * pNextEnemyShot;
    sEnemyShotSet* pNewEnemyShotSet;
    // static int kind ‚ÍíœipEnemyShotSet->kind ‚ğ‚»‚Ì‚Ü‚Ü—¬—pj

    if (pEnemyShotSet->count == 0) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 5.5;
        pEnemyShot->kind = img_enemyShotLargeBall[7];
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    if (pEnemyShotSet->count == 31) {
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            pNextEnemyShot = pEnemyShot->next;
            pNewEnemyShotSet = new sEnemyShotSet;
            pNewEnemyShotSet->count = 0;
            pNewEnemyShotSet->patternFunc = ShotBurstCircle;
            pNewEnemyShotSet->x = pEnemyShot->x;
            pNewEnemyShotSet->y = pEnemyShot->y;
            pNewEnemyShotSet->kind = pEnemyShotSet->kind;   // “G–{‘Ì‚©‚ç“n‚³‚ê‚½ kind ‚ğ‚»‚Ì‚Ü‚Üg—p

            pNewEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pNewEnemyShotSet->pEnemyShotHead->prev = pNewEnemyShotSet->pEnemyShotHead;
            pNewEnemyShotSet->pEnemyShotHead->next = pNewEnemyShotSet->pEnemyShotHead;

            pNewEnemyShotSet->prev = enemyShotSetHead.prev;
            pNewEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pNewEnemyShotSet;
            enemyShotSetHead.prev = pNewEnemyShotSet;
            delete pEnemyShot;
            pEnemyShot = pNextEnemyShot;
        }
        delete pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev->next = pEnemyShotSet->next;
        pEnemyShotSet->next->prev = pEnemyShotSet->prev;
        delete pEnemyShotSet;
        return;
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        if (pEnemyShotSet->count > 15 && pEnemyShotSet->count < 31)
            pEnemyShot->speed *= 0.75;
        pEnemyShot = pEnemyShot->next;
    }
}

void EnemyPat_Burst()
{
    static double x, y;
    if (count == 1) {
        enemy.maxHp = 100;
        enemy.hp = 100;
        enemy.x = 240.0;
        enemy.y = 35.0;
        x = y = 0.0;
    }
    if (count % 180 == 0) {
        x = player.x - enemy.x;
        y = (double)(GetRand(50) + 20) - enemy.y;
    }
    else if (count % 180 < 26) {
        enemy.x += x / 29.0;
        enemy.y += y / 29.0;
    }
    if (count % 180 == 26) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 5; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotBurst;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = PI / 4 * (double)i;
            pEnemyShotSet->kind = i;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}