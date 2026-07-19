// enemyPat_Pyramid.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// ピラミッド弾幕用ヘルパー関数
// =============================================

// ピラミッド基底層（広い低速の横列）
static void PyramidBaseLayer(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        int num = 12;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double offset = (i - num / 2.0) * 38.0;
            pEnemyShot->x = pEnemyShotSet->x + offset;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0; // 下方向
            pEnemyShot->speed = 1.8;
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 小玉・赤
            // リンク
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->y += pEnemyShot->speed;
        pEnemyShot = pEnemyShot->next;
    }
}

// ピラミッド中間層（徐々に狭くなる階段）
static void PyramidMiddleLayer(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        int layer = pEnemyShotSet->kind; // 0~2 で層を区別
        int num = 9 - layer * 2;
        double width = 280.0 - layer * 90.0;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double offset = (i - num / 2.0) * (width / (num - 1));
            pEnemyShot->x = pEnemyShotSet->x + offset;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0;
            pEnemyShot->speed = 2.2 + layer * 0.3;
            if (layer == 0)
                pEnemyShot->kind = img_enemyShotMediumBall[2]; // 中玉・緑
            else
                pEnemyShot->kind = img_enemyShotSmallBall[2]; // 小玉・緑
            // 斜面表現のため少し斜め成分をランダム追加
            if (GetRand(1) == 0) pEnemyShot->muki += (GetRand(40) - 20) / 180.0 * DX_PI * 0.2;
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

// ピラミッド頂点攻撃（高速針弾＋放射）
static void PyramidApexAttack(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        // 集中針弾
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(60) - 30) / 180.0 * DX_PI * 0.4;
            pEnemyShot->speed = 5.5 + i * 0.4;
            pEnemyShot->kind = img_enemyShotBullet[4]; // 銃弾・青
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // 放射状レーザー風
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * 2.0 * DX_PI / 7 + (pEnemyShotSet->count % 30) * 0.03;
            pEnemyShot->speed = 3.8;
            pEnemyShot->kind = img_enemyShotLaser[5]; // レーザー・マゼンタ
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

// =============================================
// 敵本体パターン
// =============================================
void EnemyPat_Pyramid_Grok()
{
    static int muki = 1;
    static int phase = 0;
    static int timer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
        timer = 0;
    }
    else {
        // 左右ゆっくり移動
        enemy.x += 1.1 * (double)muki;
        if (count % 140 == 70) muki *= -1;

        // 軽く上下移動で迫力アップ
        enemy.y = 60.0 + sin(count / 40.0) * 15.0;
    }

    timer++;

    // ピラミッド基底層（定期的に）
    if (timer % 55 == 0) {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = PyramidBaseLayer;
        p->x = enemy.x;
        p->y = enemy.y + 20.0;
        p->muki = 0;
        p->kind = 0;
        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;
        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }

    // 中間層（少し遅れて階段状）
    if (timer % 55 == 18) {
        for (int layer = 0; layer < 3; layer++) {
            sEnemyShotSet* p = new sEnemyShotSet;
            p->count = 0;
            p->patternFunc = PyramidMiddleLayer;
            p->x = enemy.x;
            p->y = enemy.y + 10.0 - layer * 22.0;
            p->muki = 0;
            p->kind = layer;
            p->pEnemyShotHead = new sEnemyShot;
            p->pEnemyShotHead->prev = p->pEnemyShotHead;
            p->pEnemyShotHead->next = p->pEnemyShotHead;
            p->prev = enemyShotSetHead.prev;
            p->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = p;
            enemyShotSetHead.prev = p;
        }
    }

    // 頂点攻撃（プレッシャー用）
    if (timer % 80 == 45) {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = PyramidApexAttack;
        p->x = enemy.x;
        p->y = enemy.y + 5.0;
        p->muki = atan2(player.y - p->y, player.x - p->x);
        p->kind = 0;
        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;
        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }
}