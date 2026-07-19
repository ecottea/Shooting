// enemyPat_Mobius.cpp
// ================================================================
// メビウスの帯をモチーフにした弾幕パターン
//
// ─ パターン解説 ─────────────────────────────────────────────────
//
// 【ShotMobiusRibbon：帯リボン】
//   半径 R の円形軌道上を進む断面（幅 2W, MOB_RIB_N 弾）を
//   MOB_RIB_SPAWN フレームごとに射出し、帯状の弾幕を形成する。
//
//   ◆ 幅方向ベクトル = (cos(u/2), sin(u/2))
//     u: 軌道角 (0 → 2π で 1 周)
//     幅方向は軌道 1 周あたり 180° 回転
//     → 1 周後にシアン（表）とマゼンタ（裏）の位置が入れ替わる
//     = メビウスの帯の「表裏が 1 つの面」の視覚化
//
//   ◆ 色の対応
//     シアン   (t > 0) : 帯の「表」側
//     白        (t = 0) : 帯の中心線
//     マゼンタ (t < 0) : 帯の「裏」側
//
// 【ShotMobiusBoundary：境界線】
//   帯の縁（境界曲線）を追跡する弾を 1 弾ずつ射出する。
//   メビウスの帯の境界は 1 本の閉曲線で、2 周（4π）して初めて閉じる。
//
//   ◆ elapsed_u ∈ [0, 2π) : t = +1 の縁 → シアン
//     elapsed_u ∈ [2π, 4π) : t = −1 の縁 → マゼンタ
//     → 見た目の「外縁」と「内縁」が実は同一の 1 本の曲線
//
// 【使用素材】
//   img_enemyShotSmallBall[3]  シアン    帯の表側
//   img_enemyShotSmallBall[5]  マゼンタ  帯の裏側
//   img_enemyShotSmallBall[6]  白        帯の中心線
//   img_enemyShotMediumBall[3] シアン    境界線の表縁
//   img_enemyShotMediumBall[5] マゼンタ  境界線の裏縁
//   sound_enemyShot_medium     帯リボン射出音
//   sound_enemyShot_heavy      境界線射出音
//
// 【発射スケジュール ─ 600 フレーム周期】
//   cycle =   1 : 反時計リボン × 2（起点 0° / 180°）
//   cycle = 150 : 境界線弾（2 周追跡、起点 0°）
//   cycle = 301 : 時計回りリボン × 2（起点 90° / 270°）
// ================================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ── 共通定数 ──────────────────────────────────────────────────────────
static const double MOB_CX = 240.0;  // 軌道中心 X（画面水平中央）
static const double MOB_CY = 265.0;  // 軌道中心 Y
static const double MOB_R = 110.0;  // 軌道半径
static const double MOB_W = 50.0;   // 帯の半幅（全幅 100 px）
static const double MOB_ORBIT_FRAMES = 300.0;  // 1 周あたりのフレーム数


// ════════════════════════════════════════════════════════════════
// ShotMobiusRibbon
// ════════════════════════════════════════════════════════════════
// MOB_RIB_SPAWN フレームごとに断面（MOB_RIB_N 弾）を射出する。
// count=0 と count=300 が同じ軌道角になるため、1 周で帯が閉じる。
//
// pEnemyShotSet->muki : 初期軌道角（ラジアン）
// pEnemyShotSet->kind : ビット0 = 回転方向（0:反時計、1:時計）
// ════════════════════════════════════════════════════════════════
static const int MOB_RIB_N = 7;    // 断面の弾数（奇数）
static const int MOB_RIB_SPAWN = 6;    // 断面射出間隔（フレーム）
static const int MOB_RIB_DURATION = 301;  // 射出期間（count=300 で閉じる）

