// enemyPat_ElectricMaze.cpp
// イライラ棒モチーフ弾幕「電網迷路フラッシュ・メイズ」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 前方宣言
static void WireMazePattern(sEnemyShotSet* pSet);
static void SparkPattern(sEnemyShotSet* pSet);

// ------------------------------------------------------------
// 電網本体（ワイヤー）を生成・更新するパターン関数
// ------------------------------------------------------------
static void WireMazePattern(sEnemyShotSet* pSet)
{
    // === 全弾を一度クリア（毎フレーム描き直し） ===
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;
        pShot->prev->next = pShot->next;
        pShot->next->prev = pShot->prev;
        delete pShot;
        pShot = nextShot;
    }

    // === パラメータ設定 ===
    const double AMPLITUDE = 250.0;             // 振幅
    const double CYCLES = 2.5;               // 波の数（画面幅で約2.5周期）
    const double PHASE_SPEED = 0.04;              // 位相変化速度
    const double GAP_SPEED = 0.008;             // 隙間の移動速度（t単位/フレーム）
    const double GAP_HALF_WIDTH = 0.045;             // 隙間の半分の幅（t値）
    const double WIRE_Y = enemy.y + 200.0;   // ワイヤーのY中心
    const double X_MIN = 0.0;              // 左端（マージン考慮）
    const double X_MAX = 480.0;             // 右端
    const double SPACING = 1.5;               // 弾の間隔（ピクセル）

    const double xRange = X_MAX - X_MIN;             // 440.0
    const double tStep = SPACING / xRange;          // tの刻み幅

    // 現在の位相
    double phase = pSet->count * PHASE_SPEED;

    // 二つの隙間の中心位置（t空間 0.0～1.0）
    double gap1Center = fmod(pSet->count * GAP_SPEED, 1.0);
    double gap2Center = fmod(gap1Center + 0.5, 1.0);

    // ワイヤーを構成する全てのtについて弾を生成
    for (double t = 0.0; t <= 1.0; t += tStep) {
        // 隙間に該当するか検査（円環的に判定）
        double dist1 = fmod(t - gap1Center + 0.5, 1.0) - 0.5;
        double dist2 = fmod(t - gap2Center + 0.5, 1.0) - 0.5;
        if (fabs(dist1) < GAP_HALF_WIDTH || fabs(dist2) < GAP_HALF_WIDTH) {
            continue; // 隙間なので弾を置かない
        }

        // 座標計算
        double x = X_MIN + t * xRange;
        double y = WIRE_Y + AMPLITUDE * sin(2.0 * DX_PI * CYCLES * t + phase);

        // 新規弾を生成
        sEnemyShot* pNew = new sEnemyShot;
        pNew->x = x;
        pNew->y = y;
        pNew->muki = 0.0;
        pNew->speed = 0.0;
        pNew->count = 0;            // メインでインクリメントされる
        pNew->kind = img_enemyShotSmallBall[0]; // 赤色小玉でワイヤーを表現

        // リンクリストに追加
        pNew->prev = pSet->pEnemyShotHead->prev;
        pNew->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pNew;
        pSet->pEnemyShotHead->prev = pNew;
    }
}

// ------------------------------------------------------------
// 誘導スパーク弾（プレイヤー狙い、隙間通過を妨害）
// ------------------------------------------------------------
static void SparkPattern(sEnemyShotSet* pSet)
{
    // 初回のみ効果音再生
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 3発の弾を自機狙いで発射（最初のフレームだけ）
    if (pSet->count == 0) {
        const int BULLET_NUM = 3;
        for (int i = 0; i < BULLET_NUM; ++i) {
            sEnemyShot* pShot = new sEnemyShot;

            // 発射位置：敵本体付近、わずかにばらつかせる
            pShot->x = enemy.x + GetRand(30) - 15.0;
            pShot->y = enemy.y + 20.0 + GetRand(20);

            // 自機へ向かう角度（基本角）に ±15° のランダム分散
            double baseAngle = atan2(player.y - pShot->y, player.x - pShot->x);
            double angleOffset = (GetRand(30) - 15.0) / 180.0 * DX_PI;
            pShot->muki = baseAngle + angleOffset;
            pShot->speed = 2.5 + GetRand(100) / 100.0 * 0.8; // 2.5～3.3

            pShot->count = 0;
            pShot->kind = img_enemyShotBullet[5]; // マゼンタ色の銃弾（スパーク感）

            // リンク
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // それ以降は弾の移動だけ（メインルーチンが自動で座標更新してくれるのでここでは何もしない）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン（エントリポイント）
// ------------------------------------------------------------
void EnemyPat_Irairabou_DeepSeek()
{
    // 初回フレームの初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;

        // メインのワイヤー迷路（持続パターン）を発動
        sEnemyShotSet* pWireSet = new sEnemyShotSet;
        pWireSet->count = 0;
        pWireSet->patternFunc = WireMazePattern;
        pWireSet->x = enemy.x;
        pWireSet->y = enemy.y;
        pWireSet->muki = 0.0;
        pWireSet->kind = 0;

        pWireSet->pEnemyShotHead = new sEnemyShot;
        pWireSet->pEnemyShotHead->prev = pWireSet->pEnemyShotHead;
        pWireSet->pEnemyShotHead->next = pWireSet->pEnemyShotHead;

        // グローバルリストに登録
        pWireSet->prev = enemyShotSetHead.prev;
        pWireSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWireSet;
        enemyShotSetHead.prev = pWireSet;
    }

    // 定期的にスパーク弾を発射（90フレームごと）
    if (count % 90 == 1) {
        sEnemyShotSet* pSparkSet = new sEnemyShotSet;
        pSparkSet->count = 0;
        pSparkSet->patternFunc = SparkPattern;
        pSparkSet->x = enemy.x;
        pSparkSet->y = enemy.y;
        pSparkSet->muki = 0.0;
        pSparkSet->kind = 0;

        pSparkSet->pEnemyShotHead = new sEnemyShot;
        pSparkSet->pEnemyShotHead->prev = pSparkSet->pEnemyShotHead;
        pSparkSet->pEnemyShotHead->next = pSparkSet->pEnemyShotHead;

        pSparkSet->prev = enemyShotSetHead.prev;
        pSparkSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSparkSet;
        enemyShotSetHead.prev = pSparkSet;
    }
}