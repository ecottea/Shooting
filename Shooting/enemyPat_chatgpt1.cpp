// パターン6：Lissajous曲線カーテン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotLissajousCurtain(sEnemyShotSet* p)
{
    const int N = 160; sEnemyShot* shot;
    if (p->count == 1) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        for (int i = 0; i < N; i++) {
            shot = new sEnemyShot;
            shot->x = p->x; shot->y = p->y;
            shot->speed = i; shot->muki = 2*PI * i / N;
            shot->kind = img_enemyShotSmallBall[i%6];
            shot->prev = p->pEnemyShotHead->prev;
            shot->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = shot;
            p->pEnemyShotHead->prev = shot;
        }
    }
    const double A = 240, B = 200, phase = p->count * 0.02;
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        int idx = (int)shot->speed;
        double t = 2*PI * idx / N;
        double x = A*sin(3*t + phase), y = B*sin(4*t);
        double rot = phase*0.5;
        double rx = x*cos(rot) - y*sin(rot);
        double ry = x*sin(rot) + y*cos(rot);
        shot->x = p->x + rx + cos(p->muki)*p->count;
        shot->y = p->y + ry + sin(p->muki)*p->count;
        shot = shot->next;
    }
}

void EnemyPat_Geometry_ChatGPT()
{
    sEnemyShotSet* p;
    if (count == 1) {
        enemy.maxHp = 100; enemy.hp = 100;
        enemy.x = 240; enemy.y = 200;
    }
    enemy.x = 240 + 50*sin(count*0.01); enemy.y = 200;
    if (count % 300 == 60) {
        p = new sEnemyShotSet;
        p->count = 0; p->patternFunc = ShotLissajousCurtain;
        p->x = enemy.x; p->y = enemy.y;
        p->muki = atan2(player.y - p->y, player.x - p->x);
        p->kind = 0;
        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead->next = p->pEnemyShotHead;
        p->prev = enemyShotSetHead.prev; p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p; enemyShotSetHead.prev = p;
    }
}