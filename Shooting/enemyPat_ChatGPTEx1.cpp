// enemyPat_Tmp.cpp
//
// テーマ：音ゲー
//
// 発想:
// ・上から4本の「レーン」が流れてくる
// ・ノーツ(菱形弾)が一定間隔で落下
// ・途中で左右にスライドするロングノーツ風配置
// ・一定時間ごとに「同時押し」配置
// ・画面下付近でノーツが横に開いてプレイヤーへ襲い掛かる
//
// count や画面外削除はメインルーチン側管理前提

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ノーツ群
// ============================================================
static void ShotRhythmGame(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --------------------------------------------------------
    // 初回生成
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {

        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const double laneX[4] = {
            120.0,
            200.0,
            280.0,
            360.0
        };

        int beat = pEnemyShotSet->kind;

        // 通常ノーツ
        if (beat % 8 != 6) {

            int lane = beat % 4;

            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = laneX[lane];
            pEnemyShot->y = -20.0;

            pEnemyShot->muki = DX_PI / 2.0;
            pEnemyShot->speed = 2.2;

            pEnemyShot->kind = img_enemyShotDiamond[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // 同時押し
        else {

            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            int lane1 = GetRand(3);
            int lane2 = (lane1 + 2) % 4;

            for (int i = 0; i < 2; i++) {

                int lane = (i == 0) ? lane1 : lane2;

                pEnemyShot = new sEnemyShot;

                pEnemyShot->x = laneX[lane];
                pEnemyShot->y = -20.0;

                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->speed = 2.4;

                pEnemyShot->kind = img_enemyShotLargeBall[1];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // --------------------------------------------------------
    // 更新
    // --------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 基本落下
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 音ゲーのスライドノーツ風
        if (pEnemyShot->y > 120.0 && pEnemyShot->y < 260.0) {

            double slide =
                sin((pEnemyShot->y - 120.0) / 140.0 * DX_PI);

            pEnemyShot->x += slide * 1.8;
        }

        // 判定ライン到達演出
        if (pEnemyShot->count == 210) {

            double angle =
                atan2(
                    player.y - pEnemyShot->y,
                    player.x - pEnemyShot->x
                );

            pEnemyShot->muki = angle;
            pEnemyShot->speed = 3.6;

            pEnemyShot->kind = img_enemyShotBullet[4];
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Otoge_ChatGPT()
{
    static int beat;

    if (count == 1) {

        enemy.x = 240.0;
        enemy.y = 60.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        beat = 0;
    }

    // DJっぽく左右に揺れる
    enemy.x = 240.0 + sin(count / 60.0) * 120.0;

    // --------------------------------------------------------
    // BPM風に一定間隔でノーツ生成
    // --------------------------------------------------------
    if (count % 12 == 0) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRhythmGame;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = DX_PI / 2.0;

        pEnemyShotSet->kind = beat++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}