// enemyPat_washingMachine.cpp
// 洗濯機モチーフの弾幕パターン例

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 洗濯機の「回転水流」をイメージした弾幕
static void ShotWashingMachine(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみセットアップ（pEnemyShotSet->count == 0 のとき）
    if (pEnemyShotSet->count == 0) {
        // 効果音（軽めの弾発射音）
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 洗濯機の「内側の渦」と「外側の円」をイメージして弾を配置
        const int innerCount = 8;   // 内側の弾数
        const int outerCount = 16;  // 外側の弾数

        // 内側の渦（小玉・シアン系）
        for (int i = 0; i < innerCount; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 等間隔の角度（渦の向きは外側へ）
            double baseAngle = (i * DX_PI * 2.0 / innerCount) + (pEnemyShotSet->count * DX_PI / 180.0);
            pEnemyShot->muki = baseAngle;

            // 内側はやや速めに外へ
            pEnemyShot->speed = 2.0 + (i % 2) * 0.5;

            // 種類と色：小玉 or 中玉、シアン系（水色っぽい）
            int type = (i % 2 == 0) ? 0 : 1; // 小玉 or 中玉
            int color = 3; // シアン

            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                break;
            }

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 外側の円（鱗弾・青系）
        for (int i = 0; i < outerCount; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 等間隔の角度（外側の円）
            double baseAngle = i * DX_PI * 2.0 / outerCount;
            pEnemyShot->muki = baseAngle;

            // 外側はゆっくり外へ
            pEnemyShot->speed = 1.0 + (i % 3) * 0.3;

            // 種類と色：鱗弾、青系
            int type = 4; // 鱗弾
            int color = 4; // 青

            switch (type) {
            case 4:
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            }

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 渦の回転イメージで少しずつ角度をずらす
        pEnemyShot->muki += DX_PI / 360.0;

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（指定どおりの関数名）
void EnemyPat_WashingMachine_Sakana()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で弾幕セットを生成
    if (count % 30 == 0) { // 30フレームごとに発射
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWashingMachine;
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