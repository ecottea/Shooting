// enemyPat_Tmp_Popcorn.cpp
// ポップコーンをモチーフにした弾幕パターン
// ファイル名例: enemyPat_Tmp_Popcorn.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// ポップコーン弾幕用パターン関数
// =============================================
static void ShotPopcorn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回：カーネル弾（未爆発の粒）を生成
    if (pEnemyShotSet->count == 0) {
        // 効果音（中くらいの射撃音を使用）
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int kernelCount = 18;  // カーネル数（調整可能）

        for (int i = 0; i < kernelCount; i++) {
            pEnemyShot = new sEnemyShot;
            // 敵の位置から少しばらけて出現
            pEnemyShot->x = pEnemyShotSet->x + GetRand(80) - 40;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(30) - 15;

            // プレイヤー方向へ緩やかに飛ぶ
            double targetAngle = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->muki = targetAngle + (GetRand(80) - 40) / 180.0 * DX_PI;  // 少し角度をばらける

            pEnemyShot->speed = 1.8 + GetRand(60) / 100.0;  // 1.8〜2.4程度の遅め速度

            // カーネルは中玉（黄色系を想定）
            pEnemyShot->kind = img_enemyShotMediumBall[1];  // 黄色寄り

            // param_i[0] = 爆発までのフレームカウンタ（約50〜70フレーム）
            pEnemyShot->param_i[0] = 55 + GetRand(25);

            // リンクに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pEnemyShot->next;  // 削除時に安全に進む

        // 移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 爆発判定
        bool shouldPop = false;
        if (pEnemyShot->param_i[0] > 0) {
            pEnemyShot->param_i[0]--;  // カウントダウン
            if (pEnemyShot->param_i[0] <= 0) {
                shouldPop = true;
            }
        }

        if (shouldPop) {
            // ポップ！ 小弾を放射
            int popCount = 7 + GetRand(3);  // 7〜10個
            double baseAngle = GetRand(360) / 180.0 * DX_PI;  // ランダム回転

            for (int i = 0; i < popCount; i++) {
                sEnemyShot* psmall = new sEnemyShot;
                psmall->x = pEnemyShot->x;
                psmall->y = pEnemyShot->y;

                double angle = baseAngle + i * (DX_PI * 2 / popCount);
                psmall->muki = angle + (GetRand(40) - 20) / 180.0 * DX_PI;  // 少しばらけ

                psmall->speed = 3.2 + GetRand(80) / 100.0;  // 高速小弾

                // 小玉（白・黄混在）
                if (i % 2 == 0) {
                    psmall->kind = img_enemyShotSmallBall[6];  // 白
                }
                else {
                    psmall->kind = img_enemyShotSmallBall[1];  // 黄
                }

                // 少し減速する演出用
                psmall->param_d[0] = psmall->speed;  // 初期速度記憶（減速用）

                psmall->prev = pEnemyShotSet->pEnemyShotHead->prev;
                psmall->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = psmall;
                pEnemyShotSet->pEnemyShotHead->prev = psmall;
            }

            // カーネル本体は消去（爆発したので）
            // リストから削除
            pEnemyShot->prev->next = pEnemyShot->next;
            pEnemyShot->next->prev = pEnemyShot->prev;
            delete pEnemyShot;
        }
        else {
            // 小弾の減速処理（すでに爆発済みの弾）
            if (pEnemyShot->param_d[0] > 0.0) {
                pEnemyShot->speed *= 0.96;  // 徐々に減速（ふわっと広がる感じ）
                if (pEnemyShot->speed < 0.6) pEnemyShot->speed = 0.6;
            }
        }

        pEnemyShot = nextShot;
    }
}

// =============================================
// 敵本体パターン（ポップコーン弾幕）
// =============================================
void EnemyPat_Popcorn_Grok()
{
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1) {
        // 初期位置（ゲーム画面 480x480）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;

        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右往復移動
        enemy.x += 1.4 * (double)muki;
        if (enemy.x < 80.0) muki = 1;
        if (enemy.x > 400.0) muki = -1;
    }

    // 定期的にポップコーン弾幕を発射（やや間隔を空ける）
    if (count % 55 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPopcorn;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // プレイヤー方向を基準に少し下向きに
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y + 40, player.x - pEnemyShotSet->x);

        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}