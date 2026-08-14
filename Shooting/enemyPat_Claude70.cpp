// enemyPat_kyoushinSamon.cpp
// 共振砂紋：クラドニ図形（振動板の砂紋）をモチーフにした4フェーズ弾幕
//
//   Phase A 励振          (   0- 79 ) … 予告。弾は出さず、共振音のみで警戒を促す
//   Phase B 定在波形成    (  80-179 ) … 節線（8方向スポーク＋同心リング3本）が中心から一斉に伸びて
//                                        基本モード（m=4相当）の模様を形成する
//   Phase C モード遷移    ( 180-399 ) … リング3本が等間隔に分離し、遷移後半で追加の4方向スポークが
//                                        新たに生えてくる（高次モードm=6相当への変化を表現）。
//                                        リングは半径ごとに逆回転し、スポーク全体もゆっくり自転する
//   Phase D 共鳴破砕      ( 400-    ) … 全弾がその場から放射方向へ加速せず等速で飛散。
//                                        同時に中心から自機狙い3wayを追加発射
//
// ※ 実在のクラドニ図形は円板の場合ベッセル関数で決まるが、本パターンでは
//   「節直径（スポーク）」と「節円（リング）」という視覚的特徴のみを抽出し、
//   軽量な三角関数の組み合わせで近似している。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ==== 振動板パラメータ ====
static const double PLATE_CX = 240.0;
static const double PLATE_CY = 240.0;
static const double MAX_R = 300.0;

// ==== フェーズ長（フレーム数） ====
static const int TELEGRAPH_LEN = 80-20;   // Phase A（pEnemyShotSet->count基準）
static const int CONVERGE_LEN = 100;  // Phase B（pShot->count基準、スポーク・リング共通）
static const int MORPH_LEN = 220;  // Phase C（pShot->count基準）
// Phase D は pShot->count >= CONVERGE_LEN + MORPH_LEN

static const int BURST_START_GLOBAL = TELEGRAPH_LEN + CONVERGE_LEN + MORPH_LEN; // = 400

// ==== 図形パラメータ ====
static const int ARMS_BASE = 8*3;  // 基本モード：節直径4本＝8方向スポーク
static const int ARMS_EXTRA = 4*3;  // 遷移後半で追加生成される高次スポーク（計12方向）
static const int DOTS_PER_ARM = 4*3;

static const int    RING_NUM = 3;
static const int    RING_DOTS = 16*3;
static const double MODE_A_RING_R[RING_NUM] = { 0.35 * MAX_R, 0.75 * MAX_R, 0.75 * MAX_R }; // 外2本が重なり見た目2本のリングに
static const double MODE_B_RING_R[RING_NUM] = { 0.30 * MAX_R, 0.60 * MAX_R, 0.90 * MAX_R }; // 等間隔3本へ分離
static const double RING_ROT_SPEED[RING_NUM] = { 0.0035, -0.0025, 0.0018 };                  // 半径ごとに逆回転させ資鳴感を出す

static const double ARM_ROT_SPEED = 0.003; // 遷移フェーズ中のスポーク自転速度
static const double BURST_SPEED = 2.6;   // 破砕後の飛散速度（等速）
static const double AIMED_SPEED = 3.2;   // 自機狙い3wayの速度

// 0〜1の範囲でなめらかな加減速を行うイージング
static double Ease(double t)
{
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t * t * (3.0 - 2.0 * t);
}

// スポーク（節直径）1発の位置を pShot->count から直接計算する
static void CalcArmShot(sEnemyShot* pShot)
{
    int  armIdx = pShot->param_i[1];          // 0〜(ARMS_BASE+ARMS_EXTRA-1)
    int  stepIdx = pShot->param_i[2];          // 0〜DOTS_PER_ARM-1
    bool isExtra = (pShot->param_i[3] == 1);

    double baseAngle = armIdx * (2.0 * DX_PI / (ARMS_BASE + ARMS_EXTRA));
    double targetR = MAX_R * (stepIdx + 1) / DOTS_PER_ARM;
    double t = (double)pShot->count;
    double r, theta;

    if (!isExtra) {
        if (t < CONVERGE_LEN) {
            r = targetR * Ease(t / CONVERGE_LEN);
            theta = baseAngle;
        }
        else if (t < CONVERGE_LEN + MORPH_LEN) {
            double mt = t - CONVERGE_LEN;
            r = targetR;
            theta = baseAngle + ARM_ROT_SPEED * mt;
        }
        else {
            double bt = t - (CONVERGE_LEN + MORPH_LEN);
            r = targetR + BURST_SPEED * bt;
            theta = baseAngle + ARM_ROT_SPEED * MORPH_LEN;
        }
    }
    else {
        // 追加スポーク：遷移フェーズ後半で中心から新たに伸びてくる
        double growStart = CONVERGE_LEN + MORPH_LEN * 0.5;
        if (t < growStart) {
            r = 0.0;
            theta = baseAngle;
        }
        else if (t < CONVERGE_LEN + MORPH_LEN) {
            double gt = (t - growStart) / (MORPH_LEN * 0.5);
            r = targetR * Ease(gt);
            theta = baseAngle;
        }
        else {
            double bt = t - (CONVERGE_LEN + MORPH_LEN);
            r = targetR + BURST_SPEED * bt;
            theta = baseAngle;
        }
    }

    pShot->x = PLATE_CX + r * cos(theta);
    pShot->y = PLATE_CY + r * sin(theta);
    pShot->muki = theta;
}

