// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotSplit(sEnemyShotSet* pEnemyShotSet);
static void ShotKunai(sEnemyShotSet* pEnemyShotSet);

// ---------------------------------------------------------
// 手裏剣弾の軌道制御（投擲 → 旋回 → 帰還 → 分裂）
// ---------------------------------------------------------
static void ShotShuriken(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->count == 0) {
            // 初期化
            // 素材: 菱形弾(img_enemyShotDiamond)の 青(4)
            pEnemyShot->kind = img_enemyShotDiamond[4];
            pEnemyShot->speed = 5.0; // 投擲速度
            pEnemyShot->param_i[0] = 0; // 0:投擲フェーズ, 1:帰還フェーズ
        }

        // フェーズ遷移: 40フレーム後に旋回開始
        if (pEnemyShot->count == 40) {
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->speed = 3.0; // 減速して旋回
        }

        if (pEnemyShot->param_i[0] == 0) {
            // 【フェーズ0】直進
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        else {
            // 【フェーズ1】帰還: ボス(enemy)の位置へ角度を補間しながら向かう
            double tx = enemy.x - pEnemyShot->x;
            double ty = enemy.y - pEnemyShot->y;
            double target_muki = atan2(ty, tx);
            double diff = target_muki - pEnemyShot->muki;

            // 角度差を-PI～PIに正規化
            while (diff > DX_PI) diff -= DX_PI * 2;
            while (diff < -DX_PI) diff += DX_PI * 2;

            // 進行方向をボスへ曲げていく（旋回）
            pEnemyShot->muki += diff * 0.08;

            // 加速して戻る
            if (pEnemyShot->speed < 7.0) pEnemyShot->speed += 0.15;

            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            // ボスに到達したら分裂
            double dist = sqrt(tx * tx + ty * ty);
            if (dist < 30.0) {
                // --- 分裂弾セットの生成 ---
                sEnemyShotSet* pNewSet = new sEnemyShotSet;
                pNewSet->count = 0;
                pNewSet->patternFunc = ShotSplit; // 後述の分裂弾関数
                pNewSet->x = pEnemyShot->x;
                pNewSet->y = pEnemyShot->y;
                pNewSet->muki = 0;
                pNewSet->kind = 0;

                // 弾リストの初期化
                pNewSet->pEnemyShotHead = new sEnemyShot;
                pNewSet->pEnemyShotHead->prev = pNewSet->pEnemyShotHead;
                pNewSet->pEnemyShotHead->next = pNewSet->pEnemyShotHead;

                // セットをグローバルリストへ接続
                pNewSet->prev = enemyShotSetHead.prev;
                pNewSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pNewSet;
                enemyShotSetHead.prev = pNewSet;

                // 自身は画面外へ飛ばして消去（メインルーチンの画面外判定に任せる）
                pEnemyShot->x = -1000;
                pEnemyShot->y = -1000;
            }
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------------------------------------
// 分裂弾の制御（手裏剣が帰還して破裂した後の弾）
// ---------------------------------------------------------
static void ShotSplit(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_extreme)) PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
       
        // 素材: 鱗弾(img_enemyShotScale)の 赤(0)
        for (int i = 0; i < 24; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = (DX_PI * 2.0 / 24.0) * i;
            p->speed = 2.5 + (GetRand(10) / 10.0);
            p->kind = img_enemyShotScale[0];

            // リスト接続
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 移動処理
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ---------------------------------------------------------
// クナイの制御（追い打ち用の直進弾）
// ---------------------------------------------------------
static void ShotKunai(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);        

        // 素材: 銃弾(img_enemyShotBullet)の 白(6)
        for (int i = 0; i < 5; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            // 自機狙い + ばらつき
            p->muki = pEnemyShotSet->muki + (GetRand(20) - 10) / 180.0 * DX_PI;
            p->speed = 6.0;
            p->kind = img_enemyShotBullet[6];

            // リスト接続
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 移動処理
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ---------------------------------------------------------
// 敵本体のパターン
// ---------------------------------------------------------
void EnemyPat_Shuriken_Qwen()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.2 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 手裏剣発射 (3秒に1回程度)
    if (count % 60 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotShuriken;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 手裏剣を3枚同時に投げる（扇状）
        for (int i = -4; i < 4; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = pSet->muki + i * 0.2;
            p->speed = 0; // ShotShuriken側で設定
            p->kind = 0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }

        // セットをグローバルリストへ接続
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // クナイ連射 (手裏剣の合間を縫うように)
    if (count % 30 == 15) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotKunai;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // セットをグローバルリストへ接続
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}