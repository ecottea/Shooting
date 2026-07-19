// enemyPat_Tmp.cpp
// 変速伸縮レーザー ― 裁きの螺旋牢獄
// 画面中央のボスが4本のレーザーアームを展開し、それぞれが異なる速度で回転しながら伸縮。
// アームの先端からは針弾（小さなレーザー弾）が接線方向に定期的に放出される。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#define ARM_COUNT         4   // アームの本数
#define SEGMENTS_PER_ARM  5   // 1本のアームを構成するレーザー弾の数（長さを表現）
#define NEEDLE_INTERVAL   5   // 針弾を発射するフレーム間隔

// アームの挙動パラメータ（各アームで異なる値を持たせる）
static const double OMEGA_BASE = 0.02;               // 回転速度の基本値
static const double A[ARM_COUNT] = { 0.03, 0.025, 0.035, 0.02 };   // 速度変動の振幅
static const double B[ARM_COUNT] = { 0.05, 0.04, 0.06, 0.045 };    // 速度変動の周波数
static const double PHI[ARM_COUNT] = { 0.0, DX_PI / 4.0, DX_PI / 2.0, 3.0 * DX_PI / 4.0 }; // 位相
static const double L_MIN = 40.0;                  // アームの最小長さ
static const double DELTA_L = 180.0;                 // 伸縮の振幅
static const double C[ARM_COUNT] = { 0.03, 0.035, 0.025, 0.04 };   // 伸縮の速度

// 弾幕パターン関数
static void LaserSpiralPrison(sEnemyShotSet* pSet)
{
    // --- 初回フレーム（count==0）：アーム描画用のレーザー弾を生成 ---
    if (pSet->count == 0) {
        // 各アームの初期角度を pSet->param_d[4]～[7] に格納
        for (int a = 0; a < ARM_COUNT; ++a) {
            pSet->param_d[4 + a] = a * (DX_PI / 2.0);   // 0°, 90°, 180°, 270°
        }

        // アームを構成するレーザー弾（セグメント）を作成
        for (int a = 0; a < ARM_COUNT; ++a) {
            for (int s = 0; s < SEGMENTS_PER_ARM; ++s) {
                sEnemyShot* shot = new sEnemyShot;
                shot->x = enemy.x;                        // 後で更新
                shot->y = enemy.y;
                shot->muki = pSet->param_d[4 + a];        // アームの向きに合わせる
                shot->speed = 0.0;                        // 自動移動しない
                shot->kind = img_enemyShotLaser[0];       // 赤色レーザー
                shot->count = 0;
                shot->margin = 20.0;
                shot->param_i[0] = a;   // アーム番号
                shot->param_i[1] = 0;   // 種類フラグ: 0=アーム描画用, 1=針弾
                shot->param_i[2] = s;   // セグメント番号

                // リンクリストへ追加
                shot->prev = pSet->pEnemyShotHead->prev;
                shot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = shot;
                pSet->pEnemyShotHead->prev = shot;
            }
        }
    }

    // --- アームの角度を更新（毎フレーム） ---
    for (int a = 0; a < ARM_COUNT; ++a) {
        double omega = A[a] * sin(B[a] * pSet->count + PHI[a]) + OMEGA_BASE;
        pSet->param_d[4 + a] += omega;
        // 0～2π の範囲に正規化
        if (pSet->param_d[4 + a] > DX_PI * 2.0) pSet->param_d[4 + a] -= DX_PI * 2.0;
        if (pSet->param_d[4 + a] < 0.0)         pSet->param_d[4 + a] += DX_PI * 2.0;
    }

    // --- アームの長さを計算 ---
    double L[ARM_COUNT];
    for (int a = 0; a < ARM_COUNT; ++a) {
        L[a] = L_MIN + DELTA_L * fabs(sin(C[a] * pSet->count));
    }

    // --- アーム描画用レーザー弾の位置を更新 ---
    sEnemyShot* shot = pSet->pEnemyShotHead->next;
    while (shot != pSet->pEnemyShotHead) {
        if (shot->param_i[1] == 0) {   // アームのセグメント
            int armIdx = shot->param_i[0];
            int segIdx = shot->param_i[2];
            double angle = pSet->param_d[4 + armIdx];
            double dist = (segIdx + 0.5) * (L[armIdx] / SEGMENTS_PER_ARM);
            shot->x = enemy.x + dist * cos(angle);
            shot->y = enemy.y + dist * sin(angle);
            shot->muki = angle;        // 回転を合わせて描画
        }
        shot = shot->next;
    }

    // --- 一定間隔で針弾（小さなレーザー弾）を発射 ---
    if (pSet->count % NEEDLE_INTERVAL == 0 && pSet->count > 0) {
        // 効果音（中程度）
        if (!CheckSoundMem(sound_enemyShot_medium))
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int a = 0; a < ARM_COUNT; ++a) {
            double angle = pSet->param_d[4 + a];
            // 接線方向（±90°）に2発
            for (int dir = 0; dir < 2; ++dir) {
                double muki = angle + (dir == 0 ? DX_PI / 2.0 : -DX_PI / 2.0);
                sEnemyShot* needle = new sEnemyShot;
                needle->x = enemy.x + L[a] * cos(angle);
                needle->y = enemy.y + L[a] * sin(angle);
                needle->muki = muki;
                needle->speed = 3.0;
                needle->kind = img_enemyShotLaser[6];   // 白色レーザー
                needle->count = 0;
                needle->margin = 100.0;
                needle->param_i[1] = 1;   // 針弾フラグ

                // リンクリストへ追加
                needle->prev = pSet->pEnemyShotHead->prev;
                needle->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = needle;
                pSet->pEnemyShotHead->prev = needle;
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_Laser_DeepSeek()
{
    // ゲーム開始時（count==1）にボスと弾幕セットを初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = LaserSpiralPrison;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // ダミーヘッダの作成（弾リストの起点）
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 全体の敵弾セットリストに接続
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}