// enemyPat_chatgpt2.cpp
// パターン13：自機狙い10WAY＋ランダム散弾
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotAimed10Way(sEnemyShotSet* p)
{
    const int colorTable[] = { 0, 1, 2, 4, 5 };
    sEnemyShot* shot;
    if (p->count==0) {
        if (CheckSoundMem(sound_enemyShot_light)==1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i=0; i<10; i++) {
            shot = new sEnemyShot; shot->x=p->x; shot->y=p->y;
            shot->muki = p->muki + (i-4.5)*0.12; shot->speed=2.2; shot->kind= img_enemyShotSmallBall[colorTable[i%5]];
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) { shot->x+=shot->speed*cos(shot->muki); shot->y+=shot->speed*sin(shot->muki); shot=shot->next; }
}

static void ShotRandomScatter(sEnemyShotSet* p)
{
    sEnemyShot* shot;
    if (p->count==0) {
        if (CheckSoundMem(sound_enemyShot_medium)==1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i=0; i<36; i++) {
            shot = new sEnemyShot; shot->x=p->x; shot->y=p->y;
            shot->muki=2*PI*i/36; shot->speed=5.0; shot->kind= img_enemyShotBullet[0]; 
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) { shot->x+=shot->speed*cos(shot->muki); shot->y+=shot->speed*sin(shot->muki); shot=shot->next; }
}

void EnemyPat_Dodge_ChatGPT()
{
    sEnemyShotSet* p;
    if (count==1) { enemy.maxHp=100; enemy.hp=100; enemy.x=240; enemy.y=70; }
    enemy.x = 240 + 120*sin(count*0.015);
    if (count%21==0) {
        p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotAimed10Way;
        p->x=enemy.x; p->y=enemy.y; p->muki=atan2(player.y-enemy.y, player.x-enemy.x); p->kind=0;
        p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
    }
    if (count%31==10) {
        p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotRandomScatter;
        p->x=enemy.x+(GetRand(200)-100)/10.0; p->y=enemy.y+(GetRand(200)-100)/10.0; p->muki=0; p->kind=0;
        p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
    }
}