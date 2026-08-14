// enemyPat_EternalForceBlizzard.cpp
//
// 絶対氷結宣言 〜エターナルフォースブリザード〜
//
// 「大仰な詠唱 → 一瞬で大気ごと氷結、相手は死ぬ」という元ネタの落差を、
// 弾幕の密度差・時間差として表現する4フェーズパターン。
//
//   Phase1: 詠唱          … 六方対称の魔法陣が中心からリングごとに時間差で展開。実弾は疎ら。
//   Phase2: 見せかけの本命 … 標準的な吹雪(斜め降雪+自機狙い氷柱3way)で油断を誘う。
//   Phase3: 一瞬の絶対    … 詠唱完成の瞬間、画面のほぼ全域が同時に凍結の壁と化す。
//   Phase4: 静寂          … 凍結が砕け散った直後、締めはただの3wayが一発だけ。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {
    constexpr int COL_RED = 0, COL_YELLOW = 1, COL_GREEN = 2, COL_CYAN = 3,
        COL_BLUE = 4, COL_MAGENTA = 5, COL_WHITE = 6, COL_BLACK = 7, COL_ORANGE = 8;

    constexpr double FIELD_CX = 240.0;
    constexpr double FIELD_CY = 240.0;

    // ---- タイムライン(全フェーズ共通の基準点) ----
    constexpr int PHASE1_END = 180; // 詠唱終了
    constexpr int RING_INTERVAL = 45;
    constexpr int RING_COUNT = 4;
    constexpr int ICICLE_START = 20;
    constexpr int ICICLE_INTERVAL = 35;

    constexpr int PHASE2_START = PHASE1_END + 1;    // 181
    constexpr int PHASE2_END = PHASE2_START + 89; // 270
    constexpr int SNOW_INTERVAL = 12;
    constexpr int OVAL3WAY_INTERVAL = 30;

    constexpr int WALL_TRIGGER = PHASE2_END + 1;              // 271 「一瞬で」凍結する瞬間
    constexpr int WALL_HOLD = 40;                          // 静止(凍結)しているフレーム数
    constexpr int FINALE_SHOT = WALL_TRIGGER + WALL_HOLD + 20; // 331 締めの一撃
    constexpr int T = FINALE_SHOT + 60;
}

// ============================================================
// Phase1: 魔法陣のリング(中心から半径0で出現し、イーズアウトで外側へ展開しつつ緩やかに自転)
// ============================================================
static void ShotMagicCircleRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double targetRadius = pEnemyShotSet->param_d[0];
        int    n = pEnemyShotSet->param_i[0];
        int    color = pEnemyShotSet->param_i[1];
        double rotSpeed = pEnemyShotSet->param_d[1];

        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < n; i++) {
            double baseAngle = (double)i / n * 2.0 * DX_PI;

            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 弾の種類一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
            // 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            pEnemyShot->x = FIELD_CX;
            pEnemyShot->y = FIELD_CY;
            pEnemyShot->kind = img_enemyShotMediumBall[color];
            pEnemyShot->param_d[0] = baseAngle;
            pEnemyShot->param_d[1] = targetRadius;
            pEnemyShot->param_d[2] = rotSpeed;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    constexpr int GROW_FRAMES = 50;
    constexpr int SHATTER_FROM = WALL_TRIGGER + WALL_HOLD; // 壁と同時刻に砕け散る

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double baseAngle = pShot->param_d[0];
        double targetRadius = pShot->param_d[1];
        double rotSpeed = pShot->param_d[2];

        double t = (pShot->count < GROW_FRAMES) ? (double)pShot->count / GROW_FRAMES : 1.0;
        double eased = 1.0 - pow(1.0 - t, 3.0); // 三次イーズアウト(中心からふわっと展開)
        double radius = targetRadius * eased;
        double angle = baseAngle + rotSpeed * pShot->count;

        if (count % T >= SHATTER_FROM) {
            // Phase3の凍結壁が砕け散る瞬間、魔法陣ごと一斉に外側へ吹き飛ぶ
            double st = (double)(count % T - SHATTER_FROM);
            radius += 0.05 * st * st + 1.0 * st;
        }

        pShot->x = FIELD_CX + radius * cos(angle);
        pShot->y = FIELD_CY + radius * sin(angle);

        pShot = pShot->next;
    }
}

