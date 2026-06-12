// enemyPat_claude.cpp
// パターン9：フィボナッチ螺旋（ひまわり）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotSunflower(sEnemyShotSet* p)
{
    sEnemyShot *shot, *next;
    p->count++;
    if (p->count == 1) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        const int N = 960; const double PHI = PI*(3-sqrt(5.0)), SCALE = 5.5;
        for (int i=0; i<N; i++) {
            double angle = p->muki + i*PHI, r = SCALE*sqrt(i+1.0);
            shot = new sEnemyShot;
            shot->x = p->x + r*cos(angle); shot->y = p->y + r*sin(angle);
            shot->muki = angle; shot->speed = 0; shot->kind = img_enemyShotSmallBall[i%4];
            shot->prev = p->pEnemyShotHead->prev; shot->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = shot; p->pEnemyShotHead->prev = shot;
        }
    }
    if (p->count == 35) {
        if (CheckSoundMem(sound_enemyShot_light)==1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        next = shot->next;
        if (p->count >= 35) {
            double target = (shot->kind%2==0)?3.6:2.4;
            if (shot->speed < target) shot->speed += 0.11;
            shot->x += shot->speed * cos(shot->muki); shot->y += shot->speed * sin(shot->muki);
        }
        if (shot->x<-50 || shot->x>530 || shot->y<-50 || shot->y>530) {
            shot->prev->next = shot->next; shot->next->prev = shot->prev; delete shot;
        }
        shot = next;
    }
    if (p->count > 350) {
        shot = p->pEnemyShotHead->next;
        while (shot != p->pEnemyShotHead) { next=shot->next; delete shot; shot=next; }
        p->prev->next = p->next; p->next->prev = p->prev; delete p->pEnemyShotHead; delete p; return;
    }
}

void EnemyPat_Geometry_Claude()
{
    sEnemyShotSet* p;
    if (count==1) { enemy.maxHp=160; enemy.hp=160; enemy.x=240; enemy.y=90; }
    enemy.x = 240 + 55*sin(count*0.016); enemy.y = 90 + 20*cos(count*0.011);
    if (count%180==60) {
        p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotSunflower;
        p->x=enemy.x; p->y=enemy.y; p->muki=count*0.04; p->kind=0;
        p->pEnemyShotHead = new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
    }
}