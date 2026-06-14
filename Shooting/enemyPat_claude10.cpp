// enemyPat_dragon.cpp
//
// ========================================================================
//  竜をモチーフにした弾幕パターン
//
//  ■ 敵の動き
//    画面上部を大きくうねる正弦波軌道 ─ 天空を舞う竜の飛翔を表現
//
//  ■ 3 種類の弾幕（攻撃の波状パターン）
//    [25f 周期] 竜炎（りゅうえん）
//                プレイヤーへ向かう扇状の炎ブレス
//                赤・黄の大玉と鱗弾を交互に並べ「炎の舌」を演出
//
//    [80f 周期] 竜鱗渦（りゅうりんうず）
//                5 腕螺旋の鱗弾が時計回りに旋回しながら広がる
//                青・シアン・マゼンタの霊的配色。毎発射ごとに基準角を
//                36 度回転させ、長期で見ると花びら状の軌跡を描く
//
//    [40f 周期] 竜爪撃（りゅうそうげき）
//                プレイヤーへ向かう 3 連高速銃弾
//                竜の爪を模した鋭い間隔（約 13 度）で放つ
// ========================================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ----------------------------------------------------------------
// ヘルパー：弾をリストの末尾に挿入
// ----------------------------------------------------------------
static inline void InsertShot(sEnemyShotSet* pss, sEnemyShot* ps)
{
    ps->prev = pss->pEnemyShotHead->prev;
    ps->next = pss->pEnemyShotHead;
    pss->pEnemyShotHead->prev->next = ps;
    pss->pEnemyShotHead->prev = ps;
}

// ----------------------------------------------------------------
// 竜炎（りゅうえん）
//   プレイヤー方向を中心に 30 度の扇で 13 発を同時発射。
//   炎の芯（中央）は黄・大玉・高速、縁は赤・鱗弾・低速。
// ----------------------------------------------------------------
static void ShotDragonFlame(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int    N = 13;
        const double SPREAD = DX_PI / 6.0; // 扇の開角 30 度

        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double t = (double)i / (N - 1);           // 0.0（左端） 〜 1.0（右端）
            pEnemyShot->muki = pEnemyShotSet->muki - SPREAD / 2.0 + SPREAD * t;

            // 炎の芯は速く、縁は遅め
            double dist = fabs(t - 0.5) * 2.0;        // 0.0（中心） 〜 1.0（端）
            pEnemyShot->speed = 3.8 - dist * 1.5;

            // 中央 45% 圏内は黄（芯）、それ以外は赤（縁）
            // 奇数 index が大玉、偶数 index が鱗弾
            int color = (dist < 0.45) ? 1 : 0;        // 1:黄、0:赤
            pEnemyShot->kind = (i % 2 == 0)
                ? img_enemyShotLargeBall[color]
                : img_enemyShotScale[color];

            InsertShot(pEnemyShotSet, pEnemyShot);
        }
    }

    // 全弾を直線移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ----------------------------------------------------------------
