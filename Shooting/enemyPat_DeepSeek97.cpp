// enemyPat_Tmp.cpp
// 東方紅魔郷 琪露诺 符卡「凍符「Perfect Freeze」」Lunatic 実装

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：パーフェクトフリーズ
static void PerfectFreezePattern(sEnemyShotSet* pEnemyShotSet)
{
    // フェーズ定義（フレーム数）
    const int FREEZE_START = 60;   // 凍結開始
    const int FREEZE_END = 90;   // 解凍開始＆追加弾
    const int NUM_INITIAL_BULLETS = 48 * 3;  // 初期散布弾数
    const int ADD_BLUE_COUNT = 8;        // 追加青弾数（偶数）

    // --- 初期化：弾の生成（count == 0 のときのみ） ---
    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // ランダムな彩色小弾をばら撒く
        for (int i = 0; i < NUM_INITIAL_BULLETS; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // ボス周辺にランダム配置（範囲 240x240）
            pEnemyShot->x = pEnemyShotSet->x + GetRand(240) - 120;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(240) - 120;

            // ランダムな方向（全方位）
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;

            // 速度：1.5 ～ 3.0 程度
            pEnemyShot->speed = (150 + GetRand(150)) / 100.0;

            // 色：小型玉のランダム色（赤,黄,緑,シアン,青,マゼンタ,白）
            pEnemyShot->kind = img_enemyShotSmallBall[GetRand(6)];

            // リンクドリストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 既存弾の処理（移動・凍結・解凍） ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 凍結開始：速度を0にする（このフレームでは動かない）
        if (pEnemyShotSet->count == FREEZE_START) {
            pShot->speed = 0.0;
        }
        // 解凍開始：凍結していた弾（speed==0）に新しいランダム方向と速度を与える
        else if (pEnemyShotSet->count == FREEZE_END) {
            if (pShot->speed == 0.0) {
                pShot->muki = GetRand(360) / 180.0 * DX_PI;
                pShot->speed = (100 + GetRand(150)) / 100.0; // 1.0 ～ 2.5
            }
        }

        // 移動（speed > 0 のときのみ）
        if (pShot->speed > 0.0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }

    // --- 解凍と同時に青い追加弾を自機狙いで発射 ---
    if (pEnemyShotSet->count == FREEZE_END) {
        // 効果音（軽め）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 自機への角度
        double angle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        for (int i = 0; i < ADD_BLUE_COUNT; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // ボス位置から
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 自機狙い（僅かにランダム性を加えてもよい）
            pEnemyShot->muki = angle + (i - (ADD_BLUE_COUNT - 1) / 2.0) * DX_PI / 12.0;
            pEnemyShot->speed = 2.5; // 固定速度
            // 青色の小型玉
            pEnemyShot->kind = img_enemyShotMediumBall[4]; // 4:青

            // リンクドリストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// 敵本体パターン
void EnemyPat_PerfectFreeze_DeepSeek()
{
    static int  muki;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200; // 固定
        muki = 1;
    }
    else {
        // 横移動（往復）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ショットセットを一度だけ作成（少し待ってから開始）
    if (count % 120 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = PerfectFreezePattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;   // 未使用
        pEnemyShotSet->kind = 0;     // 未使用

        // 弾リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルなショットセットリストへ挿入
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}