// enemyPat_deepseek2.cpp
// パターン10：避けてて楽しい弾幕（窄角5WAY＋左右制限＋ランダム）
#include "DxLib.h"
#include "gv.h"
#include <math.h>

static void ShotFunToDodge(sEnemyShotSet* p)
{
    sEnemyShot* shot;
    if (p->count == 0) {
        for (int j=0; j<=2; j++) for (int i=-(3-j); i<=3-j; i++) {
            shot = new sEnemyShot; shot->x=p->x; shot->y=p->y; shot->muki=p->muki + i*0.03;
            shot->speed=3.8+j*0.5; shot->kind= img_enemyShotBullet[0]; 
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
        }
        for (int side=0; side<2; side++) {
            double sign = (side==0)?-1:1;
            for (int i=0; i<20; i++) {
                shot = new sEnemyShot; shot->x=p->x; shot->y=p->y;
                shot->muki=p->muki + sign*(0.35+i*0.22); shot->speed=1.8; shot->kind= img_enemyShotLargeBall[1];
                shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead;
                p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
            }
        }
        for (int i=0; i<60; i++) {
            shot = new sEnemyShot; shot->x=p->x+(GetRand(200)-100)/10.0; shot->y=p->y+(GetRand(200)-100)/10.0;
            shot->muki=p->muki+((GetRand(300)-150)/100.0)*0.7; shot->speed=2.3+GetRand(100)/100.0;
            shot->kind= img_enemyShotSmallBall[2+(i%2)];
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        shot->x += shot->speed*cos(shot->muki); shot->y += shot->speed*sin(shot->muki);
        shot = shot->next;
    }
}

void Pattern10_FunToDodge()
{
    sEnemyShotSet* p; static double targetX = 240;
    if (count==1) { enemy.maxHp=180; enemy.hp=180; enemy.x=240; enemy.y=70; }
    targetX += (player.x - targetX)*0.01; enemy.x = targetX; enemy.y = 70 + 25*sin(count*0.04);
    if (count%75==0) {
        p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotFunToDodge;
        p->x=enemy.x; p->y=enemy.y; p->muki=atan2(player.y-enemy.y, player.x-enemy.x);
        p->pEnemyShotHead = new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
}