// ============================================================
// Phase1: 詠唱中の疎らな牽制(まだ本気を出していない、という体で単発の自機狙い氷柱)
// ============================================================
static void ShotAimedIcicle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->kind = img_enemyShotDiamond[COL_CYAN];
        pEnemyShot->param_d[0] = pEnemyShotSet->x;    // 発射時x
        pEnemyShot->param_d[1] = pEnemyShotSet->y;    // 発射時y
        pEnemyShot->param_d[2] = pEnemyShotSet->muki; // 自機狙い角度(発射時に固定)
        pEnemyShot->param_d[3] = 2.4;                 // 速さ

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double sx = pShot->param_d[0], sy = pShot->param_d[1];
        double muki = pShot->param_d[2], spd = pShot->param_d[3];
        pShot->x = sx + spd * cos(muki) * pShot->count;
        pShot->y = sy + spd * sin(muki) * pShot->count;
        pShot = pShot->next;
    }
}

static void SpawnAimedIcicle(double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotAimedIcicle;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = atan2(player.y - y, player.x - x);

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
// Phase2: 見せかけの本命(1) — 斜めに降る標準的な雪片弾
// ============================================================
static void ShotSnowfall(sEnemyShotSet* pEnemyShotSet)
{
    constexpr int N = 6;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < N; i++) {
            // GetRand(x) は 0 から x までの x+1 種類の整数をランダムに返す関数なので注意！
            double sx = GetRand(480);
            double sy = -10.0;
            double angle = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI; // ほぼ真下、若干のばらつき
            double spd = 1.6 + GetRand(80) / 100.0;

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = sx;
            pEnemyShot->y = sy;
            pEnemyShot->kind = img_enemyShotSmallBall[COL_WHITE];
            pEnemyShot->param_d[0] = sx;
            pEnemyShot->param_d[1] = sy;
            pEnemyShot->param_d[2] = angle;
            pEnemyShot->param_d[3] = spd;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double sx = pShot->param_d[0], sy = pShot->param_d[1];
        double angle = pShot->param_d[2], spd = pShot->param_d[3];
        pShot->x = sx + spd * cos(angle) * pShot->count;
        pShot->y = sy + spd * sin(angle) * pShot->count;
        pShot = pShot->next;
    }
}

static void SpawnSnowRow()
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotSnowfall;
    pEnemyShotSet->x = 0.0;
    pEnemyShotSet->y = 0.0;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
// Phase2: 見せかけの本命(2) / Phase4: 締めの一撃 — 自機狙い氷柱3way(共用)
// ============================================================
static void ShotAimedOval3Way(sEnemyShotSet* pEnemyShotSet)
{
    constexpr double SPREAD = 18.0 * DX_PI / 180.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = -1; i <= 1; i++) {
            double muki = pEnemyShotSet->muki + SPREAD * i;

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->kind = img_enemyShotMediumOval[COL_BLUE];
            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = muki;
            pEnemyShot->param_d[3] = 2.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double sx = pShot->param_d[0], sy = pShot->param_d[1];
        double muki = pShot->param_d[2], spd = pShot->param_d[3];
        pShot->x = sx + spd * cos(muki) * pShot->count;
        pShot->y = sy + spd * sin(muki) * pShot->count;
        pShot->muki = muki;
        pShot = pShot->next;
    }
}

