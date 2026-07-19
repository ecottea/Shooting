// enemyPat_orochiRanbu.cpp
//
// 「大蛇乱舞」パターン
// ボスを中心に8本の"へにょりレーザー"(蛇腹状の曲線レーザー)を放射状に展開し、
// うねりながら伸縮しつつ全体がゆっくり回転する弾幕。
//
// 【設計上の工夫(過去のDNAパターンの反省を踏まえたもの)】
// ・sEnemyShotSetは起動時(count==1)に8本分を1回だけ生成し、以降は再生成しない。
//   各蛇は固定本数(SEGMENTS_PER_SNAKE)のsEnemyShotを最初に確保し、
//   以後は毎フレーム座標を直接書き換えるだけなので、サイクルをまたいで
//   弾が積み上がることがない(＝世代をまたいだ蓄積によるFPS低下が起きない)。
// ・伸長→静止→収縮→小休止のサイクルは pEnemyShotSet->count のmodだけで
//   内部的にループさせるので、再生成のロジックが不要。
// ・1本の蛇について基準角(theta0)のcos/sinは1回だけ計算し、
//   20節すべてで使い回す(節ごとの再計算をしない)。
// ・8本のばらつき(振幅・波長・位相)は腕番号から決定的に算出し、GetRandは使わない。
//   (将来リプレイを付けた場合の再現性を考慮)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const int    NUM_SNAKES          = 8;      // 蛇の本数
static const int    SEGMENTS_PER_SNAKE  = 40;      // 1本あたりの節の数
static const double MAX_LENGTH          = 350.0;   // 伸びきったときの全長

// 伸長→静止→収縮→小休止 のフレーム数(合計で1サイクル)
static const int CYCLE_GROW   = 90;
static const int CYCLE_HOLD   = 120;
static const int CYCLE_SHRINK = 60;
static const int CYCLE_PAUSE  = 30;
static const int CYCLE_TOTAL  = CYCLE_GROW + CYCLE_HOLD + CYCLE_SHRINK + CYCLE_PAUSE;

static const double ROT_SPEED       = (2.0 * DX_PI) / 480.0; // 全体回転:約8秒で1周
static const double BASE_AMPLITUDE  = 22.0;   // うねりの基本振幅
static const double BASE_WAVE_SPEED = 0.045;  // うねりの基本角速度(rad/フレーム)
static const double BASE_LAG_FACTOR = 0.05;   // 根元→先端への位相遅れ係数(rad/px)

static const int    HOMING_INTERVAL = 4;   // 誘導弾の発射間隔(フレーム)
static const double HOMING_SPEED    = 2.6;  // 誘導弾の速さ

// ============================================================
//  共通ヘルパー
// ============================================================

// 0〜1を滑らかに補間(イーズイン・イーズアウト)
static double SmoothStep01(double t)
{
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t * t * (3.0 - 2.0 * t);
}

// サイクル内の経過フレーム(localT)から、現在の蛇の全長を求める
static double ComputeCurrentLen(int localT)
{
    if (localT < CYCLE_GROW) {
        double t = (double)localT / CYCLE_GROW;
        return MAX_LENGTH * SmoothStep01(t);
    }
    else if (localT < CYCLE_GROW + CYCLE_HOLD) {
        return MAX_LENGTH;
    }
    else if (localT < CYCLE_GROW + CYCLE_HOLD + CYCLE_SHRINK) {
        double t = (double)(localT - CYCLE_GROW - CYCLE_HOLD) / CYCLE_SHRINK;
        return MAX_LENGTH * (1.0 - SmoothStep01(t));
    }
    else {
        return 0.0; // 小休止:完全に収縮した状態
    }
}

// 腕番号から、その蛇固有のパラメータを決定的に算出する(GetRandは使わない)
static void GetArmParams(int armIndex, double* baseAngle, double* amplitude,
    double* waveSpeed, double* lagFactor, double* phaseOffset)
{
    *baseAngle   = (2.0 * DX_PI / NUM_SNAKES) * armIndex;
    *amplitude   = BASE_AMPLITUDE + 4.0 * ((armIndex % 3) - 1);              // ±4程度のばらつき
    *waveSpeed   = BASE_WAVE_SPEED * (1.0 + 0.08 * ((armIndex % 4) - 1.5));  // 波の速さをばらつかせる
    *lagFactor   = BASE_LAG_FACTOR * (1.0 + 0.06 * (((armIndex + 2) % 5) - 2)); // 波長をばらつかせる
    *phaseOffset = (DX_PI / 4.0) * armIndex;                                 // 初期位相をずらす
}

