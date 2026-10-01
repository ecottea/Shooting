// ============================================================
//  enemyPat_tmp.cpp
//  弾幕「ペアノ埋草」
//
//  ペアノ曲線(空間充填曲線)をモチーフにした弾幕。
//  画面を9x9のセルに分割し、ペアノ曲線の描画順にセル中心を狙った
//  自機狙い弾を連射する。弾の流れが曲線を一筆書きでなぞり、
//  画面全体を「充填」していく。
//
//   [フェーズ1] 予告      : charge音で走査開始を予告(約1秒の猶予)
//   [フェーズ2] ペアノ走査 : 81セルを曲線順に狙撃(色は3x3サブブロック単位で変化)
//   [フェーズ3] 充填完了   : 走査完了直後に全周リング弾(領域の「確定」=締め攻撃)
//
//  ※ count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//    画面外の弾の消去はメインルーチンで行うため、ここでは行わない。
//  ※ GetRand不使用: 曲線の描画順を読んで逃げる、という予読性が主眼のため
//    完全に決定論的な弾幕としている。
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 定数とペアノ曲線テーブル
// ------------------------------------------------------------
static const double SCR_SIZE = 480.0;  // ゲーム画面の一辺
static const int   GRID_DIV = 9;      // 画面の一辺の分割数(3^2)
static const int   PEANO_N = 2;      // ペアノ曲線の再帰深度
static const int   PEANO_PTS = 81;     // 走査セル数(3^(2*2))

static int s_peanoX[PEANO_PTS];        // 走査順に並んだセル座標(0〜8)
static int s_peanoY[PEANO_PTS];
static int s_peanoReady = 0;

// ペアノ曲線上のセル座標を再帰的に列挙する
// (cx,cy)         : 開始コーナーのセル座標
// (ux,uy),(vx,vy) : ブロック内部へ向かう2つの直交単位ベクトル(最初の一歩は+v方向)
// n==1 で3x3の基本形(9セル)を書き出し、それ以外は9個のサブブロックへ再帰
static void PeanoCells(int n, int cx, int cy,
    int ux, int uy, int vx, int vy,
    int outX[], int outY[], int* pIdx)
{
    if (n <= 1) {
        // 基本形: ペアノ曲線の1次近似(左列↓→中列↑→右列↓ の3本柱)
        static const int base[9][2] = {
            { 0,0 },{ 0,1 },{ 0,2 },{ 1,2 },{ 1,1 },
            { 1,0 },{ 2,0 },{ 2,1 },{ 2,2 }
        };  // {u係数, v係数}
        for (int i = 0; i < 9; i++) {
            outX[*pIdx] = cx + base[i][0] * ux + base[i][1] * vx;
            outY[*pIdx] = cy + base[i][0] * uy + base[i][1] * vy;
            (*pIdx)++;
        }
        return;
    }

    int s = 1;
    for (int i = 1; i < n; i++) s *= 3;  // サブブロックの一辺(セル数)

    // サブブロックの走査順。各要素は
    // [ブロック座標(a,b)] [開始セルのブロック内オフセット] [内部方向P,Q]
    // 開始セル = (a*s+eu, b*s+ev)、P = pu*u + pv*v、Q = qu*u + qv*v
    // (各サブ曲線は開始コーナーから対角コーナーへ抜け、隣接ブロックへ連続接続)
    static const int tbl[9][8] = {
        //  a b   eu ev   pu pv qu qv
        {  0,0,  0,0,     1, 0, 0, 1 },
        {  0,1,  2,0,    -1, 0, 0, 1 },
        {  0,2,  0,0,     1, 0, 0, 1 },
        {  1,2,  0,2,     1, 0, 0,-1 },
        {  1,1,  2,2,    -1, 0, 0,-1 },
        {  1,0,  0,2,     1, 0, 0,-1 },
        {  2,0,  0,0,     1, 0, 0, 1 },
        {  2,1,  2,0,    -1, 0, 0, 1 },
        {  2,2,  0,0,     1, 0, 0, 1 },
    };
    for (int i = 0; i < 9; i++) {
        int su = tbl[i][0] * s + tbl[i][2];
        int sv = tbl[i][1] * s + tbl[i][3];
        int nx = cx + su * ux + sv * vx;
        int ny = cy + su * uy + sv * vy;
        int nux = tbl[i][4] * ux + tbl[i][5] * vx;
        int nuy = tbl[i][4] * uy + tbl[i][5] * vy;
        int nvx = tbl[i][6] * ux + tbl[i][7] * vx;
        int nvy = tbl[i][6] * uy + tbl[i][7] * vy;
        PeanoCells(n - 1, nx, ny, nux, nuy, nvx, nvy, outX, outY, pIdx);
    }
}

