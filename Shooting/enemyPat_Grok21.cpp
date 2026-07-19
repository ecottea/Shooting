// enemyPat_volcano.cpp
// 火山をモチーフにした弾幕パターン
// 噴火 → 溶岩弾の落下 → 火の環 + 灰のばら撒き

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// 補助関数：火山噴火風の溶岩弾バースト
// =============================================
static void ShotVolcanoEruption(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 噴火音（重め）
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 中央上方向への強力な噴火（溶岩弾）
        for (int i = -6; i <= 6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y - 10.0;
            pEnemyShot->muki = DX_PI * (0.5 + i * 0.08);  // 上方向中心に扇状
            pEnemyShot->speed = 4.5 + GetRand(80) / 100.0;
            pEnemyShot->kind = img_enemyShotLargeBall[GetRand(2)]; // 赤・黄・橙系の溶岩っぽく
            pEnemyShot->param_d[0] = 0.0; // 重力用カウンタ
            pEnemyShot->param_i[0] = 1;   // 落下フェーズフラグ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 火の環（周囲に放射）
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * DX_PI * 2.0 / 24.0;
            pEnemyShot->speed = 2.8;
            pEnemyShot->kind = img_enemyShotScale[0]; // 赤系鱗弾で炎っぽく
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 1) { // 溶岩弾（重力落下）
            pEnemyShot->muki += 0.015; // 少し弧を描く
            pEnemyShot->speed += 0.08; // 加速落下
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        else {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            pEnemyShot->speed *= 0.985; // 少し減速
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// =============================================
// 補助関数：灰の乱流（細かい弾のばら撒き）
// =============================================
static void ShotAshScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 32; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(120) - 60;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(40) - 20;
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->speed = (80 + GetRand(120)) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白・灰色系
            pEnemyShot->param_d[0] = GetRand(100) / 200.0 - 0.25; // 微かな横揺れ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->muki += pEnemyShot->param_d[0] * 0.03; // ゆらゆら
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki) + 0.6; // 下方向に流れやすい灰
        pEnemyShot->speed *= 0.992;
        pEnemyShot = pEnemyShot->next;
    }
}

// =============================================
// 敵本体パターン：火山
// =============================================
void EnemyPat_Volcano_Grok()
{
    static int phase = 0;
    static int muki = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        muki = 1;
    }

    // ゆったりとした左右移動（火山の稜線をイメージ）
    enemy.x += 1.2 * (double)muki;
    if (count % 140 == 70) muki *= -1;

    // フェーズ管理
    if (count % 180 == 0) phase = (phase + 1) % 3;

    // パターン実行
    if (count % 55 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        if (phase == 0) {
            pEnemyShotSet->patternFunc = ShotVolcanoEruption;
        }
        else if (phase == 1) {
            pEnemyShotSet->patternFunc = ShotAshScatter;
        }
        else {
            pEnemyShotSet->patternFunc = ShotVolcanoEruption; // 繰り返し
        }

        // 連結
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}