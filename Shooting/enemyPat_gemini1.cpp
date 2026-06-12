// enemyPat_gemini.cpp
// パターン5：動的バラ曲線（連続発射）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotDynamicRose(sEnemyShotSet* p)
{
    const int colorTable[] = { 0, 1, 2, 4, 5 };
    sEnemyShot* shot;
    if (p->count % 2 == 0 && p->count < 400) {
        if (p->count % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        double k = 5.0 / 3.0;
        double theta = p->count * 0.05;
        double r = sin(k * theta);
        int ways = 5;
        for (int w = 0; w < ways; w++) {
            shot = new sEnemyShot;
            shot->x = p->x; shot->y = p->y;
            shot->muki = theta * 0.3 + (2*PI/ways) * w;
            shot->speed = 1.5 + 4.0 * fabs(r);
            shot->kind = img_enemyShotSmallBall[colorTable[w % 5]];
            shot->prev = p->pEnemyShotHead->prev;
            shot->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = shot;
            p->pEnemyShotHead->prev = shot;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);
        shot = shot->next;
    }
}

void EnemyPat_Geometry_Gemini()
{
    static double baseX = 240.0, baseY = 120.0;
    if (count == 1) {
        enemy.maxHp = 150; enemy.hp = 150;
        enemy.x = baseX; enemy.y = baseY;
    }
    if (count % 300 == 1) {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0; p->patternFunc = ShotDynamicRose;
        p->x = enemy.x; p->y = enemy.y; p->kind = 0; p->muki = 0;
        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead->next = p->pEnemyShotHead;
        p->prev = enemyShotSetHead.prev; p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p; enemyShotSetHead.prev = p;
    }
}