// 粉砕符「ペーパーレス・シュレッダー」
//
// シュレッダーをモチーフにした弾幕。
//   1. 給紙   : 白い小玉の格子(紙)を1行ずつ画面上部から送り出す。数行は黒い小玉(印字された文字行)
//   2. 裁断   : 刃ラインを越えた列が縦長の銃弾(細断された紙)になり、加速しながら扇状に広がる
//   3. 紙くず : 第2の刃(クロスカット)で細断された紙が菱形弾2枚に分裂し、ひらひら回転しながら落ちる
//   4. 紙詰まり: 3枚に1枚、刃ラインで紙が詰まって震え、赤くなってから放射状に破裂する
//
// 刃(ローラー)は黒い中玉2列が逆位相で左右に揺れ続けて表現する。
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
// 弾の色インデックス(img_enemyShotXxx[色])
enum {
    SHR_COLOR_RED = 0,
    SHR_COLOR_YELLOW,
    SHR_COLOR_GREEN,
    SHR_COLOR_CYAN,
    SHR_COLOR_BLUE,
    SHR_COLOR_MAGENTA,
    SHR_COLOR_WHITE,
    SHR_COLOR_BLACK,
    SHR_COLOR_ORANGE,
};

// 弾ごとの状態(param_i[0])
enum {
    SHR_ST_SHEET = 0,   // 紙(裁断前)
    SHR_ST_STRIP,       // 細断された紙
    SHR_ST_CONFETTI,    // 紙くず
    SHR_ST_JAM,         // 紙詰まりで停止中
    SHR_ST_BURST,       // 紙詰まりの破裂(直進)
};

static const int    SHR_COLS = 28;              // 紙の列数
static const int    SHR_ROWS = 10;              // 紙の行数
static const double SHR_COL_W = 16.0+10;           // 列の間隔
static const int    SHR_ROW_INTERVAL = 6;       // 何フレームごとに1行送り出すか
static const double SHR_CENTER_X = 240.0;       // 紙の中心x
static const double SHR_FEED_Y = 8.0;           // 紙の送り出し位置y
static const double SHR_FEED_SPEED = 2.0;       // 紙が降りてくる速さ
static const double SHR_BLADE_Y = 110.0;        // 第1の刃(細断)のy
static const double SHR_CROSS_Y = 290.0;        // 第2の刃(クロスカット)のy
static const double SHR_STRIP_SPREAD = 0.010;   // 細断された紙が狙い位置から外へ広がる度合い
static const double SHR_STRIP_ZIGZAG = 0.15;    // 奇数列/偶数列の横ずれ(隙間の開閉)

static const int    SHR_SHEET_INTERVAL = 100+20;   // 紙を送り出す間隔(フレーム)
static const int    SHR_SHEET_OFFSET = 20;      // count % INTERVAL がこの値の時に送り出す
static const int    SHR_JAM_CYCLE = 3;          // N枚に1枚は紙詰まり
static const int    SHR_JAM_RED_TIME = 120;     // 紙詰まり: 赤くなる時刻(紙のcount)
static const int    SHR_JAM_BURST_TIME = 150;   // 紙詰まり: 破裂する時刻(紙のcount)
static const double SHR_JAM_CENTER_Y = -120.0;  // 破裂の放射中心y(画面の上方に置いて下向きに広げる)

