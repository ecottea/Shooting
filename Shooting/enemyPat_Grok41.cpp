// enemyPat_BreakoutDanmaku.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// ブロック崩し風弾幕用ヘルパー関数群
// =============================================

// ブロック（密集小弾）生成
static void ShotBlock(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int blockWidth = 6;   // ブロック横幅（弾の個数）
        int blockHeight = 3;  // ブロック縦幅（弾の個数）
        double baseX = pEnemyShotSet->x - blockWidth * 12.0 / 2.0;
        double baseY = pEnemyShotSet->y;

        for (int y = 0; y < blockHeight; y++) {
            for (int x = 0; x < blockWidth; x++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = baseX + x * 12.0 + GetRand(6) - 3;
                pEnemyShot->y = baseY + y * 10.0 + GetRand(4) - 2;
                pEnemyShot->muki = DX_PI / 2.0;           // 下方向
                pEnemyShot->speed = 0.8 + (GetRand(40) / 100.0); // 超低速降下
                pEnemyShot->kind = img_enemyShotSmallBall[GetRand(7)]; // 小玉（色はランダム寄り）
                pEnemyShot->param_i[0] = 1; // ブロック弾フラグ

                // 双方向リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 移動（低速降下）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->y += pShot->speed;
        pShot = pShot->next;
    }
}

// 跳ね返るボール弾生成＆挙動
static void ShotBouncingBall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1セットにつき1〜2個のボール弾を投入
        int numBalls = (GetRand(1) == 0) ? 1 : 2;
        for (int i = 0; i < numBalls; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(80) - 40;
            pEnemyShot->y = pEnemyShotSet->y;
            // 斜め下方向に発射（-60°〜+60°程度）
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI * 0.7;
            pEnemyShot->speed = 3.2 + GetRand(80) / 100.0;
            pEnemyShot->kind = img_enemyShotLargeBall[GetRand(3)]; // 大玉で目立つボール
            pEnemyShot->param_i[0] = 2; // ボール弾フラグ
            pEnemyShot->param_d[0] = pEnemyShot->muki; // 元の角度保存（必要なら）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ボール挙動：左右壁で反射 + 下方向に自然落下感
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 左右壁反射
        if (pShot->x < 20.0 || pShot->x > 460.0) {
            pShot->muki = DX_PI - pShot->muki;
            pShot->x = (pShot->x < 20.0) ? 20.0 : 460.0;
        }

        // 下方向加速（ブロック崩しらしい落下）
        pShot->speed += 0.008;
        if (pShot->speed > 6.5) pShot->speed = 6.5;

        pShot = pShot->next;
    }
}

// =============================================
// 敵本体パターン
// =============================================
void EnemyPat_BlockBreak_Grok()
{
    static int muki = 1;
    static int phase = 0;
    static int shotTimer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
        shotTimer = 0;
    }

    // 敵本体：左右往復移動（ブロック崩しらしい安定した位置）
    enemy.x += 1.1 * (double)muki;
    if (enemy.x < 80.0 || enemy.x > 400.0) muki *= -1;

    // 定期的にブロック＋ボール弾を生成
    shotTimer++;

    if (shotTimer % 45 == 10) {
        // ブロック群生成
        sEnemyShotSet* pSetBlock = new sEnemyShotSet;
        pSetBlock->count = 0;
        pSetBlock->patternFunc = ShotBlock;
        pSetBlock->x = 80.0 + GetRand(320);   // 横方向にランダム配置
        pSetBlock->y = 40.0 + GetRand(60) - 30;
        pSetBlock->muki = 0.0;
        pSetBlock->kind = 0;
        pSetBlock->pEnemyShotHead = new sEnemyShot;
        pSetBlock->pEnemyShotHead->prev = pSetBlock->pEnemyShotHead;
        pSetBlock->pEnemyShotHead->next = pSetBlock->pEnemyShotHead;

        pSetBlock->prev = enemyShotSetHead.prev;
        pSetBlock->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetBlock;
        enemyShotSetHead.prev = pSetBlock;
    }

    if (shotTimer % 65 == 30) {
        // 跳ね返るボール弾投入
        sEnemyShotSet* pSetBall = new sEnemyShotSet;
        pSetBall->count = 0;
        pSetBall->patternFunc = ShotBouncingBall;
        pSetBall->x = enemy.x;
        pSetBall->y = enemy.y + 20.0;
        pSetBall->muki = atan2(player.y - pSetBall->y, player.x - pSetBall->x);
        pSetBall->kind = 1;
        pSetBall->pEnemyShotHead = new sEnemyShot;
        pSetBall->pEnemyShotHead->prev = pSetBall->pEnemyShotHead;
        pSetBall->pEnemyShotHead->next = pSetBall->pEnemyShotHead;

        pSetBall->prev = enemyShotSetHead.prev;
        pSetBall->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetBall;
        enemyShotSetHead.prev = pSetBall;
    }

    // 後半強化（HP半分以下でボール頻度アップ）
    if (enemy.hp < 100 && shotTimer % 65 == 45) {
        sEnemyShotSet* pSetBall2 = new sEnemyShotSet;
        pSetBall2->count = 0;
        pSetBall2->patternFunc = ShotBouncingBall;
        pSetBall2->x = 60.0 + GetRand(360);
        pSetBall2->y = 30.0;
        pSetBall2->kind = 2;
        pSetBall2->pEnemyShotHead = new sEnemyShot;
        pSetBall2->pEnemyShotHead->prev = pSetBall2->pEnemyShotHead;
        pSetBall2->pEnemyShotHead->next = pSetBall2->pEnemyShotHead;

        pSetBall2->prev = enemyShotSetHead.prev;
        pSetBall2->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetBall2;
        enemyShotSetHead.prev = pSetBall2;
    }
}