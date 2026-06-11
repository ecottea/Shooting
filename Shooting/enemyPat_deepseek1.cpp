// enemyPat_deepseek.cpp
// パターン4：幾何学花模様（5弁バラ曲線＋スパイラル）
#include "DxLib.h"
#include "gv.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

static void ShotRoseCurveSpiral(sEnemyShotSet* p)
{
    sEnemyShot* shot;
    const int PETAL = 5, N = 120;
    const double R = 35.0, SPEED = 2.8, ROT = 0.025;
    
    if (p->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < N; ++i) {
            double theta = 2.0 * PI * i / N;
            double r = R * cos(PETAL * theta); if (r < 0) r = -r;
            double bx = p->x + r * cos(theta), by = p->y + r * sin(theta);
            shot = new sEnemyShot;
            shot->x = bx; shot->y = by;
            shot->muki = theta; shot->speed = SPEED;
            shot->kind = img_enemyShotSmallBall[i % 6];
            shot->prev = p->pEnemyShotHead->prev;
            shot->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = shot;
            p->pEnemyShotHead->prev = shot;
        }
    }
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        shot->x += shot->speed * cos(shot->muki) + cos(p->muki) * p->count / 60;
        shot->y += shot->speed * sin(shot->muki) + sin(p->muki) * p->count / 60;
        double rot = ROT; if (p->kind == 1) rot = -rot;
        shot->muki += rot;
        shot = shot->next;
    }
}

void EnemyPat_Geometry_DeepSeek()
{
    sEnemyShotSet* p;
    static double phase = 0, baseX = 240.0, baseY = 160.0;
    if (count == 1) {
        enemy.maxHp = 80; enemy.hp = 80;
        enemy.x = baseX; enemy.y = baseY;
    }
    enemy.x = baseX + 30.0 * sin(count * 0.02); enemy.y = baseY;
    if (count % 120 == 30) {
        for (int dir = 0; dir < 2; ++dir) {
            p = new sEnemyShotSet;
            p->count = 0; p->patternFunc = ShotRoseCurveSpiral;
            p->x = enemy.x; p->y = enemy.y;
            p->muki = atan2(player.y - p->y, player.x - p->x);
            p->kind = dir;
            p->pEnemyShotHead = new sEnemyShot;
            p->pEnemyShotHead->prev = p->pEnemyShotHead->next = p->pEnemyShotHead;
            p->prev = enemyShotSetHead.prev; p->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = p; enemyShotSetHead.prev = p;
        }
        phase += PI / 16.0; if (phase > 2*PI) phase -= 2*PI;
    }
}