// enemyPat_claude_wafuu.cpp
// 和風弾幕パターン:「幽玄の陣」
//
// フェーズ  | HP帯         | 発動弾幕
// ─────────────────────────────────────────────────────
// フェーズ0 | 100 → 75%   | 桜散り            (20f毎)
// フェーズ1 |  75 → 50%   | 桜散り            (20f毎)
//           |             | 菊花紋            (90f毎 / offset45f)
// フェーズ2 |  50 → 25%   | 渦潮              (18f毎)
//           |             | 扇                (70f毎)
// フェーズ3 |  25 →  0%   | 龍火              (45f毎)
//           |             | 渦潮 速           (12f毎)
//           |             | 扇 多             (50f毎 / offset25f)
// ─────────────────────────────────────────────────────
// stageData.cpp 側で以下を追加すること:
//   extern void EnemyPat_Japanese_Claude();

#include "DxLib.h"
#include "gv.h"
#include <cmath>

static constexpr double TAU = 6.28318530717958647692; // 2π

// ── 色定数 (和風カラーパレット) ─────────────────────────────────
// img_enemyShot***[color] の color インデックス
//   0=赤  1=黄  2=緑  3=シアン  4=マゼンタ (5=青, 6=白, 7=黒 は非推奨)
static constexpr int C_RED  = 0; // 朱
static constexpr int C_GOLD = 1; // 金
static constexpr int C_CYAN = 3; // 水
static constexpr int C_HANA = 5; // 桜 (マゼンタ)

// ── ヘルパー: 弾1発をリストに追加 ──────────────────────────────
static void AddShot(sEnemyShotSet* s, double x, double y,
                    double muki, double speed, int imgHandle)
{
    sEnemyShot* p             = new sEnemyShot;
    p->x                      = x;
    p->y                      = y;
    p->muki                   = muki;
    p->speed                  = speed;
    p->kind                   = imgHandle;
    p->prev                   = s->pEnemyShotHead->prev;
    p->next                   = s->pEnemyShotHead;
    s->pEnemyShotHead->prev->next = p;
    s->pEnemyShotHead->prev       = p;
}

// ── ヘルパー: sEnemyShotSet を生成してグローバルリストに追加 ────
static sEnemyShotSet* SpawnShotSet(double x, double y, double muki,
                                    sEnemyShotSet::PatternFunc func,
                                    int kind = 0)
{
    sEnemyShotSet* s        = new sEnemyShotSet;
    s->x                    = x;
    s->y                    = y;
    s->muki                 = muki;
    s->patternFunc          = func;
    s->count                = 0;
    s->kind                 = kind;
    s->pEnemyShotHead       = new sEnemyShot;
    s->pEnemyShotHead->prev = s->pEnemyShotHead;
    s->pEnemyShotHead->next = s->pEnemyShotHead;
    s->prev                     = enemyShotSetHead.prev;
    s->next                     = &enemyShotSetHead;
    enemyShotSetHead.prev->next = s;
    enemyShotSetHead.prev       = s;
    return s;
}

// ════════════════════════════════════════════════════════════
// ① 桜散り (Sakura Chiri) — 鱗弾 12 方向
//
//  ・スパイラル: s->kind を呼び出し毎にインクリメントして渡すことで、
//    毎セット約 12.6° ずつ基底角度が回転し、連続発射で螺旋を形成する。
//
//  ・花びらの漂い: 重力ドリフト (y + 0.25/f) + sin 横揺れを移動ループに
//    加算することで、弾が舞い落ちる花びらのように見える。
// ════════════════════════════════════════════════════════════
static void ShotSakura(sEnemyShotSet* s)
{
    if (s->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) != 1)
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int    nWay      = 12;
        double       baseAngle = s->muki + s->kind * 0.22; // 約12.6°ずつ回転
        static const int palette[3] = { C_RED, C_HANA, C_GOLD };
        for (int i = 0; i < nWay; i++) {
            double angle = baseAngle + (TAU / nWay) * i;
            double speed = 1.8 + GetRand(60) * 0.01; // 1.80 〜 2.40
            AddShot(s,
                    s->x + GetRand(20) - 10, // 発射位置に微ブレ (-10〜+10)
                    s->y,
                    angle, speed,
                    img_enemyShotScale[palette[i % 3]]); // 朱・桜・金を交互
        }
    }

    // 花びらの漂い: 重力 + sin 横揺れ
    // p->x を位相ずらしに使うと弾ごとに揺れが異なり自然に見える
    sEnemyShot* p = s->pEnemyShotHead->next;
    while (p != s->pEnemyShotHead) {
        double sway = sin(s->count * 0.07 + p->x * 0.015) * 0.40;
        p->x += p->speed * cos(p->muki) + sway;
        p->y += p->speed * sin(p->muki) + 0.25; // 重力ドリフト
        p = p->next;
    }
}

