// enemyPat_henyori.cpp
// へにょりレーザーパターン実装（1パターン）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ========================
// へにょりレーザー本体処理
// ========================
static void ShotHenyoriLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射時（count==0）
    if (pEnemyShotSet->count == 0) {
        // 効果音（中くらいのレーザー音）
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 8本のへにょりレーザーを扇状に発射
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 10.0;

            // 基本角度（プレイヤー方向を中心に扇状に広げる）
            double baseAngle = pEnemyShotSet->muki + (i - 3.5) * 0.25;
            pEnemyShot->muki = baseAngle;

            pEnemyShot->speed = 2.8;  // 基本速度

            // 弾の見た目：中楕円弾をレーザーっぽく使用（色は橙系）
            pEnemyShot->kind = img_enemyShotMediumOval[8];  // 橙

            // パラメータで波を制御
            pEnemyShot->param_d[0] = baseAngle;      // 基準角度
            pEnemyShot->param_d[1] = 0.18 + i * 0.015; // 周波数（徐々に変化）
            pEnemyShot->param_d[2] = 0.6 + (i % 3) * 0.3; // 振幅（徐々に大きく）
            pEnemyShot->param_i[0] = i;  // 位相差用

            // 連結リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // へにょり波：基準角度 + 時間で振動
        double wave = sin(pShot->count * pShot->param_d[1] + pShot->param_i[0] * 0.8) * pShot->param_d[2];
        pShot->muki = pShot->param_d[0] + wave;

        // 進行
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 波の頂点付近で針弾追加（追い打ち）
        if (pShot->count % 16 == 0 && fabs(sin(pShot->count * pShot->param_d[1])) > 0.85) {
            sEnemyShot* pNeedle = new sEnemyShot;
            pNeedle->x = pShot->x;
            pNeedle->y = pShot->y;
            pNeedle->muki = pShot->muki + (GetRand(2) - 1) * 0.4;
            pNeedle->speed = 3.5;
            pNeedle->kind = img_enemyShotBullet[8];  // 橙の針弾

            pNeedle->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNeedle->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNeedle;
            pEnemyShotSet->pEnemyShotHead->prev = pNeedle;
        }

        pShot = pShot->next;
    }
}

// ========================
// 敵本体パターン
// ========================
void EnemyPat_HenyoriLaser_Grok()
{
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1) {
        // 初期位置（画面上部中央）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右ゆっくり移動
        enemy.x += 1.1 * (double)muki;
        if (count % 140 == 70) muki *= -1;
    }

    // 定期的にへにょりレーザー発射
    if (count % 45 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHenyoriLaser;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;

        // プレイヤー方向を基本に
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 連結リストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}