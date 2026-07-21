// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕パターン：六角形の輪（Phase 1：巣の構築）
// ============================================================
static void ShotHoneycombRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 回転中心を記録（敵の初期位置）
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[2] = 0.0; // 回転角度
        pEnemyShotSet->param_d[3] = 60.0; // 半径

        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = (DX_PI / 3.0) * i;
            pEnemyShot->x = pEnemyShotSet->param_d[0] + pEnemyShotSet->param_d[3] * cos(angle);
            pEnemyShot->y = pEnemyShotSet->param_d[1] + pEnemyShotSet->param_d[3] * sin(angle);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0; // 回転のみ、自前で位置更新
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 回転中心を敵位置に緩やかに追従
    pEnemyShotSet->param_d[0] += (enemy.x - pEnemyShotSet->param_d[0]) * 0.05;
    pEnemyShotSet->param_d[1] += (enemy.y - pEnemyShotSet->param_d[1]) * 0.05;
    pEnemyShotSet->param_d[2] += 0.02; // 回転速度

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    int idx = 0;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pEnemyShotSet->param_d[2] + (DX_PI / 3.0) * idx;
        pShot->x = pEnemyShotSet->param_d[0] + pEnemyShotSet->param_d[3] * cos(angle);
        pShot->y = pEnemyShotSet->param_d[1] + pEnemyShotSet->param_d[3] * sin(angle);
        pShot->muki = angle;

        if (pShot->count == 390) pShot->margin = -9999;

        pShot = pShot->next;
        idx++;
    }
}

// ============================================================
//  弾幕パターン：工蜂の出撃（Phase 2）
// ============================================================
static void ShotWorkerBee(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = pEnemyShotSet->muki + (DX_PI / 3.0) * i;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.0;
            pEnemyShot->kind = img_enemyShotScale[(i % 2 == 0) ? 1 : 8]; // 黄/橙
            pEnemyShot->param_i[0] = 0; // 行動フェーズ
            pEnemyShot->margin = 200;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 直進フェーズ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            if (pShot->count > 60) {
                pShot->param_i[0] = 1;
                // プレイヤー方向へ向き変更
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 3.5;
            }
        }
        else {
            // 追撃フェーズ（針の突き）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕パターン：女王の一刺し（Phase 3）
// ============================================================
static void ShotQueenSting(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = pEnemyShotSet->muki + i * 0.22;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 8.0;
            pEnemyShot->kind = img_enemyShotLaser[(i == 0) ? 1 : 0]; // 中央黄、両脇赤
            pEnemyShot->param_i[0] = 0; // 残りフレーム

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            if (pShot->count > 30) {
                pShot->param_i[0] = 1;
                // 蜜痕から小玉拡散
                for (int j = 0; j < 16; j++) {
                    sEnemyShot* pDrop = new sEnemyShot;
                    double dropAngle = (DX_PI / 8.0) * j;
                    pDrop->x = pShot->x;
                    pDrop->y = pShot->y;
                    pDrop->muki = dropAngle;
                    pDrop->speed = 1.5 + GetRand(10) / 10.0;
                    pDrop->kind = img_enemyShotSmallBall[GetRand(2) + 1]; // 黄/緑/シアン
                    pDrop->param_i[0] = 1; // 通常移動

                    pDrop->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pDrop->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pDrop;
                    pEnemyShotSet->pEnemyShotHead->prev = pDrop;
                }
            }
        }
        else {
            // 拡散後は通常移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕パターン：巣の崩壊（Phase 4）
// ============================================================
static void ShotHiveCollapse(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 30; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = (DX_PI / 10.0) * i + GetRand(5) / 100.0; // わずかにランダム
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.0 + GetRand(30) / 10.0;
            pEnemyShot->kind = img_enemyShotLargeBall[GetRand(3) + 1]; // 黄/緑/シアン/橙

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン：蜂モチーフ「蜜蝋六重奏」
// ============================================================
void EnemyPat_Bee_Kimi()
{
    static int phase;
    static int phaseTimer;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseTimer = 0;
    }

    // 敵の動き：中央付近でゆらゆら
    enemy.x = 240.0 + 60.0 * sin(count * 0.015);
    enemy.y = 80.0 + 20.0 * sin(count * 0.025);

    phaseTimer++;

    // Phase 0：巣の構築（0-180フレーム）
    if (phase == 0 && phaseTimer == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHoneycombRing;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    if (phase == 0 && phaseTimer >= 30) {
        phase = 1;
        phaseTimer = 0;
    }

    // Phase 1：工蜂の出撃（180-420フレーム、40フレーム毎）
    if (phase == 1 && phaseTimer % 40 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotWorkerBee;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = phaseTimer;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    if (phase == 1 && phaseTimer >= 240) {
        phase = 2;
        phaseTimer = 0;
    }

    // Phase 2：女王の一刺し（420-540フレーム）
    if (phase == 2 && phaseTimer == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotQueenSting;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    if (phase == 2 && phaseTimer >= 120) {
        phase = 3;
        phaseTimer = 0;
    }

    // Phase 3：巣の崩壊（540-600フレーム）
    if (phase == 3 && phaseTimer == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHiveCollapse;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    if (phase == 3 && phaseTimer >= 60) {
        phase = 0;
        phaseTimer = 0;
    }
}