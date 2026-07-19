// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ヘルパー関数
// ============================================================
// 弾幕セットをリストに追加する
static void AddShotSet(void(*func)(sEnemyShotSet*), double x, double y, double muki)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = 0;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ============================================================
//  弾幕パターン関数
// ============================================================

// 1. 突風ライン (高速レーザー)
// 素材: img_enemyShotLaser[6] (白)
static void ShotGust(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        // 8方向に放射状、あるいはプレイヤー狙いで発射
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = pSet->muki + i * (DX_PI * 2 / 8);
            pShot->speed = 9.0; // 非常に高速
            pShot->kind = img_enemyShotLaser[6]; // 白レーザー

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 2. 砂塵の渦 (視界阻害・滞留)
// 素材: img_enemyShotSmallBall[8] (橙)
static void ShotSandDust(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 32; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = i * (DX_PI * 2 / 32);
            pShot->speed = 0.0; // 極座標で制御するため速度は0
            pShot->kind = img_enemyShotSmallBall[8]; // 橙色の小玉

            // param_d[0]: 半径, param_d[1]: 角度
            pShot->param_d[0] = 10.0; // 初期半径
            pShot->param_d[1] = pShot->muki;
            pShot->margin = 350;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 渦巻き: 半径を増やしつつ、角度を回転させる
        pShot->param_d[0] += 0.6;
        pShot->param_d[1] += 0.08;

        // 極座標からデカルト座標へ変換
        pShot->x = pSet->x + pShot->param_d[0] * cos(pShot->param_d[1]);
        pShot->y = pSet->y + pShot->param_d[0] * sin(pShot->param_d[1]);

        pShot = pShot->next;
    }
}

// 3. 岩石弾 (蛇行する大きな岩)
// 素材: img_enemyShotLargeBall[0] (赤)
static void ShotRock(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = pSet->muki + (i - 1) * 0.3; // 多少ばらまく
            pShot->speed = 3.5;
            pShot->kind = img_enemyShotLargeBall[0]; // 赤大玉

            // param_d[0]: サインカーブの位相
            pShot->param_d[0] = GetRand(100) / 100.0 * DX_PI * 2;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->param_d[0] += 0.1;
        // 進行方向をサインカーブで変化させ、蛇行させる
        double target_muki = pSet->muki + 0.6 * sin(pShot->param_d[0]);
        pShot->muki += (target_muki - pShot->muki) * 0.1; // なめらかに追従

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 4. 砂丘の崩落 (上から降り注ぐ弾幕)
// 素材: img_enemyShotLargeBall[0] (赤) と img_enemyShotSmallBall[8] (橙) の混合
static void ShotAvalanche(sEnemyShotSet* pSet)
{
    if (pSet->count % 10 == 0) { // 4フレームごとに生成
        if (pSet->count % 20 == 0) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        for (int i = 0; i < 6; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = GetRand(480); // 画面幅480の任意の位置
            pShot->y = -30.0;
            pShot->muki = DX_PI / 2 + (GetRand(20) - 10) / 100.0; // ほぼ下向き
            pShot->speed = 3.0 + GetRand(20) / 10.0;

            // 岩と砂塵をランダムに混合
            if (GetRand(3) == 0) {
                pShot->kind = img_enemyShotLargeBall[0]; // 赤大玉
            }
            else {
                pShot->kind = img_enemyShotSmallBall[8]; // 橙小玉
            }
            pShot->margin = 40;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_SandStorm_Qwen()
{
    static int phase;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
    }
    else {
        // 砂嵐のようにゆらゆらと移動
        enemy.x = 240.0 + 50.0 * sin(count * 0.015);
        enemy.y = 60.0 + 15.0 * sin(count * 0.025);
    }

    // フェーズ管理
    if (count == 60)  phase = 1; // 突風
    if (count == 240) phase = 2; // 砂塵
    if (count == 420) phase = 3; // 岩石
    if (count == 600) phase = 4; // 崩落

    // --- フェーズ1: 突風ライン ---
    if (phase == 1 && count % 20 == 0) {
        double muki = atan2(player.y - enemy.y, player.x - enemy.x);
        AddShotSet(ShotGust, enemy.x, enemy.y, muki);
    }

    // --- フェーズ2: 砂塵の渦 ---
    if (phase == 2 && count % 15 == 0) {
        AddShotSet(ShotSandDust, enemy.x, enemy.y, 0);
    }

    // --- フェーズ3: 岩石弾 ---
    if (phase == 3 && count % 45 == 0) {
        double muki = atan2(player.y - enemy.y, player.x - enemy.x);
        AddShotSet(ShotRock, enemy.x, enemy.y, muki);
    }

    // --- フェーズ4: 砂丘の崩落 ---
    if (phase == 4) {
        AddShotSet(ShotAvalanche, 0, 0, 0);
        phase = -1;
    }
}