#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// サボテンをモチーフにした弾幕パターン
static void ShotCactus(sEnemyShotSet* pEnemyShotSet)
{
    // フェーズ1: 初期配置（茎と花の生成）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 茎の本体（縦に3つ、緑の中玉）
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y + i * 16.0;
            pShot->muki = 0.0;
            pShot->speed = 0.0; // 動かない（地形として機能）
            pShot->kind = img_enemyShotMediumBall[2]; // 2:緑
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        // 茎の腕（左右、緑の中玉）
        sEnemyShot* pShotL = new sEnemyShot;
        pShotL->x = pEnemyShotSet->x - 16.0;
        pShotL->y = pEnemyShotSet->y + 16.0;
        pShotL->muki = 0.0;
        pShotL->speed = 0.0;
        pShotL->kind = img_enemyShotMediumBall[2];
        pShotL->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShotL->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShotL;
        pEnemyShotSet->pEnemyShotHead->prev = pShotL;

        sEnemyShot* pShotR = new sEnemyShot;
        pShotR->x = pEnemyShotSet->x + 16.0;
        pShotR->y = pEnemyShotSet->y + 16.0;
        pShotR->muki = 0.0;
        pShotR->speed = 0.0;
        pShotR->kind = img_enemyShotMediumBall[2];
        pShotR->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShotR->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShotR;
        pEnemyShotSet->pEnemyShotHead->prev = pShotR;

        // 花（赤の大玉）
        sEnemyShot* pFlower = new sEnemyShot;
        pFlower->x = pEnemyShotSet->x;
        pFlower->y = pEnemyShotSet->y - 16.0;
        pFlower->muki = 0.0;
        pFlower->speed = 0.0;
        pFlower->kind = img_enemyShotLargeBall[0]; // 0:赤
        pFlower->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pFlower->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pFlower;
        pEnemyShotSet->pEnemyShotHead->prev = pFlower;
    }

    // フェーズ2: トゲの発射（30フレームごと、計5回）
    if ((pEnemyShotSet->count > 0 && pEnemyShotSet->count % 30 == 0 && pEnemyShotSet->count <= 150)
        || (pEnemyShotSet->count > 240 && pEnemyShotSet->count % 30 == 0 && pEnemyShotSet->count <= 400)) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numThorns = 16*2;
        for (int i = 0; i < numThorns; i++) {
            sEnemyShot* pThorn = new sEnemyShot;
            pThorn->x = pEnemyShotSet->x;
            pThorn->y = pEnemyShotSet->y + 16.0; // 茎の中心から発射
            double angle = (DX_PI * 2.0 / numThorns) * i + (pEnemyShotSet->count * 0.05);
            pThorn->muki = angle;
            pThorn->speed = 3.5;
            pThorn->kind = img_enemyShotLaser[6]; // 6:白の短レーザー
            pThorn->margin = 80;
            pThorn->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pThorn->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pThorn;
            pEnemyShotSet->pEnemyShotHead->prev = pThorn;
        }
    }

    // フェーズ3: 満開の散弾（count == 180）
    if (pEnemyShotSet->count == 180) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 40*3; i++) {
            sEnemyShot* pPetal = new sEnemyShot;
            pPetal->x = pEnemyShotSet->x;
            pPetal->y = pEnemyShotSet->y - 16.0; // 花の位置から発射
            double angle = (DX_PI * 2.0 / 40/3) * i;
            pPetal->muki = angle;
            // GetRand(150) は 0〜150 を返すため、/100.0 で 0.0〜1.5 になる
            pPetal->speed = 2.0 + GetRand(150) / 100.0; // 2.0 〜 3.5
            pPetal->kind = (i % 2 == 0) ? img_enemyShotSmallBall[5] : img_enemyShotSmallBall[6]; // 5:マゼンタ, 6:白
            pPetal->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pPetal->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pPetal;
            pEnemyShotSet->pEnemyShotHead->prev = pPetal;
        }
    }

    // フェーズ4: 成長（茎の延伸）（count == 240）
    if (pEnemyShotSet->count == 240) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 新しい茎の頂点（緑の中玉）
        sEnemyShot* pNewTop = new sEnemyShot;
        pNewTop->x = pEnemyShotSet->x;
        pNewTop->y = pEnemyShotSet->y - 32.0;
        pNewTop->muki = 0.0;
        pNewTop->speed = 0.0;
        pNewTop->kind = img_enemyShotMediumBall[2];
        pNewTop->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pNewTop->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pNewTop;
        pEnemyShotSet->pEnemyShotHead->prev = pNewTop;

        // 新しい花（赤の大玉）
        sEnemyShot* pNewFlower = new sEnemyShot;
        pNewFlower->x = pEnemyShotSet->x;
        pNewFlower->y = pEnemyShotSet->y - 48.0;
        pNewFlower->muki = 0.0;
        pNewFlower->speed = 0.0;
        pNewFlower->kind = img_enemyShotLargeBall[0];
        pNewFlower->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pNewFlower->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pNewFlower;
        pEnemyShotSet->pEnemyShotHead->prev = pNewFlower;
    }

    // フェーズ5: 成長後のトゲ発射（count > 240 かつ 30フレームごと）
    if (pEnemyShotSet->count > 240 && pEnemyShotSet->count % 30 == 0 && pEnemyShotSet->count <= 400) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numThorns = 20*2;
        for (int i = 0; i < numThorns; i++) {
            sEnemyShot* pThorn = new sEnemyShot;
            pThorn->x = pEnemyShotSet->x;
            pThorn->y = pEnemyShotSet->y - 32.0; // 新しい頂点から発射
            double angle = (DX_PI * 2.0 / numThorns) * i - (pEnemyShotSet->count * 0.05);
            pThorn->muki = angle;
            pThorn->speed = 4.0;
            pThorn->margin = 80;
            pThorn->kind = img_enemyShotLaser[6]; // 6:白の短レーザー
            pThorn->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pThorn->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pThorn;
            pEnemyShotSet->pEnemyShotHead->prev = pThorn;
        }
    }

    {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            if (count % 450 == 450 - 1 && pShot->kind != img_enemyShotLaser[6]) pShot->margin = -9999;
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Cactus_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0; // 上に伸びる余地を持たせる
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    // サボテンの弾幕セットは1回だけ生成し、敵の位置に固定配置する
    if (count % 450 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCactus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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
}