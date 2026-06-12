// enemyPat_claude3.cpp
// パターン12：高難易度複合（速弾・遅弾・加速・散弾）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotComplex(sEnemyShotSet* p)
{
    sEnemyShot *shot, *next; p->count++;
    if (p->count == 1) {
        if (p->kind<=1) {
            if (CheckSoundMem(sound_enemyShot_light)==1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            int ways = (p->kind== img_enemyShotBullet[0]) ? 3 : 5; double spd0 = (p->kind == 0) ? 6.0 : 6.5, spread = PI / 12.0;
            for (int i=0; i<ways; i++) {
                double angle = p->muki + spread*(i-ways/2);
                shot = new sEnemyShot; shot->x=p->x; shot->y=p->y; shot->muki=angle; shot->speed=spd0;  shot->kind= img_enemyShotSmallBall[1];
                shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead; p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
            }
        } else if (p->kind==2) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            for (int i=0; i<24; i++) {
                shot = new sEnemyShot; shot->x=p->x; shot->y=p->y; shot->muki=p->muki+2*PI*i/24; shot->speed=0.75;  shot->kind= img_enemyShotSmallBall[0];
                shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead; p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
            }
        } else if (p->kind==3) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            for (int i=0; i<12; i++) {
                shot = new sEnemyShot; shot->x=p->x; shot->y=p->y; shot->muki=p->muki+2*PI*i/12; shot->speed=0.4;  shot->kind= img_enemyShotSmallBall[2];
                shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead; p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
            }
        } else {
            if (CheckSoundMem(sound_enemyShot_light)==1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            const int N=12; const double TOTAL=PI/3.0;
            for (int i=0; i<N; i++) {
                double angle = p->muki - TOTAL/2.0 + TOTAL*i/(N-1);
                shot = new sEnemyShot; shot->x=p->x; shot->y=p->y; shot->muki=angle; shot->speed=5.5; shot->kind= img_enemyShotBullet[0];
                shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead; p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
            }
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        next = shot->next;
        if (p->kind==3 && p->count>=30 && shot->speed<5.5) shot->speed+=0.09;
        shot->x += shot->speed*cos(shot->muki); shot->y += shot->speed*sin(shot->muki);
        if (shot->x<-50 || shot->x>530 || shot->y<-50 || shot->y>530) { shot->prev->next=shot->next; shot->next->prev=shot->prev; delete shot; }
        shot = next;
    }
    int lifetime = (p->kind<=1 || p->kind==4)?250 : (p->kind==2)?840 : 250;
    if (p->count > lifetime) {
        shot = p->pEnemyShotHead->next;
        while (shot != p->pEnemyShotHead) { next=shot->next; delete shot; shot=next; }
        p->prev->next = p->next; p->next->prev = p->prev; delete p->pEnemyShotHead; delete p; return;
    }
}

void EnemyPat_Difficulty_Claude()
{
    sEnemyShotSet* p;
    if (count==1) { enemy.maxHp=enemy.hp=200; enemy.x=240; enemy.y=80; }
    int phase = (count<=100)?1:(count<=250)?2:3;
    double targetY = (phase<=2)?80.0:120.0;
    enemy.x = 240 + 65*sin(count*0.019); enemy.y += (targetY - enemy.y)*0.008;
    // 速弾
    int kind1 = (phase==1)?0:1, interval1 = (phase==1)?12:(phase==2)?8:6;
    if (count%interval1==0) {
        p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotComplex; p->kind=kind1;
        p->x=enemy.x; p->y=enemy.y+12; p->muki=atan2(player.y-p->y, player.x-p->x);
        p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
    }
    // 遅弾
    int interval2 = (phase==1)?45:(phase==2)?28:18;
    if (count%interval2==interval2/2) {
        p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotComplex; p->kind=2;
        p->x=enemy.x; p->y=enemy.y; p->muki=count*0.07;
        p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
    }
    // 加速弾
    if (phase>=2) {
        int interval3 = (phase==2)?90:55;
        if (count%interval3==interval3/3) {
            p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotComplex; p->kind=3;
            p->x=enemy.x; p->y=enemy.y; p->muki=atan2(player.y-p->y, player.x-p->x);
            p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
            p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
        }
    }
    // 散弾
    if (phase==3 && count%60==30) {
        p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotComplex; p->kind=4;
        p->x=enemy.x; p->y=enemy.y+12; p->muki=atan2(player.y-p->y, player.x-p->x);
        p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
    }
}