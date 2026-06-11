// enemyPat_handmade4.cpp
// 弾幕パターン「滝」
//
// フェーズ構成 (1フェーズ = 480f ≒ 8秒 @ 60fps)
//   Phase 0: 本流のみ
//   Phase 1: 本流 + しぶき
//   Phase 2: 本流 + しぶき + 渦
//   Phase 3: 本流 + しぶき + 渦 + 波紋  ← 以降ループ

#include "DxLib.h"
#include "gv.h"
#include <math.h>

// ================================================================
// 内部ユーティリティ
// ================================================================

// 弾1発をセットのリストへ末尾挿入
static void addShot(sEnemyShotSet* pSet,
    double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;  p->y = y;  p->muki = muki;  p->speed = speed;  p->kind = kind;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// EnemyShotSet を生成してグローバルリストへ登録
static void spawnSet(sEnemyShotSet::PatternFunc func,
    double x, double y, double muki)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;  pSet->y = y;  pSet->muki = muki;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ================================================================
// 弾幕A: 滝の本流
//   青/シアンの小玉が上から密に流れ落ちる。
//   30フレーム × 毎フレーム3発 = 90発/セット。
//   各弾のx座標を位相とした正弦波で横ゆらぎを加える。
// ================================================================
static void ShotWaterfall_Stream(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 30フレーム間、毎フレーム3発追加
    if (pSet->count < 30) {
        for (int i = 0; i < 3; i++) {
            double x = pSet->x + (GetRand(61) - 30);             // 発射源から左右±30
            double y = pSet->y + (GetRand(61) - 30);
            double muki = DX_PI * 0.5 + (GetRand(21) - 10) / 100.0; // 真下 ±0.10 rad
            double speed = 3.5 + GetRand(20) / 10.0;                  // 3.5 ~ 5.5
            int kind = (GetRand(2) == 0) ? img_enemyShotSmallBall[4]  // 青
                : img_enemyShotSmallBall[3]; // シアン
            addShot(pSet, x, y, muki, speed, kind);
        }
    }

    // 全弾移動: 直線落下 + x座標依存の正弦波ゆらぎ
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p->x += sin(pSet->count * 0.15 + p->x * 0.08) * 0.3;
        p = p->next;
    }
}

// ================================================================
// 弾幕B: しぶき
//   滝つぼ（画面下部）から白/シアン/青の小玉が放物線を描いて飛び散る。
//   count==0 で24発を扇状に一括生成し、重力で弧を描かせる。
// ================================================================
static void ShotWaterfall_Splash(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int NUM = 24;
        for (int i = 0; i < NUM; i++) {
            double t_r = (double)i / (NUM - 1);          // 0.0 ~ 1.0
            double muki = -DX_PI * 0.5 + (-0.7 + t_r * 1.4); // 真上 ±0.7 rad (±40°)
            double spd = 2.0 + GetRand(30) / 10.0;       // 2.0 ~ 5.0
            int kind;
            switch (i % 3) {
            case 0:  kind = img_enemyShotSmallBall[6]; break; // 白
            case 1:  kind = img_enemyShotSmallBall[3]; break; // シアン
            default: kind = img_enemyShotSmallBall[4]; break; // 青
            }
            addShot(pSet, pSet->x + (GetRand(41) - 20), pSet->y, muki, spd, kind);
        }
    }

    // 重力シミュレーション: 速度ベクトルのy成分を毎フレーム加速
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double vx = p->speed * cos(p->muki);
        double vy = p->speed * sin(p->muki) + 0.07; // 重力 0.07 px/frame²
        p->speed = sqrt(vx * vx + vy * vy);
        p->muki = atan2(vy, vx);
        p->x += vx;
        p->y += vy;
        p = p->next;
    }
}