// ════════════════════════════════════════════════════════════
// ② 菊花紋 (Kikukamon) — 中玉 16 方向 × 3 速度リング
//
//  外リング: 中玉 赤/金交互, speed 2.6
//  中リング: 中玉 桜色,       speed 1.4
//  内リング: 小玉 金,         speed 0.9 (半ステップ offset でずらす)
//
//  → 菊の家紋のような同心三重の花弁を形成する。
// ════════════════════════════════════════════════════════════
static void ShotKiku(sEnemyShotSet* s)
{
    if (s->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int    nWay = 16;
        const double step = TAU / nWay;
        for (int i = 0; i < nWay; i++) {
            double angle = s->muki + step * i;
            AddShot(s, s->x, s->y, angle, 2.6,
                    img_enemyShotMediumBall[i % 2 == 0 ? C_RED : C_GOLD]);
            AddShot(s, s->x, s->y, angle, 1.4,
                    img_enemyShotMediumBall[C_HANA]);
            AddShot(s, s->x, s->y, angle + step * 0.5, 0.9,
                    img_enemyShotSmallBall[C_GOLD]);
        }
    }

    sEnemyShot* p = s->pEnemyShotHead->next;
    while (p != s->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ════════════════════════════════════════════════════════════
// ③ 渦潮 (Uzushio) — 小玉 6 方向 × 2 速
//
//  baseAngle = s->kind * 15° (TAU/24) でセットごとに回転。
//  呼び出し側で kind を毎回インクリメントして渡すと、
//  連続セットが渦巻き状に広がる。
//
//  外弾: 水色 speed 2.8 / 内弾: 桜色 speed 1.5 の2重リング。
// ════════════════════════════════════════════════════════════
static void ShotUzushio(sEnemyShotSet* s)
{
    if (s->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) != 1)
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int    nWay      = 6;
        const double baseAngle = s->kind * (TAU / 24.0); // 15°ずつ回転
        const double step      = TAU / nWay;
        for (int i = 0; i < nWay; i++) {
            double angle = baseAngle + step * i;
            AddShot(s, s->x, s->y, angle, 2.8,
                    img_enemyShotSmallBall[C_CYAN]);  // 外: 水色 速め
            AddShot(s, s->x, s->y, angle, 1.5,
                    img_enemyShotSmallBall[C_HANA]);  // 内: 桜色 遅め
        }
    }

    sEnemyShot* p = s->pEnemyShotHead->next;
    while (p != s->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ════════════════════════════════════════════════════════════
// ④ 扇 (Ougi) — 銃弾 7 方向扇形 + 大玉1発
//
//  プレイヤー狙いを中心に ±45° (合計 90°) の扇形に展開。
//  外側の弾ほど速く (端: 3.2, 中心: 2.0) → 扇が視覚的に広がって見える。
//  扇の要に大玉金1発を添える。
// ════════════════════════════════════════════════════════════
static void ShotOugi(sEnemyShotSet* s)
{
    if (s->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int    nWay   = 7;
        const double spread = TAU / 8.0;           // ±45°, 合計90°の扇
        const double step   = spread / (nWay - 1);
        for (int i = 0; i < nWay; i++) {
            double angle = s->muki - spread / 2.0 + step * i;
            double dist  = fabs(i - (nWay - 1) * 0.5) / ((nWay - 1) * 0.5);
            double speed = 2.0 + dist * 1.2; // 中心: 2.0  / 端: 3.2
            AddShot(s, s->x, s->y, angle, speed,
                    img_enemyShotBullet[C_RED]);
        }
        // 扇の要: 大玉 金
        AddShot(s, s->x, s->y, s->muki, 1.2,
                img_enemyShotLargeBall[C_GOLD]);
    }

    sEnemyShot* p = s->pEnemyShotHead->next;
    while (p != s->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ════════════════════════════════════════════════════════════
// ⑤ 龍火 (Ryuuka) — 菱形弾 3 方向 × 3 波
//
//  count 0, 3, 6 の 3 タイミングで1波ずつ発射。
//  波が進むにつれて広がり増加 (+0.10rad/波) & 速度低下 (-0.5/波)。
//  炎の揺らぎを表現するため、各波で色も変化 (朱→金→桜)。
// ════════════════════════════════════════════════════════════
static void ShotRyuuka(sEnemyShotSet* s)
{
    if (s->count < 7 && s->count % 3 == 0) {
        const int waveIdx = s->count / 3; // 0, 1, 2
        static const int waveCols[3] = { C_RED, C_GOLD, C_HANA };

        if (waveIdx == 0)
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        else
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double spread = 0.15 + waveIdx * 0.10; // 波ごとに広がり増加
        double speed  = 4.0  - waveIdx * 0.50; // 波ごとに速度低下
        for (int i = -1; i <= 1; i++) {
            AddShot(s, s->x, s->y,
                    s->muki + i * spread,
                    speed,
                    img_enemyShotDiamond[waveCols[waveIdx]]);
        }
    }

    sEnemyShot* p = s->pEnemyShotHead->next;
    while (p != s->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ════════════════════════════════════════════════════════════
// メインパターン: 幽玄の陣 (Yuugen no Jin)
// ════════════════════════════════════════════════════════════
void EnemyPat_Japanese_Claude()
{
    static int    rotIdx;  // 渦潮・桜散りの回転インデックス (累積)
    static int    phase;   // 現在フェーズ (0〜3, 降格しない)
    static double moveDir; // 移動方向 (+1.0 / -1.0)

    // ─ 初期化 (バトル開始フレームのみ) ──────────────────────────
    if (count == 1) {
        enemy.x     = 240.0;
        enemy.y     =  60.0;
        enemy.maxHp = 400;
        enemy.hp    = enemy.maxHp;
        rotIdx  = 0;
        phase   = 0;
        moveDir = 1.0;
    }

    // ─ フェーズ更新 (HP比で一方向に昇格のみ、降格しない) ────────
    {
        int newPhase = 0;
        if (enemy.hp <= enemy.maxHp * 3 / 4) newPhase = 1;
        if (enemy.hp <= enemy.maxHp / 2)      newPhase = 2;
        if (enemy.hp <= enemy.maxHp / 4)      newPhase = 3;
        if (newPhase > phase) phase = newPhase;
    }

    // ─ 敵移動: 緩やかに左右往復 (フェーズが上がるほど動域拡大) ──
    {
        double speed  = 0.50 + phase * 0.15;
        double margin = 70.0 - phase * 8.0;
        enemy.x += speed * moveDir;
        if (enemy.x > 480.0 - margin) moveDir = -1.0;
        if (enemy.x <         margin)  moveDir =  1.0;
    }

    double aimAngle = atan2(player.y - (enemy.y + 10.0),
                            player.x - enemy.x);
    double sx = enemy.x;
    double sy = enemy.y + 10.0;

    // ────────────────────────────────────────────────────────────
    // フェーズ0: 桜散り (20f毎)
    // ────────────────────────────────────────────────────────────
    if (phase == 0) {
        if (count % 20 == 0)
            SpawnShotSet(sx, sy, aimAngle, ShotSakura, rotIdx++);
    }

    // ────────────────────────────────────────────────────────────
    // フェーズ1: 桜散り (20f毎) + 菊花紋 (90f毎 / offset45f)
    // ────────────────────────────────────────────────────────────
    else if (phase == 1) {
        if (count % 20 == 0)
            SpawnShotSet(sx, sy, aimAngle, ShotSakura, rotIdx++);
        if (count % 90 == 45)
            SpawnShotSet(sx, sy, aimAngle, ShotKiku);
    }

    // ────────────────────────────────────────────────────────────
    // フェーズ2: 渦潮 (18f毎) + 扇 (70f毎)
    // ────────────────────────────────────────────────────────────
    else if (phase == 2) {
        if (count % 18 == 0)
            SpawnShotSet(sx, sy, 0.0, ShotUzushio, rotIdx++);
        if (count % 70 == 0)
            SpawnShotSet(sx, sy, aimAngle, ShotOugi);
    }

    // ────────────────────────────────────────────────────────────
    // フェーズ3 (瀕死): 龍火 (45f) + 渦潮 速 (12f) + 扇 多 (50f/offset25)
    // ────────────────────────────────────────────────────────────
    else { // phase == 3
        if (count % 45 == 0)
            SpawnShotSet(sx, sy, aimAngle, ShotRyuuka);
        if (count % 12 == 0)
            SpawnShotSet(sx, sy, 0.0, ShotUzushio, rotIdx++);
        if (count % 50 == 25)
            SpawnShotSet(sx, sy, aimAngle, ShotOugi);
    }
}