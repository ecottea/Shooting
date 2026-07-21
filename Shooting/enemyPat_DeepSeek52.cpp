// enemyPat_BeeDance.cpp
// 蜂の八字舞踏「蜜導の輪舞」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//=============================================================================
// 静的内部関数
//=============================================================================

//-----------------------------------------------
// 蜂の∞軌道パターン（1セット）
//-----------------------------------------------
static void InfinityOrbit(sEnemyShotSet* pSet)
{
    // pSet->count はメインループが毎フレームインクリメント
    if (pSet->count == 0) {
        // 初期化
        pSet->param_d[0] = 40.0;          // [0] 振幅 A
        pSet->param_d[1] = (GetRand(628) / 100.0); // [1] 回転角 (ランダム初期向き)
        pSet->param_d[2] = 0.0008;        // [2] 回転速度 (rad/frame)
        pSet->param_d[3] = 0.12;          // [3] 振幅拡大率
        pSet->param_d[6] = 0.0;           // [6] 位相 t
        pSet->param_d[7] = 0.04;          // [7] 位相進行速度

        // kind で難易度調整
        if (pSet->kind == 0) {
            // フェーズ1：ゆっくり
        }
        else if (pSet->kind == 1) {
            pSet->param_d[2] = 0.0012;
            pSet->param_d[3] = 0.18;
            pSet->param_d[7] = 0.055;
        }
        else {
            // フェーズ2：高速
            pSet->param_d[2] = 0.0020;
            pSet->param_d[3] = 0.24;
            pSet->param_d[7] = 0.07;
        }

        // 蜂の2発ペアを生成（黄と黒の小玉）
        for (int i = 0; i < 2; ++i) {
            sEnemyShot* pBee = new sEnemyShot;
            pBee->param_i[0] = 1;                     // 蜂フラグ
            pBee->param_d[0] = (i == 0) ? 0.0 : 0.25; // 位相オフセット
            pBee->kind = (i == 0) ? img_enemyShotMediumBall[1]   // 黄
                : img_enemyShotMediumBall[7];  // 黒
            pBee->x = pSet->x;
            pBee->y = pSet->y;
            pBee->muki = 0.0;
            pBee->speed = 0.0;
            // リストに追加
            pBee->prev = pSet->pEnemyShotHead->prev;
            pBee->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pBee;
            pSet->pEnemyShotHead->prev = pBee;
        }

        // 効果音（軽め）
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // --- 毎フレーム処理 ---
    // 中心をプレイヤーへ緩やかに誘導
    double dx = player.x - pSet->x;
    double dy = player.y - pSet->y;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist > 1.0) {
        pSet->x += dx * 0.0015;
        pSet->y += dy * 0.0015;
    }

    // パラメータ更新
    pSet->param_d[0] += pSet->param_d[3];          // 振幅拡大
    if (pSet->param_d[0] > 200.0) pSet->param_d[0] = 200.0; // 上限
    pSet->param_d[1] += pSet->param_d[2];          // 回転
    pSet->param_d[6] += pSet->param_d[7];          // 位相進行
    if (pSet->param_d[6] > 2.0 * M_PI) pSet->param_d[6] -= 2.0 * M_PI;

    double A = pSet->param_d[0];
    double R = pSet->param_d[1];

    // 全弾更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // --- 蜂弾：∞軌道上を移動 ---
            double T = pSet->param_d[6] + pShot->param_d[0];
            double u = A * sin(T);
            double v = A * sin(T) * cos(T);
            pShot->x = pSet->x + u * cos(R) - v * sin(R);
            pShot->y = pSet->y + u * sin(R) + v * cos(R);

            // 蜜滴をばらまく（一定間隔）
            if (pShot->count % 6 == 0 && pShot->count <= 500) {
                // 接線ベクトル
                double du = A * cos(T);
                double dv = A * (cos(T) * cos(T) - sin(T) * sin(T)); // cos(2T)
                double tx = du * cos(R) - dv * sin(R);
                double ty = du * sin(R) + dv * cos(R);
                double len = sqrt(tx * tx + ty * ty);
                if (len > 0.001) { tx /= len; ty /= len; }
                double base_ang = atan2(ty, tx);

                // 垂直方向に3発（やや拡散）
                for (int j = -1; j <= 1; ++j) {
                    sEnemyShot* pDrop = new sEnemyShot;
                    pDrop->x = pShot->x;
                    pDrop->y = pShot->y;
                    pDrop->muki = base_ang + M_PI / 2.0 + j * 0.35;
                    pDrop->speed = 1.1 + (GetRand(40) - 20) * 0.005; // 0.9～1.3程度
                    pDrop->kind = img_enemyShotSmallBall[8];          // 橙（蜜滴）
                    // リストに追加
                    pDrop->prev = pSet->pEnemyShotHead->prev;
                    pDrop->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pDrop;
                    pSet->pEnemyShotHead->prev = pDrop;
                }
            }
        }
        else {
            // --- 蜜滴：等速直線運動 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

//-----------------------------------------------
// InfinityOrbit セットの生成
//-----------------------------------------------
static void StartInfinityOrbit(int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = InfinityOrbit;
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;
    pSet->muki = 0.0;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    // グローバルな弾幕リストに連結
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

//=============================================================================
// 敵本体パターン
//=============================================================================
void EnemyPat_Bee_DeepSeek()
{
    static int muki;
    static int phase;          // 0:第1波, 1:第2波, 2:第3波

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
    }
    else {
        // 左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // フェーズに応じて軌道セットを追加
    switch (phase) {
    case 0:
        if (count % 1100 == 80)  StartInfinityOrbit(0);
        if (count % 1100 == 200) StartInfinityOrbit(0);
        if (count % 1100 == 320) StartInfinityOrbit(0);
        if (count % 1100 == 440) { phase = 1; } // 次へ
        break;
    case 1:
        if (count % 1100 == 460) StartInfinityOrbit(1);
        if (count % 1100 == 560) StartInfinityOrbit(1);
        if (count % 1100 == 660) StartInfinityOrbit(1);
        if (count % 1100 == 760) { phase = 2; }
        break;
    case 2:
        if (count % 1100 == 780) StartInfinityOrbit(2);
        if (count % 1100 == 860) StartInfinityOrbit(2);
        if (count % 1100 == 940) StartInfinityOrbit(2);
        if (count % 1100 == 1020) {
            // ここでループしても良いが、HPが尽きるまで続ける
            phase = 0; // 実質無限継続
        }
        break;
    }
}