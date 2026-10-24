#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
//  弾幕：サブタレイニアンサン
//   A: 画面中央への引力(自機・A弾に共通)。外接円上に赤中玉のリングが湧き、中央へ吸い込まれる
//   B: 画面中央から赤小玉を前後に連射(引力の影響なし、発射方向は回転)
//   C: 画面中央に静止した赤大玉を密集させて核を表現
//   ボスの残り体力が減るほど、A:引力が強くなり、C:核が大きくなる
// ============================================================

// ------------------------------------------------------------
//  定数 (CX/CY のような短い名前はWindows SDKと衝突するので避ける)
// ------------------------------------------------------------
static constexpr double SUN_CENTER_X = 240.0;   // 画面中央 (ゲーム画面は 480x480)
static constexpr double SUN_CENTER_Y = 240.0;

// 引力
static constexpr double SUN_PULL_MIN = 0.5;  // 体力満タン時の引力(px/フレーム)
static constexpr double SUN_PULL_MAX = 1.6;  // 体力ゼロ時の引力(px/フレーム)
static constexpr double SUN_FORCE_RATIO = 3.0;  // spawnForceParticles用の比例定数

// A: 外接円上の赤中玉リング
static constexpr int    SUN_A_INTERVAL = 45;  // リング発生間隔(フレーム)
static constexpr int    SUN_A_RING_NUM = 60/2-5;  // 1リングあたりの弾数(多いほど間隔が狭い)
static constexpr double SUN_A_SPAWN_R = 240.0 * 1.4142135623730951; // 480x480の外接円の半径
static constexpr double SUN_A_ANGLE_STEP = 0.2*2; // リングごとの基準角の増分(rad)
static constexpr double SUN_A_MARGIN = 240.0;

// B: 中央から前後に撃つ赤小玉
static constexpr int    SUN_B_INTERVAL = 2;    // 発射間隔(フレーム)
static constexpr double SUN_B_SPEED = 3.0-1;  // 弾速(一定)
static constexpr double SUN_B_ANGLE_STEP = 0.05; // 発射ごとの回転量(rad)
static constexpr double SUN_B_RAND_RANGE = 0.05; // 角度に加える乱数の範囲 ±(rad)

// C: 核
static constexpr double SUN_CORE_SPACING = 14.0; // 核の弾の間隔(大玉の直径より小さいので重なって密集する)
static constexpr int    SUN_CORE_RING_MIN = 1;    // 体力満タン時の外周リング番号(0は中心の1発)
static constexpr int    SUN_CORE_RING_MAX = 6;    // 体力ゼロ時の外周リング番号

// ------------------------------------------------------------
//  状態 (count == 1 で初期化)
// ------------------------------------------------------------
static double s_pullSpeed;  // 現在の引力の強さ
static int    s_coreRings;  // 核の目標リング番号
static double s_angleA;     // Aリングの基準角
static double s_angleB;     // Bの発射角
static int    s_shotCountB; // Bの発射回数

// ------------------------------------------------------------
//  共通処理
// ------------------------------------------------------------
// 弾をShotSetのリストの末尾に追加
static void LinkShot(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ShotSetを生成して全体のリストに追加
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
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

// ------------------------------------------------------------
//  A: 外接円上に湧き、引力と同じ速さで中央へ向かう赤中玉のリング
//     pEnemyShotSet->muki : リングの基準角
// ------------------------------------------------------------
static void ShotRingA(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < SUN_A_RING_NUM; i++) {
            double angle = pEnemyShotSet->muki + 2.0 * DX_PI * i / SUN_A_RING_NUM;
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = SUN_CENTER_X + SUN_A_SPAWN_R * cos(angle);
            pEnemyShot->y = SUN_CENTER_Y + SUN_A_SPAWN_R * sin(angle);
            pEnemyShot->muki = angle + DX_PI; // 中央へ向かう向き
            pEnemyShot->speed = 0.0;          // 発生時の速度は0。移動は下の引力処理で行う
            pEnemyShot->margin = SUN_A_MARGIN;
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤
            LinkShot(pEnemyShotSet, pEnemyShot);
        }
    }

    // 引力による移動(加速せず、常に引力と同じ速さ)。中央に達したら消滅
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除に備えて先に控える
        double dx = SUN_CENTER_X - pShot->x;
        double dy = SUN_CENTER_Y - pShot->y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist <= s_pullSpeed) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        else {
            pShot->x += dx / dist * s_pullSpeed;
            pShot->y += dy / dist * s_pullSpeed;
        }
        pShot = pNext;
    }
}

