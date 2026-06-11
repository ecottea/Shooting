// enemyPat_comet.cpp
//
// 彗星をモチーフにした弾幕パターン
//
//  【敵の動き】
//    画面上部を楕円軌道で周回する（彗星の公転を模倣）
//    中心 (240, 120) ／水平半径 190 ／垂直半径 80 ／周期 480 f
//    → X ∈ [50, 430]  Y ∈ [40, 200]  （480×480 画面内に収まる）
//
//  【弾幕構成】
//    ① ShotCometTail     ── 毎  4 f: 軌道逆方向にシアン小玉を扇状展開（尾）
//    ② ShotCometNucleus  ── 毎 60 f: 全方位に白大玉＋シアン中玉の二重放射（核）
//    ③ ShotMeteorShower  ── 毎 28 f: プレイヤー方向に黄菱形弾の横並び列（流星群）
//
//  【弾数見積もり（同時最大）】
//    尾:     5 発/set × ~80 sets ≒ 400 発
//    核:    24 発/set ×  ~3 sets ≒  72 発
//    流星:   7 発/set ×  ~6 sets ≒  42 発
//    合計: 約 514 発  ← sEnemyShot プール 4096 以内

#include "DxLib.h"
#include "gv.h"
#include <math.h>

// ══════════════════════════════════════════════════════════════════
//  ヘルパー: sEnemyShotSet を生成してグローバルリストに追加する
// ══════════════════════════════════════════════════════════════════
static void SpawnShotSet(double x, double y, double muki,
    void (*patternFunc)(sEnemyShotSet*))
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = patternFunc;
    p->x = x;
    p->y = y;
    p->muki = muki;

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
}

// ══════════════════════════════════════════════════════════════════
//  ① 彗星の尾
//
//  進行方向の逆を向いて、シアン小玉を ±25° の扇状に 5 発射出。
//  速度を 1.2〜2.6 と変えることで尾が自然にばらけて広がる。
//  sound: なし（毎 4 f 発射のため、鳴らすと煩瑣になるため省略）
// ══════════════════════════════════════════════════════════════════
static void ShotCometTail(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        const int    NUM = 5;
        const double SPREAD = 25.0 / 180.0 * DX_PI;   // ±25° 扇

        for (int i = 0; i < NUM; i++) {
            // t ∈ [-1, +1] で均等分割
            double t = (i - (NUM - 1) / 2.0) / ((NUM - 1) / 2.0);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + DX_PI + SPREAD * t;
            pEnemyShot->speed = 1.2 + 0.35 * i;   // 1.20, 1.55, 1.90, 2.25, 2.60
            pEnemyShot->kind = img_enemyShotSmallBall[3];  // シアン

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ══════════════════════════════════════════════════════════════════
//  ② 彗星の核フラッシュ
//
//  外側: 白大玉  16 方向, speed 2.8  ──┐
//  内側: シアン中玉 8 方向, speed 1.6  ──┘ 計 24 発の二重放射
//  muki（軌道の接線方向）で全体を回転させるので、
//  フラッシュごとに向きが変わり単調にならない。
// ══════════════════════════════════════════════════════════════════
static void ShotCometNucleus(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 外側: 白大玉 16 方向
        const int out = 16;
        for (int i = 0; i < out; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (2.0 * DX_PI / out) * i;
            pEnemyShot->speed = 2.8;
            pEnemyShot->kind = img_enemyShotLargeBall[6];  // 白

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 内側: シアン中玉 8 方向（外側から 22.5° ずらし）
        const int in = 8;
        for (int i = 0; i < in; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki
                + (2.0 * DX_PI / in) * i
                + (DX_PI / in);   // 22.5° オフセット
            pEnemyShot->speed = 1.6;
            pEnemyShot->kind = img_enemyShotMediumBall[3];  // シアン

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ══════════════════════════════════════════════════════════════════
//  ③ 流星群
//
//  プレイヤー方向を向いて、黄菱形弾を進行方向に直交する軸に 7 発並べる。
//  中央の弾ほど speed が高く、三日月状のフォーメーションで迫ってくる。
//  perpX/perpY は muki に対する直交ベクトル。
// ══════════════════════════════════════════════════════════════════
static void ShotMeteorShower(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int    NUM = 7;
        const double STEP = 18.0;   // 弾の横間隔 (px)

        // 進行方向に直交するベクトル（左右方向）
        double perpX = sin(pEnemyShotSet->muki);
        double perpY = -cos(pEnemyShotSet->muki);

        for (int i = 0; i < NUM; i++) {
            int  d = i - NUM / 2;                         // -3, -2, -1, 0, 1, 2, 3
            int  absd = d >= 0 ? d : -d;
            double speedUp = (NUM / 2 - absd) * 0.2;        // 中央: +0.6, 端: +0.0

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + perpX * (d * STEP);
            pEnemyShot->y = pEnemyShotSet->y + perpY * (d * STEP);
            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->speed = 3.0 + speedUp;              // 3.0〜3.6
            pEnemyShot->kind = img_enemyShotDiamond[1];    // 黄

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ══════════════════════════════════════════════════════════════════
//  敵本体: 楕円軌道周回型ボス「彗星」
//
//  軌道パラメータ
//    中心 (CX, CY) = (240, 120)
//    水平半径 RX = 190  垂直半径 RY = 80
//    角速度 omega = 2π / 480 [rad/f]  → 8 秒で 1 周
//
//  接線方向 moveMuki を毎フレーム計算して各ショット関数に渡す。
//    尾: muki = moveMuki   （関数内で +π して逆方向に撃つ）
//    核: muki = moveMuki   （放射パターン全体を軌道位相で回転）
//    流星群: muki = プレイヤー方向  （追尾で圧力をかける）
//
//  発射スケジュール
//     4 f ごと: ShotCometTail
//    60 f ごと: ShotCometNucleus
//    28 f ごと: ShotMeteorShower
// ══════════════════════════════════════════════════════════════════
void EnemyPat_Comet_Claude()
{
    if (count == 1) {
        enemy.maxHp = enemy.hp = 120;
    }

    // ── 楕円軌道 ─────────────────────────────────────────────────
    constexpr double CX = 240.0;
    constexpr double CY = 120.0;
    constexpr double RX = 190.0;
    constexpr double RY = 80.0;
    constexpr double PERIOD = 480.0;
    const     double omega = 2.0 * DX_PI / PERIOD;

    double angle = (count - 1) * omega;
    enemy.x = CX + RX * cos(angle);
    enemy.y = CY + RY * sin(angle);

    // 接線ベクトル → 移動方向 muki
    double vx = -RX * omega * sin(angle);
    double vy = RY * omega * cos(angle);
    double moveMuki = atan2(vy, vx);

    // ── ① 彗星の尾 (4 f ごと) ───────────────────────────────────
    if (count % 4 == 0) {
        SpawnShotSet(enemy.x, enemy.y, moveMuki, ShotCometTail);
    }

    // ── ② 核フラッシュ (60 f ごと) ─────────────────────────────
    if (count % 60 == 0) {
        SpawnShotSet(enemy.x, enemy.y, moveMuki, ShotCometNucleus);
    }

    // ── ③ 流星群 (28 f ごと, プレイヤー狙い) ────────────────────
    if (count % 28 == 0) {
        double meteorMuki = atan2(player.y - enemy.y, player.x - enemy.x);
        SpawnShotSet(enemy.x, enemy.y, meteorMuki, ShotMeteorShower);
    }
}
