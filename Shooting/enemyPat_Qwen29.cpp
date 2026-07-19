// EnemyPat_SoapBubbles_Qwen.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：シャボン玉
static void ShotSoapBubble(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    // 毎フレームの処理（シャボン玉の移動と寿命管理）
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pEnemyShot->next;

        // サインカーブでゆらゆら移動
        double phase = pEnemyShot->param_d[0];
        double amp = pEnemyShot->param_d[1];
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki) + sin(pEnemyShot->count * 0.1 + phase) * amp;
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 寿命チェック（param_i[0] に設定したフレーム数を超えたら破裂）
        if (pEnemyShot->count >= pEnemyShot->param_i[0]) {
            // 破裂エフェクト：リング弾を生成
            if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            int ring_count = pEnemyShot->kind >= img_enemyShotLargeBall[0] ? 40 : 20; // リング弾の数
            double base_muki = pEnemyShot->count * 0.1; // 破裂時の角度を少しずらす
            for (int i = 0; i < ring_count; i++) {
                sEnemyShot* pRing = new sEnemyShot;
                pRing->x = pEnemyShot->x;
                pRing->y = pEnemyShot->y;
                pRing->muki = base_muki + (DX_PI * 2.0 / ring_count) * i;
                pRing->speed = 2.5;
                pRing->kind = img_enemyShotSmallBall[3]; // 小玉・シアン色（泡の飛び散り）
                pRing->param_i[0] = 99999999;

                // リストに繋ぐ
                pRing->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pRing->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pRing;
                pEnemyShotSet->pEnemyShotHead->prev = pRing;
            }

            // シャボン玉自身を消滅させる（画面外に飛ばしてメインルーチンに削除させる）
            pEnemyShot->x = -1000;
            pEnemyShot->y = -1000;
            pEnemyShot->speed = 0;
        }

        pEnemyShot = pNext;
    }

    // count == 0 のときにシャボン玉を生成
    if (pEnemyShotSet->count == 0) {
        int num_bubbles = 3; // 1回に生成するシャボン玉の数
        for (int i = 0; i < num_bubbles; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(100) - 50;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(60) - 30) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.5; // ゆっくり浮遊

            // 弾の種類と色（大玉または中玉をランダムに）
            int ball_type = GetRand(1);
            int color = 0;
            switch (GetRand(3)) {
            case 0: color = 3; break; // シアン
            case 1: color = 5; break; // マゼンタ
            case 2: color = 1; break; // 黄
            case 3: color = 2; break; // 緑
            }

            if (ball_type == 0) {
                pEnemyShot->kind = img_enemyShotLargeBall[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[color];
            }

            // パラメータ設定
            pEnemyShot->param_d[0] = GetRand(314) / 100.0; // 位相 (0～3.14)
            pEnemyShot->param_d[1] = 1.5 + GetRand(10) / 10.0; // 振幅 (1.5～2.5)
            pEnemyShot->param_i[0] = 120 + GetRand(60); // 寿命 (120～180フレーム)

            // リストに繋ぐ
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// 敵本体のパターン
void EnemyPat_SoapBubbles_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔でシャボン玉を生成
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSoapBubble;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 真下へ発射
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}