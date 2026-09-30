#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#ifndef DX_PI
#define DX_PI 3.14159265358979323846
#endif

// =========================================================================
// 弾幕パターン：螺旋の風眼（アイ・オブ・ザ・ストーム）
// =========================================================================
static void Shot_Typhoon(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->count はメインルーチンで毎フレーム自動で+1される
    int c = pEnemyShotSet->count;

    // -------------------------------------------------------------------------
    // Phase 1: 前兆と雨雲の接近 (0 〜 300フレーム / 約5秒)
    // -------------------------------------------------------------------------
    if (c < 300) {
        if (c == 1) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 雨の表現：小粒弾（シアン or 白）を斜め下へ高速でばら撒く
        if (c % 3 == 0) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + GetRand(400) - 200; // 横方向に広くばらけさせる
            pShot->y = pEnemyShotSet->y - 20.0;
            pShot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI; // 下向き ± 揺らぎ
            pShot->speed = 5.0 + GetRand(20) / 10.0; // 5.0 〜 7.0
            // 色: 3=シアン, 6=白
            pShot->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[6];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 突風の表現：誘導弾（鱗弾・黒）を左右端からプレイヤーへ向けて曲げながら発射
        if (c % 45 == 0) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = (GetRand(1) == 0) ? 0.0 : 480.0; // 画面左右端から
            pShot->y = pEnemyShotSet->y + GetRand(100) - 50;
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 2.0; // 遅めの速度
            pShot->kind = img_enemyShotScale[7]; // 7:黒

            // 誘導処理用のパラメータ
            pShot->param_d[0] = pShot->muki; // 目標角度（初期値）
            pShot->param_i[0] = 1;           // 誘導フラグ

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
    // -------------------------------------------------------------------------
    // Phase 2: 渦巻きの形成（レインバンド） (300 〜 900フレーム / 約10秒)
    // -------------------------------------------------------------------------
    else if (c < 900) {
        if (c % 3 == 0) {
            double base_angle = (c * 0.15); // 時間とともに回転する基準角度

            // 渦巻きの骨格：中粒弾（青）
            sEnemyShot* pShot1 = new sEnemyShot;
            pShot1->x = pEnemyShotSet->x;
            pShot1->y = pEnemyShotSet->y;
            pShot1->muki = base_angle;
            // 遠心力の表現：速度に幅を持たせ、外側に行くほど広がるようにする
            pShot1->speed = 2.5 + GetRand(30) / 10.0; // 2.5 〜 5.5
            pShot1->kind = img_enemyShotMediumBall[4]; // 4:青

            pShot1->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot1->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot1;
            pEnemyShotSet->pEnemyShotHead->prev = pShot1;

            // 細かい雲：小粒弾（白）を角度と速度を少しずらして重ねる（厚みの表現）
            sEnemyShot* pShot2 = new sEnemyShot;
            pShot2->x = pEnemyShotSet->x;
            pShot2->y = pEnemyShotSet->y;
            pShot2->muki = base_angle + 0.2; // 角度を少しずらす
            pShot2->speed = 3.5 + GetRand(20) / 10.0; // 3.5 〜 5.5
            pShot2->kind = img_enemyShotSmallBall[6]; // 6:白

            pShot2->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot2->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot2;
            pEnemyShotSet->pEnemyShotHead->prev = pShot2;
        }
    }
    // -------------------------------------------------------------------------
    // Phase 3: 台風の目（アイ）と稲妻 (900 〜 1500フレーム / 約10秒)
    // -------------------------------------------------------------------------
    else if (c < 1500) {
        // 台風の目：大粒弾（青）をゆっくりと追尾させる
        if (c == 900) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 0.8; // 非常に遅い速度
            pShot->kind = img_enemyShotLargeBall[4]; // 4:青
            pShot->param_i[0] = 2; // 「目」フラグ（特殊追尾用）

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 稲妻の表現：細レーザー（シアン）を8方向へ瞬間発射
        if (c % 45 == 0) {
            for (int i = 0; i < 8; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                // 8方向 + 微かな揺らぎで自然な稲妻を表現
                pShot->muki = (DX_PI * 2.0 / 8.0) * i + (GetRand(20) - 10) / 180.0 * DX_PI;
                pShot->speed = 15.0; // 高速で画面外へ飛ばし、瞬きのように見せる
                pShot->kind = img_enemyShotLaser[3]; // 3:シアン

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
    // -------------------------------------------------------------------------
    // Phase 4: 最盛期と崩壊（豪雨への移行） (1500 〜 2100フレーム / 約10秒)
    // -------------------------------------------------------------------------
    else if (c < 2100) {
        // 渦巻きの高速化・高密度化
        if (c % 2 == 0) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            double base_angle = (c * 0.3); // 回転速度をPhase2より上げる
            pShot->muki = base_angle + (GetRand(40) - 20) / 180.0 * DX_PI;
            pShot->speed = 4.0 + GetRand(40) / 10.0; // 4.0 〜 8.0
            pShot->kind = (GetRand(1) == 0) ? img_enemyShotMediumBall[4] : img_enemyShotSmallBall[6];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 崩壊予告
        if (c == 2000) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 崩壊（フィニッシュ）：全方向へ小粒弾（白）をランダムな速度で拡散
        if (c == 2030) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 72; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = (DX_PI * 2.0 / 72.0) * i;
                pShot->speed = 3.0 + GetRand(50) / 10.0; // 3.0 〜 8.0 のランダムな速度
                pShot->kind = img_enemyShotSmallBall[6]; // 6:白

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // -------------------------------------------------------------------------
    // 弾の移動処理と特殊挙動（誘導など）
    // -------------------------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 誘導弾（突風）の処理
        if (pShot->param_i[0] == 1) {
            double target_muki = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = target_muki - pShot->muki;
            // 角度の正規化（-PI 〜 PI の範囲に収める）
            while (diff > DX_PI) diff -= DX_PI * 2.0;
            while (diff < -DX_PI) diff += DX_PI * 2.0;
            pShot->muki += diff * 0.05; // 徐々に目標角度へ近づける
        }
        // 台風の目（ゆっくり追尾）の処理
        else if (pShot->param_i[0] == 2) {
            double target_muki = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = target_muki - pShot->muki;
            while (diff > DX_PI) diff -= DX_PI * 2.0;
            while (diff < -DX_PI) diff += DX_PI * 2.0;
            pShot->muki += diff * 0.02; // 非常に弱い誘導で「巨大でゆっくり動く」印象に
        }

        // 通常の移動計算
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// =========================================================================
// 敵本体のパターン
// =========================================================================
void EnemyPat_Typhoon_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0; // 画面中央上部に配置
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;

        // 弾幕セットの初期生成（このパターンでは1つのセットで全フェーズを管理する）
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = Shot_Typhoon;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵の移動：左右にゆっくり往復する
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 0) muki *= -1; // 約3秒ごとに折り返し

        // 弾幕の基準位置（渦の中心）を敵の現在位置に同期させる
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == Shot_Typhoon) {
                pSet->x = enemy.x;
                pSet->y = enemy.y + 10.0;
            }
            pSet = pSet->next;
        }
    }
}