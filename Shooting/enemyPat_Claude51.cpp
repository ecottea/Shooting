// enemyPat_dousenKairou.cpp
//
// 「銅線回廊」 - イライラ棒モチーフ弾幕
// -----------------------------------------------------------
// イライラ棒（針金の輪を触れずにコースへ沿わせてゴールまで運ぶ玩具）を
// モチーフにした、画面を貫く一本道のトンネル型弾幕。
// 2本の銅線状の壁（ワイヤー弾）がうねりながらトンネルを形成し、
// 自機はトンネルの内側だけを頼りに進む。壁に沿って走る電流弾は
// 一定時間で軌道を離脱し、トンネル内側へ弾かれて追加の危険となる。
// 要所（くびれ／バズポイント）では壁が急激に狭まり、挟み込むように
// バースト弾が発生する。
//
// フェーズ構成：
//   Phase0（導入）    ：緩やかな正弦波、幅も広い
//   Phase1（加速）    ：うねりの周波数・振幅が増加、幅が徐々に狭まる
//   Phase2（クライマックス）：2つの周波数を重ねた交差・ループ状の複雑な軌道
//
// 実装方針：
// ・弾の座標は pShot->count（生成からの経過フレーム）から式で直接算出し、
//   速度の積算（x += vx など）は行わない。
// ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//   および画面外に出た弾の削除はメインルーチン側の仕様に従う（本ファイルでは行わない）。
// ・弾幕セットは種類ごとに1つだけ生成し、以後は各パターン関数が自身の
//   count に応じて内部で弾を生成し続ける（sEnemyShotSetの無駄な量産を避ける）。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// トンネル形状パラメータ
// ============================================================

static const double SCREEN_CENTER_X = 240.0;
static const double SPAWN_Y = -10.0;      // 壁・電流弾の発生y座標（画面上端よりやや外）

static const double WIRE_FALL_SPEED = 2.4;       // 壁(ワイヤー)が下降する速さ
static const int    WIRE_SPAWN_INTERVAL = 1;     // 壁を生成する間隔（フレーム）

static const double CURRENT_FALL_SPEED = 4.6;    // 電流弾が下降する速さ（壁より速い）
static const int    CURRENT_SPAWN_INTERVAL = 26; // 電流弾を生成する間隔

static const int PHASE1_LEN = 280; // 導入：ゆったり蛇行
static const int PHASE2_LEN = 405; // 加速：うねりが増す
static const int PHASE3_LEN = 540; // クライマックス：交差・ループ状の複雑な軌道
static const int CYCLE_LEN = PHASE1_LEN + PHASE2_LEN + PHASE3_LEN;

static const int KUBIRE_CYCLE = 180; // バズポイント(くびれ)の周期
static const int KUBIRE_SPAN = 40;   // くびれが継続するフレーム数

// 負の値でも安全に周期を取るための剰余
static int Wrap(int t, int cycle)
{
    int m = t % cycle;
    if (m < 0) m += cycle;
    return m;
}

static int GetPhase(int t)
{
    int local = Wrap(t, CYCLE_LEN);
    if (local < PHASE1_LEN) return 0;
    if (local < PHASE1_LEN + PHASE2_LEN) return 1;
    return 2;
}

// トンネル中心のx座標（時刻tにおけるトンネル形状）
static double TunnelCenterX(int t)
{
    int phase = GetPhase(t);
    int local = Wrap(t, CYCLE_LEN);

    if (phase == 0) {
        double freq = 0.008;
        double amp = 70.0;
        return SCREEN_CENTER_X + amp * sin(local * freq);
    }
    else if (phase == 1) {
        double localP = local - PHASE1_LEN;
        double freq = 0.008 + 0.010 * (localP / (double)PHASE2_LEN);
        double amp = 70.0 + 40.0 * (localP / (double)PHASE2_LEN);
        return SCREEN_CENTER_X + amp * sin(localP * freq + 1.0);
    }
    else {
        double localP = local - PHASE1_LEN - PHASE2_LEN;
        // 2つの周波数を重ね、交差・ループ状の複雑な軌道を作る
        double base = SCREEN_CENTER_X + 100.0 * sin(localP * 0.020 + 2.0);
        double wobble = 35.0 * sin(localP * 0.045);
        return base + wobble;
    }
}