// ------------------------------------------------------------
// 弾幕本体: ペアノ埋草
// ------------------------------------------------------------
static void ShotPeano(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int c = pEnemyShotSet->count;

    // 既存弾の移動(等速直線運動)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1 && pShot->count >= 60 && pShot->count < 120) {
            pShot = pShot->next;
            continue;
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }

    const int warnTime = 60;  // フェーズ1: 予告時間(フレーム)
    const int firePeriod = 3;   // 走査弾の発射間隔
    const int lastFire = warnTime + (PEANO_PTS - 1) * firePeriod;
    const int burstTime = lastFire + 45;  // フェーズ3のタイミング

    if (c == 0) {
        // --- フェーズ1: 予告音 ---
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        pEnemyShotSet->param_i[0] = 0;  // 発射済みセル数
        return;
    }

    // --- フェーズ2: ペアノ走査 ---
    int fired = pEnemyShotSet->param_i[0];
    if (c >= warnTime && fired < PEANO_PTS && (c - warnTime) % firePeriod == 0) {
        if (fired == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        // 奇数ウェーブは曲線を終点から逆なぞり(パターンの単調化を防ぐ)
        int pt = (pEnemyShotSet->kind % 2 == 0) ? fired : (PEANO_PTS - 1 - fired);

        // セル中心の画面座標を狙撃
        double tx = (s_peanoX[pt] + 0.5) * SCR_SIZE / GRID_DIV;
        double ty = (s_peanoY[pt] + 0.5) * SCR_SIZE / GRID_DIV;

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = atan2(ty - pEnemyShotSet->y, tx - pEnemyShotSet->x);
        pEnemyShot->speed = hypot(tx - pEnemyShotSet->x, ty - pEnemyShotSet->y) / 60.0;
        // 3x3サブブロック単位で色を変え、再帰構造を見せる
        pEnemyShot->kind = img_enemyShotLargeBall[(pt / 9) % 2 == 0 ? 3 : 4];  // シアン/青
        pEnemyShot->param_i[0] = 1;
        // 弾リストへの追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        pEnemyShotSet->param_i[0] = fired + 1;
    }

    // --- フェーズ3: 充填完了 → 全周リングで締める ---
    if (c == burstTime) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 24; i++) {
            for (int j = 0; j < 2; j++) {  // 速度差のある二重リング
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = i * (DX_PI * 2.0 / 24.0) + j * (DX_PI * 2.0 / 48.0);
                pEnemyShot->speed = (j == 0) ? 3.2 : 1.9;
                pEnemyShot->kind = img_enemyShotMediumBall[5];  // マゼンタ中玉
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
}

// ------------------------------------------------------------
// 弾幕: 走査の合間の微弱3-way自機狙い
// ------------------------------------------------------------
static void ShotAimedSpread(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 1) * 0.14;
            pEnemyShot->speed = 2.6;
            pEnemyShot->kind = img_enemyShotSmallBall[1];  // 黄
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

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_PeanoCurve_Zai()
{
    static int waveCount = 0;
    const int wavePeriod = 420;  // 走査ウェーブの周期(フレーム)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        waveCount = 0;
        // ペアノ曲線の走査順テーブルを一度だけ生成
        if (!s_peanoReady) {
            int idx = 0;
            PeanoCells(PEANO_N, 0, 0, 1, 0, 0, 1, s_peanoX, s_peanoY, &idx);
            s_peanoReady = 1;
        }
    }

    // グリッドは画面固定のため、敵本体は上方でゆっくり揺れるだけ
    enemy.x = 240.0 + sin(count * 0.025) * 16.0;
    enemy.y = 60.0;

    // 一定周期でペアノ走査ウェーブを起こす
    if (count % wavePeriod == 40) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPeano;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = waveCount++;  // ウェーブ番号(偶奇で逆走査)
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 走査と走査の合間だけ微弱な3-wayを挟む(プレッシャー維持用)
    int m = count % wavePeriod;
    if ((m > 380 || m < 40) && count % 15 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAimedSpread;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}