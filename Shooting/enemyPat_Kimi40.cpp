// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：二重振り子（連鎖軌跡）
static void ShotDoublePendulum(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 物理パラメータの読み出し
    double& theta1 = pEnemyShotSet->param_d[0];  // 第1振り子の角度
    double& omega1 = pEnemyShotSet->param_d[1];  // 第1振り子の角速度
    double& theta2 = pEnemyShotSet->param_d[2];  // 第2振り子の角度
    double& omega2 = pEnemyShotSet->param_d[3];  // 第2振り子の角速度
    double& cx = pEnemyShotSet->param_d[4];      // 回転中心X
    double& cy = pEnemyShotSet->param_d[5];      // 回転中心Y
    double& r1 = pEnemyShotSet->param_d[6];      // 第1振り子の長さ
    double& r2 = pEnemyShotSet->param_d[7];      // 第2振り子の長さ

    int& phase = pEnemyShotSet->param_i[0];      // フェーズ管理

    // 初回フレーム：初期化
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 初期角度
        theta1 = 0.0;
        theta2 = DX_PI / 2.0;

        // 初期角速度
        omega1 = 0.05;
        omega2 = 0.0;

        // 振り子の長さ
        r1 = 60.0;
        r2 = 50.0;

        // 中心位置はセット生成時の敵位置
        cx = pEnemyShotSet->x;
        cy = pEnemyShotSet->y;

        phase = 0;
    }

    // フェーズ遷移（0:開幕 1:加速 2:カオス 3:収束）
    if (pEnemyShotSet->count == 300) {
        phase = 1; // 加速開始
    }
    else if (pEnemyShotSet->count == 600) {
        phase = 2; // カオス相
    }
    else if (pEnemyShotSet->count == 1200) {
        phase = 3; // 収束
    }

    // 角速度の制御（フェーズによる変調）
    double baseOmega1 = 0.05;
    if (phase == 1) baseOmega1 = 0.08;
    if (phase == 2) baseOmega1 = 0.12;
    if (phase == 3) baseOmega1 = 0.03;

    // 二重振り子の物理更新（簡易オイラー法）
    double dt = 1.0;

    // 第1振り子：一定角速度 + 微小ノイズ
    omega1 = baseOmega1 + 0.01 * sin(pEnemyShotSet->count * 0.02);
    theta1 += omega1 * dt;

    // 第2振り子：第1振り子との結合（カオス性の源）
    double coupling = 0.08;
    double damping = 0.001;
    double accel2 = coupling * sin(theta1 - theta2) - damping * omega2;
    omega2 += accel2 * dt;
    theta2 += omega2 * dt;

    // 弾源の位置計算
    double x1 = cx + r1 * cos(theta1);
    double y1 = cy + r1 * sin(theta1);
    double x2 = x1 + r2 * cos(theta2);
    double y2 = y1 + r2 * sin(theta2);

    // 弾源Bの速度ベクトル（数値微分）
    static double prevX2 = x2;
    static double prevY2 = y2;
    double vx = x2 - prevX2;
    double vy = y2 - prevY2;
    prevX2 = x2;
    prevY2 = y2;

    // 速度方向に弾を発射（毎2フレーム）
    if (pEnemyShotSet->count % 1 == 0) {
        double speed = 2.0;
        double dir = atan2(vy, vx);
        if (fabs(vx) > 0.001 || fabs(vy) > 0.001) {
            if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x2;
            pEnemyShot->y = y2;
            pEnemyShot->muki = dir;
            pEnemyShot->speed = speed;

            // 小玉、色は kind に応じて変化
            pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShotSet->kind % 6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_DoublePendulum_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 長寿命の二重振り子セットを生成（1セットのみ）
    if (count == 60 || count == 61) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDoublePendulum;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // セットの中心位置を敵に追従
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        if (pSet->patternFunc == ShotDoublePendulum) {
            pSet->param_d[4] = enemy.x;
            pSet->param_d[5] = enemy.y + 10.0;
        }
        pSet = pSet->next;
    }
}