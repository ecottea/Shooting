// enemyPat_gemini2.cpp
// パターン11：時間差包囲（静止→一斉狙い）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotTimeLagAmbush(sEnemyShotSet* p)
{
    const int colorTable[] = { 0, 1, 2, 4, 5 };
    sEnemyShot* shot; int local = p->count % 140;
    if (local >=0 && local<=40 && local%6==0) {
        if (CheckSoundMem(sound_enemyShot_light)==1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        int ways = 5;
        for (int i=0; i<ways; i++) {
            shot = new sEnemyShot; shot->x=p->x; shot->y=p->y;
            shot->muki = (2*PI/ways)*i + local*0.15; shot->speed = 8.5;
            shot->kind = img_enemyShotSmallBall[colorTable[i%5]];
            shot->prev = p->pEnemyShotHead->prev; shot->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = shot; p->pEnemyShotHead->prev = shot;
        }
    }
    if (local == 70) {
        if (CheckSoundMem(sound_enemyShot_medium)==1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        shot = p->pEnemyShotHead->next;
        while (shot != p->pEnemyShotHead) {
            shot->muki = atan2(player.y-shot->y, player.x-shot->x);
            shot->speed = 4.5; shot = shot->next;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        if (local < 70) shot->speed *= 0.85;
        shot->x += shot->speed*cos(shot->muki); shot->y += shot->speed*sin(shot->muki);
        shot = shot->next;
    }
}

void EnemyPat_Dodge_Gemini()
{
    if (count==1) { enemy.maxHp=180; enemy.hp=180; enemy.x=240; enemy.y=70; }
    if (count%300==30) {
        sEnemyShotSet* p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotTimeLagAmbush;
        p->x=enemy.x; p->y=enemy.y; p->kind=0; p->muki=0;
        p->pEnemyShotHead = new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
    }
}