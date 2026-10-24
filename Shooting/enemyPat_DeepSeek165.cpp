#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
// 共通定数
// ============================================================
static const double SCREEN_CX = 240.0;
static const double SCREEN_CY = 240.0;

// ============================================================
// 体力割合
// ============================================================
static double GetHpRatio()
{
    double ratio = 1.0;
    if (enemy.maxHp > 0) {
        ratio = (double)enemy.hp / (double)enemy.maxHp;
    }
    if (ratio < 0.0) ratio = 0.0;
    if (ratio > 1.0) ratio = 1.0;
    return ratio;
}

// ============================================================
// A: 引力の強さ
// 残り体力が減るほど強くなる
// ============================================================
static double GetPullSpeed()
{
    double hpRatio = GetHpRatio();
    // 初期 0.8、体力0で 2.4 程度
    return 0.8 + 1.6 * (1.0 - hpRatio);
}

// ============================================================
// 弾追加用ヘルパ
// ============================================================
static sEnemyShot* AddEnemyShot(
    sEnemyShotSet* set,
    double x,
    double y,
    double muki,
    double speed,
    int kind,
    double margin)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->margin = margin;
    p->count = 0;

    for (int i = 0; i < 16; i++) {
        p->param_i[i] = 0;
        p->param_d[i] = 0.0;
    }

    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;

    return p;
}

// ============================================================
// 弾幕セット追加用ヘルパ
// ============================================================
static sEnemyShotSet* CreateEnemyShotSet(
    double x,
    double y,
    double muki,
    int kind,
    sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* set = new sEnemyShotSet;

    set->x = x;
    set->y = y;
    set->muki = muki;
    set->patternFunc = func;
    set->count = 0;
    set->kind = kind;

    for (int i = 0; i < 16; i++) {
        set->param_i[i] = 0;
        set->param_d[i] = 0.0;
    }

    set->pEnemyShotHead = new sEnemyShot;
    set->pEnemyShotHead->prev = set->pEnemyShotHead;
    set->pEnemyShotHead->next = set->pEnemyShotHead;

    set->prev = enemyShotSetHead.prev;
    set->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = set;
    enemyShotSetHead.prev = set;

    return set;
}

// ============================================================
// A: 画面外接円上から赤中玉を発生させ、中央へ引き寄せる
// ============================================================
static void ShotGravityA(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) {
            StopSoundMem(sound_enemyShot_medium);
        }
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 480x480 の外接円半径
        const double R = sqrt(240.0 * 240.0 + 240.0 * 240.0);
        const int NUM = 16;
        const double SPACING = 0.035; // 狭い間隔

        double baseAngle = pEnemyShotSet->param_d[0];

        for (int i = 0; i < NUM; i++) {
            double ang = baseAngle + i * SPACING;
            double x = SCREEN_CX + R * cos(ang);
            double y = SCREEN_CY + R * sin(ang);

            // 赤中玉、速度0、margin 240
            AddEnemyShot(
                pEnemyShotSet,
                x,
                y,
                ang + DX_PI, // 中央向き
                0.0,
                img_enemyShotMediumBall[0],
                240.0);
        }
    }

    double pull = GetPullSpeed();

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;

        double dx = SCREEN_CX - p->x;
        double dy = SCREEN_CY - p->y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist <= pull + 1e-9) {
            // 中央に達したら消滅
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
        }
        else {
            // 加速なし、引力の大きさと同じ速度で中央へ
            p->x += dx / dist * pull;
            p->y += dy / dist * pull;
        }

        p = next;
    }
}

