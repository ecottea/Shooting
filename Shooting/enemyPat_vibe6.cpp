// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：波と粒の境界（波動と粒子の境界を表現）
static void ShotWaveAndParticle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 波動部分（波のような連続的な弾幕）
        for (int i = 0; i < 15; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 波動的な角度分散
            pEnemyShot->muki = pEnemyShotSet->muki + (i * 0.15 - 1.05); // -1.05 ~ 1.35 radian
            pEnemyShot->speed = (180 + GetRand(80)) / 100.0; // 1.8 ~ 2.6

            // 波動的な弾は「鱗弾」で統一（波のイメージ）
            int color = GetRand(7); // 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒
            pEnemyShot->kind = img_enemyShotScale[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 粒子部分（粒のような個別の弾）
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(60) - 30;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(60) - 30;
            // 粒子はプレイヤー方向に直進
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = (250 + GetRand(100)) / 100.0; // 2.5 ~ 3.5

            // 粒子的な弾は「小玉」で統一（粒のイメージ）
            int color = GetRand(7);
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 波動弾の軌道に波を加える
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 波動弾（鱗弾）は波打つように移動
        if (pEnemyShot->kind >= img_enemyShotScale[0] && pEnemyShot->kind <= img_enemyShotScale[6]) {
            double waveOffset = 10.0 * sin(pEnemyShotSet->count * 0.3 + pEnemyShot->x * 0.05);
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki) + waveOffset * 0.1;
        }
        // 粒子弾（小玉）は直進
        else {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン：波と粒の境界
void EnemyPat_Namitsubu_Vibe()
{
    static double angle = 0.0;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        angle = 0.0;
    }
    else {
        // 波状に移動（波の境界を表現）
        enemy.x = 240.0 + 120.0 * sin(angle);
        angle += 0.03;
    }

    // 80フレームごとに波と粒の境界弾幕を発射
    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWaveAndParticle;
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