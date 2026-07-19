// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：破壊光線
static void ShotDestructionBeam(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 充填フェーズ（0-40フレーム）
    if (pEnemyShotSet->count < 40) {
        if (pEnemyShotSet->count % 2 == 0) {
            for (int i = 0; i < 4; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                double angle = pEnemyShotSet->muki + (GetRand(40) - 20) / 180.0 * DX_PI;
                double distance = (pEnemyShotSet->count / 2.0) * 5;
                pEnemyShot->x = pEnemyShotSet->x + cos(angle) * distance;
                pEnemyShot->y = pEnemyShotSet->y + sin(angle) * distance;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 0.0;
                int color = pEnemyShotSet->count / 7;
                if (color > 6) color = 6;
                pEnemyShot->kind = img_enemyShotSmallBall[color];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    // 発射フェーズ（40フレーム）
    else if (pEnemyShotSet->count == 40) {
        sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 7.0 + GetRand(30) / 10.0;
            pEnemyShot = pEnemyShot->next;
        }
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
    // 持続フェーズ（41-150フレーム）
    else if (pEnemyShotSet->count > 40 && pEnemyShotSet->count < 150) {
        if (pEnemyShotSet->count % 4 == 0) {
            for (int i = 0; i < 21; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                double angle = GetRand(359) / 180.0 * DX_PI;
                pEnemyShot->x = enemy.x + GetRand(20) - 10;
                pEnemyShot->y = enemy.y + 20.0 + GetRand(10) - 5;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 5.0 + GetRand(30) / 10.0;
                pEnemyShot->kind = img_enemyShotSmallBall[GetRand(7)];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の移動
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Hakaikousen_Vibe()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (enemy.x < 50) muki = 1;
        else if (enemy.x > 430) muki = -1;
    }

    if (count % 200 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDestructionBeam;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}