// ------------------------------------------------------------
//  B: 画面中央から前後に撃つ赤小玉(引力の影響なし)
//     pEnemyShotSet->muki : 発射角、pEnemyShotSet->kind : 発射回数(効果音の間引き用)
// ------------------------------------------------------------
static void ShotCrossB(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 高頻度なので効果音は間引く
        if (pEnemyShotSet->kind % 4 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        for (int i = 0; i < 2; i++) { // 前(i=0)と後ろ(i=1)
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = SUN_CENTER_X;
            pEnemyShot->y = SUN_CENTER_Y;
            // GetRand(200) - 100 は -100〜100 の整数
            pEnemyShot->muki = pEnemyShotSet->muki + DX_PI * i
                + (GetRand(200) - 100) / 100.0 * SUN_B_RAND_RANGE;
            pEnemyShot->speed = SUN_B_SPEED;
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤
            LinkShot(pEnemyShotSet, pEnemyShot);
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  C: 画面中央に静止して密集する赤大玉(核)
//     中心の1発 + 6k発の同心円リングで構成。s_coreRings が増えると外側にリングを追加する
//     pEnemyShotSet->param_i[0] : 生成済みのリング数
// ------------------------------------------------------------
static void ShotCoreC(sEnemyShotSet* pEnemyShotSet)
{
    int built = pEnemyShotSet->param_i[0];
    int target = s_coreRings + 1; // リング0(中心)〜リングs_coreRings
    bool grown = false;

    while (built < target) {
        int r = built;
        int n = (r == 0) ? 1 : 6 * r;
        for (int i = 0; i < n; i++) {
            double angle = 2.0 * DX_PI * i / n + r * 0.5; // リングごとに位相をずらす
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = SUN_CENTER_X + SUN_CORE_SPACING * r * cos(angle);
            pEnemyShot->y = SUN_CENTER_Y + SUN_CORE_SPACING * r * sin(angle);
            pEnemyShot->muki = angle; // 中心から外向き
            pEnemyShot->speed = 0.0;  // 静止
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤
            LinkShot(pEnemyShotSet, pEnemyShot);
        }
        built++;
        grown = true;
    }
    pEnemyShotSet->param_i[0] = built;

    // 核が成長したときだけ音を鳴らす(最初の生成時は鳴らさない)
    if (grown && pEnemyShotSet->count > 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_SubterraneanSun_Claude()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        s_pullSpeed = SUN_PULL_MIN;
        s_coreRings = SUN_CORE_RING_MIN;
        s_angleA = 0.0;
        s_angleB = 0.0;
        s_shotCountB = 0;
        // C: 核は1つのShotSetを使い回す(弾が常に残るので途中で消えない)
        CreateShotSet(ShotCoreC, SUN_CENTER_X, SUN_CENTER_Y, 0.0, 0);
    }

    // ボスの残り体力に応じて、引力の強さと核の大きさを更新
    double lost = 1.0 - (double)enemy.hp / (double)enemy.maxHp; // 0:満タン 〜 1:体力ゼロ
    if (lost < 0.0) lost = 0.0;
    if (lost > 1.0) lost = 1.0;
    s_pullSpeed = SUN_PULL_MIN + (SUN_PULL_MAX - SUN_PULL_MIN) * lost;
    s_coreRings = SUN_CORE_RING_MIN + (int)((SUN_CORE_RING_MAX - SUN_CORE_RING_MIN + 1) * lost);
    if (s_coreRings > SUN_CORE_RING_MAX) s_coreRings = SUN_CORE_RING_MAX;

    // 自機への引力(画面中央へ引き寄せる)
    {
        double dx = SUN_CENTER_X - player.x;
        double dy = SUN_CENTER_Y - player.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist > 0.0) {
            double pullSpeed = (dist < s_pullSpeed) ? dist : s_pullSpeed; // 引き寄せの強さ(中心を通り越さない)
            double angle = atan2(dy, dx);                                 // 引き寄せの向き
            double ratio = SUN_FORCE_RATIO;                               // 比例定数
            spawnForceParticles(player.x, player.y, pullSpeed * cos(angle) * ratio, pullSpeed * sin(angle) * ratio);
            player.x += pullSpeed * cos(angle);
            player.y += pullSpeed * sin(angle);
        }
    }

    // A: 外接円上の赤中玉リング
    if (count % SUN_A_INTERVAL == 1) {
        CreateShotSet(ShotRingA, SUN_CENTER_X, SUN_CENTER_Y, s_angleA, 0);
        s_angleA += SUN_A_ANGLE_STEP;
    }

    // B: 中央から前後に赤小玉
    if (count % SUN_B_INTERVAL == 1) {
        CreateShotSet(ShotCrossB, SUN_CENTER_X, SUN_CENTER_Y, s_angleB, s_shotCountB++);
        s_angleB += SUN_B_ANGLE_STEP;
    }
}