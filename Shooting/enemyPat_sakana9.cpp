// enemyPat_blizzard.cpp
// 吹雪モチーフの弾幕パターン（サンプルとは大きく異なる設計）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 風の影響を表すグローバル変数（時間で変化）
static double windAngle = DX_PI / 2.0; // 初期は真下方向
static int windChangeTimer = 0;

// 吹雪パターン：画面全体に細かい弾を降らせ、風で流す
static void ShotBlizzard(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回呼び出し時に弾を生成
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 画面上部から 24 発を扇状に降らせる
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置は画面上部のランダムな位置（画面幅いっぱい）
            pEnemyShot->x = GetRand(480);
            pEnemyShot->y = -20.0; // 画面外上から登場

            // 基本は真下方向だが、左右に少し広がる
            double baseAngle = DX_PI / 2.0; // 真下
            double spread = DX_PI / 3.0;    // ±30度
            double offset = (i - 12) * (spread / 12.0); // -12..11 で均等に広がる
            pEnemyShot->muki = baseAngle + offset;

            // 速度はゆっくり〜中程度（雪が降る感じ）
            pEnemyShot->speed = (80 + GetRand(120)) / 100.0;

            // 弾の種類と色をランダムに選ぶ（サンプルと同じ範囲）
            int type = GetRand(5);  // 0..5: 小玉、中玉、大玉、銃弾、鱗弾、菱形弾
            int color = GetRand(7); // 0..7: 赤、黄、緑、シアン、青、マゼンタ、白、黒

            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotLargeBall[color];
                break;
            case 3:
                pEnemyShot->kind = img_enemyShotBullet[color];
                break;
            case 4:
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            case 5:
                pEnemyShot->kind = img_enemyShotDiamond[color];
                break;
            }

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 風向きの更新（時間でゆっくり変化）
    windChangeTimer++;
    if (windChangeTimer >= 180) {
        windChangeTimer = 0;
        // 風向きをランダムに変える（左右 ±60度）
        windAngle = DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI;
    }

    // 弾の移動処理（風の影響で流される）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本の移動（真下方向）
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 風の影響で横方向にも流される
        double windEffect = 0.5; // 風の強さ
        pEnemyShot->x += windEffect * cos(windAngle);
        pEnemyShot->y += windEffect * sin(windAngle);


        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（指定どおりの関数名）
void EnemyPat_Blizzard_Sakana()
{
    static int muki;
    if (count == 1) {
        // 初期配置（画面中央上）
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1; // 右向き
        windAngle = DX_PI / 2.0; // 初期風向き：真下
        windChangeTimer = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で吹雪弾幕を発射
    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBlizzard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = DX_PI / 2.0; // 真下方向（雪が降るイメージ）

        // 弾リストのダミーヘッドを用意
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾セットリストに挿入
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}