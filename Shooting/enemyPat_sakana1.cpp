// enemyPat_sakana.cpp
// パターン8：対数螺旋＋花びら状
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotLogSpiralPetal(sEnemyShotSet* p)
{
    const int colorTable[] = { 0, 1, 2, 4, 5 };
    sEnemyShot *shot; double r, theta; int i;
    p->count++;
    if (p->count == 1) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        const int N = 24; const double baseSpeed = 1.5;
        for (i=0; i<N; i++) {
            r = 0.5*i; theta = p->muki + 0.1*i;
            double petal = 1.0 + 0.3*sin(theta*3.0);
            shot = new sEnemyShot;
            shot->x = p->x + r*cos(theta)*petal; shot->y = p->y + r*sin(theta)*petal;
            shot->muki = theta; shot->speed = baseSpeed; shot->kind = img_enemyShotMediumBall[colorTable[p->kind%5]];
            shot->prev = p->pEnemyShotHead->prev; shot->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = shot; p->pEnemyShotHead->prev = shot;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        shot->x += shot->speed * cos(shot->muki); shot->y += shot->speed * sin(shot->muki);
        if (shot->x<-100 || shot->x>580 || shot->y<-100 || shot->y>740) {
            sEnemyShot* next = shot->next; shot->prev->next=shot->next; shot->next->prev=shot->prev; delete shot; shot = next;
        } else shot = shot->next;
    }
}

void EnemyPat_Geometry_Sakana()
{
    sEnemyShotSet* p; static double baseX=240, baseY=120;
    if (count==1) { enemy.maxHp=enemy.hp=200; enemy.x=baseX; enemy.y=baseY; }

    enemy.x = baseX + 40*sin(count*0.015); enemy.y = baseY + 30*cos(count*0.012);

    if (count%90==0) {
        const int P = 5;
        for (int i=0; i<P; i++) {
            p = new sEnemyShotSet; p->count=0; p->patternFunc=ShotLogSpiralPetal;
            p->x=enemy.x; p->y=enemy.y; p->muki=(2*PI*i)/P + count*0.02; p->kind=i;
            p->pEnemyShotHead = new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
            p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
        }
    }
}