// ================================================================
// 弾幕C: 渦（水の渦巻き）
//   3アームの螺旋弾が回転しながら外向きに広がる。
//   60フレーム × 毎フレーム3発 = 180発/セット。
//   speed は count に比例して徐々に増加（遠心力の表現）。
// ================================================================
static void ShotWaterfall_Vortex(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    if (pSet->count < 60) {
        const int ARMS = 3;
        for (int arm = 0; arm < ARMS; arm++) {
            double muki = pSet->count * 0.18 + arm * (DX_PI * 2.0 / ARMS);
            double speed = 1.5 + pSet->count * 0.03; // 1.5 ~ 3.3
            int kind;
            switch (arm) {
            case 0:  kind = img_enemyShotSmallBall[3]; break; // シアン
            case 1:  kind = img_enemyShotSmallBall[2]; break; // 緑
            default: kind = img_enemyShotSmallBall[4]; break; // 青
            }
            addShot(pSet, pSet->x, pSet->y, muki, speed, kind);
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ================================================================
// 弾幕D: 波紋（滝つぼの水面）
//   シアン → 白 → 青 の3リングが18フレーム間隔で全方向に拡散。
//   白リングは位相を半ピッチずらして隙間を埋める。
// ================================================================
static void ShotWaterfall_Ripple(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        // 第1波紋: シアン 20発, 速度1.5
        for (int i = 0; i < 20; i++) {
            double muki = DX_PI * 2.0 * i / 20;
            addShot(pSet, pSet->x, pSet->y, muki, 1.5, img_enemyShotSmallBall[3]);
        }
    }
    if (pSet->count == 18) {
        // 第2波紋: 白 20発, 速度2.5, 半ピッチずらし
        for (int i = 0; i < 20; i++) {
            double muki = DX_PI * 2.0 * i / 20 + DX_PI / 20.0;
            addShot(pSet, pSet->x, pSet->y, muki, 2.5, img_enemyShotSmallBall[6]);
        }
    }
    if (pSet->count == 36) {
        // 第3波紋: 青 20発, 速度1.0
        for (int i = 0; i < 20; i++) {
            double muki = DX_PI * 2.0 * i / 20;
            addShot(pSet, pSet->x, pSet->y, muki, 1.0, img_enemyShotSmallBall[4]);
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ================================================================
// 敵本体パターン: Pattern_Waterfall
// ================================================================
void Pattern_Waterfall_claude()
{
    // ── 初期化 ──────────────────────────────────────────────────
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 120;
    }

    // フェーズ
    const int T[] = {0, 628, 1346, 1849, 2268};
    int phase;
    if (count <= T[1]) phase = 0;
    else if (count <= T[2]) phase = 1;
    else if (count <= T[3]) phase = 2;
    else phase = 3;

    // ── 敵の動き ─────────────────────────────────────────────────
    switch (phase % 4) {
    case 0: // ゆったり横移動
        enemy.x = 240.0 + 110.0 * sin(count * 0.020);
        enemy.y = 55.0;
        break;
    case 1: // やや速い横移動
        enemy.x = 240.0 + 110.0 * sin((count - T[1]) * 0.035);
        enemy.y = 55.0;
        break;
    case 2: // 縦揺れ追加
        enemy.x = 240.0 + 100.0 * sin((count - T[2]) * 0.025);
        enemy.y = 55.0 + 20.0 * sin((count - T[2]) * 0.050);
        break;
    case 3: // 横 + 縦、複合
        enemy.x = 240.0 + 110.0 * sin((count - T[3]) * 0.030);
        enemy.y = 55.0 + 15.0 * sin((count - T[3]) * 0.060);
        break;
    }

    // ── 弾幕発射スケジュール ──────────────────────────────────────

    // 【A】本流: Phase0から常時。12フレームに1セット
    if (count % 12 == 0) {
        spawnSet(ShotWaterfall_Stream, enemy.x, enemy.y + 10.0, DX_PI * 0.5);
    }

    // 【B】しぶき: Phase1から追加。80フレームに1セット（画面下部からランダム位置）
    if (phase == 1 && count % 80 == 40) {
        double sx = 120.0 + GetRand(240); // x: 120~360
        spawnSet(ShotWaterfall_Splash, sx, 370.0, -DX_PI * 0.5);
    }

    // 【C】渦: Phase2から追加。120フレームに1セット
    if (phase == 2 && count % 120 == 0) {
        spawnSet(ShotWaterfall_Vortex, enemy.x, enemy.y + 10.0, 0.0);
    }

    // 【D】波紋: Phase3から追加。90フレームに1セット（画面中央付近のランダム位置）
    if (phase == 3 && count % 90 == 45) {
        double rx = 150.0 + GetRand(180); // x: 150~330
        double ry = 180.0 + GetRand(120); // y: 180~300
        spawnSet(ShotWaterfall_Ripple, rx, ry, 0.0);
    }
}