// リング（節円）1発の位置を pShot->count から直接計算する
static void CalcRingShot(sEnemyShot* pShot)
{
    int ringIdx = pShot->param_i[1]; // 0〜2
    int dotIdx = pShot->param_i[2]; // 0〜RING_DOTS-1

    double baseAngle = dotIdx * (2.0 * DX_PI / RING_DOTS);
    double rA = MODE_A_RING_R[ringIdx];
    double rB = MODE_B_RING_R[ringIdx];
    double rotSpeed = RING_ROT_SPEED[ringIdx];
    double t = (double)pShot->count;
    double r, theta;

    if (t < CONVERGE_LEN) {
        r = rA * Ease(t / CONVERGE_LEN);
        theta = baseAngle;
    }
    else if (t < CONVERGE_LEN + MORPH_LEN) {
        double mt = t - CONVERGE_LEN;
        r = rA + (rB - rA) * Ease(mt / MORPH_LEN);
        theta = baseAngle + rotSpeed * mt;
    }
    else {
        double bt = t - (CONVERGE_LEN + MORPH_LEN);
        r = rB + BURST_SPEED * bt;
        theta = baseAngle + rotSpeed * MORPH_LEN;
    }

    pShot->x = PLATE_CX + r * cos(theta);
    pShot->y = PLATE_CY + r * sin(theta);
    pShot->muki = theta;
}

// 破砕時の自機狙い3way（速度一定、count積分ではなくcountから直接位置を計算）
static void CalcAimedShot(sEnemyShot* pShot)
{
    double angle = pShot->param_d[0];
    double speed = pShot->param_d[1];
    double t = (double)pShot->count;

    pShot->x = pShot->param_d[2] + speed * cos(angle) * t;
    pShot->y = pShot->param_d[3] + speed * sin(angle) * t;
    pShot->muki = angle;
}

// 弾幕：共振砂紋
static void ShotChladni(sEnemyShotSet* pEnemyShotSet)
{
    // Phase A→B：節線弾（スポーク＋リング）を一斉召喚
    if (pEnemyShotSet->count == TELEGRAPH_LEN) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // スポーク：基本8本（白・菱形弾）＋追加4本（黄・菱形弾）
        for (int a = 0; a < ARMS_BASE + ARMS_EXTRA; a++) {
            bool isExtra = (a >= ARMS_BASE);
            for (int s = 0; s < DOTS_PER_ARM; s++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->kind = isExtra ? img_enemyShotDiamond[1] : img_enemyShotDiamond[6];
                pShot->param_i[0] = 0; // 0 = スポーク
                pShot->param_i[1] = a;
                pShot->param_i[2] = s;
                pShot->param_i[3] = isExtra ? 1 : 0;
                pShot->margin = 240;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        // リング：内側からシアン・青・マゼンタの小玉
        static const int ringColor[RING_NUM] = { 3, 4, 5 };
        for (int r = 0; r < RING_NUM; r++) {
            for (int d = 0; d < RING_DOTS; d++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->kind = img_enemyShotSmallBall[ringColor[r]];
                pShot->param_i[0] = 1; // 1 = リング
                pShot->param_i[1] = r;
                pShot->param_i[2] = d;
                pShot->margin = 240;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // Phase D 突入：自機狙い3way（赤・中玉）を追加発射
    if (pEnemyShotSet->count == BURST_START_GLOBAL) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - PLATE_CY, player.x - PLATE_CX);
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotLargeBall[0];
            pShot->param_i[0] = 2; // 2 = 自機狙い
            pShot->param_d[0] = baseAngle + i * (DX_PI / 12.0);
            pShot->param_d[1] = AIMED_SPEED;
            pShot->param_d[2] = PLATE_CX;
            pShot->param_d[3] = PLATE_CY;
            pShot->margin = 240;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 全弾、毎フレーム formula から位置を再計算する（速度積分はしない）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case 0: CalcArmShot(pShot);   break;
        case 1: CalcRingShot(pShot);  break;
        case 2: CalcAimedShot(pShot); break;
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン：共振砂紋
void EnemyPat_Chladni_Claude()
{
    if (count == 1) {
        enemy.x = PLATE_CX;
        enemy.y = PLATE_CY;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        // 振動板がわずかに息づくような微振動演出
        enemy.y = PLATE_CY + sin(count * 0.02) * 6.0;
    }

    if (count % 460 == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotChladni;
        pEnemyShotSet->x = PLATE_CX;
        pEnemyShotSet->y = PLATE_CY;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->alive = 99999;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}