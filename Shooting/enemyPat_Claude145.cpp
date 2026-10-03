#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕「臆病者の置き土産」
//  ボスが被弾するたびに自機と反対方向へダッシュし、通った軌跡に停止弾を置いていく。
//  連続ヒットが増えるほどダッシュが速く、置き土産が密になる。
//  画面端まで逃げると、震えたあとに全方位弾をバーストして逆方向へ再ダッシュする。
// ============================================================

// ---------------- 調整用定数 ----------------
// 連続ヒットの段階: 0=1~2発 / 1=3~5発 / 2=6発以上
static const int    COMBO_RESET_FRAMES = 30;                 // 最後のヒットからこのフレーム当たらないとリセット(0.5秒)
static const int    DASH_FRAMES = 20;                 // 1回のダッシュの長さ
static const double DASH_SPEED[3] = { 4.0, 6.0, 9.0 };  // 段階ごとのダッシュ速度
static const int    TRAIL_INTERVAL[3] = { 8, 4, 1 };        // 段階ごとの置き土産の間隔(フレーム)
static const int    TIER_COLOR[3] = { 2, 8, 0 };        // 段階ごとの弾の色(2:緑、8:橙、0:赤)
static const int    BURST_WAY[3] = { 16, 20, 24 };     // 段階ごとのバースト弾の方向数
static const int    TRAIL_STOP_FRAMES = 60;                 // 置き土産が静止している時間(約1秒)
static const double TRAIL_ACCEL = 0.04;               // 静止後の加速度
static const double TRAIL_MAX_SPEED = 3.2;                // 置き土産の最高速度
static const int    TREMBLE_FRAMES = 30;                 // 追い詰められて震える時間(0.5秒)
static const double EDGE_L = 24.0;               // 画面端(左)
static const double EDGE_R = 456.0;              // 画面端(右)
static const double DRIFT_SPEED = 0.98;               // 通常時の漂い速度
static const double DRIFT_L = 100.0;              // 漂いの折り返し(左)
static const double DRIFT_R = 380.0;              // 漂いの折り返し(右)
static const int    NORMAL_SHOT_INTERVAL = 45;                 // 通常時の3way弾の間隔

// ---------------- 状態管理 ----------------
enum {
    ST_IDLE,     // 通常時（漂いながら3way）
    ST_DASH,     // 被弾して逃走中
    ST_TREMBLE,  // 画面端で震えている（予兆・チャンスタイム）
};

static int    state;          // 現在の状態
static int    stateCount;     // 状態内のカウンタ
static double vx;             // 横方向の速度
static int    driftDir;       // 漂いの向き(1 or -1)
static int    prevHp;         // 前フレームのHP（被弾検出用）
static int    combo;          // 連続ヒット数
static int    comboTimer;     // コンボリセットまでの残りフレーム
static int    dashDir;        // ダッシュの向き(1 or -1)
static int    dashTier;       // ダッシュ開始時の段階
static int    dashLeft;       // ダッシュ残りフレーム
static int    trailTimer;     // 次の置き土産までのフレーム
static int    shotTimer;      // 次の3way弾までのフレーム
static double trembleBaseX;   // 震える前のX座標
static int    trembleTier;    // 震え始めた時点の段階

// ---------------- 共通処理 ----------------
// 連続ヒット数から段階(0~2)を求める
static int ComboTier()
{
    if (combo >= 6) return 2;
    if (combo >= 3) return 1;
    return 0;
}

// 効果音を頭から鳴らし直す
static void PlaySE(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 弾セットを1つ作ってリストに繋ぐ
static sEnemyShotSet* AddShotSet(double x, double y, double muki, int kind, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// 弾を1発作って弾セットのリストに繋ぐ
static void AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 弾セット内の全弾を等速直進させる
static void MoveShotsStraight(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------- 弾幕：通常時の自機狙い3way ----------------
// pEnemyShotSet->muki: 自機への向き
static void ShotAimed3Way(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySE(sound_enemyShot_light);
        for (int i = -1; i <= 1; i++) {
            // 中楕円弾(10.5x7.0)、色は青(4)
            AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                pEnemyShotSet->muki + i * (DX_PI / 12.0), 2.8, img_enemyShotMediumOval[4]);
        }
    }
    MoveShotsStraight(pEnemyShotSet);
}

// ---------------- 弾幕：置き土産（1発だけの弾セット） ----------------
// pEnemyShotSet->kind: 弾の色
// pShot->param_i[0]: 動き出したかどうか
static void ShotTrail(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 中玉(7.0x7.0)
        AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, 0.0, 0.0,
            img_enemyShotMediumBall[pEnemyShotSet->kind]);
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->count >= TRAIL_STOP_FRAMES) {
            if (pShot->param_i[0] == 0) {
                // 動き出す瞬間に自機の方向を向く（以降は直進）
                pShot->param_i[0] = 1;
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            }
            pShot->speed += TRAIL_ACCEL;
            if (pShot->speed > TRAIL_MAX_SPEED) pShot->speed = TRAIL_MAX_SPEED;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ---------------- 弾幕：追い詰められた時の全方位バースト ----------------
// pEnemyShotSet->muki: 自機への向き
// pEnemyShotSet->kind: 弾の色
// pEnemyShotSet->param_i[0]: 方向数
static void ShotBurst(sEnemyShotSet* pEnemyShotSet)
{
    int way = pEnemyShotSet->param_i[0];
    if (pEnemyShotSet->count == 0 || pEnemyShotSet->count == 12) {
        bool second = (pEnemyShotSet->count != 0);
        if (!second) PlaySE(sound_enemyShot_heavy);
        // 1周目は自機が弾と弾の隙間に来る向き、2周目は自機狙いで少し遅い
        double offset = second ? 0.0 : DX_PI / way;
        double speed = second ? 1.8 : 2.6;
        for (int i = 0; i < way; i++) {
            // 鱗弾(4.0x3.0)
            AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                pEnemyShotSet->muki + offset + 2.0 * DX_PI * i / way, speed,
                img_enemyShotScale[pEnemyShotSet->kind]);
        }
    }
    MoveShotsStraight(pEnemyShotSet);
}

