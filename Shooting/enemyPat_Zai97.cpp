// enemyPat_Tmp.cpp
// 凍符「パーフェクトフリーズ」(Lunatic) - 東方紅魔郷 2面BOSS チルノ
// 散布→全弾凍結→自機狙い→ランダム融解 のサイクルを繰り返す弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕パターン：パーフェクトフリーズ
// ============================================================
//  フェーズ構成：
//    Phase 0: 散布   - 虹色弾を全方向にランダム発射
//    Phase 1: 凍結   - 全散布弾が停止し白氷化、予告音
//    Phase 2: 狙撃   - 凍結中に6-way自機狙い青弾を3回発射
//    Phase 3: 融解   - 全凍結弾がランダム方向へ動き出す
//    → Phase 0 へ戻る（サイクル継続）
//
//  param_i[0] : フェーズ番号 (0-3)
//  param_i[1] : フェーズ内タイマー
// ============================================================
static void ShotPerfectFreeze(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;
    int phase = pEnemyShotSet->param_i[0];

    // フェーズ内タイマーを先にインクリメント
    pEnemyShotSet->param_i[1]++;

    switch (phase) {

        // --------------------------------------------------------
        //  Phase 0: 散布（虹色弾を全方向へ）
        // --------------------------------------------------------
    case 0:
        if (pEnemyShotSet->param_i[1] == 1) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // 虹色パレット（赤黄緑シアン青マゼンタ橙）
            const int rainbowColors[] = { 0, 1, 2, 3, 4, 5, 8 };
            const int NUM_COLORS = 7;

            for (int i = 0; i < 30*10; i++) {
                pShot = new sEnemyShot;
                pShot->x = enemy.x + GetRand(30) - 15;
                pShot->y = enemy.y + GetRand(20) - 10;
                pShot->muki = GetRand(359) * DX_PI / 180.0;
                pShot->speed = (130 + GetRand(170)) / 100.0; // 1.3～3.0
                pShot->param_i[0] = 0;   // 弾種別：散布弾（凍結対象）
                pShot->param_i[1] = 0;   // 状態：未凍結
                pShot->kind = img_enemyShotSmallBall[rainbowColors[i % NUM_COLORS]];

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        if (pEnemyShotSet->param_i[1] >= 70) {
            pEnemyShotSet->param_i[0] = 1;
            pEnemyShotSet->param_i[1] = 0;
        }
        break;

        // --------------------------------------------------------
        //  Phase 1: 凍結（全散布弾を停止・白氷化）
        // --------------------------------------------------------
    case 1:
        if (pEnemyShotSet->param_i[1] == 1) {
            // 凍結予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            // 全散布弾を凍結
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] == 0 && pShot->param_i[1] == 0) {
                    pShot->speed = 0.0;
                    pShot->param_i[1] = 1;                    // 凍結状態へ
                    pShot->kind = img_enemyShotSmallBall[6];  // 白(氷)
                }
                pShot = pShot->next;
            }
        }

        if (pEnemyShotSet->param_i[1] >= 30) {
            pEnemyShotSet->param_i[0] = 2;
            pEnemyShotSet->param_i[1] = 0;
        }
        break;

        // --------------------------------------------------------
        //  Phase 2: 狙撃（凍結中に自機狙い6-way青弾×3回）
        // --------------------------------------------------------
    case 2:
        // param_i[1] == 1, 51, 101 で発射（3回）
        if (pEnemyShotSet->param_i[1] % 30 == 1 && pEnemyShotSet->param_i[1] <= 61) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            double baseAngle = atan2(player.y - enemy.y, player.x - enemy.x);

            // 6-way自機狙い（Lunatic仕様）
            for (int i = 0; i < 6; i++) {
                pShot = new sEnemyShot;
                pShot->x = enemy.x;
                pShot->y = enemy.y + 10.0;
                pShot->muki = baseAngle + (i - 2.5) * DX_PI / 18.0; // 約10°間隔
                pShot->speed = (280 + GetRand(80)) / 100.0;         // 2.8～3.6
                pShot->param_i[0] = 1;   // 弾種別：狙撃弾（凍結しない）
                pShot->param_i[1] = 0;
                pShot->kind = img_enemyShotMediumBall[4];  // 青

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        if (pEnemyShotSet->param_i[1] >= 120) {
            pEnemyShotSet->param_i[0] = 3;
            pEnemyShotSet->param_i[1] = 0;
        }
        break;

        // --------------------------------------------------------
        //  Phase 3: 融解（全凍結弾がランダム方向へ動き出す）
        // --------------------------------------------------------
    case 3:
        if (pEnemyShotSet->param_i[1] == 1) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 凍結弾にランダムな方向・速度を与える
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] == 0 && pShot->param_i[1] == 1) {
                    pShot->muki = GetRand(359) * DX_PI / 180.0;
                    pShot->speed = (80 + GetRand(220)) / 100.0; // 0.8～3.0（ランダム）
                    pShot->param_i[1] = 0;                      // 凍結解除
                    // 白のまま（氷が溶けて滑り出すイメージ）
                }
                pShot = pShot->next;
            }
        }

        if (pEnemyShotSet->param_i[1] >= 150) {
            // サイクル先頭へ戻る
            pEnemyShotSet->param_i[0] = 0;
            pEnemyShotSet->param_i[1] = 0;
        }
        break;
    }

    // 全弾移動（speed=0の凍結弾はその場に静止）
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_PerfectFreeze_Zai()
{
    static int muki;

    if (count == 1) {
        // 初期配置
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // パーフェクトフリーズ用の弾幕セットを生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPerfectFreeze;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = 0;  // Phase 0: 散布から開始
        pEnemyShotSet->param_i[1] = 0;  // サブタイマー初期化

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵はゆっくり左右に移動
        enemy.x += 0.35 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }
}