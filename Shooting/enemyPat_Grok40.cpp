#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 二重振り子風弾幕パターン
static void ShotDoublePendulum(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double PI = DX_PI;
    const int NUM_CHAINS = 6; // 振り子の本数

    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 0;
        for (int i = 0; i < NUM_CHAINS; i++) {
            pEnemyShotSet->param_d[i] = (GetRand(40) - 20) / 100.0; // 位相オフセット
        }
    }

    pEnemyShotSet->param_i[0]++;
    int phase = pEnemyShotSet->param_i[0];
    double baseAngle = pEnemyShotSet->muki;

    // 第一振り子（大きな弧・青系中玉/中楕円）
    if (phase % 3 == 0) {
        for (int c = 0; c < NUM_CHAINS; c++) {
            double offset = pEnemyShotSet->param_d[c];
            double upperAngle = sin((phase + offset * 30) * 0.025) * 1.2;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + cos(baseAngle + upperAngle) * 25.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(baseAngle + upperAngle) * 25.0 + phase * 1.8;

            pEnemyShot->muki = baseAngle + upperAngle * 0.6;
            pEnemyShot->speed = 2.8 + sin(phase * 0.1) * 0.4;

            pEnemyShot->kind = (c % 3 == 0) ? img_enemyShotMediumOval[3] : img_enemyShotMediumBall[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 第二振り子（高速カオス・赤系小弾）
    if (phase % 2 == 0) {
        for (int c = 0; c < NUM_CHAINS; c++) {
            double offset = pEnemyShotSet->param_d[c];
            double upperAngle = sin((phase + offset * 30) * 0.025) * 1.2;
            double lowerAngle = sin((phase + offset * 45) * 0.085) * 2.5;

            double tipX = pEnemyShotSet->x + cos(baseAngle + upperAngle) * 48.0;
            double tipY = pEnemyShotSet->y + sin(baseAngle + upperAngle) * 48.0 + phase * 1.8;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = tipX;
            pEnemyShot->y = tipY;

            pEnemyShot->muki = baseAngle + upperAngle + lowerAngle * 0.7;
            pEnemyShot->speed = 3.8 + fabs(sin(phase * 0.2)) * 1.2;

            pEnemyShot->kind = (c % 2 == 0) ? img_enemyShotSmallBall[0] : img_enemyShotDiamond[8];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾移動 + 第二振り子らしい微小な方向変化
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->kind == img_enemyShotSmallBall[0] || pEnemyShot->kind == img_enemyShotDiamond[8]) {
            pEnemyShot->muki += sin(phase * 0.15) * 0.035;
        }
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_DoublePendulum_Grok()
{
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 1.1 * (double)muki;
        if (count % 140 == 70) muki *= -1;
        if (count % 85 == 0) {
            enemy.y = 50.0 + GetRand(30);
        }
    }

    // 二重振り子セット生成
    if (count % 150 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDoublePendulum;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x)
            + (GetRand(10) - 5) / 180.0 * DX_PI;

        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}