// 腕armIndexの、根元からの弧長sにおける座標と向きを求める。
// (蛇の胴体セグメントの位置決めと、先端からの誘導弾発射位置の両方で使う共通ロジック)
static void ComputeSnakeNode(int armIndex, double s, int globalCount,
    double rootX, double rootY, double* outX, double* outY, double* outMuki)
{
    double baseAngle, amplitude, waveSpeed, lagFactor, phaseOffset;
    GetArmParams(armIndex, &baseAngle, &amplitude, &waveSpeed, &lagFactor, &phaseOffset);

    // 全体回転込みの基準角。1本につきcos/sinは1回だけ計算して使い回す。
    double theta0 = baseAngle + ROT_SPEED * globalCount;
    double cosT = cos(theta0);
    double sinT = sin(theta0);

    double wavePhase = waveSpeed * globalCount - lagFactor * s + phaseOffset;
    double lateral = amplitude * sin(wavePhase);

    // 基準方向theta0に対して垂直方向にlateralだけオフセットする
    double px = rootX + s * cosT - lateral * sinT;
    double py = rootY + s * sinT + lateral * cosT;

    // 接線方向(スプライトの向き)は有限差分で近似する
    const double ds = 2.0;
    double wavePhase2 = waveSpeed * globalCount - lagFactor * (s + ds) + phaseOffset;
    double lateral2 = amplitude * sin(wavePhase2);
    double px2 = rootX + (s + ds) * cosT - lateral2 * sinT;
    double py2 = rootY + (s + ds) * sinT + lateral2 * cosT;

    *outX = px;
    *outY = py;
    *outMuki = atan2(py2 - py, px2 - px);
}

// ============================================================
//  弾幕:蛇の胴体(へにょりレーザー本体)
// ============================================================
static void ShotHenyoroLaserBody(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 節を最初に必要な数だけ確保しておく。
        // 以後は座標を直接書き換えるだけなので、フレームごとのnew/deleteが発生しない。
        for (int i = 0; i < SEGMENTS_PER_SNAKE; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0; // 座標は毎フレーム直接更新するため未使用
            pEnemyShot->kind = img_enemyShotDiamond[pEnemyShotSet->kind % 8]; // 腕番号=色番号
            pEnemyShot->param_i[0] = i; // セグメント番号(根元からの並び順)
            pEnemyShot->margin = 400;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    int armIndex = pEnemyShotSet->kind;
    int localT = pEnemyShotSet->count % CYCLE_TOTAL;
    double currentLen = ComputeCurrentLen(localT);

    double rootX = enemy.x;
    double rootY = enemy.y;

    double segSpacing = MAX_LENGTH / (SEGMENTS_PER_SNAKE - 1);

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        int i = pEnemyShot->param_i[0];
        double sTarget = i * segSpacing;
        // 伸長中/収縮中は、まだ現在の全長に達していない節は先端位置に重ねておく
        // (この重なりが「伸びていく蛇の頭」に見える)
        double s = (sTarget < currentLen) ? sTarget : currentLen;

        double nx, ny, nmuki;
        ComputeSnakeNode(armIndex, s, count, rootX, rootY, &nx, &ny, &nmuki);

        pEnemyShot->x = nx;
        pEnemyShot->y = ny;
        pEnemyShot->muki = nmuki;

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  弾幕:先端から放つ自機狙いの誘導弾(細い針状の一撃)
// ============================================================
static void ShotHomingNeedle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = HOMING_SPEED;
        pEnemyShot->kind = img_enemyShotBullet[pEnemyShotSet->kind % 8]; // 針状なのでレーザー画像は使わない
        pEnemyShot->margin = 400;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体のパターン:大蛇乱舞
// ============================================================
void EnemyPat_HenyoriLaser_Claude()
{
    static int muki;
    static int homingArmCounter;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        homingArmCounter = 0;

        // 8本の蛇を最初に1回だけ生成する。
        // 以降はShotHenyoroLaserBody内部でサイクルをループさせるだけで、
        // ショットセットの再生成は一切行わない。
        for (int arm = 0; arm < NUM_SNAKES; arm++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotHenyoroLaserBody;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->kind = arm; // 腕番号(色・パラメータ算出のキーとして使う)

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    else {
        // ボスは左右にゆっくり往復
        enemy.x += 0.6 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }

    // 一定間隔ごとに、腕を1本ずつ順番に選んで、
    // その腕が「静止(伸びきり)」フェーズにあるときだけ先端から誘導弾を発射する。
    // 8本の蛇はすべてcount==1の同フレームで生成されているため、
    // 各pEnemyShotSet->countはこの関数のcountよりちょうど1小さい値になる。
    if (count > 1 && (count - 1) % HOMING_INTERVAL == 0) {
        int localT = (count - 1) % CYCLE_TOTAL;
        if (localT >= CYCLE_GROW && localT < CYCLE_GROW + CYCLE_HOLD) {
            int arm = homingArmCounter % NUM_SNAKES;
            homingArmCounter++;

            double tipX, tipY, tipMuki;
            ComputeSnakeNode(arm, MAX_LENGTH, count, enemy.x, enemy.y, &tipX, &tipY, &tipMuki);

            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotHomingNeedle;
            pEnemyShotSet->x = tipX;
            pEnemyShotSet->y = tipY;
            pEnemyShotSet->muki = atan2(player.y - tipY, player.x - tipX);
            pEnemyShotSet->kind = arm;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}