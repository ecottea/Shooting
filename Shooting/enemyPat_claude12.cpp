// enemyPat_tmp.cpp
// ─────────────────────────────────────────────────────────────────────────────
// モチーフ : 太陽（太陽神）
//
// 弾幕パターン一覧:
//   ShotSunRing      黄大玉×12 + 赤中玉×12 の二重リング。
//                    毎フレーム方向を微小回転させ、螺旋状に広がる（太陽の自転）。
//   ShotSolarFlare   赤大玉×8 + 黄菱形弾×8 を 16 方向に配置。
//                    最初の 60 フレームは低速で漂い、その後急激に加速する
//                    （太陽フレアの爆発的放出）。
//   ShotSunParticle  自機狙いを中心に ±0.24rad の扇形 5 発を
//                    3f ごと × 15 波（計 75 発）連射（太陽風）。
//   ShotSunSpiral    3 本腕の黄小玉螺旋。EnemyPat_Sun_Claude から 0.25rad ずつ
//                    回転した角度で繰り返し呼ばれ、螺旋軌道を形成（日輪回転）。
//
// 敵の動き:
//   x = 240 + 100 * sin(count * 0.02)  ← 振幅 100px・周期 約 314f の正弦波往復
//   y = 50  (固定)
//
// 弾幕スケジュール（count 基準・全パターンが異なるフレームに発火する設計）:
//   螺旋   :  count % 6  == 0    初回 f=6
//   輪     :  count % 120 == 5   初回 f=5
//   粒子雨 :  count % 90 == 30   初回 f=30
//   フレア :  count % 180 == 60  初回 f=60
//
// 注意:
//   count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//   画面外弾の削除はメインルーチンで行う。
//   GetRand(x) は 0 以上 x 以下の整数を返す。
// ─────────────────────────────────────────────────────────────────────────────

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================================================
// 内部ユーティリティ
// =============================================================================

// sEnemyShotSet を 1 つ生成してグローバルリストへ接続する
static sEnemyShotSet* CreateShotSet(double x, double y, double muki,
    void(*patternFunc)(sEnemyShotSet*))
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
    return p;
}

// sEnemyShot を 1 つ生成して指定 ShotSet の循環リストへ追加する
static void AddShot(sEnemyShotSet* pSet,
    double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* s = new sEnemyShot;
    s->x = x;
    s->y = y;
    s->muki = muki;
    s->speed = speed;
    s->kind = kind;

    s->prev = pSet->pEnemyShotHead->prev;
    s->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = s;
    pSet->pEnemyShotHead->prev = s;
}

