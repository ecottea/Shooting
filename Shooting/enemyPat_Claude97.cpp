// enemyPat_PerfectFreeze.cpp
// 東方紅魔郷 2面ボス・チルノの凍符「パーフェクトフリーズ」を再現した弾幕パターン。
//
// 構成(無限ループ、1サイクル = CYCLE_LENGTH フレーム):
//   1. 散乱フェーズ : 全方位にカラフルな弾をパルス状にばらまく(等速直線)
//   2. 氷結フェーズ : 一定の絶対フレームで、生成タイミングに関わらず全弾が同時に急停止する
//   3. 隙間フェーズ : 弾が凍っている間に、青の自機狙い多方向弾(4way/5wayを交互)を撃ち込む
//   4. 解凍フェーズ : 氷結していた弾が、各々ランダムな新しい向きへゆっくり加速しながら再び動き出す
//
// 位置はすべて pShot->count ではなくグローバルの count と、
// 弾生成時に param_d[]/param_i[] へ記録したパラメータから式で算出する(速度積分を行わない)。
// これにより、生成タイミングが異なる弾でも「同じ絶対フレームで一斉に凍る」挙動を実現している。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {
    // サイクル全体の長さ
    const int CYCLE_LENGTH = 420;

    // 散乱フェーズ: サイクル開始から何フレームの間、何フレームおきに弾をばらまくか
    const int SCATTER_WINDOW = 100;
    const int SCATTER_INTERVAL = 5;
    const int SCATTER_SHOT_NUM = 14; // 1パルスあたりの発射数

    // 氷結・解凍のタイミング(サイクル内の絶対フレーム)
    const int FREEZE_OFFSET = 110; // この瞬間、生成済みの全弾が同時に停止する
    const int THAW_OFFSET = 230;   // この瞬間から、各弾が新しい向きへ再加速し始める

    // 隙間フェーズ: 氷結中に撃つ自機狙い多方向弾(青)のタイミング
    const int VOLLEY_START = 130;
    const int VOLLEY_END = 220;
    const int VOLLEY_INTERVAL = 18;
    const double VOLLEY_SPREAD_DEG = 40.0; // way弾全体の広がり角(度)
    const double VOLLEY_SPEED = 2.3;

    // 解凍後の弾の挙動(じわーっと加速しながら散っていくイメージ)
    const double THAW_SPEED = 0.6;
    const double THAW_ACCEL = 0.012;
}

