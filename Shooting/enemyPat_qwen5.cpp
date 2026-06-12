// enemyPat_waterfall.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --------------------------------------------------------
// パターン2: 画面左右から交差して降り注ぐ「交差滝」
// --------------------------------------------------------
static void ShotCrossWaterfall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count % 3 == 0) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i - 1) * 12.0;
            pEnemyShot->y = pEnemyShotSet->y;

            // 設定された角度(muki)に向けて発射
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(10) - 5) / 100.0 + sin(pEnemyShotSet->count * 0.02) * 1.3;
            pEnemyShot->speed = 3.0;

            int color = 4 + GetRand(2); // 青?マゼンタ
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

void EnemyPat_Waterfall_Qwen()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        enemy.y += sin(count * 0.01) * 0.5; // 上下にゆらゆら
    }

    if (count == 60) {
        // 左からの滝 (右下がり)
        sEnemyShotSet* pLeft = new sEnemyShotSet;
        pLeft->count = 0;
        pLeft->patternFunc = ShotCrossWaterfall;
        pLeft->x = 20.0;
        pLeft->y = 20.0;
        pLeft->muki = DX_PI * 70.0 / 180.0; // 70度
        pLeft->pEnemyShotHead = new sEnemyShot;
        pLeft->pEnemyShotHead->prev = pLeft->pEnemyShotHead;
        pLeft->pEnemyShotHead->next = pLeft->pEnemyShotHead;
        pLeft->prev = enemyShotSetHead.prev;
        pLeft->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pLeft;
        enemyShotSetHead.prev = pLeft;

        // 右からの滝 (左下がり)
        sEnemyShotSet* pRight = new sEnemyShotSet;
        pRight->count = 0;
        pRight->patternFunc = ShotCrossWaterfall;
        pRight->x = 460.0;
        pRight->y = 20.0;
        pRight->muki = DX_PI * 110.0 / 180.0; // 110度
        pRight->pEnemyShotHead = new sEnemyShot;
        pRight->pEnemyShotHead->prev = pRight->pEnemyShotHead;
        pRight->pEnemyShotHead->next = pRight->pEnemyShotHead;
        pRight->prev = enemyShotSetHead.prev;
        pRight->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRight;
        enemyShotSetHead.prev = pRight;
    }
}

