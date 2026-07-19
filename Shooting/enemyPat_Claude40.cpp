// enemyPat_doublePendulum.cpp
//
// 二重振り子パターン「カオス・ペンデュラム」
//
// ・第1腕/第2腕を弾のチェーンで描画し、腕そのものが当たり判定を持つ
// ・提示フェーズ  : 腕をゆっくり広げて静止に近い状態で形を見せる
// ・解放フェーズ  : ラグランジアンの運動方程式をRK4で積分し、
//                   実際の二重振り子のようにカオス的に振れ回る
//                   先端(ペン先)から一定間隔で軌跡弾を放出する
// ・霧散フェーズ  : 一定時間後、腕の弾を放射状の自由飛行弾に変換して
//                   攻撃を締めくくり、画面外へ抜けさせて自然消滅させる
// ・同じ支点から初期角度をごくわずかにずらした複数の振り子を同時発生させ、
//   解放後に軌道が急速に分岐していく様子(バタフライ効果)を見せる
//
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  パターン用パラメータ（見た目・タイミング調整用）
// ============================================================

// 腕を構成する弾の数
static const int PEND_ARM1_NUM = 6;
static const int PEND_ARM2_NUM = 6;

// 腕の見た目の長さ(px) ※物理計算用の長さとは別
static const double PEND_ARM1_LEN = 75.0;
static const double PEND_ARM2_LEN = 70.0;

// フェーズの長さ(フレーム数)
static const int PEND_EASE_FRAMES = 25;   // 提示フェーズ内、垂直→基準角へ広がる時間
static const int PEND_DISPLAY_FRAMES = 100;  // 提示フェーズ全体の長さ
static const int PEND_RELEASE_FRAMES = 480 * 2;  // 解放フェーズ(カオス運動)の長さ

// 軌跡弾(すり流し弾)関連
static const int    PEND_TRAIL_INTERVAL = 3;    // 何フレームおきに軌跡弾を撃つか
static const double PEND_TRAIL_SPEED_SCALE = 0.35; // 先端速度→軌跡弾速度への変換係数
static const double PEND_TRAIL_SPEED_MIN = 0.6;
static const double PEND_TRAIL_SPEED_MAX = 2.6;

// 提示フェーズでの基準姿勢・微振動
static const double PEND_BASE_THETA1 = 65.0 / 180.0 * DX_PI;
static const double PEND_BASE_THETA2 = -50.0 / 180.0 * DX_PI;
static const double PEND_IDLE_AMP = 4.0 / 180.0 * DX_PI;
static const double PEND_IDLE_FREQ1 = 2.0 * DX_PI / 70.0;
static const double PEND_IDLE_FREQ2 = 2.0 * DX_PI / 55.0;

// 物理演算パラメータ(見た目用のスケール。実際の重力加速度とは無関係)
static const double PEND_G = 0.005;
static const double PEND_M1 = 1.0;
static const double PEND_M2 = 1.0;
static const double PEND_L1 = 1.0; // 物理計算専用の腕長比。描画長(PEND_ARM1_LEN)とは別物
static const double PEND_L2 = 1.0;

// 霧散フェーズでの飛散速度
static const double PEND_BURST_SPEED_MIN = 1.5;
static const double PEND_BURST_SPEED_MAX = 3.0;

// バリエーション(バタフライ効果用の初期角オフセットと色)
static const int    PEND_VARIANT_NUM = 7;
static const int    colorTable[PEND_VARIANT_NUM] = { 0, 8, 1, 2, 3, 4, 5 }; // 赤,黄,シアン,青,橙
static const double angleOffsetTable[PEND_VARIANT_NUM] = { -0.060, -0.040, -0.020, 0.0, 0.020, 0.040, 0.060 };

// ============================================================
//  二重振り子の運動方程式(点質量モデル)
// ============================================================