static void ShotMobiusRibbon(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ── 断面を射出 ─────────────────────────────────────────────────
    if (pEnemyShotSet->count < MOB_RIB_DURATION &&
        pEnemyShotSet->count % MOB_RIB_SPAWN == 0)
    {
        int    dir = (pEnemyShotSet->kind & 1) ? 1 : -1;
        double u = pEnemyShotSet->muki
            + dir * (pEnemyShotSet->count / MOB_ORBIT_FRAMES) * DX_PI * 2.0;

        // 断面中心の座標
        double cx = MOB_CX + MOB_R * cos(u);
        double cy = MOB_CY + MOB_R * sin(u);

        // メビウスの幅方向ベクトル: 軌道 1 周で 180° 回転する
        //   u=0   → (1, 0)      u=π/2 → (cos π/4, sin π/4)
        //   u=π  → (0, 1)      u=2π  → (-1, 0)  ← 1周後に反転！
        double wx = cos(u * 0.5);
        double wy = sin(u * 0.5);

        for (int i = 0; i < MOB_RIB_N; i++) {
            // 帯幅方向の位置パラメータ t ∈ [-1, +1]
            double t = -1.0 + 2.0 * i / (double)(MOB_RIB_N - 1);
            double abst = (t < 0.0) ? -t : t;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + t * MOB_W * wx;
            pEnemyShot->y = cy + t * MOB_W * wy;

            // 軌道中心から外向きに飛ばす
            pEnemyShot->muki = atan2(pEnemyShot->y - MOB_CY,
                pEnemyShot->x - MOB_CX);
            // 帯の端（|t| 大）ほどわずかに速い
            pEnemyShot->speed = 0.50 + 0.35 * abst;

            // 色: 裏（マゼンタ） / 中心線（白） / 表（シアン）
            if (i < MOB_RIB_N / 2)
                pEnemyShot->kind = img_enemyShotSmallBall[5]; // マゼンタ（裏）
            else if (i == MOB_RIB_N / 2)
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白（中心線）
            else
                pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン（表）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ── 全弾の移動 ──────────────────────────────────────────────────
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}


// ════════════════════════════════════════════════════════════════
// ShotMobiusBoundary
// ════════════════════════════════════════════════════════════════
// 帯の境界曲線（1 本）を辿る弾を MOB_BND_SPAWN フレームごとに射出。
// 境界は 2 周（4π）で一巡する → MOB_BND_DURATION フレームかけて完結。
//
//   elapsed_u ∈ [0, 2π) : t = +1 の縁（シアン）
//   elapsed_u ∈ [2π, 4π): t = −1 の縁（マゼンタ） ← 同じ 1 本の続き
//
// 色が変わる count=300 のタイミングで、生成位置が軌道上で約 100 px
// 跳ぶように見える。これが 2D 投影上のメビウス「ねじれ」の接続点。
//
// pEnemyShotSet->muki : 初期軌道角
// ════════════════════════════════════════════════════════════════
static const int MOB_BND_SPAWN = 3;    // 射出間隔（フレーム）
static const int MOB_BND_DURATION = 600;  // 射出期間（2 周分）

static void ShotMobiusBoundary(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count < MOB_BND_DURATION &&
        pEnemyShotSet->count % MOB_BND_SPAWN == 0)
    {
        // 2 周（4π）を MOB_BND_DURATION フレームで均等に進む
        double elapsed_u = (pEnemyShotSet->count / (double)MOB_BND_DURATION) * DX_PI * 4.0;
        double u = pEnemyShotSet->muki + elapsed_u;

        // 前半 1 周: t=+1（シアン）/ 後半 1 周: t=−1（マゼンタ）
        double t;
        int    col;
        if (elapsed_u < DX_PI * 2.0) {
            t = +1.0;
            col = 3;  // シアン（表縁）
        }
        else {
            t = -1.0;
            col = 5;  // マゼンタ（裏縁 = 同一曲線の後半）
        }

        double cx = MOB_CX + MOB_R * cos(u);
        double cy = MOB_CY + MOB_R * sin(u);
        double wx = cos(u * 0.5);
        double wy = sin(u * 0.5);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = cx + t * MOB_W * wx;
        pEnemyShot->y = cy + t * MOB_W * wy;
        pEnemyShot->muki = atan2(pEnemyShot->y - MOB_CY,
            pEnemyShot->x - MOB_CX);
        pEnemyShot->speed = 1.7;
        pEnemyShot->kind = img_enemyShotMediumBall[col];

        pEnemyShot->muki *= -1;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // ── 全弾の移動 ──────────────────────────────────────────────────
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}


// ── ショットセット生成ヘルパー ─────────────────────────────────────────
static void CreateShotSet(
    void(*func)(sEnemyShotSet*),
    double muki,
    int    kind)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = MOB_CX;
    p->y = MOB_CY;
    p->muki = muki;
    p->kind = kind;

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
}


// ════════════════════════════════════════════════════════════════
// EnemyPat_Mobius_Claude
// ════════════════════════════════════════════════════════════════
void EnemyPat_Mobius_Claude()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 600 フレーム周期（発射サイクルと同期）で左右に揺れる
        enemy.x = 240.0 + 70.0 * sin(count * DX_PI / 180.0 * 0.6);
    }

    const int cycle = count % 600;

    // ▶ 反時計リボン（起点 0° / 180° 対称）
    //   → 帯が 1 周すると幅方向が 180° 反転し、シアンとマゼンタが入れ替わる
    if (cycle == 1) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        CreateShotSet(ShotMobiusRibbon, 0.0, 0);  // 起点  0°
        CreateShotSet(ShotMobiusRibbon, DX_PI, 0);  // 起点 180°（点対称）
    }

    // ▶ 境界線弾（帯の唯一の縁を 2 周 = 600 フレームかけて追跡）
    //   前半 300f: シアン（t=+1 の縁）
    //   後半 300f: マゼンタ（t=−1 の縁 = 同じ 1 本の続き）
    if (cycle == 150) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        CreateShotSet(ShotMobiusBoundary, 0.0, 0);
    }

    // ▶ 時計回りリボン（起点 90° / 270° 対称）
    //   → 逆方向の帯が同じ軌道を再度カバーする
    if (cycle == 301) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        CreateShotSet(ShotMobiusRibbon, DX_PI * 0.5, 1);  // 起点  90°
        CreateShotSet(ShotMobiusRibbon, DX_PI * 1.5, 1);  // 起点 270°（点対称）
    }
}