// ---------------- 敵本体の動き ----------------
// 被弾したので逃げ始める（震え中は呼ばない）
static void StartDash()
{
    if (state == ST_IDLE) {
        // 通常時からの逃走開始：置き土産を即座に置き、3way弾のタイマーをリセット
        PlaySE(sound_enemyShot_medium);
        trailTimer = 0;
        shotTimer = 0;
    }
    // 自機と反対方向へ逃げる（同じX座標ならランダム）
    if (enemy.x > player.x)      dashDir = 1;
    else if (enemy.x < player.x) dashDir = -1;
    else                         dashDir = GetRand(1) ? 1 : -1;
    dashTier = ComboTier();
    dashLeft = DASH_FRAMES;
    state = ST_DASH;
}

void EnemyPat_Escape_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        state = ST_IDLE;
        stateCount = 0;
        vx = 0.0;
        driftDir = 1;
        prevHp = enemy.hp;
        combo = 0;
        comboTimer = 0;
        dashDir = 1;
        dashTier = 0;
        dashLeft = 0;
        trailTimer = 0;
        shotTimer = 0;
        trembleBaseX = enemy.x;
        trembleTier = 0;
    }

    // ---- 被弾検出と連続ヒットの管理 ----
    int hit = prevHp - enemy.hp;
    prevHp = enemy.hp;
    if (hit > 0) {
        combo += hit;
        comboTimer = COMBO_RESET_FRAMES;
        if (state != ST_TREMBLE) StartDash(); // 震え中は逃げずにチャンスタイム
    }
    else if (comboTimer > 0) {
        if (--comboTimer == 0) combo = 0;
    }

    // ---- 状態ごとの処理 ----
    switch (state) {
    case ST_IDLE:
        // 漂い移動（ダッシュ後の速度は滑らかに漂い速度へ戻る）
        if (enemy.x < DRIFT_L) driftDir = 1;
        if (enemy.x > DRIFT_R) driftDir = -1;
        vx += (DRIFT_SPEED * driftDir - vx) * 0.12;
        enemy.x += vx;
        if (enemy.x < EDGE_L) { enemy.x = EDGE_L; vx = 0.0; }
        if (enemy.x > EDGE_R) { enemy.x = EDGE_R; vx = 0.0; }
        // 自機狙い3way
        if (++shotTimer >= NORMAL_SHOT_INTERVAL) {
            shotTimer = 0;
            double muki = atan2(player.y - (enemy.y + 10.0), player.x - enemy.x);
            AddShotSet(enemy.x, enemy.y + 10.0, muki, 0, ShotAimed3Way);
        }
        break;

    case ST_DASH:
        // 逃走中は弾を撃たず、通った場所に停止弾を置いていく
        vx = DASH_SPEED[dashTier] * dashDir;
        enemy.x += vx;
        if (trailTimer > TRAIL_INTERVAL[dashTier]) trailTimer = TRAIL_INTERVAL[dashTier];
        if (trailTimer <= 0) {
            AddShotSet(enemy.x, enemy.y + 10.0, 0.0, TIER_COLOR[dashTier], ShotTrail);
            trailTimer = TRAIL_INTERVAL[dashTier];
        }
        trailTimer--;
        // 画面端に追い詰められた
        if ((dashDir < 0 && enemy.x <= EDGE_L) || (dashDir > 0 && enemy.x >= EDGE_R)) {
            enemy.x = (dashDir < 0) ? EDGE_L : EDGE_R;
            vx = 0.0;
            trembleTier = dashTier;
            stateCount = 0;
            state = ST_TREMBLE;
            break;
        }
        if (--dashLeft <= 0) {
            shotTimer = 0;
            state = ST_IDLE;
        }
        break;

    case ST_TREMBLE:
        if (stateCount == 0) {
            trembleBaseX = enemy.x;
            vx = 0.0;
            PlaySE(sound_enemyShot_light); // 予告音
        }
        enemy.x = trembleBaseX + ((stateCount & 1) ? 1.5 : -1.5);
        stateCount++;
        if (stateCount >= TREMBLE_FRAMES) {
            enemy.x = trembleBaseX;
            // 全方位バースト
            double muki = atan2(player.y - (enemy.y + 10.0), player.x - enemy.x);
            sEnemyShotSet* pBurst = AddShotSet(enemy.x, enemy.y + 10.0, muki, TIER_COLOR[trembleTier], ShotBurst);
            pBurst->param_i[0] = BURST_WAY[trembleTier];
            // 画面中央へ向かって再びダッシュ
            dashDir = (enemy.x < 240.0) ? 1 : -1;
            dashTier = trembleTier;
            dashLeft = DASH_FRAMES;
            trailTimer = 0;
            state = ST_DASH;
        }
        break;
    }
}