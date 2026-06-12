// enemyPat_sakana2.cpp
// パターン15：ランダム方向多数発射＋速度変化
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotRandomDirectionMultiSpeed(sEnemyShotSet* p)
{
    sEnemyShot* shot;
    if (p->count==0) {
        if (CheckSoundMem(sound_enemyShot_light)==1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i=0; i<80; i++) {
            shot = new sEnemyShot; shot->x=p->x; shot->y=p->y;
            shot->muki = 2*PI*GetRand(1000)/1000.0; shot->speed=8.0; shot->kind= img_enemyShotBullet[0];
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        shot->x += shot->speed*cos(shot->muki); shot->y += shot->speed*sin(shot->muki);
        if (p->count>20 && p->count<40) shot->speed *= 0.92;
        else if (p->count>=40) { shot->speed *= 0.98; if (shot->speed<1) shot->speed=1; }
        shot = shot->next;
    }
}

void EnemyPat_Dodge_Sakana()
{
    sEnemyShotSet* p;
    if (count%16==0) {
        p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotRandomDirectionMultiSpeed;
        p->x=enemy.x; p->y=enemy.y+10;
        p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
    } else if (count==1) { enemy.x=240; enemy.y=40; enemy.maxHp=enemy.hp=200; }
}