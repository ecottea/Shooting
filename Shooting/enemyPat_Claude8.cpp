// enemyPat_tmp.cpp
// 雷をモチーフにした弾幕パターン「雷霆ノ裁キ」
//
// ┌──────────┬───────────────────────────────────────────────────────────────┐
// │ フェーズ │ 内容                                                          │
// ├──────────┼───────────────────────────────────────────────────────────────┤
// │ 帯電     │ count   1~240  60Fごとに ShotLightningBolt                    │
// │ 放電     │ count 241~480  45Fごとに ShotThunderRing                      │
// │          │               90F周期の中間(45F目)に ShotLightningBolt         │
// │ 雷鳴     │ count 481~    30Fごとに ShotLightningBolt                     │
// │          │               60Fごとに ShotThunderRing                       │
// │          │               120F周期の中間(60F目)に ShotStaticField + 瞬移  │
// └──────────┴───────────────────────────────────────────────────────────────┘

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ヘルパー: sEnemyShotSet を生成してリストの末尾に追加する
// ============================================================
static void FireShotSet(double x, double y,
    sEnemyShotSet::PatternFunc func, double muki)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ============================================================
// パターンA: ShotLightningBolt — 雷撃弾
//
// 中心: プレイヤー狙いの高速銃弾 7発 (±7° 内に集中)
// 分岐: 左(シアン) 右(青) 各4発、20°刻みで広がり、外ほど遅い
// → 雷が枝分かれする形状を再現
// ============================================================
static void ShotLightningBolt(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // ── 中心弾: 高速銃弾 7発 ──────────────────────────────
        static const int boltColors[] = { 3, 4, 6 }; // シアン, 青, 白
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(10) - 5);
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(14) - 7) / 180.0 * DX_PI;
            pEnemyShot->speed = 4.0 + GetRand(300) / 100.0; // 4.0 ~ 7.0
            pEnemyShot->kind = img_enemyShotBullet[boltColors[GetRand(2)]];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ── 分岐弾: 左右各4発 ─────────────────────────────────
        for (int side = -1; side <= 1; side += 2) {
            int color = (side == -1) ? 3 : 4; // 左:シアン / 右:青
            for (int i = 1; i <= 4; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = pEnemyShotSet->muki + side * i * 20.0 / 180.0 * DX_PI;
                pEnemyShot->speed = 4.0 - i * 0.4; // 3.6, 3.2, 2.8, 2.4
                pEnemyShot->kind = img_enemyShotSmallBall[color];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// パターンB: ShotThunderRing — 放電輪弾
//
// 16方向への同時放電。muki に累積オフセットを渡すことで
// 毎回リングが少しずつ回転し、螺旋状の弾幕を形成する
// 色: 黄 / シアン / 白 を i%3 で交互に割り当て
// ============================================================
static void ShotThunderRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        static const int ringColors[] = { 1, 3, 6 }; // 黄, シアン, 白
        const int numShots = 16;
        for (int i = 0; i < numShots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (2.0 * DX_PI / numShots) * i + pEnemyShotSet->muki;
            pEnemyShot->speed = 2.0 + GetRand(200) / 100.0; // 2.0 ~ 4.0
            pEnemyShot->kind = img_enemyShotMediumBall[ringColors[i % 3]];

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

// ============================================================
// パターンC: ShotStaticField — 静電気弾
//
// 広い横範囲(±200px)からシアン/白の小玉がランダム方向に飛散
// 「空気が電気を帯びて静電気が走る」イメージ
// 色: シアン(3) 75%、白(6) 25%
// ============================================================
static void ShotStaticField(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(400) - 200); // ±200px の広範囲
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(40) - 20);
            pEnemyShot->muki = GetRand(360) * DX_PI / 180.0;            // 0°~360° 完全ランダム
            pEnemyShot->speed = 1.0 + GetRand(200) / 100.0;              // 1.0 ~ 3.0
            // GetRand(3) は 0,1,2,3 の 4 値 → 0,1,2 < 3 で 75% シアン
            int color = (GetRand(3) < 3) ? 3 : 6;
            pEnemyShot->kind = img_enemyShotSmallBall[color];

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

// ============================================================
// 敵本体: EnemyPat_Thunder_Claude
// ============================================================
void EnemyPat_Thunder_Claude()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // フェーズが進むほど移動速度が上がる
        double spd = (count <= 240) ? 1.0
            : (count <= 480) ? 1.6
            : 2.2;
        enemy.x += spd * (double)muki;

        // 画面端 (x: 60~420) で折り返し
        if (enemy.x < 60.0) { enemy.x = 60.0; muki = 1; }
        if (enemy.x > 420.0) { enemy.x = 420.0; muki = -1; }
    }

    double fireX = enemy.x;
    double fireY = enemy.y + 15.0;
    double aimAngle = atan2(player.y - fireY, player.x - fireX);

    // ── フェーズ1 (count 1~240): 帯電 ────────────────────────
    if (count <= 240) {
        if (count % 60 == 0) {
            FireShotSet(fireX, fireY, ShotLightningBolt, aimAngle);
        }
    }
    // ── フェーズ2 (count 241~480): 放電 ──────────────────────
    else if (count <= 480) {
        int t = count - 240; // t: 1 ~ 240

        if (t % 45 == 0) {
            // リングを毎回 (π/16 = 11.25°) ずつ回転させる → 螺旋状の弾幕に
            double ringAngle = (t / 45) * (DX_PI / 16.0);
            FireShotSet(fireX, fireY, ShotThunderRing, ringAngle);
        }
        if (t % 90 == 45) {
            // リングの中間タイミングで雷撃弾を差し込む
            FireShotSet(fireX, fireY, ShotLightningBolt, aimAngle);
        }
    }
    // ── フェーズ3 (count 481~): 雷鳴 ─────────────────────────
    else {
        int t = count - 480; // t: 1, 2, 3, ...

        if (t % 30 == 0) {
            FireShotSet(fireX, fireY, ShotLightningBolt, aimAngle);
        }
        if (t % 60 == 0) {
            double ringAngle = (t / 60) * (DX_PI / 16.0);
            FireShotSet(fireX, fireY, ShotThunderRing, ringAngle);
        }
        if (t % 120 == 60) {
            // 静電気弾 + 瞬移 (雷の速さで位置を変える)
            FireShotSet(fireX, fireY, ShotStaticField, 0.0);
            enemy.x = 60.0 + GetRand(360); // 60 ~ 420 にワープ
            muki = (enemy.x < 240.0) ? 1 : -1;
        }
    }
}