// ============================================================
// B: 中央から赤小玉を前後に撃つ
// 引力の影響を受けない
// ============================================================
static void ShotRotatingFrontBackB(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double base = pEnemyShotSet->muki;

        // 微小な角度乱数 -2～+2 度
        double r1 = (GetRand(4) - 2) / 180.0 * DX_PI;
        double r2 = (GetRand(4) - 2) / 180.0 * DX_PI;

        const double SPEED = 3.0;

        // 前
        AddEnemyShot(
            pEnemyShotSet,
            SCREEN_CX,
            SCREEN_CY,
            base + r1,
            SPEED,
            img_enemyShotSmallBall[0],
            20.0);

        // 後ろ
        AddEnemyShot(
            pEnemyShotSet,
            SCREEN_CX,
            SCREEN_CY,
            base + DX_PI + r2,
            SPEED,
            img_enemyShotSmallBall[0],
            20.0);
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
// C: 中央に静止した赤大玉を密集させて核を表現
// 残り体力が減るほど核を大きくする
// ============================================================
static void ShotCoreC(sEnemyShotSet* pEnemyShotSet)
{
    double hpRatio = GetHpRatio();
    double grow = 1.0 - hpRatio;

    int desiredCount = 8 + (int)(grow * 24.0);
    if (desiredCount < 8) desiredCount = 8;
    if (desiredCount > 40) desiredCount = 40;

    double coreRadius = 10.0 + grow * 30.0;

    int spawned = pEnemyShotSet->param_i[0];

    if (spawned == 0 && desiredCount > 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) {
            StopSoundMem(sound_enemyShot_heavy);
        }
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    if (spawned < desiredCount) {
        for (int i = spawned; i < desiredCount; i++) {
            double ang = GetRand(359) / 180.0 * DX_PI;
            double r = coreRadius * sqrt((double)GetRand(1000) / 1000.0);

            double x = SCREEN_CX + r * cos(ang);
            double y = SCREEN_CY + r * sin(ang);

            // 赤大玉、静止
            AddEnemyShot(
                pEnemyShotSet,
                x,
                y,
                0.0,
                0.0,
                img_enemyShotLargeBall[0],
                20.0);
        }

        pEnemyShotSet->param_i[0] = desiredCount;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_SubterraneanSun_DeepSeek()
{
    if (count == 1) {
        enemy.x = SCREEN_CX;
        enemy.y = SCREEN_CY;
        enemy.maxHp = enemy.hp = 200; // 200から変更しない

        // C: 核は1つのセットで管理し、体力に応じて追加していく
        CreateEnemyShotSet(SCREEN_CX, SCREEN_CY, 0.0, 0, ShotCoreC);
    }

    // --------------------------------------------------------
    // A: 常に画面中央への引力を自機に発生させる
    // --------------------------------------------------------
    double dx = SCREEN_CX - player.x;
    double dy = SCREEN_CY - player.y;
    double dist = sqrt(dx * dx + dy * dy);

    if (dist > 1e-9) {
        double angle = atan2(dy, dx);
        double pullSpeed = GetPullSpeed();
        double ratio = 3.0;

        spawnForceParticles(
            player.x,
            player.y,
            pullSpeed * cos(angle) * ratio,
            pullSpeed * sin(angle) * ratio);

        if (dist <= pullSpeed) {
            player.x = SCREEN_CX;
            player.y = SCREEN_CY;
        }
        else {
            player.x += pullSpeed * cos(angle);
            player.y += pullSpeed * sin(angle);
        }
    }

    // --------------------------------------------------------
    // A: 中頻度で外接円上に赤中玉を発生
    // --------------------------------------------------------
    if (count >= 2 && count % 24 == 1) {
        sEnemyShotSet* set = CreateEnemyShotSet(
            SCREEN_CX,
            SCREEN_CY,
            0.0,
            0,
            ShotGravityA);

        // 発生基準角度を時間経過で変化させる
        set->param_d[0] = count * 0.02;
    }

    // --------------------------------------------------------
    // B: 高頻度で中央から赤小玉を前後に撃つ
    // --------------------------------------------------------
    if (count >= 2 && count % 6 == 1) {
        CreateEnemyShotSet(
            SCREEN_CX,
            SCREEN_CY,
            count * 0.02, // 撃つ方向を時間経過で回転
            0,
            ShotRotatingFrontBackB);
    }
}