static void SpawnAimedOval3Way(double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotAimedOval3Way;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = atan2(player.y - y, player.x - x);

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
// Phase3: 一瞬の絶対 — 画面のほぼ全域を同心シェル状に埋め尽くす凍結の壁。
//         1本だけの安全回廊を除いて即着(予告なし)、静止ののち外側へ砕け散る。
// ============================================================
static void ShotFreezeWall(sEnemyShotSet* pEnemyShotSet)
{
    constexpr int    SHELL_COUNT = 8;
    constexpr double GAP_HALF_WIDTH = 13.0 * DX_PI / 180.0; // 安全回廊の半角(合計約26度)

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // リプレイ再現性のあるGetRandで安全回廊の角度を決定
        double gapAngle = GetRand(359) / 180.0 * DX_PI;

        for (int shell = 0; shell < SHELL_COUNT; shell++) {
            double radius = 30.0 + shell * 40.0; // 30, 70, 110, ... 310 (画面端まで届く)
            int    n = 16 + shell * 6;      // 外側ほど弾数を増やして密度を維持
            bool   altShape = (shell % 2 == 1);

            for (int i = 0; i < n; i++) {
                double angle = (double)i / n * 2.0 * DX_PI;
                double diff = angle - gapAngle;
                while (diff > DX_PI)  diff -= 2.0 * DX_PI;
                while (diff < -DX_PI) diff += 2.0 * DX_PI;
                if (fabs(diff) < GAP_HALF_WIDTH) continue; // 安全回廊には配置しない

                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = FIELD_CX + radius * cos(angle);
                pEnemyShot->y = FIELD_CY + radius * sin(angle);
                pEnemyShot->kind = altShape
                    ? img_enemyShotSmallBall[COL_WHITE]
                    : img_enemyShotScale[COL_CYAN];
                pEnemyShot->param_d[0] = angle;
                pEnemyShot->param_d[1] = radius;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pShot->param_d[0];
        double radius = pShot->param_d[1];

        if (pShot->count > WALL_HOLD) {
            // 静止(凍結)から一転、外側へ加速しながら砕け散る
            double st = (double)(pShot->count - WALL_HOLD);
            radius += 0.05 * st * st + 1.0 * st;
        }

        pShot->x = FIELD_CX + radius * cos(angle);
        pShot->y = FIELD_CY + radius * sin(angle);
        pShot->muki = angle;

        pShot = pShot->next;
    }
}

static void SpawnFreezeWall()
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotFreezeWall;
    pEnemyShotSet->x = FIELD_CX;
    pEnemyShotSet->y = FIELD_CY;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_EternalForceBlizzard_Claude()
{
    if (count == 1) {
        // ゲーム画面は480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    
    if (count % T <= PHASE1_END) {
        // 詠唱に集中し、微動だにしない
    }
    else if (count % T <= PHASE2_END) {
        // 見せかけの本命フェーズだけ緩やかに左右へ揺れる
        enemy.x = 240.0 + 40.0 * sin((count % T - PHASE2_START) * 0.02);
    }
    else {
        // 一瞬の絶対を放つ瞬間、再び静止する
        enemy.x = 240.0;
    }

    // ---- Phase1: 詠唱。六方対称の魔法陣を、リングごとに時間差をつけて中心から展開する ----
    if (count % T >= 1 && count % T <= 1 + RING_INTERVAL * (RING_COUNT - 1) && (count % T - 1) % RING_INTERVAL == 0) {
        static const double radii[RING_COUNT] = { 40.0, 90.0, 140.0, 190.0 };
        static const int    nums[RING_COUNT] = { 12, 18, 24, 30 };
        static const int    colors[RING_COUNT] = { COL_CYAN, COL_BLUE, COL_WHITE, COL_CYAN };

        int ringIndex = (count % T - 1) / RING_INTERVAL;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMagicCircleRing;
        pEnemyShotSet->x = FIELD_CX;
        pEnemyShotSet->y = FIELD_CY;
        pEnemyShotSet->param_d[0] = radii[ringIndex];
        pEnemyShotSet->param_i[0] = nums[ringIndex];
        pEnemyShotSet->param_i[1] = colors[ringIndex];
        pEnemyShotSet->param_d[1] = (ringIndex % 2 == 0) ? 0.0025 : -0.0030;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 詠唱中の牽制、まだ本気を出していないという体で疎らに撃つ
    if (count % T >= ICICLE_START && count % T <= PHASE1_END && (count % T - ICICLE_START) % ICICLE_INTERVAL == 0) {
        SpawnAimedIcicle(enemy.x, enemy.y + 10.0);
    }

    // ---- Phase2: 見せかけの本命。標準的な吹雪だけを見せて油断を誘う ----
    if (count % T >= PHASE2_START && count % T <= PHASE2_END && (count % T - PHASE2_START) % SNOW_INTERVAL == 0) {
        SpawnSnowRow();
    }
    if (count % T >= PHASE2_START && count % T <= PHASE2_END && (count % T - PHASE2_START) % OVAL3WAY_INTERVAL == 0) {
        SpawnAimedOval3Way(enemy.x, enemy.y + 10.0);
    }

    // ---- Phase3: 一瞬の絶対。詠唱完成の瞬間、画面のほぼ全域が同時に凍結する ----
    if (count % T == WALL_TRIGGER) {
        SpawnFreezeWall();
    }

    // ---- Phase4: 「相手は死ぬ」の後の静寂。凍結が砕け散った直後、締めはただの一撃だけ ----
    if (count % T == FINALE_SHOT) {
        SpawnAimedOval3Way(FIELD_CX, FIELD_CY);
    }
}