// ============================================================
//  ヘルパー
// ============================================================
// 効果音を頭出しして鳴らす
static void PlaySe(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ShotSet を作って全体リストの末尾に繋ぐ
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
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

// ShotSet の弾リスト末尾に弾を1つ追加する
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ============================================================
//  刃(ローラー)
//   param_i[0]: 0=上段 / 1=下段    param_d[0]: 基準x
// ============================================================
static void ShotPaperBlade(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 上段: 中玉29個 / 下段: 半個ずらして中玉28個(歯車が噛み合っているように見せる)
        for (int i = 0; i < 29; i++) {
            double baseX = 16.0 + 16.0 * i;
            sEnemyShot* pShot = AddShot(pEnemyShotSet, baseX, SHR_BLADE_Y - 6.0, 0.0, 0.0,
                img_enemyShotMediumBall[SHR_COLOR_BLACK]);
            pShot->param_i[0] = 0;
            pShot->param_d[0] = baseX;
        }
        for (int i = 0; i < 28; i++) {
            double baseX = 24.0 + 16.0 * i;
            sEnemyShot* pShot = AddShot(pEnemyShotSet, baseX, SHR_BLADE_Y + 6.0, 0.0, 0.0,
                img_enemyShotMediumBall[SHR_COLOR_BLACK]);
            pShot->param_i[0] = 1;
            pShot->param_d[0] = baseX;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 上段と下段を逆位相で左右に揺らす
        double sway = sin(pShot->count * 0.10) * 7.0;
        if (pShot->param_i[0] == 1) sway = -sway;
        double newX = pShot->param_d[0] + sway;
        double dx = newX - pShot->x;
        pShot->muki = (dx >= 0.0) ? 0.0 : DX_PI; // 実際に動いている左右方向を向かせる
        pShot->speed = fabs(dx);
        pShot->x = newX;
        pShot = pShot->next;
    }
}

// ============================================================
//  クロスカット: 細断された紙 → 菱形弾2枚
// ============================================================
static void CrossCut(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pShot)
{
    const double baseVx = pShot->param_d[0];
    const double baseVy = pShot->param_d[1];
    const int col = pShot->param_i[1];
    const int row = pShot->param_i[2];
    const int printed = pShot->param_i[3];
    const double px = pShot->x;
    const double py = pShot->y;

    for (int k = 0; k < 2; k++) {
        sEnemyShot* pConfetti = pShot;
        if (k == 1) pConfetti = AddShot(pEnemyShotSet, px, py, 0.0, 0.0, 0);

        // 印字された紙片は黒、それ以外は白を中心に黄・シアンを少し混ぜる
        int colorIdx;
        if (printed) {
            colorIdx = SHR_COLOR_BLACK;
        }
        else {
            int r = GetRand(99);
            colorIdx = (r < 70) ? SHR_COLOR_WHITE : (r < 85) ? SHR_COLOR_YELLOW : SHR_COLOR_CYAN;
        }
        double vx = baseVx * 0.5 + (GetRand(200) - 100) / 100.0 * 0.9;
        double vy = baseVy * 0.5 + GetRand(100) / 100.0 * 0.6;

        pConfetti->kind = img_enemyShotDiamond[colorIdx];
        pConfetti->x = px;
        pConfetti->y = py;
        pConfetti->param_i[0] = SHR_ST_CONFETTI;
        pConfetti->param_i[1] = col;
        pConfetti->param_i[2] = row;
        pConfetti->param_i[3] = printed;
        pConfetti->param_d[0] = vx;                                  // 横速度
        pConfetti->param_d[1] = vy;                                  // 縦速度
        pConfetti->param_d[2] = (GetRand(100) - 50) / 100.0 * 0.24;  // 回転速度
        pConfetti->param_d[3] = GetRand(628) / 100.0;                // ひらひら揺れの位相
        pConfetti->muki = GetRand(628) / 100.0;                      // 初期の向きはバラバラ
        pConfetti->speed = sqrt(vx * vx + vy * vy);
    }
}

// ============================================================
//  紙詰まりの破裂: 大玉リング + 小玉リング3層
// ============================================================
static void SpawnJamBurst(sEnemyShotSet* pEnemyShotSet)
{
    const double cx = SHR_CENTER_X;
    const double cy = SHR_BLADE_Y + 30.0;

    // 大玉のリング(橙と赤を交互に)
    const int bigN = 12;
    const double bigOffset = GetRand(628) / 100.0;
    for (int i = 0; i < bigN; i++) {
        double ang = bigOffset + 2.0 * DX_PI * i / bigN;
        int colorIdx = (i % 2 == 0) ? SHR_COLOR_ORANGE : SHR_COLOR_RED;
        sEnemyShot* pShot = AddShot(pEnemyShotSet, cx, cy, ang, 1.6, img_enemyShotLargeBall[colorIdx]);
        pShot->param_i[0] = SHR_ST_BURST;
        pShot->param_d[2] = 0.0;   // 加速なし
        pShot->param_d[3] = 1.6;   // 最高速
    }

    // 小玉のリング3層(速さと色を変え、層ごとに半歩ずらす)
    const double layerSpeed[3] = { 1.8, 2.4, 3.0 };
    const int layerColor[3] = { SHR_COLOR_RED, SHR_COLOR_ORANGE, SHR_COLOR_YELLOW };
    const int smallN = 36;
    const double smallOffset = GetRand(628) / 100.0;
    for (int layer = 0; layer < 3; layer++) {
        for (int i = 0; i < smallN; i++) {
            double ang = smallOffset + 2.0 * DX_PI * (i + layer * 0.5) / smallN;
            sEnemyShot* pShot = AddShot(pEnemyShotSet, cx, cy, ang, layerSpeed[layer],
                img_enemyShotSmallBall[layerColor[layer]]);
            pShot->param_i[0] = SHR_ST_BURST;
            pShot->param_d[2] = 0.0;
            pShot->param_d[3] = layerSpeed[layer];
        }
    }
}

// ============================================================
//  紙1枚分の弾幕
//   ShotSet: param_i[0]=紙の通し番号 / param_i[1]=紙詰まりか / param_i[2]=裁断音済 / param_i[3]=クロスカット音済
//            param_d[0]=細断された紙の隙間が開く狙い位置x
//   弾    : param_i[0]=状態 / param_i[1]=列 / param_i[2]=行 / param_i[3]=印字行か / param_i[4]=赤化済
//            param_d[0]=vx / param_d[1]=vy / param_d[2]=ay or 回転速度 or 加速度 / param_d[3]=位相 or 最高速
//            param_d[4]=紙詰まり時の基準x / param_d[5]=紙詰まり時の停止y / param_d[6]=震えの位相
// ============================================================
static void ShotPaperSheet(sEnemyShotSet* pEnemyShotSet)
{
    const int t = pEnemyShotSet->count;
    const bool isJam = (pEnemyShotSet->param_i[1] != 0);
    const double targetX = pEnemyShotSet->param_d[0];

    // --- 給紙: 一定間隔で1行(白い小玉28個)を送り出す ---
    if (t % SHR_ROW_INTERVAL == 0 && t / SHR_ROW_INTERVAL < SHR_ROWS) {
        const int row = t / SHR_ROW_INTERVAL;
        const bool printRow = (row >= 2 && row <= 8 && row % 2 == 0); // 文字行
        for (int col = 0; col < SHR_COLS; col++) {
            double x = SHR_CENTER_X + (col - (SHR_COLS - 1) / 2.0) * SHR_COL_W;
            // 文字行は左右3列の余白を残して黒にする
            int printed = (printRow && col >= 3 && col <= SHR_COLS - 4) ? 1 : 0;
            sEnemyShot* pShot = AddShot(pEnemyShotSet, x, SHR_FEED_Y, DX_PI / 2.0, SHR_FEED_SPEED,
                img_enemyShotSmallBall[printed ? SHR_COLOR_BLACK : SHR_COLOR_WHITE]);
            pShot->param_i[0] = SHR_ST_SHEET;
            pShot->param_i[1] = col;
            pShot->param_i[2] = row;
            pShot->param_i[3] = printed;
        }
    }

    // --- 紙詰まり: 赤くなる予告 / 破裂 ---
    if (isJam && t == SHR_JAM_RED_TIME) PlaySe(sound_enemyCharge);
    if (isJam && t == SHR_JAM_BURST_TIME) {
        PlaySe(sound_enemyShot_extreme);
        SpawnJamBurst(pEnemyShotSet);
    }

    sEnemyShot* pHead = pEnemyShotSet->pEnemyShotHead;
    sEnemyShot* pEnd = pHead->prev; // このフレームの処理中に追加された弾は次フレームから動かす
    sEnemyShot* pShot = pHead->next;
    while (pShot != pHead) {
        sEnemyShot* pNext = pShot->next;
        const bool isLast = (pShot == pEnd);

        switch (pShot->param_i[0]) {
        case SHR_ST_SHEET: {
            // 紙: まっすぐ下へ。muki は下向きのまま
            pShot->y += SHR_FEED_SPEED;
            if (pShot->y >= SHR_BLADE_Y) {
                const int col = pShot->param_i[1];
                const int row = pShot->param_i[2];
                const int odd = col & 1;
                pShot->kind = img_enemyShotBullet[pShot->param_i[3] ? SHR_COLOR_BLACK : SHR_COLOR_WHITE];
                if (!isJam) {
                    // 裁断: 縦長の銃弾になり、加速しながら扇状に広がる。奇数列/偶数列で加速をずらす
                    if (!pEnemyShotSet->param_i[2]) {
                        pEnemyShotSet->param_i[2] = 1;
                        PlaySe(sound_enemyShot_medium);
                    }
                    pShot->param_i[0] = SHR_ST_STRIP;
                    pShot->param_d[0] = (pShot->x - targetX) * SHR_STRIP_SPREAD
                        + (odd ? SHR_STRIP_ZIGZAG : -SHR_STRIP_ZIGZAG);
                    pShot->param_d[1] = odd ? 2.0 : 2.2;
                    pShot->param_d[2] = odd ? 0.05 : 0.06;
                    pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
                    pShot->speed = sqrt(pShot->param_d[0] * pShot->param_d[0] + pShot->param_d[1] * pShot->param_d[1]);
                }
                else {
                    // 紙詰まり: 刃のすぐ下で止まって積み重なる(後から来た行ほど刃に近い位置で止まる)
                    pShot->param_i[0] = SHR_ST_JAM;
                    pShot->param_d[4] = pShot->x;
                    pShot->param_d[5] = SHR_BLADE_Y + 30.0 - row * 3.0;
                    pShot->param_d[6] = GetRand(628) / 100.0;
                    pShot->muki = DX_PI / 2.0;
                    pShot->speed = 0.0;
                }
            }
            break;
        }
        case SHR_ST_STRIP: {
            // 細断された紙: 加速しながら落ちる。muki は進行方向
            double vx = pShot->param_d[0];
            double vy = pShot->param_d[1] + pShot->param_d[2];
            if (vy > 6.0) vy = 6.0;
            pShot->param_d[1] = vy;
            pShot->x += vx;
            pShot->y += vy;
            pShot->muki = atan2(vy, vx);
            pShot->speed = sqrt(vx * vx + vy * vy);
            if (pShot->y >= SHR_CROSS_Y) {
                if (!pEnemyShotSet->param_i[3]) {
                    pEnemyShotSet->param_i[3] = 1;
                    PlaySe(sound_enemyShot_light);
                }
                CrossCut(pEnemyShotSet, pShot);
            }
            break;
        }
        case SHR_ST_CONFETTI: {
            // 紙くず: ゆるく加速しながらひらひら揺れ、回転して落ちる
            double vx = pShot->param_d[0] * 0.995;
            double vy = pShot->param_d[1] + 0.015;
            if (vy > 2.6) vy = 2.6;
            pShot->param_d[0] = vx;
            pShot->param_d[1] = vy;
            double flutter = sin(pShot->count * 0.12 + pShot->param_d[3]) * 0.6;
            pShot->x += vx + flutter;
            pShot->y += vy;
            pShot->muki += pShot->param_d[2]; // 回転
            pShot->speed = sqrt(vx * vx + vy * vy);
            break;
        }
        case SHR_ST_JAM: {
            // 詰まった紙: 停止位置へ寄っていき、細かく震える。muki は下向きのまま
            pShot->y += (pShot->param_d[5] - pShot->y) * 0.12;
            pShot->x = pShot->param_d[4] + sin(pShot->count * 0.9 + pShot->param_d[6]) * 1.4;
            if (t >= SHR_JAM_RED_TIME && pShot->param_i[4] == 0) {
                pShot->param_i[4] = 1;
                pShot->kind = img_enemyShotBullet[SHR_COLOR_RED];
            }
            if (t >= SHR_JAM_BURST_TIME) {
                // 破裂: 画面上方の一点から放射状に(少しだけ角度をばらつかせる)
                double ang = atan2(pShot->y - SHR_JAM_CENTER_Y, pShot->x - SHR_CENTER_X)
                    + (GetRand(100) - 50) / 100.0 * (DX_PI / 36.0);
                pShot->param_i[0] = SHR_ST_BURST;
                pShot->kind = img_enemyShotBullet[(pShot->param_i[1] & 1) ? SHR_COLOR_ORANGE : SHR_COLOR_RED];
                pShot->muki = ang;
                pShot->speed = 2.0 + GetRand(150) / 100.0;
                pShot->param_d[2] = 0.03; // 加速度
                pShot->param_d[3] = 5.5;  // 最高速
            }
            break;
        }
        case SHR_ST_BURST: {
            // 破裂した弾: 進行方向に直進(muki は進行方向のまま)
            if (pShot->speed < pShot->param_d[3]) pShot->speed += pShot->param_d[2];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
        }

        if (isLast) break;
        pShot = pNext;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Shredder_Claude()
{
    static int sheetNo; // 何枚目の紙か

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        sheetNo = 0;

        // 刃(ローラー)は最初に1回だけ作り、パターン中ずっと揺らし続ける
        PlaySe(sound_enemyCharge);
        CreateShotSet(ShotPaperBlade, SHR_CENTER_X, SHR_BLADE_Y);
    }
    else {
        // 本体はゆっくり左右に揺れる
        enemy.x = 240.0 + sin(count * 0.02) * 60.0;
    }

    // 一定間隔で紙を1枚送り出す。隙間が開く位置は、送り出した時点の自機のx座標
    if (count % SHR_SHEET_INTERVAL == SHR_SHEET_OFFSET) {
        double targetX = player.x;
        if (targetX < 60.0) targetX = 60.0;
        if (targetX > 420.0) targetX = 420.0;

        sEnemyShotSet* pEnemyShotSet = CreateShotSet(ShotPaperSheet, SHR_CENTER_X, SHR_FEED_Y);
        pEnemyShotSet->param_i[0] = sheetNo;
        pEnemyShotSet->param_i[1] = (sheetNo % SHR_JAM_CYCLE == SHR_JAM_CYCLE - 1) ? 1 : 0; // 3枚目ごとに紙詰まり
        pEnemyShotSet->param_i[2] = 0;
        pEnemyShotSet->param_i[3] = 0;
        pEnemyShotSet->param_d[0] = targetX;
        sheetNo++;
    }
}