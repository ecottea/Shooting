// enemyPat_grok.cpp
// パターン7：幾何学美（多角形＋星形＋螺旋）
#include "DxLib.h"
#include "gv.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotGeometryMulti(sEnemyShotSet* p)
{
    sEnemyShot *shot, *next; double rad, spd; int i, n;
    p->count++;
    if (p->count == 1) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        switch (p->kind) {
        case 0: n = 70; for (i=0; i<n; i++) { rad = p->muki + i*(2*PI/n);
            shot = new sEnemyShot; shot->x=p->x; shot->y=p->y; shot->muki=rad; shot->speed=2.4; shot->kind= img_enemyShotSmallBall[0];
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead; p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
            shot = new sEnemyShot; shot->x=p->x; shot->y=p->y; shot->muki=rad; shot->speed=4.1; shot->kind= img_enemyShotSmallBall[1];
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead; p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
        } break;
        case 1: n = 90; for (i=0; i<n; i++) { rad = p->muki + i*(PI/5.0); spd = (i%2==0)?3.7:4.6;
            shot = new sEnemyShot; shot->x=p->x; shot->y=p->y; shot->muki=rad; shot->speed=spd; shot->kind= img_enemyShotSmallBall[2];
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead; p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
        } break;
        case 2: n = 110; for (i=0; i<n; i++) { rad = p->muki + i*(2*PI/n);
            shot = new sEnemyShot; shot->x=p->x; shot->y=p->y; shot->muki=rad; shot->speed=2.3; shot->kind= img_enemyShotBullet[0];
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead; p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
        } break;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        next = shot->next;
        shot->x += shot->speed * cos(shot->muki); shot->y += shot->speed * sin(shot->muki);
        if (shot->x<-50 || shot->x>530 || shot->y<-50 || shot->y>530) { shot->prev->next=shot->next; shot->next->prev=shot->prev; delete shot; }
        shot = next;
    }
    if (p->count > 380) {
        shot = p->pEnemyShotHead->next;
        while (shot != p->pEnemyShotHead) { next=shot->next; delete shot; shot=next; }
        p->prev->next = p->next; p->next->prev = p->prev; delete p;
    }
}

void EnemyPat_Geometry_Grok()
{
    sEnemyShotSet* p;
    if (count == 1) { enemy.maxHp=150; enemy.hp=150; enemy.x=240; enemy.y=120; }
    enemy.x = 240 + 48*sin(count*0.016); enemy.y = 125 + 38*cos(count*0.013);
    if (count % 85 == 0) {
        for (int type=0; type<3; type++) {
            p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotGeometryMulti;
            p->x=enemy.x; p->y=enemy.y; p->muki=count*0.023 + type*(2*PI/3); p->kind=type;
            p->pEnemyShotHead = new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
            p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
        }
    }
}