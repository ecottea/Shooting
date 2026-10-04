#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  四天王奥義「三歩必殺」
//  ------------------------------------------------------------
//  1. 一段階目：高密度の内円を静止配置
//  2. 二段階目：隙間のある外円を静止配置
//     （この隙間がプレイヤーの安全地帯となる）
//  3. 三段階目：外側全域を高密度で埋める
//  4. 全弾を外側へ一斉発射
//
//  プレイヤーは二段階目の隙間に入っていなければ
//  発射時に弾に呑まれる。
// ============================================================

// 共有状態
static bool g_burst = false;  // true になったら全弾発射

// ---- 弾をセットに追加するヘルパー ----
static void AddShot(sEnemyShotSet* pEnemyShotSet,
    double x, double y, double muki, double speed, int kind)
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

// ---- 発射フェーズの共通更新処理 ----
static void BurstUpdate(sEnemyShotSet* pEnemyShotSet, double burstSpeed)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 発射フェーズで静止弾に速度を与える
        if (g_burst && pShot->speed == 0.0) {
            pShot->speed = burstSpeed;
        }
        // 速度があれば移動
        if (pShot->speed > 0.0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ---- 一段階目：高密度の内円（静止） ----
static void ShotStep1(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int num = 36;
        for (int i = 0; i < num; i++) {
            double angle = i * 2.0 * DX_PI / num;
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + 45.0 * cos(angle),
                pEnemyShotSet->y + 45.0 * sin(angle),
                angle, 0.0,
                img_enemyShotMediumBall[0]);  // 赤い小玉
        }
    }
    BurstUpdate(pEnemyShotSet, 3.5);
}

// ---- 二段階目：隙間のある外円（静止） ----
static void ShotStep2(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 32発のうち4箇所（各2発分）を隙間としてスキップ
        // 隙間は 0°, 90°, 180°, 270° 付近にできる
        const int num = 32;
        double base_angle = GetRand(360) / 360.0 * DX_PI * 2.0;
        for (int i = 0; i < num; i++) {
            if (i % 8 < 2-1) continue;  // 隙間

            double angle = base_angle + i * 2.0 * DX_PI / num;
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + 90.0 * cos(angle),
                pEnemyShotSet->y + 90.0 * sin(angle),
                angle, 0.0,
                img_enemyShotLargeBall[4]);  // 青い中玉
        }
    }
    BurstUpdate(pEnemyShotSet, 3.5);
}

// ---- 三段階目：外側を埋める ----
static void ShotStep3(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 複数の同心円で外側を埋める
        const double radii[] = { 120.0, 150.0, 180.0, 210.0, 240.0, 270.0, 300.0 };
        const int    counts[] = { 36,    44,    52,    60,    68,    76,    84 };
        const int    rings = 7;

        for (int r = 0; r < rings; r++) {
            double base_angle = GetRand(360) / 360.0 * DX_PI * 2.0;
            for (int i = 0; i < counts[r]*3/2; i++) {
                double angle = base_angle + i * 2.0 * DX_PI / counts[r]/3*2;
                AddShot(pEnemyShotSet,
                    pEnemyShotSet->x + radii[r] * cos(angle),
                    pEnemyShotSet->y + radii[r] * sin(angle),
                    angle, 0.0,
                    img_enemyShotScale[5]);  // マゼンタの鱗弾
            }
        }
    }
    BurstUpdate(pEnemyShotSet, 4.0);
}

// ---- ショットセットを生成するヘルパー ----
static void CreateShotSet(sEnemyShotSet::PatternFunc func, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_SanpoHissatsu_DeepSeek()
{
    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        g_burst = false;
    }

    // 300フレーム周期で「三歩必殺」を繰り返す
    int cycle = count % 300;

    // 予告音
    if (cycle == 20) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 一段階目：高密度の内円
    if (cycle == 30) {
        CreateShotSet(ShotStep1, 1);
    }

    // 二段階目：隙間のある外円
    if (cycle == 70) {
        CreateShotSet(ShotStep2, 2);
    }

    // 三段階目：外側を埋める
    if (cycle == 110) {
        CreateShotSet(ShotStep3, 3);
    }

    // 全弾発射！
    if (cycle == 150) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        g_burst = true;
    }

    // リセット（次の周期へ）
    if (cycle == 0 && count > 1) {
        g_burst = false;
    }
}