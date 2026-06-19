// enemyPat_washingMachine.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 洗濯機モチーフ弾幕：回転ドラム + 泡＆水しぶき
static void ShotWashingMachine(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int num = 16;
        double baseAngle = pEnemyShotSet->muki;

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double rot = (pEnemyShotSet->count / 3.0) * 0.3 + i * (DX_PI * 2.0 / num);
            pEnemyShot->muki = baseAngle + rot;

            pEnemyShot->speed = 1.8 + (i % 3) * 0.4;

            int type = GetRand(3);
            int color = (GetRand(3) == 0) ? 4 : 6; // 青・白中心

            switch (type) {
            case 0:
            case 1:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                pEnemyShot->speed *= 0.75;
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            default:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                break;
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 更新処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot->speed *= 0.995;
        pEnemyShot->muki += 0.008 * sin(pEnemyShotSet->count / 12.0);

        pEnemyShot = pEnemyShot->next;
    }
}

// 強力すすぎ用（大量水しぶき）
static void ShotStrongRinse(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 28; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(90) - 45;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(90) - 45) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.4 + GetRand(150) / 100.0;

            int color = (i % 4 == 0) ? 3 : 4; // シアン・青

            if (i % 5 == 0)
                pEnemyShot->kind = img_enemyShotLargeBall[color];
            else
                pEnemyShot->kind = img_enemyShotScale[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 単純直進＋少し減速
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot->speed *= 0.98;

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体パターン：洗濯機
void EnemyPat_WashingMachine_Grok()
{
    static double floatY = 0.0;
    static int muki = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        floatY = 0.0;
        muki = 1;
    }

    // 洗濯機らしい上下振動
    floatY = sin(count / 22.0) * 14.0;
    enemy.y = 60.0 + floatY;

    // 左右移動
    enemy.x += muki;
    if (enemy.x < 50) muki = 1;;
    if (enemy.x > 480 - 50) muki = -1;

    // 通常の回転ドラム弾幕
    if (count % 15 == 0) {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = ShotWashingMachine;
        p->x = enemy.x;
        p->y = enemy.y + 32.0;
        p->muki = atan2(player.y - p->y, player.x - p->x) + (GetRand(50) - 25) / 180.0 * DX_PI;

        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }

    // 強力すすぎ（大量水しぶき）
    if (count % 165 == 110) {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = ShotStrongRinse;
        p->x = enemy.x;
        p->y = enemy.y + 35.0;
        p->muki = 0.0; // 使用しない

        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }
}