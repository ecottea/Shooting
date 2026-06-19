// enemyPat_washingMachine.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 洗濯機をモチーフにした弾幕：洗濯サイクル（洗い・すすぎ・脱水の3フェーズ）
static void ShotWashingMachineCycle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // フェーズに応じた効果音
        if (pEnemyShotSet->kind == 0) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        if (pEnemyShotSet->kind == 1) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        if (pEnemyShotSet->kind == 2) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        switch (pEnemyShotSet->kind) {
            // --- フェーズ0: 洗い（同心円状に青色の大玉を発射）---
        case 0: {
            int numRings = 3;
            for (int r = 0; r < numRings; r++) {
                int numBullets = 12 + r * 4;
                double radius = 20.0 + r * 25.0;
                double angleStep = DX_PI * 2 / numBullets;

                for (int i = 0; i < numBullets; i++) {
                    sEnemyShot* pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pEnemyShotSet->x + cos(pEnemyShotSet->muki + i * angleStep) * radius;
                    pEnemyShot->y = pEnemyShotSet->y + sin(pEnemyShotSet->muki + i * angleStep) * radius;
                    pEnemyShot->muki = pEnemyShotSet->muki + i * angleStep + DX_PI / 2;
                    pEnemyShot->speed = 1.0 + r * 0.5;
                    pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青色の大玉

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
            break;
        }

              // --- フェーズ1: すすぎ（ランダムな方向に白色の小玉を発射）---
        case 1: {
            int numBubbles = 120;
            for (int i = 0; i < numBubbles; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
                pEnemyShot->y = pEnemyShotSet->y + GetRand(40) - 20;
                pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
                pEnemyShot->speed = 1.2 + GetRand(80) / 100.0;
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白色の小玉

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
            break;
        }

              // --- フェーズ2: 脱水（高速回転するシアン色の菱形弾を発射）---
        case 2: {
            int numBullets = 100;
            double angleStep = DX_PI * 2 / numBullets;

            for (int i = 0; i < numBullets; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = pEnemyShotSet->muki + i * angleStep;
                pEnemyShot->speed = 3.5 + sin(pEnemyShotSet->count * 0.2) * 0.5;
                pEnemyShot->kind = img_enemyShotDiamond[3]; // シアン色の菱形弾

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
            break;
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
void EnemyPat_WashingMachine_Vibe()
{
    static int moveDirection = 1;
    static double rotationAngle = 0.0;
    static int phase = 0;
    static int phaseTimer = 0;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 300;
        moveDirection = 1;
        rotationAngle = 0.0;
        phase = 0;
        phaseTimer = 0;
    }
    else {
        // 敵の動き：サイン波を加えた水平移動
        enemy.x += 1.0 * moveDirection * (0.7 + 0.3 * sin(count * 0.05));
        if (enemy.x < 40 || enemy.x > 440) moveDirection *= -1;

        // 回転角度を徐々に増加
        rotationAngle += DX_PI / 120;

        // フェーズ切り替え（300フレームごと）
        phaseTimer++;
        if (phaseTimer >= 300) {
            phase = (phase + 1) % 3;
            phaseTimer = 0;
        }
    }

    // 40フレームごとに弾幕を発射
    if (count % 40 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWashingMachineCycle;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = rotationAngle;
        pEnemyShotSet->kind = phase; // 現在のフェーズを渡す

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}