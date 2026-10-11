#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：平安京の悪夢
// ============================================================

// 弾セットを生成して enemyShotSetHead に繋ぐ
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// 弾を1つ追加
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->count = 0;
    p->margin = 60;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;

    return p;
}

// セット内の弾をすべて削除
static void DeleteAllShots(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;
        p->prev->next = p->next;
        p->next->prev = p->prev;
        delete p;
        p = next;
    }
}

// ============================================================
// 基本パターン更新
// ============================================================
static void UpdateBasicPattern(sEnemyShotSet* pSet)
{
    if (pSet->param_i[5]) return; // 終了済み

    int T = pSet->param_i[1];
    int c = pSet->count;

    const int wait1 = 60;                 // 出現後 1 秒待ち
    const int moveStart = wait1;
    const int moveEnd = moveStart + 2 * T; // 加速 T + 減速 T
    const int afterWaitEnd = moveEnd + 60; // 移動後 1 秒待ち
    const int finish = afterWaitEnd + 60;  // 消去後 1 秒待ち

    // 移動中：レーザー→中玉、smoothstep で加速減速
    if (c >= moveStart && c < moveEnd) {
        double u = (double)(c - moveStart) / (2.0 * T);
        double s = u * u * (3.0 - 2.0 * u); // 0→1、両端で速度0

        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (c == moveStart) {
                p->kind = img_enemyShotMediumBall[0]; // 赤中玉
            }
            p->x = p->param_d[0] + (p->param_d[2] - p->param_d[0]) * s;
            p->y = p->param_d[1] + (p->param_d[3] - p->param_d[1]) * s;
            p->speed = 0.0;
            p = p->next;
        }
    }
    // 移動終了：中玉→レーザー
    else if (c == moveEnd) {
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            p->x = p->param_d[2];
            p->y = p->param_d[3];
            p->kind = img_enemyShotLaser[0]; // 赤レーザー
            p->speed = 0.0;
            p = p->next;
        }
    }

    // 移動後 1 秒待って全レーザー消去
    if (c == afterWaitEnd) {
        DeleteAllShots(pSet);
        pSet->param_i[5] = 1;
        return;
    }

    // 消去後 1 秒待って基本パターン終了
    if (c >= finish) {
        // 何もしない（終了）
    }
}

// ============================================================
// 基本パターン生成
// transpose = 0 : 通常
// transpose = 1 : x/y 転置
// 戻り値：基本パターンの総時間
// ============================================================
static int CreateBasicPattern(int transpose)
{
    // パラメータ生成
    const int W = 80;                 // x 方向間隔（固定）
    int N = 4 + GetRand(4);           // 4..8
    int T = 45 + GetRand(45);         // 45..90
    int H = 30 + GetRand(20);         // 30..50

    int P[8], R[8], D[8], Q[8];

    // ランダム順列 P
    for (int i = 0; i < N; i++) P[i] = i;
    for (int i = N - 1; i > 0; i--) {
        int j = GetRand(i); // 0..i
        int tmp = P[i]; P[i] = P[j]; P[j] = tmp;
    }

    // ランダムビット列 R, D と Q
    for (int i = 0; i < N; i++) {
        R[i] = GetRand(1); // 0 or 1
        D[i] = GetRand(1); // 0 or 1
        Q[i] = ((P[i] - i) % N + N) % N;
    }

    sEnemyShotSet* pSet = CreateShotSet(UpdateBasicPattern, enemy.x, enemy.y, 0.0, transpose);
    pSet->param_i[0] = N;
    pSet->param_i[1] = T;
    pSet->param_i[2] = W;
    pSet->param_i[3] = H;
    pSet->param_i[4] = transpose;
    pSet->param_i[5] = 0; // 終了フラグ

    int Imax = (int)(480.0 / W);
    int Jmax = (int)(480.0 / H);

    // グリッド上に赤レーザーを出現させる
    for (int j = 0; j <= Jmax; j++) {
        int r = j % N;
        int q = j / N;

        for (int i = 0; i <= Imax; i++) {
            int i2, j2;

            if (R[r]) i2 = i + 1;
            else      i2 = i - 1;

            if (D[r]) j2 = j + Q[r];
            else      j2 = j + Q[r] - N;

            double sx, sy, tx, ty, muki;

            if (!transpose) {
                // 通常：下向き赤レーザー
                sx = i * W;
                sy = j * H;
                tx = i2 * W;
                ty = j2 * H;
                muki = DX_PI / 2.0;
            }
            else {
                // x/y 転置：右向き赤レーザー
                sx = j * H;
                sy = i * W;
                tx = j2 * H;
                ty = i2 * W;
                muki = 0.0;
            }

            sEnemyShot* p = AddShot(pSet, sx, sy, muki, 0.0, img_enemyShotLaser[0]);
            p->param_d[0] = sx;
            p->param_d[1] = sy;
            p->param_d[2] = tx;
            p->param_d[3] = ty;
        }
    }

    // 予告音
    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

    return 2 * T + 180;
}

