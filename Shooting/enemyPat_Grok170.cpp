#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 転世「一条戻り橋」用 戻り弾幕パターン
// 画面外（版辺）から生成され、敵本体（慧音）へ向かって収束する粒弾
static void ShotReturningBridge(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 画面外半径（中心から十分外側）
    const double outer_radius = 380.0+50;

    // 発射タイミング（16フレーム周期で完全同期）
    if (pEnemyShotSet->count % 16 == 0 && pEnemyShotSet->count < 16) {
        // 青粒弾用効果音（軽め）
        if (pEnemyShotSet->kind == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        // 赤粒弾用効果音（中）
        else {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 4way × 8連 の合計32発を、角度をずらして配置
        const int num_groups = 8;
        const int ways = 4;
        const double group_step = 2.0 * DX_PI / num_groups;
        const double way_offset = 0.08; // 4wayの微小角度オフセット

        // 現在の基準角度（param_d[0] に保持）
        double base_angle = pEnemyShotSet->param_d[0];

        for (int g = 0; g < num_groups; g++) {
            for (int w = 0; w < ways; w++) {
                double ang = base_angle + g * group_step + (w - 1.5) * way_offset;

                // 画面外の生成位置（敵を中心とした円周上）
                double sx = pEnemyShotSet->x + outer_radius * cos(ang);
                double sy = pEnemyShotSet->y + outer_radius * sin(ang);

                // 敵本体へ向かう向き
                double dx = pEnemyShotSet->x - sx;
                double dy = pEnemyShotSet->y - sy;
                double muki = atan2(dy, dx);

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = sx;
                pEnemyShot->y = sy;
                pEnemyShot->muki = muki;
                pEnemyShot->speed = 2.4 + (pEnemyShotSet->kind == 0 ? 0.15 : 0.0); // 青をわずかに速く
                pEnemyShot->margin = 480;

                // 小玉（粒弾）を使用。青=4、赤=0
                if (pEnemyShotSet->kind == 0) {
                    pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青
                }
                else {
                    pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤
                }

                // リスト接続
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 角度を進める（解析値を再現）
        // 青: 1周期あたり -2π/15
        // 赤: 1周期あたり +4π/35
        if (pEnemyShotSet->kind == 0) {
            pEnemyShotSet->param_d[0] -= (2.0 * DX_PI / 15.0);
        }
        else {
            pEnemyShotSet->param_d[0] += (4.0 * DX_PI / 35.0);
        }
    }

    // 弾の移動（直線で敵方向へ）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        if (pShot->count * pShot->speed >= outer_radius) pShot->margin = -9999;

        pShot = pShot->next;
    }
}

// 敵本体パターン
// 転世「一条戻り橋」
void EnemyPat_Modoribashi_Grok()
{
    static double base_angle_blue;
    static double base_angle_red;
    static int wave_count;

    if (count == 1) {
        // 初期配置（画面中央上部）
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 変更禁止

        // 開幕角度のみランダム（以降は完全パターン）
        // GetRand(x) は 0 以上 x 以下の x+1 種類
        base_angle_blue = (GetRand(359) / 180.0) * DX_PI;
        base_angle_red = base_angle_blue + (DX_PI / 2.0); // 相対角度をずらして開始
        wave_count = 0;
    }
    else {
        // わずかに左右に揺れる程度の移動（パターンの邪魔にならない範囲）
        enemy.x = 240.0 + 30.0 * sin(count * 0.015);
        enemy.y = 80.0 + 8.0 * sin(count * 0.011);
    }

    // 16フレームごとに青・赤のショットセットを生成（完全同期）
    if (count % 18 == 1) {
        // 青粒弾セット
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotReturningBridge;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0; // 青
            pSet->param_d[0] = base_angle_blue; // 基準角度を渡す
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 赤粒弾セット
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotReturningBridge;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 1; // 赤
            pSet->param_d[0] = base_angle_red;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 次の波のために基準角度を更新（解析値）
        // 32フレームごとに相対角度が π/105 ずれる関係を維持
        base_angle_blue -= (2.0 * DX_PI / 15.0);
        base_angle_red += (4.0 * DX_PI / 35.0);

        wave_count++;
    }
}
