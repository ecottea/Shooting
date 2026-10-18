#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 銃符「ルナティックガン」用 弾幕パターン関数（高難易度版）
// ============================================================

// 1. 固定の鈍足銃弾列（塀）を形成するパターン
static void ShotLunaticGunWall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 列数を増やし、隙間を狭くして通過経路を限定する
        const int numWalls = 7;
        const double baseAngles[] = { -0.8, -0.53, -0.26, 0.0, 0.26, 0.53, 0.8 };

        for (int i = 0; i < numWalls; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngles[i];
            pEnemyShot->speed = 1.0; // 鈍足な速度で密度で制限する

            // 青い銃弾で壁を表現
            pEnemyShot->kind = img_enemyShotBullet[4];

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

// 2. 自機依存の銃弾塊で圧殺するパターン（高密度・連続攻撃）
static void ShotLunaticGunCluster(sEnemyShotSet* pEnemyShotSet)
{
    // count を使って、複数波の攻撃を行う（0フレーム目と20フレーム目に発射）
    if (pEnemyShotSet->count == 0 || pEnemyShotSet->count == 20) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        double aimMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 弾数を増やし、広がり角度を広げて誘導を難しくする
        const int numCluster = 13;
        const double spreadAngle = 0.9;

        for (int i = 0; i < numCluster; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double angleOffset = -spreadAngle / 2.0 + (spreadAngle / (numCluster - 1)) * i;
            pEnemyShot->muki = aimMuki + angleOffset;
            pEnemyShot->speed = 3.2; // 速度を上げて圧殺感を強める

            // シアンの中玉で塊を表現
            pEnemyShot->kind = img_enemyShotMediumBall[3];

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
// 敵本体のパターン
// ============================================================
void EnemyPat_LunaticGun_Qwen()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 移動速度を上げ、ボス周り回避時のリスクを高める
        enemy.x += 1.2 * (double)muki;
        if (count % 100 == 50) {
            muki *= -1;
        }
    }

    // 1. 壁の形成
    if (count % 200 == 30) {
        sEnemyShotSet* pWallSet = new sEnemyShotSet;
        pWallSet->count = 0;
        pWallSet->patternFunc = ShotLunaticGunWall;
        pWallSet->x = enemy.x;
        pWallSet->y = enemy.y + 10.0;
        pWallSet->muki = DX_PI / 2.0;
        pWallSet->kind = 0;
        pWallSet->pEnemyShotHead = new sEnemyShot;
        pWallSet->pEnemyShotHead->prev = pWallSet->pEnemyShotHead;
        pWallSet->pEnemyShotHead->next = pWallSet->pEnemyShotHead;

        pWallSet->prev = enemyShotSetHead.prev;
        pWallSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWallSet;
        enemyShotSetHead.prev = pWallSet;
    }
    // 2. 圧殺弾塊（1波目）
    else if (count % 200 == 90) {
        sEnemyShotSet* pClusterSet = new sEnemyShotSet;
        pClusterSet->count = 0;
        pClusterSet->patternFunc = ShotLunaticGunCluster;
        pClusterSet->x = enemy.x;
        pClusterSet->y = enemy.y + 10.0;
        pClusterSet->muki = 0.0;
        pClusterSet->kind = 1;
        pClusterSet->pEnemyShotHead = new sEnemyShot;
        pClusterSet->pEnemyShotHead->prev = pClusterSet->pEnemyShotHead;
        pClusterSet->pEnemyShotHead->next = pClusterSet->pEnemyShotHead;

        pClusterSet->prev = enemyShotSetHead.prev;
        pClusterSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pClusterSet;
        enemyShotSetHead.prev = pClusterSet;
    }
    // 3. 圧殺弾塊（2波目：誘導を狂わせるための追撃）
    else if (count % 200 == 140) {
        sEnemyShotSet* pClusterSet2 = new sEnemyShotSet;
        pClusterSet2->count = 0;
        pClusterSet2->patternFunc = ShotLunaticGunCluster;
        pClusterSet2->x = enemy.x;
        pClusterSet2->y = enemy.y + 10.0;
        pClusterSet2->muki = 0.0;
        pClusterSet2->kind = 1;
        pClusterSet2->pEnemyShotHead = new sEnemyShot;
        pClusterSet2->pEnemyShotHead->prev = pClusterSet2->pEnemyShotHead;
        pClusterSet2->pEnemyShotHead->next = pClusterSet2->pEnemyShotHead;

        pClusterSet2->prev = enemyShotSetHead.prev;
        pClusterSet2->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pClusterSet2;
        enemyShotSetHead.prev = pClusterSet2;
    }
}