// enemyPat_spiralSeam.cpp
// 弾幕パターン：螺旋の縫い目（Spiral Seam）
// 6本のへにょりレーザー（1セット=1レーザー）が交差し、菱形の安全地帯を作る
// セットごとにパラメータが変化し、毎回違った形のへにょりレーザーが飛んでくる

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// レーザー1本の軌跡生成
// param_i[0] : レーザー番号 (0-5)
// param_i[1] : セットごとのシード値（難易度変化用）
// param_d[0] : 開始X
// param_d[1] : 開始Y
// param_d[2] : 進行距離
// param_d[3] : 位相
// param_d[4] : 振幅
// param_d[5] : 基本角度
// param_d[6] : 前回X
// param_d[7] : 前回Y
// ------------------------------------------------------------
static void ShotSpiralSeam(sEnemyShotSet* pSet)
{
    const int i = pSet->param_i[0];
    const int seed = pSet->param_i[1];
    const bool fromLeft = (i < 3);
    const int frame = pSet->count;

    // --- セットごとに変化するパラメータ ---
    // seed を使って毎回違う特性を持つレーザーにする
    const int waveType = (seed + i) % 4;           // 0:正弦波, 1:鋸歯波, 2:三角波, 3:方形波近似
    const int ampPattern = (seed / 3 + i) % 3;   // 振幅変化パターン
    const int speedPattern = (seed / 5 + i) % 3; // 速度変化パターン
    const int colorBase = (seed + i * 2) % 6;    // 色のベース
    const int phaseShift = (seed * 7 + i * 13) % 360; // 位相シフト（度）

    // --- 初期化 ---
    if (frame == 0) {
        if (i == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        pSet->param_d[0] = fromLeft ? -50.0 : 530.0;
        pSet->param_d[1] = 60.0 + (i % 3) * 180.0;
        pSet->param_d[2] = 0.0;
        pSet->param_d[3] = (i % 3) * (2.0 * DX_PI / 3.0) + phaseShift / 180.0 * DX_PI;
        pSet->param_d[4] = 20.0;
        pSet->param_d[5] = fromLeft ? 0.0 : DX_PI;
        pSet->param_d[6] = pSet->param_d[0];
        pSet->param_d[7] = pSet->param_d[1];
    }

    // --- フェーズ別の振幅計算（パターンで変化） ---
    double amp;
    if (ampPattern == 0) {
        // パターン0: 標準（20→120→0）
        if (frame < 120) {
            amp = 20.0 + 100.0 * (frame / 120.0);
        }
        else if (frame < 240) {
            amp = 120.0;
        }
        else if (frame < 300) {
            amp = 120.0 * (1.0 - (frame - 240) / 60.0);
        }
        else {
            amp = 0.0;
        }
    }
    else if (ampPattern == 1) {
        // パターン1: 遅い増大（20→150→0）
        if (frame < 150) {
            amp = 20.0 + 130.0 * (frame / 150.0);
        }
        else if (frame < 270) {
            amp = 150.0;
        }
        else if (frame < 330) {
            amp = 150.0 * (1.0 - (frame - 270) / 60.0);
        }
        else {
            amp = 0.0;
        }
    }
    else {
        // パターン2: 急激な増大（20→180→0）
        if (frame < 90) {
            amp = 20.0 + 160.0 * (frame / 90.0);
        }
        else if (frame < 210) {
            amp = 180.0;
        }
        else if (frame < 270) {
            amp = 180.0 * (1.0 - (frame - 210) / 60.0);
        }
        else {
            amp = 0.0;
        }
    }

    // --- フェーズ別の速度計算（パターンで変化） ---
    double speed;
    if (speedPattern == 0) {
        // パターン0: 標準（2.8→1.6→加速）
        if (frame < 120) {
            speed = 2.8;
        }
        else if (frame < 240) {
            speed = 1.6;
        }
        else if (frame < 300) {
            speed = 2.8 + (frame - 240) / 60.0 * 2.0;
        }
        else {
            speed = 4.8;
        }
    }
    else if (speedPattern == 1) {
        // パターン1: 高速（3.5→2.0→加速）
        if (frame < 120) {
            speed = 3.5;
        }
        else if (frame < 240) {
            speed = 2.0;
        }
        else if (frame < 300) {
            speed = 3.5 + (frame - 240) / 60.0 * 2.5;
        }
        else {
            speed = 6.0;
        }
    }
    else {
        // パターン2: 低速で粘着（2.0→1.0→加速）
        if (frame < 120) {
            speed = 2.0;
        }
        else if (frame < 240) {
            speed = 1.0;
        }
        else if (frame < 300) {
            speed = 2.0 + (frame - 240) / 60.0 * 1.5;
        }
        else {
            speed = 3.5;
        }
    }

    // --- 収束：角度を中央(240,240)へ向ける ---
    double baseAngle = pSet->param_d[5];
    if (frame >= 240 && frame < 300) {
        const double targetAngle = atan2(240.0 - pSet->param_d[1], 240.0 - pSet->param_d[0]);
        double diff = targetAngle - baseAngle;
        while (diff > DX_PI) diff -= 2.0 * DX_PI;
        while (diff < -DX_PI) diff += 2.0 * DX_PI;
        double t = (frame - 240) / 60.0;
        if (t > 1.0) t = 1.0;
        baseAngle += diff * t;
        pSet->param_d[5] = baseAngle;
    }

    // --- 進行距離更新 ---
    pSet->param_d[2] += speed;
    const double dist = pSet->param_d[2];
    const double phase = pSet->param_d[3];
    const double startX = pSet->param_d[0];
    const double startY = pSet->param_d[1];
    const double prevX = pSet->param_d[6];
    const double prevY = pSet->param_d[7];

    // --- 波形変位（へにょり本体） ---
    // waveType で波形を変化させる
    double wave;
    const double arg = dist * 0.04 + phase;
    if (waveType == 0) {
        // 正弦波（標準の蛇行）
        wave = amp * sin(arg);
    }
    else if (waveType == 1) {
        // 鋸歯波（ギザギザした蛇行）
        wave = amp * (2.0 * (arg / (2.0 * DX_PI) - floor(arg / (2.0 * DX_PI) + 0.5)));
    }
    else if (waveType == 2) {
        // 三角波（折り返しの鋭い蛇行）
        wave = amp * (2.0 / DX_PI * asin(sin(arg)));
    }
    else {
        // 方形波近似（急激な方向転換）
        wave = amp * (sin(arg) > 0.0 ? 1.0 : -1.0) * (0.7 + 0.3 * sin(arg * 3.0));
    }

    const double cx = startX + cos(baseAngle) * dist - sin(baseAngle) * wave;
    const double cy = startY + sin(baseAngle) * dist + cos(baseAngle) * wave;

    // --- 軌跡弾生成 ---
    if (frame < 330 && cx > -80.0 && cx < 560.0 && cy > -80.0 && cy < 560.0) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = cx;
        pShot->y = cy;
        pShot->speed = 0.0;

        double tangent = atan2(cy - prevY, cx - prevX);
        if (frame == 0) tangent = baseAngle;
        pShot->muki = tangent;

        // 色：seed と i で毎回異なる配色
        int colorIdx;
        if (frame >= 120 && frame < 180) {
            colorIdx = (colorBase + i) % 7;
        }
        else if (frame >= 240) {
            colorIdx = (colorBase + i + 3) % 7;
        }
        else {
            colorIdx = (colorBase + i + 1) % 7;
        }

        pShot->kind = img_enemyShotBullet[colorIdx];
        pShot->margin = 80.0;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // 前回位置を更新
    pSet->param_d[6] = cx;
    pSet->param_d[7] = cy;

    // --- 軌跡弾の寿命管理 ---
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->count > 80) {
            sEnemyShot* pNext = pShot->next;
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = pNext;
            continue;
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_HenyoriLaser_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (count % 120 == 60) muki *= -1;
        enemy.y = 50.0 + 10.0 * sin(count * 0.03);
    }

    if (count % 200 == 30) {
        for (int i = 0; i < 6; i++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSpiralSeam;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 20.0;
            pSet->muki = 0.0;
            pSet->kind = shot_count++;
            pSet->param_i[0] = i;          // レーザー番号
            pSet->param_i[1] = shot_count; // セットごとのシード値

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}