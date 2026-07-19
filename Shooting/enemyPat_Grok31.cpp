// enemyPat_Tmp.cpp
// 戻り弾幕パターン：【幽霊の帰還（Ghost Return）】

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// 戻り弾幕用ヘルパー関数
// =============================================
static void ShotGhostReturn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double playerDistThreshold = 120.0;  // ターン開始距離

    if (pEnemyShotSet->count == 0) {
        // 発射音（中くらいの音）
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 8〜12発程度を扇状に発射
        int num = 9 + (GetRand(3));  // 9〜12発
        double baseAngle = pEnemyShotSet->muki;

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置（敵の少し下）
            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y + 15.0;

            // 角度にバラつきを持たせる
            double angleOffset = (GetRand(120) - 60) / 180.0 * DX_PI * 0.6;
            pEnemyShot->muki = baseAngle + angleOffset;

            // 初期速度（遅め）
            pEnemyShot->speed = 1.8 + GetRand(60) / 100.0;

            // 紫〜白系統の薄い中玉を使う
            pEnemyShot->kind = img_enemyShotMediumBall[4];  // 青〜紫系（インデックスは調整可）

            // パラメータで状態管理
            pEnemyShot->param_i[0] = 0;        // 状態：0=接近中, 1=ターン1(背後へ), 2=帰還中
            pEnemyShot->param_d[0] = 0.0;      // ターン開始時の距離記憶用
            pEnemyShot->param_i[1] = 0;        // ターン回数

            // リンク
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 各弾の更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double dx = player.x - pEnemyShot->x;
        double dy = player.y - pEnemyShot->y;
        double dist = sqrt(dx * dx + dy * dy);

        int& state = pEnemyShot->param_i[0];
        int& turnCount = pEnemyShot->param_i[1];

        switch (state) {
        case 0: // 接近中（遅い + 弱誘導）
            if (dist < playerDistThreshold && turnCount == 0) {
                // ターン開始
                state = 1;
                turnCount = 1;
                pEnemyShot->param_d[0] = dist; // 記憶（必要なら）
                pEnemyShot->speed = 4.2;       // 急加速
            }
            else {
                // 弱い誘導
                double targetAngle = atan2(dy, dx);
                pEnemyShot->muki = pEnemyShot->muki * 0.92 + targetAngle * 0.08;
            }
            break;

        case 1: // ターン1：背後へ加速進行
            if (turnCount == 1 && (dist > 180.0 || pEnemyShot->y < 50.0 || pEnemyShot->y > 430.0)) {
                // 十分背後に抜けたらUターン
                state = 2;
                turnCount = 2;
                pEnemyShot->speed = 5.8;  // 帰還加速
            }
            break;

        case 2: // 帰還中（強い誘導）
            // 強めの誘導でプレイヤーに戻る
        {
            double targetAngle = atan2(dy, dx);
            pEnemyShot->muki = pEnemyShot->muki * 0.95 + targetAngle * 0.05;
        }
        break;
        }

        // 移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// =============================================
// 敵本体パターン
// =============================================
void EnemyPat_Reverse_Grok()
{
    static int muki = 1;
    static int shotTimer = 0;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotTimer = 0;
    }
    else {
        // 敵の動き：ゆったり左右往復
        enemy.x += 1.35 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;

        // 軽い上下揺れ
        enemy.y = 60.0 + sin(count / 45.0) * 12.0;
    }

    // 定期的に弾幕発射
    shotTimer++;
    if (shotTimer >= 55) {  // 約55フレームごとに発射（調整可）
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGhostReturn;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;

        // プレイヤー方向をベースに発射
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 弾リスト初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾セットリストに連結
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shotTimer = 0;
    }
}