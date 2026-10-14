#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 前方宣言
static void ShotDoubleSlit(sEnemyShotSet* pEnemyShotSet);
static void ShotWall(sEnemyShotSet* pEnemyShotSet);
static void ShotObserver(sEnemyShotSet* pEnemyShotSet);

// ============================================================
// 弾幕：二重スリット干渉
// ============================================================
static void ShotDoubleSlit(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki;
        pShot->speed = 3.5;
        pShot->kind = img_enemyShotSmallBall[6]; // 白小玉（粒子）

        // param_d[1] に元の発射角度を保存（粒子性収束時に使用）
        pShot->param_d[1] = pEnemyShotSet->muki;

        // 干渉縞の目標角度を計算
        // 自機周辺の縦方向の縞模様を表現するため、基準角度から5段階の明線角度のいずれかに割り当てる
        double base_muki = pEnemyShotSet->muki;
        int fringe_index = GetRand(4); // 0 ~ 4
        double offset = (fringe_index - 2) * 0.12; // -0.24, -0.12, 0, 0.12, 0.24 ラジアン
        pShot->param_d[0] = base_muki + offset;

        // param_i[0]: 0=波動性(干渉縞), 1=粒子性(直進)
        pShot->param_i[0] = pEnemyShotSet->param_i[0];

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // スリット位置 (y = 240.0) を通過したら軌道補間を開始
        if (pShot->y > 240.0) {
            if (pShot->param_i[0] == 0) {
                // 波動性：干渉縞の目標角度に滑らかに補間
                pShot->muki += (pShot->param_d[0] - pShot->muki) * 0.08;
            }
            else {
                // 粒子性：観測により元の直進角度に収束
                pShot->muki += (pShot->param_d[1] - pShot->muki) * 0.08;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕：スリットの壁（静止）
// ============================================================
static void ShotWall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 画面中央 (y=240) に横一列の壁を配置。特定の範囲をスキップして二重スリットを作る
        for (int x = 20; x <= 460; x += 20) {
            // x=200付近とx=280付近を隙間（スリット）として空ける
            if ((x >= 190 && x <= 210) || (x >= 270 && x <= 290)) continue;

            sEnemyShot* pWall = new sEnemyShot;
            pWall->x = (double)x;
            pWall->y = 240.0;
            pWall->muki = 0.0; // 縦向き
            pWall->speed = 0.0; // 静止
            pWall->kind = img_enemyShotMediumOval[1]; // 黄中楕円弾（壁として視認性が高い）
            pWall->param_i[0] = 99; // 壁フラグ

            pWall->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pWall->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pWall;
            pEnemyShotSet->pEnemyShotHead->prev = pWall;
        }
    }
    // 壁は speed=0.0 なので移動計算は実質的に不要

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (count % 300 == 300-1) pShot->margin = -9999;
        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕：観測機（静止）
// ============================================================
static void ShotObserver(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pObs = new sEnemyShot;
        pObs->x = pEnemyShotSet->x;
        pObs->y = pEnemyShotSet->y;
        pObs->muki = 0.0;
        pObs->speed = 0.0; // 静止
        pObs->kind = img_enemyShotLargeBall[0]; // 赤大玉（観測機として目立つ色とサイズ）
        pObs->param_i[0] = 98; // 観測機フラグ

        pObs->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pObs->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pObs;
        pEnemyShotSet->pEnemyShotHead->prev = pObs;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (count % 300 == 300-1) pShot->margin = -9999;
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン：二重スリット実験
// ============================================================
void EnemyPat_DoubleSlit_Qwen()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵は左右にゆっくり移動
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    int cycle = count % 300;

    // [1] サイクル開始時：スリットの壁を設置
    if (cycle == 1) {
        sEnemyShotSet* pWallSet = new sEnemyShotSet;
        pWallSet->count = 0;
        pWallSet->patternFunc = ShotWall;
        pWallSet->x = 0.0;
        pWallSet->y = 240.0;
        pWallSet->pEnemyShotHead = new sEnemyShot;
        pWallSet->pEnemyShotHead->prev = pWallSet->pEnemyShotHead;
        pWallSet->pEnemyShotHead->next = pWallSet->pEnemyShotHead;

        pWallSet->prev = enemyShotSetHead.prev;
        pWallSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWallSet;
        enemyShotSetHead.prev = pWallSet;
    }

    // [2] 波動性フェーズ終了直前：観測機の配置と干渉縞の収束
    if (cycle == 150) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 左スリットに観測機配置
        sEnemyShotSet* pObsSet1 = new sEnemyShotSet;
        pObsSet1->count = 0;
        pObsSet1->patternFunc = ShotObserver;
        pObsSet1->x = 200.0;
        pObsSet1->y = 240.0;
        pObsSet1->pEnemyShotHead = new sEnemyShot;
        pObsSet1->pEnemyShotHead->prev = pObsSet1->pEnemyShotHead;
        pObsSet1->pEnemyShotHead->next = pObsSet1->pEnemyShotHead;
        pObsSet1->prev = enemyShotSetHead.prev;
        pObsSet1->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pObsSet1;
        enemyShotSetHead.prev = pObsSet1;

        // 右スリットに観測機配置
        sEnemyShotSet* pObsSet2 = new sEnemyShotSet;
        pObsSet2->count = 0;
        pObsSet2->patternFunc = ShotObserver;
        pObsSet2->x = 280.0;
        pObsSet2->y = 240.0;
        pObsSet2->pEnemyShotHead = new sEnemyShot;
        pObsSet2->pEnemyShotHead->prev = pObsSet2->pEnemyShotHead;
        pObsSet2->pEnemyShotHead->next = pObsSet2->pEnemyShotHead;
        pObsSet2->prev = enemyShotSetHead.prev;
        pObsSet2->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pObsSet2;
        enemyShotSetHead.prev = pObsSet2;

        // 既存の「二重スリット」弾セットをすべて粒子性（直進）に切り替え
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotDoubleSlit) {
                pSet->param_i[0] = 1; // セット全体を粒子性フラグに
                sEnemyShot* pShot = pSet->pEnemyShotHead->next;
                while (pShot != pSet->pEnemyShotHead) {
                    pShot->param_i[0] = 1; // 個別の弾も粒子性フラグに
                    pShot = pShot->next;
                }
            }
            pSet = pSet->next;
        }
    }

    // [3] 粒子の連射（サイクルの 1~240 フレーム目まで、5フレームごと）
    if (cycle >= 1 && cycle <= 240) {
        if (cycle == 1) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDoubleSlit;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;

        // 自機を狙う角度を基準に、少しばらけさせて発射
        double base_muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->muki = base_muki + (GetRand(40) - 20) / 180.0 * DX_PI;

        // cycle 120 以降に生成された弾は、最初から粒子性として扱う
        pSet->param_i[0] = (cycle <= 150) ? 0 : 1;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}