// enemyPat_popcornKettle.cpp
//
// 「ポップコーン・ケトル」
// カーネル弾（未破裂）が放物線を描いて漂い、ランダムな遅延の後にその場で弾け、
// 不揃いな角度でポップ弾（破裂後）をばら撒く二段階構造の弾幕。
// 破裂の瞬間には塩演出として速く短命な粒弾も追加する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// param_i[0] の値
//   0 : カーネル弾（未破裂・重力で放物線を描いて漂う）
//   1 : ポップ弾／塩弾（破裂後・徐々に減速して漂う）

// 弾幕：ポップコーン・ケトル
static void ShotPopcornKettle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    const int kKernelNum = 3; // 1バーストで発射するカーネル弾の数

    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧
        // PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        // PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        // PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        // PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < kKernelNum; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x + GetRand(60) - 30;
            pEnemyShot->y = pEnemyShotSet->y;

            // ほぼ真上（±30度）へ勢いよく飛び出す
            double angle = -DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
            double spd = (150 + GetRand(100)) / 100.0; // 1.50〜2.50

            pEnemyShot->param_d[0] = spd * cos(angle);  // vx
            pEnemyShot->param_d[1] = spd * sin(angle);  // vy（負で上方向）
            pEnemyShot->param_d[2] = 0.025;              // 重力加速度
            pEnemyShot->param_i[0] = 0;                  // 0:カーネル弾
            pEnemyShot->param_i[1] = 60 + GetRand(120) + 60;  // 破裂までの遅延フレーム（60〜180）

            // 弾の種類一覧: 小玉、中玉、大玉、銃弾、鱗弾、菱形弾
            // 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄：未破裂のカーネル

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pEnemyShot->next; // 削除に備えて先に保存

        if (pEnemyShot->param_i[0] == 0) {
            // --- カーネル弾（未破裂）---
            pEnemyShot->x += pEnemyShot->param_d[0];
            pEnemyShot->y += pEnemyShot->param_d[1];
            pEnemyShot->param_d[1] += pEnemyShot->param_d[2]; // 重力で徐々に下向きへ

            if (pEnemyShot->count >= pEnemyShot->param_i[1]) {
                if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                // --- 破裂：ポップ弾を不揃いな角度でばら撒く ---
                int popNum = 5 + GetRand(2); // 5〜7発
                double baseAngle = GetRand(1000) / 1000.0 * 2.0 * DX_PI; // バーストごとに向きをずらす

                for (int i = 0; i < popNum; i++) {
                    sEnemyShot* pPop = new sEnemyShot;

                    double angle = baseAngle + (2.0 * DX_PI * i / popNum)
                        + (GetRand(30) - 15) / 180.0 * DX_PI; // ±15度の不揃い
                    double spd = (200 + GetRand(150)) / 100.0; // 2.00〜3.50

                    pPop->x = pEnemyShot->x;
                    pPop->y = pEnemyShot->y;
                    pPop->param_d[0] = spd * cos(angle);
                    pPop->param_d[1] = spd * sin(angle);
                    pPop->param_d[2] = 0.965; // 減速率（オイルにふわっと浮くイメージ）
                    pPop->param_i[0] = 1;     // 1:ポップ弾

                    pPop->kind = img_enemyShotMediumBall[6]; // 白：弾けたポップコーン

                    pPop->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pPop->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pPop;
                    pEnemyShotSet->pEnemyShotHead->prev = pPop;
                }

                // --- 塩演出：弾ける瞬間に飛び散る速く短命な粒 ---
                int saltNum = 1 + GetRand(1); // 1〜2発
                for (int i = 0; i < saltNum; i++) {
                    sEnemyShot* pSalt = new sEnemyShot;

                    double angle = GetRand(1000) / 1000.0 * 2.0 * DX_PI;
                    double spd = (350 + GetRand(150)) / 100.0 + 5; // 3.50〜5.00

                    pSalt->x = pEnemyShot->x;
                    pSalt->y = pEnemyShot->y;
                    pSalt->param_d[0] = spd * cos(angle);
                    pSalt->param_d[1] = spd * sin(angle);
                    pSalt->param_d[2] = 0.90; // 素早く失速する
                    pSalt->param_i[0] = 1;    // ポップ弾と同じ移動則で扱う

                    pSalt->kind = img_enemyShotDiamond[6]; // 白い菱形：塩の粒

                    pSalt->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSalt->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSalt;
                    pEnemyShotSet->pEnemyShotHead->prev = pSalt;
                }

                // カーネル弾自身はリストから除去して破棄
                pEnemyShot->prev->next = pEnemyShot->next;
                pEnemyShot->next->prev = pEnemyShot->prev;
                delete pEnemyShot;
            }
        }
        else {
            // --- ポップ弾／塩弾（破裂後）---
            pEnemyShot->x += pEnemyShot->param_d[0];
            pEnemyShot->y += pEnemyShot->param_d[1];
            pEnemyShot->param_d[0] *= pEnemyShot->param_d[2];
            pEnemyShot->param_d[1] *= pEnemyShot->param_d[2];

            if (pEnemyShot->count >= 300) pEnemyShot->x = 99999;
        }

        pEnemyShot = pNext;
    }
}

// 敵本体のパターン：ポップコーン・ケトル
void EnemyPat_Popcorn_Claude()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 一定間隔でケトルからカーネルをバースト発射
    if (count % 15 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPopcornKettle;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}