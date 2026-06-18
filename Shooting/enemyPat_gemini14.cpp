// enemyPat_poisonGas.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：滞留する毒ガス（減速・滞留からのホーミング）
static void ShotPoisonGas(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- 発射時の処理 ---
    if (pEnemyShotSet->count == 0) {
        // 重ための音で毒を吐き出すイメージ
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 1回のセットで毒の泡を 20 個生成
        for (int i = 0; i < 20; i++) {
            pEnemyShot = new sEnemyShot;
            // 敵の中心から少し散らした位置から発生させる
            pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;

            // 全方位に発射 (GetRand(360) は 0~360 の値を返す)
            pEnemyShot->muki = (GetRand(360) / 180.0) * DX_PI;
            // 初速は少しバラけさせる (2.00 ~ 5.00)
            pEnemyShot->speed = (200 + GetRand(300)) / 100.0 + 10;

            // 毒らしい色（緑:2 または マゼンタ:5）
            int color = (GetRand(1) == 0) ? 2 : 5;
            // 毒らしい形（中玉:1 または 大玉:2）
            int type = (GetRand(1) == 0) ? 1 : 2;

            if (type == 1) {
                pEnemyShot->kind = img_enemyShotMediumBall[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotLargeBall[color];
            }

            // 双方向リストに接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 毎フレームの弾の更新処理 ---
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShotSet->count < 90) {
            // 【フェーズ1：滞留と蠢き】
            // 空気抵抗のように急速に減速し、空間に漂わせる
            pEnemyShot->speed *= 0.93;

            // 毒ガスのようにゆらゆらと揺れる動きを加える
            // 弾自身のx座標を位相のオフセットに使うことで、各弾の揺らぎをバラけさせる
            pEnemyShot->muki += sin((double)pEnemyShotSet->count * 0.15 + pEnemyShot->x * 0.1) * 0.05;
        }
        else {
            // 【フェーズ2：プレイヤーへの強襲】
            // プレイヤーの方向を計算
            double targetMuki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);

            // ターゲットとの角度の差を計算し、最短距離で旋回させる
            double diff = targetMuki - pEnemyShot->muki;
            while (diff > DX_PI)  diff -= DX_PI * 2;
            while (diff < -DX_PI) diff += DX_PI * 2;

            pEnemyShot->muki += diff * 0.03; // 少しずつ自機の方へ向く（誘導）
            pEnemyShot->speed += 0.05;       // 徐々に加速する
        }

        // 座標の更新 (極座標からXY座標への変換)
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Poison_Gemini()
{
    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 空中に浮かぶ毒々しい生物のように、8の字を描いて不気味に漂う
        enemy.x = 240.0 + sin((double)count / 60.0) * 120.0;
        enemy.y = 80.0 + sin((double)count / 30.0) * 20.0;
    }

    // 120カウント（約2秒）ごとに毒ガス弾幕を放出
    if (count % 60 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPoisonGas;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0; // 今回は弾セット自体の向きは使用しない

        // ダミーヘッドノードの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 双方向リストに接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}