// 竜鱗渦（りゅうりんうず）
//   5 本腕 × 4 発 = 計 20 発の鱗弾螺旋。
//   各弾が毎フレーム muki を +0.020 rad ずつ変化させ、
//   放物線を描きながら外へ広がる（とぐろを巻く竜の胴体を表現）。
//   pEnemyShotSet->muki は呼び出し側が毎回回転させた基準角。
// ----------------------------------------------------------------
static void ShotDragonSpiralScale(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int ARMS = 5;
        const int PER_ARM = 4;
        const int palette[4] = { 4, 3, 5, 6 }; // 青、シアン、マゼンタ、白

        for (int arm = 0; arm < ARMS; arm++) {
            for (int j = 0; j < PER_ARM; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                // 5 腕を等間隔に配置し、各腕の内側から 20 度ずつずらす
                double baseAngle = (2.0 * DX_PI / ARMS) * arm;
                double armOffset = (DX_PI / 9.0) * j;   // 20 度 × j
                pEnemyShot->muki = pEnemyShotSet->muki + baseAngle + armOffset;

                // 内→外で加速（螺旋が外側ほど大きく開く）
                pEnemyShot->speed = 1.0 + j * 0.55;

                pEnemyShot->kind = img_enemyShotScale[palette[j % 4]];

                InsertShot(pEnemyShotSet, pEnemyShot);
            }
        }
    }

    // 全弾を時計回りに緩やかに曲げながら移動（旋回する竜体）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->muki += 0.020;
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ----------------------------------------------------------------
// 竜爪撃（りゅうそうげき）
//   プレイヤーへ向けた 3 本の高速銃弾を一瞬で放つ。
//   約 13 度の鋭い角度差で並べることで「3 本爪の切り傷」を演出。
// ----------------------------------------------------------------
static void ShotDragonClaw(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int    CLAW_NUM = 3;
        const double GAP = DX_PI / 14.0; // 約 12.9 度間隔

        for (int i = 0; i < CLAW_NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + GAP * (i - 1); // -1, 0, +1
            pEnemyShot->speed = 5.2;
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤い銃弾

            InsertShot(pEnemyShotSet, pEnemyShot);
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ================================================================
// 敵本体：竜（EnemyPat_Dragon_Claude）
// ================================================================
void EnemyPat_Dragon_Claude()
{
    // 螺旋弾の基準角（毎発射で 36 度ずつ回転させる）
    static double spiralRot;

    // ── 初期化 ──────────────────────────────────────────────────
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        spiralRot = 0.0;
    }
    else {
        // 横：160 px 振れ幅の正弦波（周期 320 f）
        // 縦：18 px 振れ幅の正弦波（周期 180 f）で上下にゆらゆら
        enemy.x = 240.0 + 160.0 * sin(count * DX_PI / 160.0);
        enemy.y = 55.0 + 18.0 * sin(count * DX_PI / 90.0);
    }

    // ── 竜炎：25 フレームごと ──────────────────────────────────
    if (count % 35 == 5) {
        sEnemyShotSet* pss = new sEnemyShotSet;
        pss->count = 0;
        pss->patternFunc = ShotDragonFlame;
        pss->x = enemy.x;
        pss->y = enemy.y + 12.0;
        pss->muki = atan2(player.y - pss->y, player.x - pss->x);

        pss->pEnemyShotHead = new sEnemyShot;
        pss->pEnemyShotHead->prev = pss->pEnemyShotHead;
        pss->pEnemyShotHead->next = pss->pEnemyShotHead;

        pss->prev = enemyShotSetHead.prev;
        pss->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pss;
        enemyShotSetHead.prev = pss;
    }

    // ── 竜鱗渦：80 フレームごと（基準角を 36 度ずつ回転）────
    if (count % 80 == 0 && count > 1) {
        spiralRot += DX_PI / 5.0; // 36 度 ─ 10 回転で基準角が一周

        sEnemyShotSet* pss = new sEnemyShotSet;
        pss->count = 0;
        pss->patternFunc = ShotDragonSpiralScale;
        pss->x = enemy.x;
        pss->y = enemy.y;
        pss->muki = spiralRot;

        pss->pEnemyShotHead = new sEnemyShot;
        pss->pEnemyShotHead->prev = pss->pEnemyShotHead;
        pss->pEnemyShotHead->next = pss->pEnemyShotHead;

        pss->prev = enemyShotSetHead.prev;
        pss->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pss;
        enemyShotSetHead.prev = pss;
    }

    // ── 竜爪撃：40 フレームごと（竜炎と交互になるよう offset 20）
    if (count % 40 == 200) {
        sEnemyShotSet* pss = new sEnemyShotSet;
        pss->count = 0;
        pss->patternFunc = ShotDragonClaw;
        pss->x = enemy.x;
        pss->y = enemy.y + 12.0;
        pss->muki = atan2(player.y - pss->y, player.x - pss->x);

        pss->pEnemyShotHead = new sEnemyShot;
        pss->pEnemyShotHead->prev = pss->pEnemyShotHead;
        pss->pEnemyShotHead->next = pss->pEnemyShotHead;

        pss->prev = enemyShotSetHead.prev;
        pss->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pss;
        enemyShotSetHead.prev = pss;
    }
}