// トンネルの半幅（くびれ区間で狭くなる）
static double TunnelHalfWidth(int t)
{
    int phase = GetPhase(t);
    int local = Wrap(t, CYCLE_LEN);

    double base;
    if (phase == 0) {
        base = 100.0;
    }
    else if (phase == 1) {
        base = 100.0 - 35.0 * ((local - PHASE1_LEN) / (double)PHASE2_LEN);
    }
    else {
        base = 60.0;
    }

    if (phase >= 1) {
        int m = Wrap(t, KUBIRE_CYCLE);
        if (m < KUBIRE_SPAN) {
            double p = m / (double)KUBIRE_SPAN;
            double narrow = sin(p * DX_PI); // 0→1→0 で滑らかに絞る
            base -= narrow * 35.0;
        }
    }

    if (base < 22.0) base = 22.0; // 自機が絶対に通れなくなる幅は避ける
    return base;
}

// ============================================================
// 生成ヘルパー（enemyPat_sampleForAI.cpp の作法に準拠）
// ============================================================

static sEnemyShotSet* NewShotSet(sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = 0.0;
    pSet->y = 0.0;
    pSet->muki = 0.0;
    pSet->kind = 0;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

static sEnemyShot* AddShot(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ============================================================
// ワイヤー(壁)：2本の銅線状の壁を形成する
// 各shot: param_d[0]=固定x, param_d[1]=基準y, param_d[2]=落下速度
// ============================================================
static void WirePattern(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % WIRE_SPAWN_INTERVAL == 0) {
        int t = pEnemyShotSet->count; // このセットの経過フレーム＝弾幕全体の経過フレーム
        double centerX = TunnelCenterX(t);
        double halfW = TunnelHalfWidth(t);

        const int sides[2] = { -1, +1 };
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pShot = AddShot(pEnemyShotSet);
            double x0 = centerX + sides[i] * halfW;

            // 弾の種類一覧: 鱗弾(4.0x3.0)は小さくカーブへの追従がよいため壁に採用
            // 弾の色一覧: 8:橙 → 銅線の質感を表現
            pShot->kind = img_enemyShotScale[8];
            pShot->param_d[0] = x0;
            pShot->param_d[1] = SPAWN_Y;
            pShot->param_d[2] = WIRE_FALL_SPEED;
            pShot->x = x0;
            pShot->y = SPAWN_Y;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // xは誕生時に固定した位置のまま、yだけcountから直接計算する
        pShot->x = pShot->param_d[0];
        pShot->y = pShot->param_d[1] + pShot->param_d[2] * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
// 電流弾：ワイヤーに沿って壁より速く流れ、途中でトンネル内側へ弾かれる
// 各shot:
//   param_d[0] = 誕生時刻 t_birth（トンネル形状参照用）
//   param_d[1] = 追従する側 side（-1 or +1）
//   param_i[0] = 離脱するタイミング（生成からの経過フレーム）
//   param_i[1] = 離脱済みフラグ（0:追従中 / 1:離脱済み）
//   param_d[2],[3] = 離脱した瞬間の座標
//   param_d[4],[5] = 離脱後の速度(x,y)
//   param_d[6] = 離脱した瞬間の経過フレーム
// ============================================================
static void CurrentPattern(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % CURRENT_SPAWN_INTERVAL == 0) {
        int t = pEnemyShotSet->count;
        int phase = GetPhase(t);
        if (phase >= 0) { // 導入フェーズには出さず、加速以降のみ出現させる
            int side = (pEnemyShotSet->count / CURRENT_SPAWN_INTERVAL) % 2 == 0 ? -1 : +1;

            sEnemyShot* pShot = AddShot(pEnemyShotSet);
            double centerX = TunnelCenterX(t);
            double halfW = TunnelHalfWidth(t);

            // 弾の種類一覧: 中楕円弾(10.5x7.0)、色6:白 → 発光する電流のイメージ
            pShot->kind = img_enemyShotMediumOval[6];
            pShot->param_d[0] = (double)t;
            pShot->param_d[1] = (double)side;
            pShot->param_i[0] = 40 + GetRand(50); // 離脱まで40〜90フレーム
            pShot->param_i[1] = 0;
            pShot->x = centerX + side * halfW * 0.85;
            pShot->y = SPAWN_Y;
            pShot->margin = 480;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int birthT = (int)pShot->param_d[0];
        int side = (int)pShot->param_d[1];

        if (pShot->param_i[1] == 0 && pShot->count >= pShot->param_i[0]) {
            // 離脱の瞬間：現在地点と弾かれる方向を確定する
            pShot->param_i[1] = 1;
            pShot->param_d[2] = pShot->x;
            pShot->param_d[3] = pShot->y;
            pShot->param_d[4] = -side * (1.2 + GetRand(180) / 100.0); // トンネル内側へ
            pShot->param_d[5] = CURRENT_FALL_SPEED + GetRand(150) / 100.0;
            pShot->param_d[6] = (double)pShot->count;
        }

        if (pShot->param_i[1] == 0) {
            // ワイヤーに沿って追従する（現在のyに相当する仮想生成時刻を逆算）
            double tWire = birthT + pShot->count * (1.0 - CURRENT_FALL_SPEED / WIRE_FALL_SPEED);
            double centerX = TunnelCenterX((int)tWire);
            double halfW = TunnelHalfWidth((int)tWire);
            pShot->x = centerX + side * halfW * 0.85;
            pShot->y = SPAWN_Y + CURRENT_FALL_SPEED * pShot->count;
        }
        else {
            // 離脱後：直線的にトンネル内側へ弾かれていく
            double dt = pShot->count - pShot->param_d[6];
            pShot->x = pShot->param_d[2] + pShot->param_d[4] * dt;
            pShot->y = pShot->param_d[3] + pShot->param_d[5] * dt;
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 挟み込みバースト：くびれ開始の瞬間、左右の壁から内側へ突き刺す
// 各shot: param_d[0]=固定x0, [1]=基準y0, [2]=y方向速度, [3]=x方向速度
// ============================================================
static void PinchBurstPattern(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;
    int phase = GetPhase(t);

    if (phase >= 0 && Wrap(t, KUBIRE_CYCLE) == 0) {
        double centerX = TunnelCenterX(t);
        double halfW = TunnelHalfWidth(t);

        const int BURST_NUM = 3;
        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < BURST_NUM; i++) {
                sEnemyShot* pShot = AddShot(pEnemyShotSet);
                double x0 = centerX + side * halfW;
                double y0 = SPAWN_Y - i * 14.0; // 少し手前(上流)から連なって発生させる

                // 弾の種類一覧: 銃弾(5.0x2.0)、色3:シアン → 壁と区別できる警告的な閃光
                pShot->kind = img_enemyShotBullet[3];
                pShot->param_d[0] = x0;
                pShot->param_d[1] = y0;
                pShot->param_d[2] = WIRE_FALL_SPEED;
                pShot->param_d[3] = -side * (2.2 + i * 0.3); // 内側へ突き刺す速さ
                pShot->x = x0;
                pShot->y = y0;
                pShot->margin = 480;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->y = pShot->param_d[1] + pShot->param_d[2] * pShot->count;
        pShot->x = pShot->param_d[0] + pShot->param_d[3] * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン：「銅線回廊」
// ============================================================
void EnemyPat_Irairabou_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 30.0;
        enemy.maxHp = enemy.hp = 300; // 200で固定

        // 3種類の弾幕セットをそれぞれ1つだけ生成する。
        // 以後は各パターン関数が自身の count に応じて内部で弾を生成し続ける。
        NewShotSet(WirePattern);
        NewShotSet(CurrentPattern);
        NewShotSet(PinchBurstPattern);
    }
    else {
        // 銅線を紡ぐ座に留まりながら、わずかに左右へ揺れる
        enemy.x = 240.0 + 15.0 * sin(count * 0.02);
    }
}