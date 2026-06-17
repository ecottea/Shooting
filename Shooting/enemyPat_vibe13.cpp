// enemyPat_leafCutter.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：回転するはっぱカッター（3枚の刃から弾を連射）
static void ShotLeafCutter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 3枚の刃の角度を計算（120度間隔で回転）
    double baseAngle = (pEnemyShotSet->count * 5.0) * DX_PI / 180.0; // 5度/フレームで回転

    // 3枚の刃それぞれから弾を発射
    for (int blade = 0; blade < 3; blade++) {
        double bladeAngle = baseAngle + (blade * 120.0 * DX_PI / 180.0);

        // 1枚の刃から3発を扇状に発射
        for (int i = 0; i < 3; i++) {
            if (pEnemyShotSet->count % 12 == 0) { // 5フレームごとに1発ずつ
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + 30.0 * cos(bladeAngle);
                pEnemyShot->y = pEnemyShotSet->y + 30.0 * sin(bladeAngle);
                // 刃の方向に対して±15度のばらつき
                pEnemyShot->muki = bladeAngle + (GetRand(30) - 15) / 180.0 * DX_PI;
                pEnemyShot->speed = 2.5 + GetRand(10) / 10.0; // 2.5～3.5

                // 葉っぱ型弾（鱗弾）を緑色で設定
                pEnemyShot->kind = img_enemyShotScale[2]; // 2:緑

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
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
void EnemyPat_RazorLeaf_Vibe()
{
    static int moveDirection = 1;
    static int bladeAngle = 0; // 刃の回転角度（度数）

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        moveDirection = 1;
        bladeAngle = 0;
    }
    else {
        // 敵を上下に移動（波動運動）
        enemy.y += 0.8 * sin(count * 0.05);

        // 画面端で移動方向を反転
        enemy.x += 1.2 * moveDirection;
        if (enemy.x < 80.0 || enemy.x > 400.0) {
            moveDirection *= -1;
        }
    }

    // 40フレームごとに弾幕セットを作成
    if (count % 180 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLeafCutter;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0; // 基本角度は関数内で制御

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}