// =============================================================================
// ShotSunRing（太陽輪）
//   黄大玉 12 発（外リング・低速）と赤中玉 12 発（内リング・高速）を
//   均等に配置し、毎フレーム方向をわずかに回転させながら外向きに広がる。
//   二重の螺旋軌跡が太陽のコロナ・自転を表現する。
// =============================================================================
static void ShotSunRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int N = 12;
        for (int i = 0; i < N; i++) {
            double a = (2.0 * DX_PI * i) / N;
            // 外リング: 黄大玉（低速・広い螺旋）
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                a, 1.2,
                img_enemyShotLargeBall[1]);  // 黄
            // 内リング: 赤中玉（高速・半ピッチずれ）
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                a + DX_PI / N, 2.2,
                img_enemyShotMediumBall[0]); // 赤
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->count <= 600) p->muki += 0.018;                       // 約 1 度/フレーム回転（自転演出）
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// =============================================================================
// ShotSolarFlare（太陽フレア）
//   赤大玉 8 発と黄菱形弾 8 発を 16 方向に交互配置。
//   count < 60 の間は低速で漂い、count >= 60 から急加速する。
//   フレアが突如爆発的に放出される緊張感を演出。
// =============================================================================
static void ShotSolarFlare(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const int N = 30;
        for (int i = 0; i < N; i++) {
            double a = (2.0 * DX_PI * i) / N;
            // 赤大玉（主弾）
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                a, 0.5,
                img_enemyShotLargeBall[0]); // 赤
            // 黄菱形弾（中間角度に挿入して 16 方向の星形を形成）
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                a + DX_PI / N, 0.5,
                img_enemyShotDiamond[1]);   // 黄
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->count >= 60) {                  // 60 フレーム後から急加速
            p->speed += 0.12;
            if (p->speed > 7.0) p->speed = 7.0;
        }
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// =============================================================================
// ShotSunParticle（太陽粒子雨）
//   自機方向を中心に ±0.24rad（0.12 刻み）の扇形 5 発を
//   3 フレームごとに 15 波（計 75 発）連射する。
//   速度をランダムに散らし、密な弾幕で太陽風を表現。
// =============================================================================
static void ShotSunParticle(sEnemyShotSet* pEnemyShotSet)
{
    // 3f ごと・合計 15 波分（count = 0, 3, 6, ..., 42）
    if (pEnemyShotSet->count % 3 == 0 && pEnemyShotSet->count < 45) {
        if (pEnemyShotSet->count == 0)
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int FAN = 5;
        for (int i = 0; i < FAN; i++) {
            double offset = (i - FAN / 2) * 0.12;       // -0.24 / -0.12 / 0.00 / +0.12 / +0.24 rad
            double speed = 3.0 + GetRand(4) * 0.25;    // 3.00 / 3.25 / 3.50 / 3.75 / 4.00
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                pEnemyShotSet->muki + offset, speed,
                img_enemyShotSmallBall[1]); // 黄
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// =============================================================================
// ShotSunSpiral（日輪螺旋）
//   3 本腕の黄小玉を等角度（120 度間隔）に発射。
//   EnemyPat_Sun_Claude が呼び出しのたびに muki を 0.25rad ずつ増やすことで
//   全体として螺旋状の弾道パターンを形成する（日輪の回転）。
// =============================================================================
static void ShotSunSpiral(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const int ARMS = 3;
        for (int i = 0; i < ARMS; i++) {
            double a = pEnemyShotSet->muki + (2.0 * DX_PI * i) / ARMS;
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                a, 2.5,
                img_enemyShotSmallBall[1]); // 黄
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// =============================================================================
// EnemyPat_Sun_Claude（太陽神）
//   ゲーム画面上部中央（x=240, y=50）に居座り、
//   正弦波で左右に揺れながら 4 種の太陽弾幕を展開する。
// =============================================================================
void EnemyPat_Sun_Claude()
{
    static double spiralAngle; // 螺旋の開始角度（毎発 0.25rad 増加）

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        spiralAngle = 0.0;
    }
    else {
        // 正弦波による左右往復（振幅 ±100px・周期 約 314f ≈ 5.2 秒）
        enemy.x = 240.0 + 100.0 * sin(count * 0.02);
    }

    // ── 日輪螺旋: 6f ごと ─────────────────────────────────────
    // 3 本腕 × 毎 6f 発火 = 密な螺旋弾幕。0.25rad 刻みで回転。
    if (count % 6 == 0) {
        CreateShotSet(enemy.x, enemy.y, spiralAngle, ShotSunSpiral);
        spiralAngle += 0.25;
    }

    // ── 太陽輪: 120f ごと（初回 f=5）────────────────────────
    // 黄大玉 + 赤中玉の二重リングが自転しながら広がる。
    if (count % 120 == 5) {
        CreateShotSet(enemy.x, enemy.y, 0.0, ShotSunRing);
    }

    // ── 太陽粒子雨: 90f ごと（初回 f=30）────────────────────
    // 自機狙い扇形連射。muki を発射時の自機方向に設定。
    if (count % 90 == 30) {
        double muki = atan2(player.y - enemy.y, player.x - enemy.x);
        CreateShotSet(enemy.x, enemy.y, muki, ShotSunParticle);
    }

    // ── 太陽フレア: 180f ごと（初回 f=60）───────────────────
    // 16 方向の星形バースト。60f 間の溜めから急加速する。
    if (count % 180 == 60) {
        CreateShotSet(enemy.x, enemy.y, 0.0, ShotSolarFlare);
    }
}