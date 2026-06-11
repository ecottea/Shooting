// enemyPat_sample1.cpp
// パターン1：双竜蓮華（回転渦巻き＆交差曲線弾）
#include "DxLib.h"
#include "gv.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

// 弾幕：双竜蓮華の弾制御
static void ShotLotus(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 弾セット生成時のみの処理（発射）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int ways = 6; // 6方向の螺旋
        double angleInterval = (2.0 * PI) / ways;

        // 1. 外回り（シアン中玉・右回転）
        for (int i = 0; i < ways; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * angleInterval;
            pEnemyShot->speed = 2.5; // 少し速め
            pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン(3)

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 2. 内回り（マゼンタ小玉・左回転）
        for (int i = 0; i < ways; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 逆回転にするため、基準角(muki)をマイナスにする
            pEnemyShot->muki = -pEnemyShotSet->muki + i * angleInterval;
            pEnemyShot->speed = 1.5; // 少し遅め
            pEnemyShot->kind = img_enemyShotSmallBall[5]; // マゼンタ(4)

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの弾の移動・軌道変化処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        
        // 発射後60フレームの間だけ軌道を曲げて花びらの形を作る
        if (pEnemyShotSet->count < 60) {
            if (pEnemyShot->kind == img_enemyShotMediumBall[3]) {
                pEnemyShot->muki += 0.015; // シアンは右へカーブ
            } else if (pEnemyShot->kind == img_enemyShotSmallBall[4]) {
                pEnemyShot->muki -= 0.015; // マゼンタは左へカーブ
            }
        }

        // 角度と速度からXY座標を更新
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }

    // カウントアップ (軌道変化ロジックの判定用)
    pEnemyShotSet->count++;
}

// 敵本体の行動パターン
void Pattern1_Lotus()
{
    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0; // 画面中央を想定
        enemy.y = 120.0; // 少し上の方
        enemy.maxHp = 100;
        enemy.hp = enemy.maxHp;
    }
    else {
        // 敵の移動：リサージュ図形による滑らかな8の字移動
        enemy.x = 240.0 + 120.0 * sin(count * PI / 120.0);
        enemy.y = 120.0 + 30.0 * sin(count * PI / 60.0);
    }

    // 6フレーム間隔で継続的に弾セットを生成
    if (count % 6 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLotus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // 時間経過でベース角度を回転させる（これが螺旋模様になる理由）
        pEnemyShotSet->muki = (count % 360) * (PI / 180.0) * 1.5;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}