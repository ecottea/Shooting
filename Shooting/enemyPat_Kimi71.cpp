// enemyPat_flickerBulb.cpp
// 切れかけ電球モチーフ弾幕「明滅回廊（フリッカー・コリドー）」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 明滅回廊：切れかけ電球のように明滅する弾幕
// ------------------------------------------------------------
// 【使う素材一覧】
//   弾種：大玉(20x20)・中玉(7x7)・小玉(2.5x2.5)・鱗弾(4x3)
//   色  ：黄(1)・白(6)・黒(7)・橙(8)
//   SE  ：sound_enemyShot_medium（発射）/ sound_enemyCharge（予告）/ sound_enemyShot_light（破裂）
//
// 【param_i の用途】
//   [0] 種別 (0:点灯中電球 1:消灯中電球 2:火花弾 3:破裂破片)
//   [1] 次の明滅までの残りフレーム
//   [2] 電球ID（0〜5）
//   [3] 破裂フラグ（0:通常 1:破裂中）
//   [4] 予兆フラグ（0:未閃光 1:閃光済）
// 【param_d の用途】
//   [0] 基本速度の保存
// ------------------------------------------------------------
static void ShotFlickerCorridor(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- 定数 ---
    const int BULB_COUNT = 6;    // 電球弾の数
    const double BASE_SPEED = 1.8;  // 点灯時の基本速度
    const double DIM_SPEED = 0.2;  // 消灯時の速度（極めて遅く）
    const int FLICKER_MIN = 45;   // 点灯最短フレーム
    const int FLICKER_MAX = 85;   // 点灯最長フレーム
    const int DIM_MIN = 30;   // 消灯最短フレーム
    const int DIM_MAX = 60;   // 消灯最長フレーム
    const int FLASH_PRE = 6;    // 消灯予兆フレーム数

    // ========== 初期化 ==========
    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 予告音（ちらつき開始を暗示）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < BULB_COUNT; i++) {
            pEnemyShot = new sEnemyShot;

            // 横一列に配置（中央240を基準に400px幅で広がる）
            double spread = 400.0;
            double offset = (BULB_COUNT > 1) ? (spread / (BULB_COUNT - 1)) * i - spread / 2.0 : 0.0;
            pEnemyShot->x = pEnemyShotSet->x + offset;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;

            // 下方へ向かう（わずかに角度をずらす）
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = BASE_SPEED;

            // 点灯状態：黄色の大玉
            pEnemyShot->kind = img_enemyShotLargeBall[1]; // 1:黄

            // 明滅管理パラメータ
            pEnemyShot->param_i[0] = 0; // 点灯
            pEnemyShot->param_i[1] = FLICKER_MIN + GetRand(FLICKER_MAX - FLICKER_MIN);
            pEnemyShot->param_i[2] = i; // ID
            pEnemyShot->param_i[3] = 0; // 未破裂
            pEnemyShot->param_i[4] = 0; // 未閃光

            pEnemyShot->param_d[0] = BASE_SPEED; // 基本速度保存

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ========== 火花（散弾）生成 ==========
    // 点灯中の電球から周囲に火花を散らす
    if (pEnemyShotSet->count % 12 == 0) {
        sEnemyShot* pScan = pEnemyShotSet->pEnemyShotHead->next;
        while (pScan != pEnemyShotSet->pEnemyShotHead) {
            if (pScan->param_i[0] == 0 && pScan->param_i[3] == 0) {
                sEnemyShot* pSpark = new sEnemyShot;
                pSpark->x = pScan->x;
                pSpark->y = pScan->y;
                pSpark->muki = GetRand(360) / 180.0 * DX_PI; // ランダム全方位
                pSpark->speed = 0.8 + GetRand(120) / 100.0;   // 0.8〜2.0
                pSpark->kind = img_enemyShotScale[8];        // 8:橙（火花風）
                pSpark->param_i[0] = 2; // 火花識別子

                pSpark->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSpark->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSpark;
                pEnemyShotSet->pEnemyShotHead->prev = pSpark;
            }
            pScan = pScan->next;
        }
    }

    // ========== 全弾更新 ==========
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // リスト削除対応用

        // ----- 火花弾の更新 -----
        if (pShot->param_i[0] == 2) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed *= 0.92; // 減衰
            pShot = pNext;
            continue;
        }

        // ----- 破裂破片の更新 -----
        if (pShot->param_i[0] == 3) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pNext;
            continue;
        }

        // ----- 破裂判定（画面下部到達 or 寿命） -----
        // param_i[0] == 3 の破片は除外して無限ループを防ぐ
        if (pShot->param_i[3] == 0 && (pShot->y > 460.0 || pEnemyShotSet->count > 380)) {
            pShot->param_i[3] = 1; // 破裂フラグON

            // 自機方向を中心に扇状に拡散
            double aimAngle = atan2(player.y - pShot->y, player.x - pShot->x);
            const int WAY = 5;
            const double SPREAD = DX_PI / 4.0; // 45度幅

            for (int w = 0; w < WAY; w++) {
                sEnemyShot* pFrag = new sEnemyShot;
                pFrag->x = pShot->x;
                pFrag->y = pShot->y;

                double step = (WAY > 1) ? SPREAD / (WAY - 1) : 0.0;
                pFrag->muki = aimAngle - SPREAD / 2.0 + step * w
                    + (GetRand(10) - 5) / 180.0 * DX_PI; // わずかな乱数

                pFrag->speed = 2.2 + GetRand(130) / 100.0; // 2.2〜3.5
                pFrag->kind = img_enemyShotSmallBall[8];  // 8:橙
                pFrag->param_i[0] = 3; // 破片識別子

                pFrag->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pFrag->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pFrag;
                pEnemyShotSet->pEnemyShotHead->prev = pFrag;
            }

            // 元の電球弾は画面外へ飛ばし、メインルーチンに消去させる
            pShot->margin = -100;

            // 破裂音（1セットにつき1回のみ）
            if (pShot->param_i[2] == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            pShot = pNext;
            continue;
        }

        // ----- 破裂中の元弾は画面外へ飛ばすだけ -----
        if (pShot->param_i[3] == 1) {
            pShot->margin = -100;
            pShot = pNext;
            continue;
        }

        // ----- 明滅タイマー管理 -----
        pShot->param_i[1]--;

        // 消灯予兆：消灯直前に白く閃光
        if (pShot->param_i[0] == 0 && pShot->param_i[1] <= FLASH_PRE && pShot->param_i[1] > 0) {
            if (pShot->param_i[4] == 0) {
                pShot->kind = img_enemyShotMediumBall[6]; // 6:白（閃光）
                pShot->param_i[4] = 1;
            }
        }

        // 状態遷移
        if (pShot->param_i[1] <= 0) {
            if (pShot->param_i[0] == 0) {
                // 点灯 → 消灯
                pShot->param_i[0] = 1;
                pShot->param_i[1] = DIM_MIN + GetRand(DIM_MAX - DIM_MIN);
                pShot->kind = img_enemyShotSmallBall[7]; // 7:黒（消灯）
                pShot->speed = DIM_SPEED;
                pShot->param_i[4] = 0;
            }
            else {
                // 消灯 → 点灯
                pShot->param_i[0] = 0;
                pShot->param_i[1] = FLICKER_MIN + GetRand(FLICKER_MAX - FLICKER_MIN);
                pShot->kind = img_enemyShotLargeBall[1]; // 1:黄（点灯）
                pShot->speed = pShot->param_d[0];
                pShot->param_i[4] = 0;
            }
        }

        // ----- 位置更新 -----
        // 消灯時はわずかにブレる（風に煽られたような動き）
        double driftX = 0.0;
        if (pShot->param_i[0] == 1) {
            driftX = sin(pEnemyShotSet->count * 0.08 + pShot->param_i[2] * 1.3) * 0.25;
        }

        pShot->x += pShot->speed * cos(pShot->muki) + driftX;
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_FlickeringLight_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくりと左右に揺れる
        enemy.x += 0.7 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 160フレームごとに弾幕セットを生成
    if (count % 160 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFlickerCorridor;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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