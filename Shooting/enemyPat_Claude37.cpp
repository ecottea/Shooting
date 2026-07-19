// enemyPat_torus.cpp
//
// 立体トーラス（ドーナツ）ワイヤーフレーム弾幕
// -----------------------------------------------------------
// トーラス表面上の格子点(u, v)を3D空間に配置し、
//   1) 回転させながら奥から手前へ「接近」させる（透視投影）
//   2) 一定時間後、各点をその瞬間の位置・向きのまま実弾として放出する
// という2段階構成で、単なる2D軌道ではなく立体物が迫ってくる
// ように見える弾幕を実現する。
//
// 立体感を強調する工夫：
//   ・速度差　：放出時、手前(z大)の点ほど速く飛ぶ（scale に比例）
//   ・明暗差　：手前は白、中間はシアン、奥は青（弾は全て小玉に統一）
//
// また、回転速度(X軸・Y軸)は弾幕発生のたびにランダムに変化するため、
// 毎回異なる傾き方・回り方でトーラスが迫ってくる。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  調整用定数
// ============================================================
static const double TORUS_MAJOR_RADIUS = 70.0 * 2;   // トーラスの大円半径
static const double TORUS_MINOR_RADIUS = 26.0 * 2;   // トーラスの管（チューブ）半径
static const int    TORUS_U_DIVISIONS = 16 * 2;      // 大円方向の分割数
static const int    TORUS_V_DIVISIONS = 8 * 2;       // 管方向の分割数

// 回転速度は固定値ではなく、弾幕を撃つたびにこの範囲内でランダムに決定する
// （符号もランダムなので、回転方向も毎回変わる）
static const double TORUS_ROTATION_SPEED_MIN = 0.006;  // 1フレームあたりの回転速度の下限（弧度）
static const double TORUS_ROTATION_SPEED_MAX = 0.020;  // 1フレームあたりの回転速度の上限（弧度）

static const double TORUS_FOCAL_LENGTH = 300.0;  // 透視投影の焦点距離（小さいほど遠近感が強調される）
static const double TORUS_INITIAL_DEPTH_OFFSET = 260.0; // 形成開始時の「奥行きオフセット」。0へ向けて減少＝接近演出
static const int    TORUS_FORMATION_FRAMES = 180;    // 形成～接近フェーズの長さ（フレーム数）

static const double TORUS_RELEASE_BASE_SPEED = 1.5;    // 放出時の基準速度（scale倍されるので手前ほど速くなる）

static const double TORUS_DEPTH_NEAR_THRESHOLD = 37.5;  // これより z が大きければ「手前」扱い（旧25.0の1.5倍）
static const double TORUS_DEPTH_FAR_THRESHOLD = -37.5; // これより z が小さければ「奥」扱い（旧-25.0の1.5倍）

static const int    TORUS_ATTACK_INTERVAL = 200;    // トーラス弾幕を繰り返し発生させる間隔（フレーム）

// ============================================================
//  奥行き(z)に応じて弾の見た目（色）を決定
//  ※弾種は全て小玉に統一し、色の違いだけで奥行きを表現する
// ============================================================
static int TorusDepthToKind(double z)
{
    // 色一覧: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
    if (z > TORUS_DEPTH_NEAR_THRESHOLD) {
        // 手前：明るい白
        return img_enemyShotSmallBall[6];
    }
    else if (z < TORUS_DEPTH_FAR_THRESHOLD) {
        // 奥：暗めの青
        return img_enemyShotSmallBall[4];
    }
    else {
        // 中間：シアン
        return img_enemyShotSmallBall[3];
    }
}

