#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ----------------------------------------------------------------
// 弾幕処理：モアレの檻（Grid of Moiré）
// ----------------------------------------------------------------
static void ShotMoire(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 弾セット生成時の初期化処理（count のインクリメントはメインルーチンが行う仕様）
    if (pEnemyShotSet->count == 0) {
        int shotType = pEnemyShotSet->param_i[0]; // 0: A弾, 1: B弾, 2: C弾

        if (shotType == 0) {
            // 第1層：A弾（時計回り超低速渦）
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            int ways = 24;
            double baseAngle = pEnemyShotSet->muki;
            double speed = 0.3; // 1px/frame以下の超低速 (0.3px/f)

            for (int i = 0; i < ways; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAngle + (2.0 * DX_PI / ways) * i;
                pEnemyShot->speed = speed;
                pEnemyShot->kind = img_enemyShotSmallBall[3]; // 小玉（シアン）

                // リストへの追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        else if (shotType == 1) {
            // 第2層：B弾（反時計回り超低速渦）
            int ways = 16;
            double baseAngle = pEnemyShotSet->muki;
            double speed = 0.5; // 1px/frame以下の超低速 (0.5px/f)

            for (int i = 0; i < ways; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAngle + (2.0 * DX_PI / ways) * i;
                pEnemyShot->speed = speed;
                pEnemyShot->kind = img_enemyShotSmallBall[0]; // 小玉（赤）

                // リストへの追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        else if (shotType == 2) {
            // アクセント：C弾（自機狙い緩やか扇弾）
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            int ways = 3;
            double baseAngle = pEnemyShotSet->muki;
            double speed = 0.8; // 1px/frame以下の低速 (0.8px/f)
            double angleStep = 10.0 * DX_PI / 180.0; // 10度間隔

            for (int i = 0; i < ways; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAngle + (i - 1) * angleStep; // -10度, 0度, +10度
                pEnemyShot->speed = speed;
                pEnemyShot->kind = img_enemyShotMediumBall[1]; // 中玉（黄）

                // リストへの追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 各弾の移動処理（毎フレーム更新）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ----------------------------------------------------------------
// 弾セット追加用ヘルパー関数
// ----------------------------------------------------------------
static void AddMoireShotSet(double x, double y, double muki, int shotType)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotMoire;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->param_i[0] = shotType;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ----------------------------------------------------------------
// 敵本体のパターン関数
// ----------------------------------------------------------------
void EnemyPat_SlowBullet_Gemini()
{
    static int muki;
    static double angleA;
    static double angleB;

    if (count == 1) {
        // 初期配置（画面サイズ: 480x480想定）
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 300; // HPは200固定
        muki = 1;
        angleA = 0.0;
        angleB = 0.0;
    }
    else {
        // 干渉模様の形状を崩さないため、極めてゆっくりと左右移動する
        enemy.x += 0.3 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }

    // 60フレーム（1秒）ごとに A弾 と B弾 を発射して模様を合成
    if (count % 15 == 1) {
        // 第1層：A弾（24WAY・時計回りオフセット）
        AddMoireShotSet(enemy.x, enemy.y + 10.0, angleA, 0);
        angleA += 3.5 * DX_PI / 180.0; // 時計回りに+3.5度回転

        // 第2層：B弾（16WAY・反時計回りオフセット）
        AddMoireShotSet(enemy.x, enemy.y + 10.0, angleB, 1);
        angleB -= 5.0 * DX_PI / 180.0; // 反時計回りに-5.0度回転
    }

    // 180フレーム（3秒）ごとに C弾（3WAY・自機狙い）を発射
    if (count % 180 == 1) {
        double angleToPlayer = atan2(player.y - (enemy.y + 10.0), player.x - enemy.x);
        AddMoireShotSet(enemy.x, enemy.y + 10.0, angleToPlayer, 2);
    }
}