// 状態(th1,om1,th2,om2)から加速度(dom1,dom2)を求める
static void PendulumDeriv(double th1, double om1, double th2, double om2,
    double& dth1, double& dom1, double& dth2, double& dom2)
{
    const double m1 = PEND_M1;
    const double m2 = PEND_M2;
    const double g = PEND_G;

    double delta = th1 - th2;
    double den = 2.0 * m1 + m2 - m2 * cos(2.0 * delta);
    if (fabs(den) < 1e-6) den = (den < 0.0) ? -1e-6 : 1e-6; // ゼロ割防止

    dth1 = om1;
    dth2 = om2;

    dom1 = (-g * (2.0 * m1 + m2) * sin(th1)
        - m2 * g * sin(th1 - 2.0 * th2)
        - 2.0 * sin(delta) * m2 * (om2 * om2 * PEND_L2 + om1 * om1 * PEND_L1 * cos(delta))
        ) / (PEND_L1 * den);

    dom2 = (2.0 * sin(delta) * (
        om1 * om1 * PEND_L1 * (m1 + m2)
        + g * (m1 + m2) * cos(th1)
        + om2 * om2 * PEND_L2 * m2 * cos(delta)
        )
        ) / (PEND_L2 * den);
}

// RK4で1ステップ積分(呼び出し側で複数回呼んで安定性を上げる)
static void PendulumStepRK4(double& th1, double& om1, double& th2, double& om2, double dt)
{
    double k1_th1, k1_om1, k1_th2, k1_om2;
    double k2_th1, k2_om1, k2_th2, k2_om2;
    double k3_th1, k3_om1, k3_th2, k3_om2;
    double k4_th1, k4_om1, k4_th2, k4_om2;

    PendulumDeriv(th1, om1, th2, om2, k1_th1, k1_om1, k1_th2, k1_om2);
    PendulumDeriv(th1 + 0.5 * dt * k1_th1, om1 + 0.5 * dt * k1_om1,
        th2 + 0.5 * dt * k1_th2, om2 + 0.5 * dt * k1_om2,
        k2_th1, k2_om1, k2_th2, k2_om2);
    PendulumDeriv(th1 + 0.5 * dt * k2_th1, om1 + 0.5 * dt * k2_om1,
        th2 + 0.5 * dt * k2_th2, om2 + 0.5 * dt * k2_om2,
        k3_th1, k3_om1, k3_th2, k3_om2);
    PendulumDeriv(th1 + dt * k3_th1, om1 + dt * k3_om1,
        th2 + dt * k3_th2, om2 + dt * k3_om2,
        k4_th1, k4_om1, k4_th2, k4_om2);

    th1 += dt / 6.0 * (k1_th1 + 2.0 * k2_th1 + 2.0 * k3_th1 + k4_th1);
    om1 += dt / 6.0 * (k1_om1 + 2.0 * k2_om1 + 2.0 * k3_om1 + k4_om1);
    th2 += dt / 6.0 * (k1_th2 + 2.0 * k2_th2 + 2.0 * k3_th2 + k4_th2);
    om2 += dt / 6.0 * (k1_om2 + 2.0 * k2_om2 + 2.0 * k3_om2 + k4_om2);
}

