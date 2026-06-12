// enemyPat13.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotJapanese(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {

        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double base = pEnemyShotSet->muki;

        // 扇状の桜吹雪
        for (int i = -6; i <= 6; i++) {

            sEnemyShot* p = new sEnemyShot;

            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;

            p->muki = base + i * 0.12;
            p->speed = 1.5 + fabs(i) * 0.08;

            p->kind = img_enemyShotScale[5]; // ピンク系

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        // 陰陽玉
        for (int i = 0; i < 2; i++) {

            sEnemyShot* p = new sEnemyShot;

            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;

            p->muki = base + (i ? 0.35 : -0.35);
            p->speed = 1.0;

            p->kind = img_enemyShotLargeBall[i ? 7 : 0];

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;

    while (p != pEnemyShotSet->pEnemyShotHead) {

        // 桜吹雪っぽい揺れ
        if (imageData[p->kind].radius < 12.0) {
            p->muki += sin((pEnemyShotSet->count + p->x) * 0.03) * 0.01;
        }

        p->x += cos(p->muki) * p->speed;
        p->y += sin(p->muki) * p->speed;

        p = p->next;
    }
}

void EnemyPat_Japanese_ChatGPT()
{
    static double angle;

    if (count == 1) {

        enemy.x = 240;
        enemy.y = 70;

        enemy.maxHp = 150;
        enemy.hp = enemy.maxHp;

        angle = 0.0;
    }

    angle += 0.02;

    enemy.x = 240 + cos(angle * 0.7) * 120;

    // 扇子弾
    if (count % 30 == 0) {

        sEnemyShotSet* p = new sEnemyShotSet;

        p->count = 0;
        p->patternFunc = ShotJapanese;

        p->x = enemy.x;
        p->y = enemy.y;

        p->muki = atan2(
            player.y - p->y,
            player.x - p->x);

        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }

    // 風車弾
    if (count % 8 == 0) {

        sEnemyShotSet* p = new sEnemyShotSet;

        p->count = 0;
        p->patternFunc = ShotJapanese;

        p->x = enemy.x;
        p->y = enemy.y;

        p->muki = angle * 2.5;

        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }
}