// enemyPat_grok2.cpp
// パターン14：自機狙い7WAY＋左右カーテン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void Shot7WayCurtain(sEnemyShotSet* p)
{
    sEnemyShot *shot, *next; double rad; int i, n;
    p->count++;
    if (p->count==1) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        if (p->kind==0) {
            n=7; for (i=-3; i<=3; i++) {
                rad = p->muki + i*0.24; shot = new sEnemyShot;
                shot->x=p->x; shot->y=p->y; shot->muki=rad; shot->speed=3.9+abs(i)*0.15; shot->kind= img_enemyShotMediumBall[5];
                shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead;
                p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
            }
            for (i=0; i<40; i++) {
                rad = p->muki + (GetRand(140)-70)*0.018; shot = new sEnemyShot;
                shot->x=p->x; shot->y=p->y; shot->muki=rad; shot->speed=2.7+GetRand(80)/90.0; shot->kind= img_enemyShotDiamond[1];
                shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead;
                p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
            }
        } else {
            double baseAngle = (p->muki>0)?0.3:PI-0.3; n=9;
            for (i=0; i<n; i++) {
                rad = baseAngle + (i-4)*0.22; shot = new sEnemyShot;
                shot->x=p->x; shot->y=p->y; shot->muki=rad; shot->speed=4.2; shot->kind= img_enemyShotLargeBall[0];
                shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead;
                p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
            }
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        next = shot->next; shot->x+=shot->speed*cos(shot->muki); shot->y+=shot->speed*sin(shot->muki);
        if (shot->x<-50 || shot->x>530 || shot->y<-50 || shot->y>530) { shot->prev->next=next; next->prev=shot->prev; delete shot; }
        shot = next;
    }
    if (p->count>500) {
        shot = p->pEnemyShotHead->next;
        while (shot != p->pEnemyShotHead) { next=shot->next; delete shot; shot=next; }
        p->prev->next=p->next; p->next->prev=p->prev; delete p;
    }
}

void EnemyPat_Dodge_Grok()
{
    sEnemyShotSet* p;
    if (count==1) { enemy.maxHp=enemy.hp=200; enemy.x=240; enemy.y=80; }

    enemy.x = 240 + 95*sin(count*0.023); enemy.y = 95 + 30*cos(count*0.019);

    if (count%45==0) {
        p = new sEnemyShotSet; p->count=0; p->patternFunc=Shot7WayCurtain;
        p->x=enemy.x; p->y=enemy.y; p->muki=atan2(player.y-enemy.y, player.x-enemy.x); p->kind=0;
        p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
    }
    if (count%72==20) {
        for (int side=-1; side<=1; side+=2) {
            p = new sEnemyShotSet; p->count=0; p->patternFunc=Shot7WayCurtain;
            p->x=enemy.x+side*25; p->y=enemy.y; p->muki=side; p->kind=1;
            p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
            p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
        }
    }
}