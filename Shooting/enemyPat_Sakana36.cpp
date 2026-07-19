// enemyPat_popcorn.cpp
// ポップコーン弾幕パターン（Popcorn Pop Shot）の実装

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ポップコーン弾幕用の弾パターン関数
static void ShotPopcorn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 粒弾（初期状態）の生成
    if (pEnemyShotSet->count == 0) {
        // 効果音：中くらいの弾幕音
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 粒弾を 5 発ほどばら撒く
        for (int i = 0; i < 15; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 少しランダムに散らす
            double angle = pEnemyShotSet->muki + (GetRand(80) - 40) / 180.0 * DX_PI;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 1.5 + (GetRand(50)) / 100.0; // ゆっくりめ

            // 粒弾は「小玉」で黄色（ポップコーンの粒をイメージ）
            pEnemyShot->kind = img_enemyShotSmallBall[1]; // 1:黄色

            // パラメータ初期化
            pEnemyShot->param_i[0] = 0; // 0:粒弾状態, 1:ポップ後
            pEnemyShot->param_i[1] = 30 + GetRand(20); // ポップするまでのフレーム数

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存の弾の更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 粒弾状態ならカウントダウン
        if (pEnemyShot->param_i[0] == 0) {
            pEnemyShot->param_i[1]--;
            if (pEnemyShot->param_i[1] <= 0) {
                // ポップコーン弾に変化
                pEnemyShot->param_i[0] = 1;
                pEnemyShot->kind = img_enemyShotLargeBall[6]; // 6:白（ポップコーン色）
                pEnemyShot->speed *= 1.8; // ポップ後は少し速く
            }
        }

        // 移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Popcorn_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60)
            muki *= -1;
    }

    // 一定間隔でポップコーン弾幕セットを生成
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPopcorn;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        // 弾リストのダミーヘッド
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}