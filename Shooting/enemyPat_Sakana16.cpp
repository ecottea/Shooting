// enemyPat_go_hardcore.cpp
// 囲碁をモチーフにした、難易度重視の弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 囲碁の「星」と「石」をイメージした、高速・高密度の弾幕パターン
static void ShotGoStoneHard(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 9路盤の星の位置をイメージ
        const int numStars = 9;
        const double starOffsets[9][2] = {
            { -120.0, -120.0 }, {   0.0, -120.0 }, { 120.0, -120.0 },
            { -120.0,    0.0 }, {   0.0,    0.0 }, { 120.0,    0.0 },
            { -120.0,  120.0 }, {   0.0,  120.0 }, { 120.0,  120.0 }
        };

        for (int i = 0; i < numStars; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + starOffsets[i][0] * 0.5;
            pEnemyShot->y = pEnemyShotSet->y + starOffsets[i][1] * 0.5;

            // 星の中心から外側へ放射状に発射
            double dx = pEnemyShot->x - pEnemyShotSet->x;
            double dy = pEnemyShot->y - pEnemyShotSet->y;
            pEnemyShot->muki = atan2(dy, dx);
            if (i == 4) pEnemyShot->muki = pEnemyShotSet->muki;

            // 速度を大幅に上げる（3.0〜3.5）
            pEnemyShot->speed = 3.0 + (GetRand(50) / 100.0);

            // 弾種は「中玉」のみに絞る
            // 色は黒(0)と白(6)のみ
            int color = GetRand(1) == 0 ? 7 : 6; // 0:黒っぽい色, 6:白
            pEnemyShot->kind = img_enemyShotMediumBall[color];

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

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（関数名は指定どおり）
void EnemyPat_Go_Sakana()
{
    static int muki;
    static int phase = 0; // 0: 通常移動, 1: 高速弾幕
    static int phaseCounter = 0;

    if (count == 1) {
        // 初期位置（画面中央上寄り）
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // かなり強く
        muki = 1;
        phase = 0;
        phaseCounter = 0;
    }
    else {
        // フェーズに応じた移動
        if (phase == 0) {
            // 通常移動：左右にゆっくり往復
            enemy.x += 0.8 * (double)muki;
            if (count % 150 == 75) muki *= -1;

            // 一定時間ごとに高速弾幕フェーズへ
            if (count % 120 == 0) {
                phase = 1;
                phaseCounter = 0;
            }
        }
        else if (phase == 1) {
            // 高速弾幕フェーズ：少し止まって高速弾幕を撃つ
            phaseCounter++;
            if (phaseCounter >= 90) {
                phase = 0; // 通常移動に戻る
                phaseCounter = 0;
            }
        }
    }

    // 高速弾幕フェーズ中は、3フレームごとに高密度発射
    if (phase == 1 && count % 3 == 0) {
        if (count % 6 == 0) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGoStoneHard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x)
            + (GetRand(100) - 50) / 200.; // 弾側で向きを計算

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}