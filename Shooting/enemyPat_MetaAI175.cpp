#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ==========================================
// 設定
// ==========================================
static const int PENDULUM_NUM = 9;
static const int SEGMENT_NUM = 12; // 1本あたりの糸の粒数
static const double PIVOT_Y = 70.0;
static const double GRAVITY = 900.0; // 周期計算用 g
static const double MAX_ANGLE = 35.0 * DX_PI / 180.0; // 最大振れ角
static const double TIME_STEP = 0.06/3; // 時間の進み方
static const double PIVOT_SPREAD = 42.0; // 支点間隔

// 前方宣言
static void ShotRelease(sEnemyShotSet* pEnemyShotSet);

// ------------------------------------------
// 放出弾：振り子が最下点を通過した時に撃つ3Way
// ------------------------------------------
static void ShotRelease(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 自機狙い + 20度ずつ広げる
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 1) * (20.0 * DX_PI / 180.0)*3;
            // GetRand(x)は0〜xのx+1種類なので注意
            pEnemyShot->speed = 2.8 + GetRand(60) / 100.0;
            // 中玉(7.0x7.0) 橙(8) を放出弾に使用：視認性が高く、おもりと区別しやすい
            pEnemyShot->kind = img_enemyShotMediumBall[8];
            pEnemyShot->margin = 240;

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

// ------------------------------------------
// 本命：九連ペンデュラムウェーブ
// ------------------------------------------
static void ShotNonaryPendulum(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回生成：おもり9個 + 糸(9*12=108個) = 117個
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        for (int p = 0; p < PENDULUM_NUM; p++) {
            double basePivotX = 240.0 + (p - (PENDULUM_NUM - 1) / 2.0) * PIVOT_SPREAD;
            double L = 300.0 + p * 14.0; // 右に行くほど長く：周期差で波を作る
            double omega = sqrt(GRAVITY / L);

            // 糸：小玉(2.5x2.5) 白(6) を使用。最も小さく、糸に見立てやすい。
            // 当たり判定はあるが半径が最小なので理不尽になりにくい
            for (int s = 0; s < SEGMENT_NUM; s++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
                pEnemyShot->param_i[0] = p;        // 振り子番号
                pEnemyShot->param_i[1] = s;        // 何番目の糸か
                pEnemyShot->param_i[2] = 0;        // 0=糸
                pEnemyShot->param_d[0] = L;
                pEnemyShot->param_d[1] = basePivotX;
                pEnemyShot->param_d[2] = PIVOT_Y;
                pEnemyShot->param_d[3] = omega;
                pEnemyShot->margin = 240;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
            // おもり：大玉(20.0x20.0) 青(4) を使用。大きく重く見える。
            pEnemyShot = new sEnemyShot;
            pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青
            pEnemyShot->param_i[0] = p;
            pEnemyShot->param_i[1] = SEGMENT_NUM; // 糸より外側
            pEnemyShot->param_i[2] = 1; // 1=おもり
            pEnemyShot->param_d[0] = L;
            pEnemyShot->param_d[1] = basePivotX;
            pEnemyShot->param_d[2] = PIVOT_Y;
            pEnemyShot->param_d[3] = omega;
            pEnemyShot->param_d[5] = 0.0; // 前フレームのtheta保存用
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 600フレームで1サイクル：0-360波、360-420収束、420-480収束維持、480-600再展開
    int cycle = pEnemyShotSet->count % 600;
    double converge = 0.0;
    if (cycle < 360) converge = 0.0;
    else if (cycle < 420) converge = (cycle - 360) / 60.0;
    else if (cycle < 480) converge = 1.0;
    else converge = 1.0 - (cycle - 480) / 120.0;

    // 更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int pIdx = pShot->param_i[0];
        int segIdx = pShot->param_i[1];
        int isWeight = pShot->param_i[2];

        double L = pShot->param_d[0];
        double basePivotX = pShot->param_d[1];
        double pivotY = pShot->param_d[2];
        double omega = pShot->param_d[3];

        double basePivot = basePivotX;
        double curPivotX = 240.0 + (basePivot - 240.0) * (1.0 - converge);

        // 波の計算：長さでωが違う + 初期位相を少しずらす
        double t = pEnemyShotSet->count * TIME_STEP;
        double theta = MAX_ANGLE * sin(omega * t + pIdx * 0.35);

        double frac = 1.0;
        if (isWeight == 0) {
            frac = (segIdx + 1) / (double)SEGMENT_NUM;
        }

        double wx = curPivotX + (L * frac) * sin(theta);
        double wy = pivotY + (L * frac) * cos(theta);

        pShot->x = wx;
        pShot->y = wy;

        // おもりが最下点を右向きに通過した瞬間に放出
        if (isWeight == 1 && converge < 0.05) {
            double prevTheta = pShot->param_d[5];
            // 負から正へクロス = 最下点通過
            if (prevTheta < 0.0 && theta >= 0.0) {
                sEnemyShotSet* pNewSet = new sEnemyShotSet;
                pNewSet->count = 0;
                pNewSet->patternFunc = ShotRelease;
                pNewSet->x = wx;
                pNewSet->y = wy;
                pNewSet->muki = atan2(player.y - wy, player.x - wx);
                pNewSet->kind = pIdx;
                pNewSet->pEnemyShotHead = new sEnemyShot;
                pNewSet->pEnemyShotHead->prev = pNewSet->pEnemyShotHead;
                pNewSet->pEnemyShotHead->next = pNewSet->pEnemyShotHead;
               
                pNewSet->prev = enemyShotSetHead.prev;
                pNewSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pNewSet;
                enemyShotSetHead.prev = pNewSet;
            }
            pShot->param_d[5] = theta;
        }

        pShot = pShot->next;
    }
}

// ==========================================
// 敵本体
// ==========================================
void EnemyPat_PendulumWave_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 1回だけペンデュラムシステムを生成
    if (count == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNonaryPendulum;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        // pEnemyShotHeadはShotNonaryPendulumのcount==0内で生成する
        pEnemyShotSet->pEnemyShotHead = nullptr;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}