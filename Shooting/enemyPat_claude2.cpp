// enemyPat_Claude2.cpp
// パターン16：振り子掃射＋時間差3連自機狙い
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotPendulumSweep(sEnemyShotSet* p)
{
    const int colorTable[] = { 0, 1, 2, 4, 5 };
    sEnemyShot* shot;
    if (p->count==0) { if (CheckSoundMem(sound_enemyShot_light)==1) StopSoundMem(sound_enemyShot_light); PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK); }
    if (p->count<160 && p->count%3==0) {
        double sweep = sin(p->count*0.055)*(PI*5.0/18.0);
        shot = new sEnemyShot; shot->x=p->x; shot->y=p->y; shot->muki=p->muki+sweep; shot->speed=4.0;
        shot->kind= img_enemyShotSmallBall[colorTable[(p->count/3)%5]];
        shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead;
        p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) { shot->x+=shot->speed*cos(shot->muki); shot->y+=shot->speed*sin(shot->muki); shot=shot->next; }
}

static void ShotTripleDelayedAim(sEnemyShotSet* p)
{
    const int colorTable[] = { 0, 1, 2, 4, 5 };
    sEnemyShot* shot;
    if (p->count==0 || p->count==10 || p->count==20 || p->count==30 || p->count==40) {
        if (CheckSoundMem(sound_enemyShot_light)==1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        int phase = p->count/10;
        shot = new sEnemyShot; shot->x=enemy.x; shot->y=enemy.y;
        shot->muki=atan2(player.y-enemy.y, player.x-enemy.x); shot->speed=4.0+phase; shot->kind= img_enemyShotMediumBall[colorTable[phase]];
        shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead;
        p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) { shot->x+=shot->speed*cos(shot->muki); shot->y+=shot->speed*sin(shot->muki); shot=shot->next; }
}

void EnemyPat_Dodge_Claude()
{
    sEnemyShotSet* p;
    if (count==1) { enemy.maxHp=enemy.hp=200; }
    enemy.x = 240 + 160*sin(count*0.022); enemy.y = 42 + 12*sin(count*0.031);
    int interval1 = (enemy.hp<=enemy.maxHp/2)?90:120;
    if (count%interval1==2) {
        p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotPendulumSweep; p->kind=0;
        p->x=enemy.x; p->y=enemy.y; p->muki=atan2(player.y-enemy.y, player.x-enemy.x);
        p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
    }
    if (enemy.hp<=enemy.maxHp/2 && count%60==31) {
        p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotTripleDelayedAim; p->kind=0;
        p->x=enemy.x; p->y=enemy.y; p->muki=atan2(player.y-enemy.y, player.x-enemy.x);
        p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
    }
}