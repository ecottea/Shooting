// enemyPat_igo.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：囲碁モチーフ（石を置くような弾幕）
// 5x5の格子状に弾を配置し、中心から外に広がる
static void ShotGoGrid(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 5x5の格子に弾を配置
        for (int i = -2; i <= 2; i++) {
            for (int j = -2; j <= 2; j++) {
                pEnemyShot = new sEnemyShot;
                // 中心からのオフセット
                pEnemyShot->x = pEnemyShotSet->x + i * 20.0;
                pEnemyShot->y = pEnemyShotSet->y + j * 20.0;
                // 中心に向かう方向を計算
                double angle = atan2(j, i);
                if (i == 0 && j == 0) angle = (double)GetRand(360) / 180.0 * DX_PI; // 中心はランダム方向
                pEnemyShot->muki = angle;
                // 中心に近いほど速い
                pEnemyShot->speed = 1.0 + (2.0 - (abs(i) + abs(j)) * 0.2);

                // 囲碁の石をイメージした色（黒と白を交互に配置）
                int color = ((i + j) % 2 == 0) ? 7 : 6; // 黒 or 白
                // 中心は大玉、周囲は小玉・中玉
                if (i == 0 && j == 0) {
                    pEnemyShot->kind = img_enemyShotLargeBall[color];
                }
                else if (abs(i) + abs(j) <= 1) {
                    pEnemyShot->kind = img_enemyShotMediumBall[color];
                }
                else {
                    pEnemyShot->kind = img_enemyShotSmallBall[color];
                }

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

// 弾幕：囲碁モチーフ（石が連鎖するような弾幕）
// 8方向に弾を発射し、時間経過で方向を変化させる
static void ShotGoChain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 8方向に弾を配置
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 3; j++) {
                pEnemyShot = new sEnemyShot;
                double angle = (i * DX_PI / 4.0) + (j * 0.1); // 45度ごとの方向
                pEnemyShot->x = pEnemyShotSet->x + cos(angle) * (j * 15.0);
                pEnemyShot->y = pEnemyShotSet->y + sin(angle) * (j * 15.0);
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 2.5 - (j * 0.5); // 内側ほど速い

                // 囲碁の石をイメージした色（黒と白を交互に配置）
                int color = (i % 2 == 0) ? 7 : 6; // 黒 or 白
                pEnemyShot->kind = img_enemyShotScale[color];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の移動と方向変更
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 120フレームごとに方向を少し変化させる
        if (pEnemyShotSet->count % 120 == 0) {
            pEnemyShot->muki += (GetRand(60) - 30) / 180.0 * DX_PI;
        }
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 弾幕：囲碁モチーフ（プレイヤーを追尾する石のような弾）
// 5方向に追尾弾を発射
static void ShotGoHoming(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->speed = 1.5 + (i * 0.2);

            // 囲碁の石をイメージした色（黒と白を交互に配置）
            int color = (i % 2 == 0) ? 7 : 6; // 黒 or 白
            pEnemyShot->kind = img_enemyShotDiamond[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動（プレイヤー追尾）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 10フレームごとに方向を更新
        if (pEnemyShotSet->count % 10 == 0) {
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        }
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Go_Vibe()
{
    static int muki = 1;
    static int phase = 0; // フェーズ管理

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 300;
        muki = 1;
        phase = 0;
    }
    else {
        // 敵の移動（左右に往復）
        enemy.x += 0.5 * (double)muki;
        if (enemy.x < 80.0 || enemy.x > 400.0) muki *= -1;

        // 画面上部に留まる
        enemy.y = 40.0 + sin(count / 20.0) * 20.0;
    }

    // フェーズごとに弾幕を切り替え
    if (count % 300 == 0) {
        phase = (phase + 1) % 3;
    }

    // フェーズ0: 格子弾幕
    if (phase == 0 && count % 15 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGoGrid;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // フェーズ1: 連鎖弾幕
    if (phase == 1 && count % 20 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGoChain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // フェーズ2: 追尾弾幕
    if (phase == 2 && count % 80 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGoHoming;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}