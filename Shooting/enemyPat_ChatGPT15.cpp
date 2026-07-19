// enemyPat_WashingMachine.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 洗濯槽の中を回る水流弾
// ============================================================
static void ShotWashingWater(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int NUM = 24;

        for (int i = 0; i < NUM; i++) {
            pEnemyShot = new sEnemyShot;

            double ang = DX_PI * 2.0 * i / NUM;

            pEnemyShot->x = pEnemyShotSet->x + cos(ang) * 18.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(ang) * 18.0;

            pEnemyShot->muki = ang;
            pEnemyShot->speed = 0.0;

            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    double rot = pEnemyShotSet->count * 0.08;

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 洗濯槽内を回転
        double baseAng = pEnemyShot->muki + rot * 0.25;

        // 回転しながら徐々に外側へ
        double r = 18.0 + pEnemyShotSet->count * 0.45;

        pEnemyShot->x = pEnemyShotSet->x + cos(baseAng) * r;
        pEnemyShot->y = pEnemyShotSet->y + sin(baseAng) * r;

        pEnemyShot = pEnemyShot->next;
    }

    // 脱水開始
    if (pEnemyShotSet->count == 90) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

            double dx = pEnemyShot->x - pEnemyShotSet->x;
            double dy = pEnemyShot->y - pEnemyShotSet->y;

            pEnemyShot->muki = atan2(dy, dx);
            pEnemyShot->speed = 4.5;

            pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青

            pEnemyShot = pEnemyShot->next;
        }
    }

    // 脱水後は直進
    if (pEnemyShotSet->count > 90) {
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;

            pEnemyShot = pEnemyShot->next;
        }
    }
}

// ============================================================
// 敵本体
// 洗濯機モチーフ：
// ・左右移動
// ・洗濯槽のように弾を回転
// ・一定時間後に脱水して全方向へ射出
// ============================================================
void EnemyPat_WashingMachine_ChatGPT()
{
    static int moveDir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
    }
    else {
        enemy.x += moveDir * 1.2;

        if (enemy.x > 380.0) moveDir = -1;
        if (enemy.x < 100.0) moveDir = 1;
    }

    // 洗濯サイクル
    if (count % 120 == 0) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWashingWater;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}