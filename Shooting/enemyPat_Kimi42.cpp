// enemyPat_pyramid.cpp
// ピラミッドモチーフ弾幕：三層の秘宝（Trinity Relic）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------
// Phase 1: 頂点の光（Apex Beam）
// 頂点からプレイヤー方向へ高速レーザーを1本発射し、
// その先端から左右に小さな菱形弾（三角モチーフ）が散らばる。
// ---------------------------------------------------------
static void ShotApex(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回フレーム：レーザー本体を生成
    if (pEnemyShotSet->count == 0) {
        // 予告音（頂点が光る演出）
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // レーザー（黄）：古代の光をイメージ
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 10.0;
        pEnemyShot->kind = img_enemyShotLaser[1]; // 1:黄

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 先端位置計算用に方向・速度を保存
        pEnemyShotSet->param_d[0] = pEnemyShotSet->muki;
        pEnemyShotSet->param_d[1] = 10.0;
    }

    // レーザー先端の座標を計算（等速直線運動を想定）
    double tipX = pEnemyShotSet->x + pEnemyShotSet->count * pEnemyShotSet->param_d[1] * cos(pEnemyShotSet->param_d[0]);
    double tipY = pEnemyShotSet->y + pEnemyShotSet->count * pEnemyShotSet->param_d[1] * sin(pEnemyShotSet->param_d[0]);

    // 6フレーム毎に先端から左右へ小弾を生成（60フレームまで）
    // GetRand(x) は 0〜x の x+1 種類を返す
    if (pEnemyShotSet->count % 3 == 0 && pEnemyShotSet->count > 0 && pEnemyShotSet->count < 60) {
        for (int i = -1; i <= 1; i += 2) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = tipX;
            pEnemyShot->y = tipY;
            // レーザー方向から ±30度ずらした方向へ
            pEnemyShot->muki = pEnemyShotSet->param_d[0] + (double)i * DX_PI / 6.0;
            pEnemyShot->speed = 2.2;
            // 白い菱形弾：三角のかけらをイメージ
            pEnemyShot->kind = img_enemyShotDiamond[6]; // 6:白

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------------------------------------
// Phase 2: 内部通路（Inner Labyrinth）
// ピラミッド3つの側面から、渦巻き状に弾が広がる。
// 左側面は右回り、右側面は左回り、正面は緩やかに右回り。
// ---------------------------------------------------------
static void ShotLabyrinth(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 3つの入口（左、中央、右）から各10発
        for (int e = 0; e < 3; e++) {
            double baseX = pEnemyShotSet->x + (e - 1) * 45.0; // -45, 0, +45
            double baseY = pEnemyShotSet->y + 15.0;

            for (int i = 0; i < 20; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = baseX;
                pEnemyShot->y = baseY;
                double baseAngle = (DX_PI * 2.0 / 10.0) * i;
                pEnemyShot->muki = baseAngle;
                pEnemyShot->speed = 1.2 + i * 0.2; // 内側が遅い
                // シアン中玉：神秘の光をイメージ
                pEnemyShot->kind = img_enemyShotMediumBall[3]; // 3:シアン

                // 回転方向：左は右回り(+)、右は左回り(-)、中央は緩やかに右回り
                if (e == 0) pEnemyShot->param_d[0] = 0.025;
                else if (e == 2) pEnemyShot->param_d[0] = -0.025;
                else pEnemyShot->param_d[0] = 0.015;

                pEnemyShot->param_d[0] *= 0.5;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 螺旋更新：毎フレーム方向を少し回転させる
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->muki += pEnemyShot->param_d[0];
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------------------------------------
// Phase 3: 基部の封印（Base Seal）
// 底辺4隅から弾が上昇し、頂点で3方向（正四面体風）に分裂。
// 同時に全方位へ大玉の衝撃波を発生させる。
// ---------------------------------------------------------
static void ShotBaseSeal(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 底辺4隅（ピラミッドの底辺をイメージ）
        double corners[4][2] = {
            {pEnemyShotSet->x - 50.0, pEnemyShotSet->y + 35.0},
            {pEnemyShotSet->x + 50.0, pEnemyShotSet->y + 35.0},
            {pEnemyShotSet->x - 25.0, pEnemyShotSet->y + 20.0},
            {pEnemyShotSet->x + 25.0, pEnemyShotSet->y + 20.0}
        };

        // 頂点座標（敵より少し上）
        double apexX = pEnemyShotSet->x;
        double apexY = pEnemyShotSet->y - 30.0;

        // 同心円状衝撃波：全方位に黄大玉
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 40.0;
            pEnemyShot->muki = (DX_PI * 2.0 / 12.0) * i;
            pEnemyShot->speed = 1.8;
            pEnemyShot->kind = img_enemyShotLargeBall[1]; // 1:黄

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 4隅から頂点へ向かう弾（橙鱗弾：古代の守護者のイメージ）
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = corners[i][0];
            pEnemyShot->y = corners[i][1];
            pEnemyShot->muki = atan2(apexY - corners[i][1], apexX - corners[i][0]);
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotScale[8]; // 8:橙
            pEnemyShot->param_i[0] = 0; // 0:上昇中
            pEnemyShot->param_d[0] = apexX;
            pEnemyShot->param_d[1] = apexY;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 更新：上昇中の弾が頂点付近に到達したら3方向に分裂
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 0) {
            double dx = pEnemyShot->param_d[0] - pEnemyShot->x;
            double dy = pEnemyShot->param_d[1] - pEnemyShot->y;
            // 25px以内で到達判定
            if (dx * dx + dy * dy < 625.0) {
                pEnemyShot->param_i[0] = 1; // 分裂済みに変更
                // 正四面体風：3方向（120度間隔）に分岐
                // GetRand(2) は 0,1,2 の3種類を返す
                int dir = GetRand(2);
                double baseAngles[3] = { DX_PI / 2.0, DX_PI * 7.0 / 6.0, DX_PI * 11.0 / 6.0 };
                // ±15度のランダムばらつき（GetRand(30) は 0〜30 の31種類）
                pEnemyShot->muki = baseAngles[dir] + (GetRand(30) - 15) / 180.0 * DX_PI;
                pEnemyShot->speed = 2.5;
            }
        }
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------------------------------------
// 敵本体：ピラミッド型ボス
// 3つのPhaseを300フレーム毎に切り替えて繰り返す。
// ---------------------------------------------------------
void EnemyPat_Pyramid_Kimi()
{
    static int phase = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 300;
        phase = 0;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += sin((double)count / 180.0 * DX_PI) * 0.7;
    }

    // 300フレーム毎にPhase切り替え
    if (count % 300 == 1) {
        phase = (phase + 1) % 3;
    }

    // 90フレーム毎に弾幕セットを生成
    if (count % 90 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        pEnemyShotSet->kind = phase;

        // Phaseに応じて関数を割り当て
        switch (phase) {
        case 0:
            pEnemyShotSet->patternFunc = ShotApex;
            break;
        case 1:
            pEnemyShotSet->patternFunc = ShotLabyrinth;
            break;
        case 2:
            pEnemyShotSet->patternFunc = ShotBaseSeal;
            break;
        }

        // 弾リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}