// ============================================================
//  弾幕：二重振り子(カオス・ペンデュラム)
// ============================================================
//
// sEnemyShot の param_i[0] を役割フラグとして使う
//   0 = 腕バレット(毎フレーム座標を腕の形に強制配置)
//   1 = 自由飛行弾(軌跡弾、または霧散後の腕バレット。speed/mukiで飛ぶ)
// 腕バレットのみ param_i[1](1=第1腕/2=第2腕), param_i[2](腕上のindex)を使用
//
// pEnemyShotSet の param_d[0..3] を物理状態(theta1,omega1,theta2,omega2)として使用
// pEnemyShotSet の kind をバリエーション番号(0〜4、初期角オフセットと色)として使用
//
static void ShotDoublePendulum(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    const int variant = pEnemyShotSet->kind % PEND_VARIANT_NUM;
    const double angleOffset = angleOffsetTable[variant];
    const int color = colorTable[variant];

    // ---- 初期化(発生1フレーム目) ----
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = 0.0; // theta1
        pEnemyShotSet->param_d[1] = 0.0; // omega1
        pEnemyShotSet->param_d[2] = 0.0; // theta2
        pEnemyShotSet->param_d[3] = 0.0; // omega2

        // 第1腕のバレット
        for (int i = 1; i <= PEND_ARM1_NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->kind = img_enemyShotSmallBall[color];
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->param_i[0] = 0; // role: 腕
            pEnemyShot->param_i[1] = 1; // 第1腕
            pEnemyShot->param_i[2] = i;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 第2腕のバレット(先端だけ大きめの弾にして視認性を上げる)
        for (int i = 1; i <= PEND_ARM2_NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->kind = (i == PEND_ARM2_NUM) ? img_enemyShotMediumBall[color] : img_enemyShotSmallBall[color];
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->param_i[0] = 0; // role: 腕
            pEnemyShot->param_i[1] = 2; // 第2腕
            pEnemyShot->param_i[2] = i;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- フェーズ判定 ----
    const int  localFrame = pEnemyShotSet->count;
    const bool isDisplayPhase = (localFrame < PEND_DISPLAY_FRAMES);
    const bool isReleasePhase = (!isDisplayPhase) && (localFrame < PEND_DISPLAY_FRAMES + PEND_RELEASE_FRAMES);
    const bool isBurstStartFrame = (localFrame == PEND_DISPLAY_FRAMES + PEND_RELEASE_FRAMES);

    double theta1, theta2;

    if (isDisplayPhase) {
        // 提示フェーズ: 垂直姿勢から基準角へ広がり、その後は基準角を中心に微振動
        const double baseT1 = PEND_BASE_THETA1 + angleOffset;
        const double baseT2 = PEND_BASE_THETA2 + angleOffset;

        if (localFrame < PEND_EASE_FRAMES) {
            double t = (double)localFrame / (double)PEND_EASE_FRAMES;
            double s = t * t * (3.0 - 2.0 * t); // smoothstep
            theta1 = baseT1 * s;
            theta2 = baseT2 * s;
        }
        else {
            double idleT = (double)(localFrame - PEND_EASE_FRAMES);
            theta1 = baseT1 + PEND_IDLE_AMP * sin(PEND_IDLE_FREQ1 * idleT);
            theta2 = baseT2 + PEND_IDLE_AMP * sin(PEND_IDLE_FREQ2 * idleT + 1.3);
        }

        // 解放フェーズへ引き継ぐため、常に物理状態(角度)も更新しておく
        pEnemyShotSet->param_d[0] = theta1;
        pEnemyShotSet->param_d[2] = theta2;
    }
    else if (isReleasePhase) {
        // 解放フェーズ突入の瞬間、角速度0から実際の物理積分を開始する
        if (localFrame == PEND_DISPLAY_FRAMES) {
            pEnemyShotSet->param_d[1] = 0.0;
            pEnemyShotSet->param_d[3] = 0.0;
        }

        double th1 = pEnemyShotSet->param_d[0];
        double om1 = pEnemyShotSet->param_d[1];
        double th2 = pEnemyShotSet->param_d[2];
        double om2 = pEnemyShotSet->param_d[3];

        // dt=0.5を2回積分して安定性を確保
        PendulumStepRK4(th1, om1, th2, om2, 0.5);
        PendulumStepRK4(th1, om1, th2, om2, 0.5);

        pEnemyShotSet->param_d[0] = th1;
        pEnemyShotSet->param_d[1] = om1;
        pEnemyShotSet->param_d[2] = th2;
        pEnemyShotSet->param_d[3] = om2;

        theta1 = th1;
        theta2 = th2;
    }
    else {
        // 霧散フェーズ: 直前の姿勢を保持(腕バレットはこの後すぐ自由飛行に切り替わる)
        theta1 = pEnemyShotSet->param_d[0];
        theta2 = pEnemyShotSet->param_d[2];
    }

    // ---- 腕の代表座標(支点・関節・先端)を計算 ----
    const double pivotX = pEnemyShotSet->x;
    const double pivotY = pEnemyShotSet->y;
    const double jointX = pivotX + PEND_ARM1_LEN * sin(theta1);
    const double jointY = pivotY + PEND_ARM1_LEN * cos(theta1);
    const double tipX = jointX + PEND_ARM2_LEN * sin(theta2);
    const double tipY = jointY + PEND_ARM2_LEN * cos(theta2);

    // 先端の速度(見た目の腕長ベースの概算。軌跡弾の射出方向・速さに使用)
    const double omega1 = pEnemyShotSet->param_d[1];
    const double omega2 = pEnemyShotSet->param_d[3];
    const double tipVx = PEND_ARM1_LEN * omega1 * cos(theta1) + PEND_ARM2_LEN * omega2 * cos(theta2);
    const double tipVy = -PEND_ARM1_LEN * omega1 * sin(theta1) - PEND_ARM2_LEN * omega2 * sin(theta2);

    // ---- 弾リストの更新 ----
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShot->param_i[0] == 0) {
            // 腕バレット
            if (isBurstStartFrame) {
                // 霧散: 支点からの方向へ放射状の自由飛行に切り替える
                double dx = pEnemyShot->x - pivotX;
                double dy = pEnemyShot->y - pivotY;
                double dist = sqrt(dx * dx + dy * dy);
                double outMuki = (dist > 0.001) ? atan2(dy, dx) : (GetRand(3600) / 3600.0 * 2.0 * DX_PI);

                pEnemyShot->param_i[0] = 1; // role: 自由飛行
                pEnemyShot->muki = outMuki;
                pEnemyShot->speed = PEND_BURST_SPEED_MIN
                    + (PEND_BURST_SPEED_MAX - PEND_BURST_SPEED_MIN) * (GetRand(1000) / 1000.0);
            }
            else {
                // 通常時: 腕の形状に沿って座標を再配置(提示フェーズ・解放フェーズ共通)
                if (pEnemyShot->param_i[1] == 1) {
                    double t = (double)pEnemyShot->param_i[2] / (double)PEND_ARM1_NUM;
                    pEnemyShot->x = pivotX + (jointX - pivotX) * t;
                    pEnemyShot->y = pivotY + (jointY - pivotY) * t;
                }
                else {
                    double t = (double)pEnemyShot->param_i[2] / (double)PEND_ARM2_NUM;
                    pEnemyShot->x = jointX + (tipX - jointX) * t;
                    pEnemyShot->y = jointY + (tipY - jointY) * t;
                }
            }
        }

        if (pEnemyShot->param_i[0] == 1) {
            // 自由飛行(軌跡弾、または霧散した腕バレット)
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }

    // ---- 軌跡弾(すり流し弾)の発射: 解放フェーズのみ ----
    if (isReleasePhase && (localFrame % PEND_TRAIL_INTERVAL == 0)) {
        double vlen = sqrt(tipVx * tipVx + tipVy * tipVy);
        double trailMuki = (vlen > 0.001) ? atan2(tipVy, tipVx) : (GetRand(3600) / 3600.0 * 2.0 * DX_PI);
        double trailSpeed = vlen * PEND_TRAIL_SPEED_SCALE;
        if (trailSpeed < PEND_TRAIL_SPEED_MIN) trailSpeed = PEND_TRAIL_SPEED_MIN;
        if (trailSpeed > PEND_TRAIL_SPEED_MAX) trailSpeed = PEND_TRAIL_SPEED_MAX;

        pEnemyShot = new sEnemyShot;
        pEnemyShot->kind = img_enemyShotDiamond[color];
        pEnemyShot->x = tipX;
        pEnemyShot->y = tipY;
        pEnemyShot->muki = trailMuki;
        pEnemyShot->speed = trailSpeed;
        pEnemyShot->param_i[0] = 1; // role: 自由飛行

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_DoublePendulum_Claude()
{
    static int nextWaveFrame;

    const int WAVE_INTERVAL = 1220; // 次の波(振り子群)を発生させるまでの間隔
    const int NUM_PENDULUMS = 7;   // 同時発生させる振り子の数(バタフライ効果用)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        nextWaveFrame = 30; // 出現から60フレーム後に最初の振り子を発生させる
    }
    else {
        // 左右にゆったり漂う(支点は発生時の位置に固定されるため機体そのものの動き)
        enemy.x = 240.0 + 60.0 * sin(count / 130.0);
    }

    if (count == nextWaveFrame) {
        for (int i = 0; i < NUM_PENDULUMS; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotDoublePendulum;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 15.0;
            pEnemyShotSet->kind = i; // バリエーション番号(初期角オフセット・色に使用)

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        nextWaveFrame = count + WAVE_INTERVAL;
    }
}