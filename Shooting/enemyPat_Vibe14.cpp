// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：毒の連鎖分裂弾（大玉→中玉→小玉の3段階分裂）
static void ShotPoisonChainSplit(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 大玉を1発発射
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 0.8;
        pEnemyShot->kind = img_enemyShotLargeBall[2]; // 緑の大玉

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
    else if (pEnemyShotSet->count == 20) {
        // 20フレーム後に大玉が6方向に中玉へ分裂
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* next = pEnemyShot->next;

            if (pEnemyShot->kind == img_enemyShotLargeBall[2]) {
                const int N = 12;
                for (int i = 0; i < N; i++) {
                    sEnemyShot* newShot = new sEnemyShot;
                    newShot->x = pEnemyShot->x;
                    newShot->y = pEnemyShot->y;
                    newShot->muki = pEnemyShot->muki + (i * 360.0 / N) / 180.0 * DX_PI;
                    newShot->speed = 1.8 + GetRand(10) / 10.0;
                    newShot->kind = img_enemyShotMediumBall[2]; // 緑の中玉

                    newShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    newShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = newShot;
                    pEnemyShotSet->pEnemyShotHead->prev = newShot;
                }
                // 大玉をリンクから解除
                pEnemyShot->x = 9999;
            }

            pEnemyShot = next;
        }
    }
    else if (pEnemyShotSet->count == 40) {
        // 40フレーム後に中玉が4方向に小玉へ分裂
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* next = pEnemyShot->next;

            if (pEnemyShot->kind == img_enemyShotMediumBall[2]) {
                const int N = 9;
                for (int i = 0; i < N; i++) {
                    sEnemyShot* newShot = new sEnemyShot;
                    newShot->x = pEnemyShot->x;
                    newShot->y = pEnemyShot->y;
                    newShot->muki = pEnemyShot->muki + (i * 360.0 / N) / 180.0 * DX_PI + (GetRand(20) - 10) / 180.0 * DX_PI;
                    newShot->speed = 2.8 + GetRand(15) / 10.0;
                    newShot->kind = img_enemyShotSmallBall[2]; // 緑の小玉

                    newShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    newShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = newShot;
                    pEnemyShotSet->pEnemyShotHead->prev = newShot;
                }
                // 中玉をリンクから解除
                pEnemyShot->x = 9999;
            }
            pEnemyShot = next;
        }
    }

    // 弾の移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Poison_Vibe()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵を8の字を描くように移動（速度と振幅を強化）
        enemy.x = 240.0 + 180.0 * sin(count * 0.015);
        enemy.y = 120.0 + 100.0 * sin(count * 0.03);
    }

    // 60フレームごとに弾幕を発射
    if (count % 60 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPoisonChainSplit;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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