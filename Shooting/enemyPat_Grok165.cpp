#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// 画面中心
static const double CENTER_X = 240.0;
static const double CENTER_Y = 240.0;
static const double SCREEN_RADIUS = 240.0 * 1.41421356237; // 外接円半径 ≈ 339.41

// 引力の強さをボス残り体力から計算（HP減少で増加）
static double GetPullSpeed()
{
    double ratio = 1.0 - (double)enemy.hp / (double)enemy.maxHp;
    if (ratio < 0.0) ratio = 0.0;
    if (ratio > 1.0) ratio = 1.0;
    // 基本 0.35 ～ 最大 1.8 程度
    return 0.35 + 1.45 * ratio;
}

// 核の半径をボス残り体力から計算（HP減少で増加）
static double GetCoreRadius()
{
    double ratio = 1.0 - (double)enemy.hp / (double)enemy.maxHp;
    if (ratio < 0.0) ratio = 0.0;
    if (ratio > 1.0) ratio = 1.0;
    // 基本 12 ～ 最大 70 程度
    return 12.0 + 58.0 * ratio;
}

// ------------------------------------------------------------
// A: 外接円上に発生し、引力で中心へ一定速度で引き寄せられる赤中玉
// ------------------------------------------------------------
static void ShotGravityA(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int NUM = 48; // 狭い間隔で多数
    const double R = SCREEN_RADIUS + 10.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < NUM; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = pEnemyShotSet->muki + (double)i * (2.0 * DX_PI / (double)NUM);
            pEnemyShot->x = CENTER_X + R * cos(ang);
            pEnemyShot->y = CENTER_Y + R * sin(ang);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->margin = 240.0;
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤中玉
            pEnemyShot->count = 0;
            // リンク
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：中心へ一定速度で移動、到達で消滅
    double pull = GetPullSpeed();
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double dx = CENTER_X - pShot->x;
        double dy = CENTER_Y - pShot->y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < 8.0) {
            // 中心到達で消去
            sEnemyShot* toDel = pShot;
            pShot = pShot->next;
            toDel->prev->next = toDel->next;
            toDel->next->prev = toDel->prev;
            delete toDel;
            continue;
        }

        // 加速せず、常に引力の大きさと同じ速度で中心方向へ
        double ang = atan2(dy, dx);
        pShot->muki = ang;
        pShot->speed = pull;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// B: 中心から前後に撃つ、引力非影響の赤小玉（方向は時間で回転）
// ------------------------------------------------------------
static void ShotRadialB(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 前後2発
        for (int k = 0; k < 2; k++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = CENTER_X;
            pEnemyShot->y = CENTER_Y;
            // 基準角度 + 微小乱数
            double randAng = (GetRand(16) - 8) / 180.0 * DX_PI; // 約 ±4.5°
            pEnemyShot->muki = pEnemyShotSet->muki + (k == 0 ? 0.0 : DX_PI) + randAng;
            pEnemyShot->speed = 2.4; // 一定速度
            pEnemyShot->margin = 20.0;
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤小玉
            pEnemyShot->count = 0;
            // リンク
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 通常移動（引力非影響）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// C: 中心に密集する静止した赤大玉（核）。HP減少で半径拡大
// ------------------------------------------------------------
static void ShotCoreC(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int NUM = 16;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < NUM; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = (double)i * (2.0 * DX_PI / (double)NUM);
            pEnemyShot->param_d[0] = ang; // 固定角度を記憶
            pEnemyShot->x = CENTER_X;
            pEnemyShot->y = CENTER_Y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->margin = 20.0;
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤大玉
            pEnemyShot->count = 0;
            // リンク
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：現在の核半径に合わせて位置を更新（静止に見えるが拡大に追従）
    double r = GetCoreRadius();
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double ang = pShot->param_d[0];
        pShot->x = CENTER_X + r * cos(ang);
        pShot->y = CENTER_Y + r * sin(ang);
        pShot->speed = 0.0;
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_SubterraneanSun_Grok()
{
    static double aBaseAngle = 0.0; // A発生基準角度
    static double bBaseAngle = 0.0; // B射撃方向
    static int coreCreated = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = CENTER_X;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 60*20; // 200から変更しないこと
        aBaseAngle = 0.0;
        bBaseAngle = 0.0;
        coreCreated = 0;
    }
    enemy.hp--;

    // ---- 自機への引力適用 ----
    {
        double pullSpeed = GetPullSpeed();
        double dx = CENTER_X - player.x;
        double dy = CENTER_Y - player.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist > 0.5) {
            double angle = atan2(dy, dx);
            // 位置更新
            player.x += pullSpeed * cos(angle);
            player.y += pullSpeed * sin(angle);
            // 力の可視化パーティクル
            const double ratio = 3.0;
            spawnForceParticles(player.x, player.y,
                pullSpeed * cos(angle) * ratio,
                pullSpeed * sin(angle) * ratio);
        }
    }

    // ---- C: 核を1回だけ生成 ----
    if (coreCreated == 0 && count >= 2) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCoreC;
        pEnemyShotSet->x = CENTER_X;
        pEnemyShotSet->y = CENTER_Y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
        coreCreated = 1;
    }

    // ---- A: 中頻度で外接円上に赤中玉リング発生 ----
    // 発生基準角度を時間経過で変化
    aBaseAngle += 0.018;
    if (count % 14 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGravityA;
        pEnemyShotSet->x = CENTER_X;
        pEnemyShotSet->y = CENTER_Y;
        pEnemyShotSet->muki = aBaseAngle; // 基準角度
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ---- B: 高頻度で中心から前後に赤小玉 ----
    bBaseAngle += 0.065;
    if (count % 6 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRadialB;
        pEnemyShotSet->x = CENTER_X;
        pEnemyShotSet->y = CENTER_Y;
        pEnemyShotSet->muki = bBaseAngle;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}