// ============================================================
// 大玉列：上下方向
// ============================================================
static void ShotVerticalLargeColumn(sEnemyShotSet* pSet)
{
    int duration = pSet->param_i[0];
    int c = pSet->count;

    // 一定間隔で大玉を追加
    if (c <= duration && c % 6 == 0) {
        int fromTop = pSet->param_i[1]; // 0:下から, 1:上から
        double x = player.x;
        double y, muki;

        if (fromTop) {
            y = -30.0;
            muki = DX_PI / 2.0;
        }
        else {
            y = 510.0;
            muki = -DX_PI / 2.0;
        }

        AddShot(pSet, x, y, muki, 3.0, img_enemyShotLargeBall[0]);
    }

    // 大玉移動
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
// 大玉列：左右方向
// ============================================================
static void ShotHorizontalLargeColumn(sEnemyShotSet* pSet)
{
    int duration = pSet->param_i[0];
    int c = pSet->count;

    if (c <= duration && c % 6 == 0) {
        int fromLeft = pSet->param_i[1]; // 0:右から, 1:左から
        double y = player.y;
        double x, muki;

        if (fromLeft) {
            x = -30.0;
            muki = 0.0;
        }
        else {
            x = 510.0;
            muki = DX_PI;
        }

        AddShot(pSet, x, y, muki, 3.0, img_enemyShotLargeBall[0]);
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Heiankyo_DeepSeek()
{
    static int muki;
    static int patternTimer;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200 から変更しない
        muki = 1;
        patternTimer = 30;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (patternTimer > 0) patternTimer--;

    if (patternTimer <= 0) {
        double hpRatio = (double)enemy.hp / enemy.maxHp;
        int duration = 0;

        // HP 2/3 以上：基本パターン + 上下大玉列
        if (hpRatio >= 2.0 / 3.0) {
            duration = CreateBasicPattern(0);

            sEnemyShotSet* pCol = CreateShotSet(ShotVerticalLargeColumn, enemy.x, enemy.y, 0.0, 0);
            pCol->param_i[0] = duration;
            pCol->param_i[1] = GetRand(1); // 0:下から, 1:上から

            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        // HP 2/3～1/3：x/y 転置基本パターン + 左右大玉列
        else if (hpRatio >= 1.0 / 3.0) {
            duration = CreateBasicPattern(1);

            sEnemyShotSet* pCol = CreateShotSet(ShotHorizontalLargeColumn, enemy.x, enemy.y, 0.0, 0);
            pCol->param_i[0] = duration;
            pCol->param_i[1] = GetRand(1); // 0:右から, 1:左から

            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        // HP 1/3 以下：基本パターンと x/y 転置基本パターンを同時発生
        else {
            int d1 = CreateBasicPattern(0);
            int d2 = CreateBasicPattern(1);
            duration = (d1 > d2) ? d1 : d2;
        }

        patternTimer = duration + 30;
    }
}