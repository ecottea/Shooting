// enemyPat_volcano.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：噴火直前の予兆（小さな火山灰を放出）
static void ShotVolcanoPrelude(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 20; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (i * 18.0 / 180.0 * DX_PI) + (GetRand(30) - 15.0) / 180.0 * DX_PI;
            pEnemyShot->speed = (100 + GetRand(100)) / 100.0;

            // 火山灰は小玉で赤/橙/黄/黒
            int color = GetRand(4);
            if (color == 3) color = 7; // 黒
            else if (color == 2) color = 8; // 橙
            else if (color == 1) color = 1; // 黄
            else color = 0; // 赤
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

// 弾幕：本噴火（溶岩の弧を描く弾幕）
static void ShotVolcanoEruption(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme) == 1) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 溶岩の弧を描く弾を24個放出
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 弧を描くために、角度を調整
            double angle = (i * 15.0 / 180.0 * DX_PI) - (DX_PI / 2.0);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = (150 + GetRand(100)) / 100.0;

            // 溶岩は中玉で赤/橙
            int color = GetRand(2) == 0 ? 0 : 8;
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 弧を描くように、重力を加える
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki) + 0.05 * pEnemyShot->count;
        pEnemyShot = pEnemyShot->next;
    }
}

// 弾幕：火山弾の集中砲火
static void ShotVolcanoBombardment(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // プレイヤーを狙う火山弾を8個放出
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // プレイヤーの方向に向かって飛ぶ
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(40) - 20.0) / 180.0 * DX_PI;
            pEnemyShot->speed = (200 + GetRand(100)) / 100.0;

            // 火山弾は大玉で赤/橙
            int color = GetRand(2) == 0 ? 0 : 8;
            pEnemyShot->kind = img_enemyShotLargeBall[color];

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

// 弾幕：火山の落石（ランダムな方向に大きな岩を落下）
static void ShotVolcanoRockfall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // ランダムな方向に落石を6個放出
        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(100) - 50;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (GetRand(60) - 30.0 + 90.0) / 180.0 * DX_PI; // 下方向を中心に広がる
            pEnemyShot->speed = (120 + GetRand(80)) / 100.0;

            // 落石は鱗弾で灰色（黒）
            pEnemyShot->kind = img_enemyShotScale[7]; // 黒

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki) + 0.1 * pEnemyShot->count; // 重力を加える
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Volcano_Vibe()
{
    static int phase = 0; // 0:準備, 1:噴火, 2:クールダウン
    static int timer = 0;
    static double shake = 0.0;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        timer = 0;
        shake = 0.0;
    }
    else {
        // 火山の揺れを表現
        shake += 0.02 * sin(count * 0.15);
        enemy.x = 240.0 + 10.0 * sin(count * 0.1) + shake * 5.0;

        // フェーズ管理
        timer++;
        if (phase == 0 && timer >= 120) {
            phase = 1;
            timer = 0;
        }
        else if (phase == 1 && timer >= 180) {
            phase = 2;
            timer = 0;
        }
        else if (phase == 2 && timer >= 120) {
            phase = 0;
            timer = 0;
        }
    }

    // 弾幕発射
    if (phase == 0 && count % 30 == 0) {
        // 予兆：小さな火山灰を放出
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotVolcanoPrelude;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else if (phase == 1) {
        if (count % 45 == 0) {
            // 本噴火：溶岩の弧を描く弾幕
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotVolcanoEruption;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 20.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        if (count % 90 == 30) {
            // 火山弾の集中砲火
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotVolcanoBombardment;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 20.0;
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
    else if (phase == 2 && count % 60 == 0) {
        // クールダウン：落石
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotVolcanoRockfall;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
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