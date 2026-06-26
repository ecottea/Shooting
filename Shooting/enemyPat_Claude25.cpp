// enemyPat_fan.cpp
// モチーフ：扇風機

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕サブルーチン①：扇状風弾（ShotFanWave）
//
// muki を中心に左右 37.5 度（計 75 度）の扇形に N 発を一斉発射。
// 中央の弾ほど speed が速く、風圧の強さを表現する。
//
// [sEnemyShotSet 使用フィールド]
//   x, y  : 発射位置
//   muki  : 扇の中心方向（ラジアン）
// [使用素材]
//   img_enemyShotScale[3]  : シアン鱗弾（風のイメージ）
//   sound_enemyShot_light  : 軽い連射音
// ============================================================
static void ShotFanWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int    N = 9;
        const double SPREAD = DX_PI * 75.0 / 180.0; // 扇の総開き角

        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // t : -0.5（左端）〜 0（中央）〜 +0.5（右端）
            double t = (double)i / (N - 1) - 0.5;
            pEnemyShot->muki = pEnemyShotSet->muki + SPREAD * t;
            // 中央(t=0) が最速 4.0、両端(t=±0.5) が最遅 3.5
            pEnemyShot->speed = 3.5 + 1.0 * (0.5 - fabs(t));
            pEnemyShot->kind = img_enemyShotScale[3]; // シアン

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 直進
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 弾幕サブルーチン②：回転羽根弾（ShotFanBlade）
//
// 120 度等間隔の 3 発を発射し、毎フレーム muki を回転させる。
// 弾は円弧を描く弾道（理論半径 ≒ 86px）で画面を漂い、
// 扇風機の羽根が回転する様子を表現する。
//
// [sEnemyShotSet 使用フィールド]
//   x, y  : 発射位置
//   muki  : 羽根①の初期方向（ラジアン）
//   kind  : 回転方向（+1 = 時計回り、-1 = 反時計回り）
// [sEnemyShot 使用フィールド]
//   param_d[0] : 毎フレームの muki 回転量（ラジアン）
// [使用素材]
//   img_enemyShotDiamond[4] : 青菱形弾（羽根のイメージ）
//   sound_enemyShot_medium  : 羽根展開の重い音
// ============================================================
static void ShotFanBlade(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1 フレームあたり 2 度回転。方向は kind の符号で決まる。
        // speed=3.0、ω=π/90 のとき理論円弧半径 = 3.0/(π/90) ≒ 86px
        const double ROT_SPEED = DX_PI / 90.0 * (double)pEnemyShotSet->kind;

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 3 枚羽根を 120 度等間隔に配置
            pEnemyShot->muki = pEnemyShotSet->muki + (2.0 * DX_PI / 3.0) * i;
            pEnemyShot->speed = 7.0;
            pEnemyShot->kind = img_enemyShotScale[4]; // 青

            // 角速度を param_d[0] に格納して毎フレーム参照
            pEnemyShot->param_d[0] = ROT_SPEED;

            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム向きを回転させながら移動（円弧弾道）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->muki += pEnemyShot->param_d[0];
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体：EnemyPat_ElectricFan_Claude（扇風機）
//
// ■ 動き
//   fan_dir の sin 波で発射方向を ±60 度スイープ（首振り）。
//   敵の X 座標も同じ sin 波で左右 80px を往復し、
//   扇風機の頭が向きを変える雰囲気を出す。
//   首振り半周期 = 180f（約 3 秒）。
//
// ■ 弾幕の構成
//   [6f ごと] ShotFanWave : 首振り方向に扇状の風弾を 9 発発射。
//             スイープと連動して左右に掃引する弾幕カーテンを形成。
//   [60f ごと] ShotFanBlade : 120 度等間隔の 3 枚羽根弾を発射。
//             時計回り／反時計回りを交互に切り替え、
//             円弧弾道で画面中央部に変化をもたらす。
// ============================================================
void EnemyPat_ElectricFan_Claude()
{
    static double fan_dir;    // 首振り位相（ラジアン、毎フレーム加算）
    static double fan_angle;  // 現在の発射中心方向（ラジアン）
    static int    blade_sign; // 羽根弾の回転方向（+1 or -1）

    // ---- 初期化（count == 1 のときのみ実行）----
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        fan_dir = 0.0;
        fan_angle = DX_PI / 2.0; // 初期発射方向：真下
        blade_sign = 1;
    }
    else {
        // 首振り：約 180f で ±60 度を半往復（sin 波）
        fan_dir += DX_PI / 90.0;
        fan_angle = DX_PI / 2.0 + sin(fan_dir) * (DX_PI * 60.0 / 180.0);

        // 敵の X 座標を首振りに連動させる（左右 80px の範囲）
        enemy.x = 240.0 + sin(fan_dir) * 80.0;
    }

    // ---- 弾幕①：扇状風弾（6 フレームに 1 回）----
    if (count % 6 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFanWave;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = fan_angle;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ---- 弾幕②：回転羽根弾（60 フレームに 1 回）----
    if (count % 70 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFanBlade;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = fan_angle;
        pEnemyShotSet->kind = blade_sign; // 回転方向を kind で渡す

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        blade_sign *= -1; // 次回は逆方向に回転
    }
}