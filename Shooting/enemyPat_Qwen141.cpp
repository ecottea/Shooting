#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotMouryou(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    int phase = (pEnemyShotSet->count / 180) % 2;
    int t = pEnemyShotSet->count % 180;

    if (t % 4 == 0) {
        int arms = 4;
        double base_angle = 0.0;
        int color;
        int num_segments;
        double rot_speed;

        if (phase == 0) {
            color = 4;
            num_segments = 3;
            rot_speed = -0.04;
        }
        else {
            color = 0;
            num_segments = 5;
            rot_speed = 0.05;
        }

        for (int i = 0; i < arms; i++) {
            double arm_angle = base_angle + i * (DX_PI / 2.0);
            for (int j = 1; j <= num_segments; j++) {
                sEnemyShot* pShot = new sEnemyShot;
                double radius = 32.0 * j;
                pShot->param_d[0] = radius;
                pShot->param_d[1] = arm_angle;
                pShot->param_d[2] = rot_speed;
                pShot->kind = img_enemyShotLaser[color];
                pShot->speed = 0.0;
                pShot->margin = 480;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_d[2] != 0.0) {
            pShot->param_d[1] += pShot->param_d[2];
            pShot->x = pEnemyShotSet->x + cos(pShot->param_d[1]) * pShot->param_d[0];
            pShot->y = pEnemyShotSet->y + sin(pShot->param_d[1]) * pShot->param_d[0];
            pShot->muki = pShot->param_d[1];
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        if (pShot->count >= 600) pShot->margin = -9999;
        pShot = pShot->next;
    }

    if (t % 1 == 0) {
        int arms = 4;
        double base_angle = (phase == 0) ? (-pEnemyShotSet->count * 0.04) : (pEnemyShotSet->count * 0.05);
        int color = 5;

        for (int i = 0; i < arms; i++) {
            double gap_angle = base_angle + (DX_PI / 4.0) + i * (DX_PI / 2.0);
            double spread = (double)(GetRand(20) - 10) / 180.0 * DX_PI;
            double shoot_angle = gap_angle + spread;

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = shoot_angle;
            pShot->speed = 2.5 + (double)GetRand(100) / 100.0;
            pShot->kind = img_enemyShotScale[color];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
}

void EnemyPat_Zendera_Qwen()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMouryou;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (count % 120 == 60) muki *= -1;

        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet = pSet->next;
        }
    }
}