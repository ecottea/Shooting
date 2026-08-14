// enemyPat_tmp.cpp
// 弾幕：絶対零度の螺旋牢獄（Absolute Zero Spiral Prison）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 第1〜3フェーズ統合：螺旋牢獄本体
// ------------------------------------------------------------
static void ShotSpiralPrison(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* p;

    // --- フェーズ1：霜の予兆 (count 0〜59) ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 16方向に氷晶弾（中玉・青）を放射
        for (int i = 0; i < 16; i++) {
            p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = DX_PI * 2.0 * i / 16.0;
            p->speed = 1.2;
            p->kind = img_enemyShotMediumBall[4]; // 青
            p->param_i[0] = 0; // 0:氷晶弾
            p->param_d[0] = 0.012; // 時計回り回転速度

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // --- フェーズ2：吹雪の螺旋 (count 60〜299) ---
    if (pEnemyShotSet->count >= 60 && pEnemyShotSet->count < 300) {
        // 毎フレーム2発、反対方向からの二重螺旋雪弾（小玉・白）
        for (int j = 0; j < 2; j++) {
            p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            double baseAngle = (pEnemyShotSet->count - 60) * 0.12;
            if (j == 1) baseAngle += DX_PI;
            p->muki = baseAngle;
            // 速度を少しずつ変化させて層状に
            p->speed = 1.8 + ((pEnemyShotSet->count - 60) % 40) * 0.04;
            p->kind = img_enemyShotSmallBall[6]; // 白
            p->param_i[0] = 1; // 1:雪弾
            p->param_d[0] = (j == 0) ? 0.008 : -0.008;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // --- フェーズ3：エターナル・フリーズ (count 300〜) ---
    if (pEnemyShotSet->count == 300) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count == 315) {
        // 衝撃波（中玉・白）24方向
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 24; i++) {
            p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = DX_PI * 2.0 * i / 24.0;
            p->speed = 2.5;
            p->kind = img_enemyShotMediumBall[6]; // 白
            p->param_i[0] = 2; // 2:衝撃波
            p->param_d[0] = 0.0;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // --- 全弾移動処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count < 60) {
            // フェーズ1：氷晶弾をゆっくり螺旋化
            if (pShot->param_i[0] == 0) {
                pShot->muki += pShot->param_d[0];
            }
        }
        else if (pEnemyShotSet->count >= 60 && pEnemyShotSet->count < 300) {
            // フェーズ2：雪弾を螺旋化、氷晶弾も継続回転
            if (pShot->param_i[0] == 1) {
                pShot->muki += pShot->param_d[0];
            }
            else if (pShot->param_i[0] == 0) {
                pShot->muki += pShot->param_d[0] * 0.5;
            }
        }
        else if (pEnemyShotSet->count >= 300 && pEnemyShotSet->count < 350) {
            // フェーズ3：氷晶弾と雪弾をプレイヤー狙いに切り替え＆加速
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                if (pEnemyShotSet->count == 300) {
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                }
                if (pShot->speed < 4.5) pShot->speed += 0.12;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 冷気の壁：画面四辺から中央へ侵食する大玉
// ------------------------------------------------------------
static void ShotColdWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* p;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 上辺（5個）
        for (int i = 0; i < 5; i++) {
            p = new sEnemyShot;
            p->x = 48.0 + i * 96.0;
            p->y = -30.0;
            p->muki = DX_PI / 2.0;
            p->speed = 0.9;
            p->kind = img_enemyShotLargeBall[3]; // シアン大玉
            p->param_i[0] = 10;
            p->margin = 40;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        // 下辺（5個）
        for (int i = 0; i < 5; i++) {
            p = new sEnemyShot;
            p->x = 48.0 + i * 96.0;
            p->y = 510.0;
            p->muki = -DX_PI / 2.0;
            p->speed = 0.9;
            p->kind = img_enemyShotLargeBall[3];
            p->param_i[0] = 10;
            p->margin = 40;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        // 左辺（5個）
        for (int i = 0; i < 5; i++) {
            p = new sEnemyShot;
            p->x = -30.0;
            p->y = 48.0 + i * 96.0;
            p->muki = 0.0;
            p->speed = 0.9;
            p->kind = img_enemyShotLargeBall[3];
            p->param_i[0] = 10;
            p->margin = 40;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        // 右辺（5個）
        for (int i = 0; i < 5; i++) {
            p = new sEnemyShot;
            p->x = 510.0;
            p->y = 48.0 + i * 96.0;
            p->muki = DX_PI;
            p->speed = 0.9;
            p->kind = img_enemyShotLargeBall[3];
            p->param_i[0] = 10;
            p->margin = 40;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 氷柱：画面中央にX字状のレーザー
// ------------------------------------------------------------
static void ShotIcePillar(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* p;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // X字状に4本の白レーザー
        for (int i = 0; i < 4; i++) {
            p = new sEnemyShot;
            p->muki = DX_PI / 4.0 + DX_PI * 0.5 * i; // 45°, 135°, 225°, 315°
            p->x = 240.0 + 64.0 * cos(p->muki);
            p->y = 240.0 + 64.0 * sin(p->muki);
            p->speed = 0.0; // 静止
            p->kind = img_enemyShotLaser[6]; // 白レーザー
            p->param_i[0] = 20;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // レーザーは動かない
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_EternalForceBlizzard_Kimi()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // ボスは中央上部で微動
    enemy.x = 240.0 + sin(count * 0.02) * 20.0;
    enemy.y = 60.0 + cos(count * 0.03) * 10.0;

    const int T = 500;
    int countT = count % T;

    // 60f：螺旋牢獄本体を発動
    if (countT == 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSpiralPrison;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 120f：冷気の壁を発動（フェーズ2と同期）
    if (countT == 120) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotColdWall;
        pSet->x = 0.0;
        pSet->y = 0.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 360f：氷柱を発動（フェーズ3と同期）
    if (countT == 360) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotIcePillar;
        pSet->x = 240.0;
        pSet->y = 240.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}