// ============================================================
// 弾幕: 散乱→氷結→解凍(式ベースで一斉挙動を制御)
// ============================================================
static void ShotPerfectFreezeScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < SCATTER_SHOT_NUM; i++) {
            pEnemyShot = new sEnemyShot;

            // GetRand(x) は 0〜x の x+1 種類を返すので注意。リプレイ再現性のため乱数はここでのみ消費する。
            double muki0 = GetRand(3599) / 3600.0 * 2.0 * DX_PI;      // 全方位ランダム
            double speed0 = (150 + GetRand(150)) / 100.0 * 1.3;              // 1.50〜3.00
            double thawMuki = GetRand(3599) / 3600.0 * 2.0 * DX_PI;    // 解凍後に飛ぶ新しい向き(あらかじめ決めておく)

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = muki0;

            pEnemyShot->param_d[0] = pEnemyShotSet->x;  // 発生位置 x0
            pEnemyShot->param_d[1] = pEnemyShotSet->y;  // 発生位置 y0
            pEnemyShot->param_d[2] = speed0;            // 氷結前の速さ
            pEnemyShot->param_d[3] = muki0;             // 氷結前の向き
            pEnemyShot->param_d[4] = thawMuki;           // 解凍後の向き
            pEnemyShot->param_i[0] = count;              // 発生した瞬間のグローバルフレーム(スポーン基準)

            // 弾の種類と半径一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
            // 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            // 「全方位に放たれたカラフルな弾」の再現のため、氷弾は小玉を全色ランダムに使用する。
            int colorIdx = GetRand(8);
            pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int spawnCount = pShot->param_i[0];

        // この弾が属するサイクルの氷結・解凍フレームを、生成フレームから逆算する。
        // (無限ループでもサイクルをまたいで正しいタイミングを維持するため)
        int cycleIndex = (spawnCount - 1) / CYCLE_LENGTH;
        int freezeFrame = cycleIndex * CYCLE_LENGTH + FREEZE_OFFSET;
        int thawFrame = cycleIndex * CYCLE_LENGTH + THAW_OFFSET;

        double x0 = pShot->param_d[0];
        double y0 = pShot->param_d[1];
        double speed0 = pShot->param_d[2];
        double muki0 = pShot->param_d[3];
        double thawMuki = pShot->param_d[4];

        if (count <= freezeFrame) {
            // --- 散乱フェーズ: 等速直線で飛ぶ ---
            double t = (double)(count - spawnCount);
            pShot->x = x0 + speed0 * cos(muki0) * t;
            pShot->y = y0 + speed0 * sin(muki0) * t;
            pShot->muki = muki0;
        }
        else {
            // 氷結した瞬間の位置(以降はこの座標が解凍の起点になる)
            double preT = (double)(freezeFrame - spawnCount);
            double fx = x0 + speed0 * cos(muki0) * preT;
            double fy = y0 + speed0 * sin(muki0) * preT;

            if (count <= thawFrame) {
                // --- 氷結フェーズ: その場で完全停止 ---
                pShot->x = fx;
                pShot->y = fy;
                pShot->muki = muki0;
            }
            else {
                // --- 解凍フェーズ: 新しい向きへゆるやかに加速しながら再始動 ---
                double thawT = (double)(count - thawFrame);
                double dist = THAW_SPEED * thawT + 0.5 * THAW_ACCEL * thawT * thawT;
                pShot->x = fx + cos(thawMuki) * dist;
                pShot->y = fy + sin(thawMuki) * dist;
                pShot->muki = thawMuki;
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕: 氷結中に隙間を突く、自機狙いの青い多方向弾(4way/5way)
// ============================================================
static void ShotPerfectFreezeVolley(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int wayNum = pEnemyShotSet->kind; // 呼び出し側で4か5を設定しておく
        double baseMuki = pEnemyShotSet->muki; // 呼び出し側で計算済みの自機狙い角
        double spreadTotal = VOLLEY_SPREAD_DEG / 180.0 * DX_PI;

        for (int i = 0; i < wayNum; i++) {
            pEnemyShot = new sEnemyShot;

            double offset = (wayNum == 1) ? 0.0
                : (-spreadTotal / 2.0 + spreadTotal * i / (wayNum - 1));
            double muki0 = baseMuki + offset;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = muki0;

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = VOLLEY_SPEED;
            pEnemyShot->param_d[3] = muki0;
            pEnemyShot->param_i[0] = count;

            // 元ネタの「青弾」を再現するため、中玉の青(色4)を使用。
            pEnemyShot->kind = img_enemyShotMediumBall[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double x0 = pShot->param_d[0];
        double y0 = pShot->param_d[1];
        double speed0 = pShot->param_d[2];
        double muki0 = pShot->param_d[3];
        int spawnCount = pShot->param_i[0];

        double t = (double)(count - spawnCount);
        pShot->x = x0 + speed0 * cos(muki0) * t;
        pShot->y = y0 + speed0 * sin(muki0) * t;
        // muki は自機狙い方向で固定のため更新不要

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン: 凍符「パーフェクトフリーズ」
// ============================================================
void EnemyPat_PerfectFreeze_Claude()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆったりとした左右移動
        enemy.x += 0.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    int cyclePos = (count - 1) % CYCLE_LENGTH;

    // 氷結する瞬間の予告音(演出)
    if (cyclePos == FREEZE_OFFSET) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- 散乱フェーズ: パルス状にカラフルな弾をばらまく ---
    if (cyclePos < SCATTER_WINDOW && cyclePos % SCATTER_INTERVAL == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPerfectFreezeScatter;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // --- 隙間フェーズ: 氷結中に自機狙い多方向弾(4way/5wayを交互)を撃ち込む ---
    if (cyclePos >= VOLLEY_START && cyclePos < VOLLEY_END
        && (cyclePos - VOLLEY_START) % VOLLEY_INTERVAL == 0) {

        int volleyIndex = (cyclePos - VOLLEY_START) / VOLLEY_INTERVAL;
        int wayNum = (volleyIndex % 2 == 0) ? 4 : 5;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPerfectFreezeVolley;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = wayNum;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // count, pEnemyShotSet->count, pEnemyShot->count のインクリメントと、
    // 画面外に出た弾/空になった弾セットの削除はメインルーチン側の仕様に委ねる。
}