// ============================================================
//  弾幕：立体トーラス（形成・接近 → 放出）
// ============================================================
static void ShotTorus(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ---- 初回のみ：トーラス表面の格子点を全て子弾として生成 ----
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_heavy)) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int ui = 0; ui < TORUS_U_DIVISIONS; ui++) {
            for (int vi = 0; vi < TORUS_V_DIVISIONS; vi++) {
                pEnemyShot = new sEnemyShot;

                double u = (2.0 * DX_PI * ui) / TORUS_U_DIVISIONS;
                double v = (2.0 * DX_PI * vi) / TORUS_V_DIVISIONS;

                pEnemyShot->param_d[0] = u;   // 大円方向パラメータ
                pEnemyShot->param_d[1] = v;   // 管方向パラメータ
                pEnemyShot->param_i[0] = 0;   // フェーズ：0=形成中, 1=放出済み

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[3]; // 仮の初期色（シアン）

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ---- 毎フレーム：回転角と接近オフセットを更新 ----
    // param_d[4]/[5] にはこのトーラス個体専用のランダムな回転速度が入っている
    double rotX = pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[4];
    double rotY = pEnemyShotSet->param_d[1] += pEnemyShotSet->param_d[5];

    double depthOffset = TORUS_INITIAL_DEPTH_OFFSET
        - (TORUS_INITIAL_DEPTH_OFFSET / TORUS_FORMATION_FRAMES) * pEnemyShotSet->count;
    if (depthOffset < 0.0) depthOffset = 0.0;

    // ---- 子弾の更新 ----
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShot->param_i[0] == 0) {
            // フェーズ0：トーラス上を回転しながら接近する表示専用の弾

            double u = pEnemyShot->param_d[0];
            double v = pEnemyShot->param_d[1];

            // トーラス表面座標
            double x0 = (TORUS_MAJOR_RADIUS + TORUS_MINOR_RADIUS * cos(v)) * cos(u);
            double y0 = (TORUS_MAJOR_RADIUS + TORUS_MINOR_RADIUS * cos(v)) * sin(u);
            double z0 = TORUS_MINOR_RADIUS * sin(v);

            // X軸周り回転
            double x1 = x0;
            double y1 = y0 * cos(rotX) - z0 * sin(rotX);
            double z1 = y0 * sin(rotX) + z0 * cos(rotX);

            // Y軸周り回転
            double x2 = x1 * cos(rotY) + z1 * sin(rotY);
            double y2 = y1;
            double z2 = -x1 * sin(rotY) + z1 * cos(rotY);

            // 透視投影（denom が小さくなりすぎる＝カメラを突き抜けるのを防止）
            double denom = TORUS_FOCAL_LENGTH + depthOffset + z2;
            if (denom < 1.0) denom = 1.0;
            double scale = TORUS_FOCAL_LENGTH / denom;

            double screenX = scale * x2;
            double screenY = scale * y2;

            pEnemyShot->x = pEnemyShotSet->x + screenX;
            pEnemyShot->y = pEnemyShotSet->y + screenY;
            pEnemyShot->kind = TorusDepthToKind(z2);

            pEnemyShot->param_d[2] = z2;    // 放出時の速度計算用に保存
            pEnemyShot->param_d[3] = scale; // 同上

            // 形成フェーズ終了 → 実弾として放出
            if (pEnemyShotSet->count >= TORUS_FORMATION_FRAMES) {
                double muki = atan2(screenY, screenX);
                if (screenX == 0.0 && screenY == 0.0) {
                    muki = (GetRand(3600) / 3600.0) * 2.0 * DX_PI;
                }
                pEnemyShot->muki = muki;
                pEnemyShot->speed = TORUS_RELEASE_BASE_SPEED * scale; // 手前(scale大)ほど速い
                pEnemyShot->param_i[0] = 1;
            }
        }
        else {
            // フェーズ1：放出済み。直進する実弾として扱う
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_3D_Claude()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        enemy.x += 0.4 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 一定間隔でトーラス弾幕を発生させる
    if ((count - 1) % TORUS_ATTACK_INTERVAL == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTorus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_d[0] = 0.0; // 回転角X（累積値の初期値）
        pEnemyShotSet->param_d[1] = 0.0; // 回転角Y（累積値の初期値）

        // 回転速度をこの個体専用にランダム決定（大きさ・符号とも毎回変える）
        {
            double rangeWidth = TORUS_ROTATION_SPEED_MAX - TORUS_ROTATION_SPEED_MIN;

            double magX = TORUS_ROTATION_SPEED_MIN + rangeWidth * (GetRand(1000) / 1000.0);
            double signX = (GetRand(1) == 0) ? 1.0 : -1.0;
            pEnemyShotSet->param_d[4] = magX * signX;

            double magY = TORUS_ROTATION_SPEED_MIN + rangeWidth * (GetRand(1000) / 1000.0);
            double signY = (GetRand(1) == 0) ? 1.0 : -1.0;
            pEnemyShotSet->param_d[5] = magY * signY;
        }

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}