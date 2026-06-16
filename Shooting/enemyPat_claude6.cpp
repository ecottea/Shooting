// enemyPat_waveParticleBoundary.cpp
// ボスパターン「波と粒の境界」
//
//  モチーフ: 量子力学の波と粒子の二重性
//    ・観測前  = 波   : 輪状にゆっくり広がるシアンの弾幕
//    ・観測後  = 粒子 : 全弾が一斉に自機方向へ急収束し赤く変色
//    ・"境界"の瞬間がこのパターンの核心
//
//  HP フェーズ移行
//    Phase 0 (HP > 400) : 波面のみ ─ 回転オフセット付き螺旋展開
//    Phase 1 (HP > 200) : 波面崩壊 + 補間波面
//    Phase 2 (HP ≤ 200) : 波面崩壊 + 高速粒子流 + 背景波面の三重攻撃

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  内部ユーティリティ
// ============================================================

// 弾 1 個を sEnemyShotSet のリストに追加
static void AddShot(sEnemyShotSet* s,
    double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;

    p->prev = s->pEnemyShotHead->prev;
    p->next = s->pEnemyShotHead;
    s->pEnemyShotHead->prev->next = p;
    s->pEnemyShotHead->prev = p;
}

// sEnemyShotSet を生成してグローバルリストに登録
static void FireSet(double x, double y, double muki,
    sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* s = new sEnemyShotSet;
    s->count = 0;
    s->patternFunc = func;
    s->x = x;
    s->y = y + 10.0;
    s->muki = muki;

    s->pEnemyShotHead = new sEnemyShot;
    s->pEnemyShotHead->prev = s->pEnemyShotHead;
    s->pEnemyShotHead->next = s->pEnemyShotHead;

    s->prev = enemyShotSetHead.prev;
    s->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = s;
    enemyShotSetHead.prev = s;
}

// ============================================================
//  弾幕①「波面」ShotWaveFront
//
//  36 発のシアン中玉を全方位へ一定速で展開する。
//  pEnemyShotSet->muki を角度オフセットとして使い、
//  複数の輪を重ねたとき螺旋模様に見えるようにする。
// ============================================================
static void ShotWaveFront(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        const int N = 36;
        for (int i = 0; i < N; i++) {
            double muki = 2.0 * DX_PI * i / N + pEnemyShotSet->muki;
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                muki, 1.2,
                img_enemyShotMediumBall[3]); // シアン
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  弾幕②「波面崩壊」ShotWaveCollapse
//
//  展開した輪（波）が、50 フレーム後に全弾へ一斉方向転換する。
//  「観測によって波動関数が粒子に収束する」瞬間を表現。
//
//    count == 0  : シアン 48 発が輪状に低速展開（波の状態）
//    count == 50 : 全弾が自機方向へ急加速、赤い小玉に変色（崩壊）
//
//  ポイント: 輪が広がった後で向き直すため、各弾の現在位置から
//  自機への角度がそれぞれ微妙に異なり、収束弾幕になる。
// ============================================================
static void ShotWaveCollapse(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        const int N = 48;
        for (int i = 0; i < N; i++) {
            double muki = 2.0 * DX_PI * i / N;
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                muki, 0.8,
                img_enemyShotMediumBall[3]); // シアン（波）
        }
    }

    // 観測イベント ─ 一斉に自機方向へ向き直す（崩壊）
    if (pEnemyShotSet->count == 50) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            p->muki = atan2(player.y - p->y, player.x - p->x)
                + (GetRand(24) - 12) / 180.0 * DX_PI; // ±12° のばらつき
            p->speed = 3.0;
            p->kind = img_enemyShotSmallBall[0]; // 赤（粒子）
            p = p->next;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  弾幕③「粒子流」ShotParticleStream
//
//  自機方向に高速マゼンタ銃弾を 3 本束で撃ち込む。
//  「純粋な粒子」としての直線的な振る舞いを表現。
// ============================================================
static void ShotParticleStream(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        double base = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        for (int i = -1; i <= 1; i++) {
            double offset = i * (10.0 / 180.0 * DX_PI); // ±10°
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                base + offset, 4.5,
                img_enemyShotBullet[5]); // マゼンタ銃弾
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  ボス本体「波と粒の境界」Pattern_WaveParticleBoundary
//
//  移動: サイン波揺れ + 左右往復
//  HP フェーズ（maxHp = 600 を基準）:
//    Phase 0 (hp > 400): 波面を 60f ごとに螺旋展開
//    Phase 1 (hp > 200): 波面崩壊（90f）+ 補間波面（30f）
//    Phase 2 (hp ≤ 200): 波面崩壊（70f）+ 粒子流（18f）+ 背景波面（40f）
// ============================================================
void EnemyPat_Namistubu_Claude()
{
    static double bossVX;
    static double wobblePhase;

    // ── 初期化 ──
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 300;
        bossVX = 1.2;
        wobblePhase = 0.0;
    }

    // ── 移動 ──
    enemy.x += bossVX;
    wobblePhase += 0.03;
    enemy.y = 60.0 + sin(wobblePhase) * 20.0;
    if (enemy.x < 60.0 || enemy.x > 420.0) bossVX = -bossVX;

    // ── フェーズ判定 ──
    const int phase = (enemy.hp > enemy.maxHp * 2 / 3) ? 0
        : (enemy.hp > enemy.maxHp * 1 / 3) ? 1
        : 2;

    const double ex = enemy.x;
    const double ey = enemy.y;
    const double toPlayer = atan2(player.y - ey, player.x - ex);

    // ── 弾幕発射 ──

    if (phase == 0) {
        // 波面のみ ─ count * 0.05 rad ずつ回転させると螺旋状に重なる
        if (count % 60 == 0) {
            FireSet(ex, ey, count * 0.05, ShotWaveFront);
        }
    }
    else if (phase == 1) {
        // 波面崩壊（90f ごと）
        if (count % 90 == 0) {
            FireSet(ex, ey, 0.0, ShotWaveCollapse);
        }
        // 補間波面（30f ごと、自機方向へオフセット）
        if (count % 50 == 25) {
            FireSet(ex, ey, toPlayer, ShotWaveFront);
        }
    }
    else {
        // 波面崩壊（70f ごと）
        if (count % 70 == 0) {
            FireSet(ex, ey, 0.0, ShotWaveCollapse);
        }
        // 高速粒子流（18f ごと）
        if (count % 18 == 0) {
            FireSet(ex, ey, toPlayer, ShotParticleStream);
        }
        // 背景の波面も維持（40f ごと、高速回転オフセット）
        if (count % 40 == 20) {
            FireSet(ex, ey, count * 0.10, ShotWaveFront);
        }
    }
}
