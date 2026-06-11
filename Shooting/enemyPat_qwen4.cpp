// enemyPat_sample4.cpp
// パターン4：和風弾幕（桜吹雪・扇の舞・龍の鱗）
#include "DxLib.h"
#include "gv.h"
#include <math.h>

#define PI 3.14159265358979323846

// -----------------------------------------------------------
// 弾幕1：桜吹雪（円環状に弾を撒き、回転させながら拡散）
// -----------------------------------------------------------
static void ShotSakura(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int num_shots = 24;
        double base_angle = pEnemyShotSet->muki;
        
        for (int i = 0; i < num_shots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_angle + (2.0 * PI * i / num_shots);
            pEnemyShot->speed = 2.2;
            
            // 桜色（マゼンタ、赤、白）
            int color = GetRand(2) == 0 ? 5 : (GetRand(1) == 0 ? 0 : 6);
            int type = GetRand(2);
            if (type == 0) pEnemyShot->kind = img_enemyShotSmallBall[color];
            else           pEnemyShot->kind = img_enemyShotMediumBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    
    // 弾の移動と回転
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->muki += 0.03; // 角速度を加えて回転させる
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// -----------------------------------------------------------
// 弾幕2：扇の舞（自機に向かって扇状に広がる中玉・大玉）
// -----------------------------------------------------------
static void ShotFan(sEnemyShotSet* pEnemyShotSet)
{
    const int colorTable[] = { 2, 3, 5 };
    sEnemyShot* pEnemyShot;
    
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num_shots = 15;
        double base_angle = pEnemyShotSet->muki;
        double spread = PI / 2.5; // 約72度広げる
        
        for (int i = 0; i < num_shots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 扇状に角度を分配
            pEnemyShot->muki = base_angle - spread / 2.0 + (spread * i / (num_shots - 1));
            pEnemyShot->speed = 2.5 + (i % 3) * 0.8; // 速度差をつけて波を作る
            
            // 和風カラー（緑、シアン、マゼンタ）
            int color = colorTable[i % 3];
            int type = i % 2;
            if (type == 0) pEnemyShot->kind = img_enemyShotMediumBall[color];
            else           pEnemyShot->kind = img_enemyShotLargeBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    
    // 弾の移動（直進）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// -----------------------------------------------------------
// 弾幕3：龍の鱗（自機狙いの鱗弾・菱形弾を連射）
// -----------------------------------------------------------
static void ShotDragonScale(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int num_shots = 3;
        for (int i = 0; i < num_shots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 1) * 0.1; // 多少ばらまく
            pEnemyShot->speed = 4.0;
            
            // 龍の鱗色（緑、シアン）
            int color = (i % 2 == 0) ? 2 : 3;
            int type = GetRand(1);
            if (type == 0) pEnemyShot->kind = img_enemyShotScale[color];
            else           pEnemyShot->kind = img_enemyShotDiamond[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    
    // 弾の移動（直進）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// -----------------------------------------------------------
// 敵本体のパターン
// -----------------------------------------------------------
void Pattern4_Wafu()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = 150;
        enemy.hp = enemy.maxHp;
    }
    else {
        // ゆっくりと8の字を描くように移動
        enemy.x = 240.0 + 150.0 * sin(count * 0.015);
        enemy.y = 60.0 + 40.0 * sin(count * 0.03);
    }

    // 1. 桜吹雪 (count 0～240)
    if (count % 45 == 0 && count < 360) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSakura;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = count * 0.2; // 撒く角度を少しずつずらす

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 2. 扇の舞 (count 240～480)
    if (count % 60 == 0 && count >= 240) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFan;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 3. 龍の鱗 (count 480～)
    if (count % 15 == 0 && count >= 480) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDragonScale;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}