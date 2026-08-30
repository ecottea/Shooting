// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
// 信号機弾幕：各フェーズの弾幕パターン関数
// ============================================================

// 青信号：安全地帯（緑色の大玉が緩やかに広がる）
static void ShotTrafficGreen(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numShots = 7;
        double startAngle = DX_PI / 2.0 - 0.6; // 下向きを中心に広げる
        double angleStep = 1.2 / (numShots - 1);

        for (int i = 0; i < numShots; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = startAngle + angleStep * i;
            pShot->speed = 1.5 + GetRand(50) / 100.0; // 1.5 ~ 2.0 (遅い)
            pShot->kind = img_enemyShotLargeBall[2];  // 緑色の大玉

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 黄信号：注意・準備（黄色の中玉がプレイヤーを追うように発射）
static void ShotTrafficYellow(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int numShots = 5;
        double baseMuki = pEnemyShotSet->muki;

        for (int i = 0; i < numShots; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            // プレイヤー方向を基準に少しずらして発射（誘導的な挙動）
            pShot->muki = baseMuki + (i - 2) * 0.25;
            pShot->speed = 3.0 + GetRand(100) / 100.0; // 3.0 ~ 4.0 (中程度)
            pShot->kind = img_enemyShotMediumBall[1];  // 黄色の中玉

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 赤信号：障害物（黒色の巨大な玉がゆっくり上へ移動し、画面外で自動消去される）
static void ShotTrafficRedBarrier(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = -DX_PI / 2.0; // 上向き
        pShot->speed = 0.3;         // 非常に遅く移動（実質的な壁として機能し、最終的に画面外へ行って消去される）
        pShot->kind = img_enemyShotLargeBall[7]; // 黒色の巨大な玉

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 赤信号：危険（赤色の小玉が高速で放射状に発射）
static void ShotTrafficRedBullet(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int numShots = 8;
        double angleStep = DX_PI * 2.0 / numShots;

        for (int i = 0; i < numShots; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = angleStep * i + (count * 0.05); // 回転しながら放射
            pShot->speed = 5.0 + GetRand(100) / 100.0;    // 5.0 ~ 6.0 (高速)
            pShot->kind = img_enemyShotSmallBall[0];      // 赤色の小玉

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
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
void EnemyPat_TrafficLight_Qwen()
{
    static int phase = 0; // 0:青, 1:黄, 2:赤

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
    }

    // 敵の動き: 左右にゆっくり往復
    enemy.x = 240.0 + 100.0 * sin(count * 0.02);
    enemy.y = 80.0 + 20.0 * sin(count * 0.03);

    // フェーズ管理 (各フェーズ 300フレーム = 約5秒, 合計15秒でループ)
    int cycle_count = (count - 1) % 900;
    if (cycle_count < 300) {
        phase = 0; // 青信号
    }
    else if (cycle_count < 600) {
        phase = 1; // 黄信号
    }
    else {
        phase = 2; // 赤信号
    }

    // 赤信号時の自機制限: 「止まれ」のルールとして、画面上部へ行きすぎないようにする
    if (phase == 2) {
        if (player.y > 280.0) {
            spawnForceParticles(player.x, player.y, 0, -5);
            spawnForceParticles(player.x, player.y, 0, -5);
            spawnForceParticles(player.x, player.y, 0, -5);
            player.y = 280.0;
            spawnForceParticles(player.x, player.y, 0, -5);
            spawnForceParticles(player.x, player.y, 0, -5);
            spawnForceParticles(player.x, player.y, 0, -5);
        }
    }

    // 各フェーズの弾幕発射トリガー
    if (phase == 0) {
        // 青信号: 15フレームごとに扇状発射
        if (cycle_count % 15 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotTrafficGreen;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = DX_PI / 2.0;
            pSet->kind = 0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    else if (phase == 1) {
        // 黄信号: 10フレームごとにプレイヤー方向へ発射
        if (cycle_count % 10 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotTrafficYellow;
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
    }
    else if (phase == 2) {
        // 赤信号: 静止障害物はフェーズ開始時に配置
        if (cycle_count == 600) {
            for (int i = 0; i < 3; i++) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotTrafficRedBarrier;
                pSet->x = 120.0 + i * 120.0; // 120, 240, 360 (画面を3レーンに分割)
                pSet->y = 240.0 + GetRand(100) - 50; // 多少ばらつかせる
                pSet->muki = 0;
                pSet->kind = i;

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        // 赤信号: 高速弾は 8フレームごとに放射状
        if (cycle_count % 8 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotTrafficRedBullet;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = 0;
            pSet->kind = 0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}