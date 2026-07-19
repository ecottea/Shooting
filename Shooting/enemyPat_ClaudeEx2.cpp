// enemyPat_Claude4.cpp
// パターン17：幾何弾幕ボス（ハイポトロコイド→アストロイド→ネフロイド）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotHypotrochoid(sEnemyShotSet* p)
{
    sEnemyShot* shot;
    if (p->count==0) {
        if (CheckSoundMem(sound_enemyShot_medium)==1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        const int N=200; const double R_r=38.0, kappa=2.5, d=24.0, phi=p->muki;
        for (int i=0; i<N; i++) {
            double t = PI*4.0/N*i + phi, sx = R_r*cos(t)+d*cos(kappa*t), sy = R_r*sin(t)-d*sin(kappa*t);
            shot = new sEnemyShot; shot->x=p->x+sx; shot->y=p->y+sy; shot->muki=atan2(sy,sx); shot->speed=2.2;
            shot->kind= img_enemyShotSmallBall[i*4/N];
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead; p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) { shot->x+=shot->speed*cos(shot->muki); shot->y+=shot->speed*sin(shot->muki); shot=shot->next; }
}

static void ShotAstroid(sEnemyShotSet* p)
{
    sEnemyShot* shot;
    if (p->count==0) {
        if (CheckSoundMem(sound_enemyShot_heavy)==1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        const int N=96; const double a=55.0, phi=p->muki; const double speeds[2]={1.8,3.6};
        for (int layer=0; layer<2; layer++) for (int i=0; i<N; i++) {
            double t=2*PI*i/N, ct=cos(t), st=sin(t), sx0=a*ct*ct*ct, sy0=a*st*st*st;
            double sx=sx0*cos(phi)-sy0*sin(phi), sy=sx0*sin(phi)+sy0*cos(phi);
            shot = new sEnemyShot; shot->x=p->x+sx; shot->y=p->y+sy; shot->muki=atan2(sy,sx); shot->speed=speeds[layer];
            shot->kind= img_enemyShotSmallBall[i*4/N];
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead; p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) { shot->x+=shot->speed*cos(shot->muki); shot->y+=shot->speed*sin(shot->muki); shot=shot->next; }
}

static void ShotNephroid(sEnemyShotSet* p)
{
    sEnemyShot* shot;
    if (p->count==0) {
        if (CheckSoundMem(sound_enemyShot_light)==1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        const int N=120; const double r=12.0, phi=p->muki;
        for (int i=0; i<N; i++) {
            double t=2*PI*i/N, sx0=r*(3*cos(t)-cos(3*t)), sy0=r*(3*sin(t)-sin(3*t));
            double sx=sx0*cos(phi)-sy0*sin(phi), sy=sx0*sin(phi)+sy0*cos(phi);
            shot = new sEnemyShot; shot->x=p->x+sx; shot->y=p->y+sy; shot->muki=atan2(sy,sx); shot->speed=2.5;
            shot->kind= img_enemyShotSmallBall[i*4/N];
            shot->prev=p->pEnemyShotHead->prev; shot->next=p->pEnemyShotHead; p->pEnemyShotHead->prev->next=shot; p->pEnemyShotHead->prev=shot;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) { shot->x+=shot->speed*cos(shot->muki); shot->y+=shot->speed*sin(shot->muki); shot=shot->next; }
}

void EnemyPat_Geometry_Claude2()
{
    sEnemyShotSet* p; static int shotPhase;
    if (count==1) { enemy.x=240; enemy.y=-20; enemy.maxHp=250; enemy.hp=250; shotPhase=0; }
    if (count<90) enemy.y += (48.0-enemy.y)*0.08;
    if (count>=90) enemy.x = 240 + 85*sin((count-90)*0.009);
    if (count%40==30) {
        p = new sEnemyShotSet; p->count=0; p->kind=0; p->x=enemy.x; p->y=enemy.y;
        switch (shotPhase%3) {
        case 0: p->patternFunc=ShotHypotrochoid; p->muki=(shotPhase/3)*(2*PI/5); break;
        case 1: p->patternFunc=ShotAstroid;      p->muki=(shotPhase/3)*(PI/8); break;
        case 2: p->patternFunc=ShotNephroid;     p->muki=(shotPhase/3)*(PI/3); break;
        }
        p->pEnemyShotHead=new sEnemyShot; p->pEnemyShotHead->prev=p->pEnemyShotHead->next=p->pEnemyShotHead;
        p->prev=enemyShotSetHead.prev; p->next=&enemyShotSetHead; enemyShotSetHead.prev->next=p; enemyShotSetHead.prev=p;
        shotPhase++;
    }
}