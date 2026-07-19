// enemyPat_tmp.cpp
// インベーダーゲームモチーフ弾幕：「侵攻の階梯」修正版

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const int INVADER_COLS = 8;
static const int INVADER_ROWS = 4;
static const int INVADER_NUM = 32;

// --------------------------------------------------
//  第1列：斜め45°拡散弾（左右交互3way）【緑・小玉】
// --------------------------------------------------
static void ShotInvader_Green(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    if (pSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int sign = (pSet->kind % 2 == 0) ? 1 : -1;
        for (int i = -1; i <= 1; i++) {
            p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = DX_PI / 2.0 + sign * (DX_PI / 4.0) * i;
            p->speed = 2.5;
            p->kind = img_enemyShotSmallBall[2];
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// --------------------------------------------------
//  第2列：プレイヤー狙い低速誘導弾【青・中玉】
// --------------------------------------------------
static void ShotInvader_Blue(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    if (pSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        p->speed = 1.5;
        p->kind = img_enemyShotMediumBall[4];
        p->param_d[0] = p->muki;
        p->param_d[1] = 0.03;
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double target = atan2(player.y - p->y, player.x - p->x);
        double diff = target - p->param_d[0];
        while (diff > DX_PI)  diff -= 2.0 * DX_PI;
        while (diff < -DX_PI) diff += 2.0 * DX_PI;
        p->param_d[0] += diff * p->param_d[1];
        p->muki = p->param_d[0];

        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// --------------------------------------------------
//  第3列：縦一列連射弾（機関銃式）【紫・銃弾】
// --------------------------------------------------
static void ShotInvader_Purple(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    if (pSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    if (pSet->count % 4 == 0 && pSet->count < 20) {
        p = new sEnemyShot;
        p->x = pSet->x + GetRand(10) - 5;
        p->y = pSet->y;
        p->muki = DX_PI / 2.0;
        p->speed = 3.5;
        p->kind = img_enemyShotBullet[5];
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// --------------------------------------------------
//  第4列：下方爆発→小弾円形拡散【赤・大玉→小玉】
// --------------------------------------------------
static void ShotInvader_Red(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    if (pSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = DX_PI / 2.0;
        p->speed = 1.5;
        p->kind = img_enemyShotLargeBall[0];
        p->param_i[0] = 0;
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    if (pSet->count == 60) {
        p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) {
                double ex = p->x;
                double ey = p->y;
                p->x = -1000.0;
                p->param_i[0] = 1;

                for (int i = 0; i < 8; i++) {
                    sEnemyShot* sp = new sEnemyShot;
                    sp->x = ex;
                    sp->y = ey;
                    sp->muki = (DX_PI / 4.0) * i + (GetRand(10) - 5) / 180.0 * DX_PI;
                    sp->speed = 2.0 + GetRand(100) / 100.0;
                    sp->kind = img_enemyShotSmallBall[0];
                    sp->prev = pSet->pEnemyShotHead->prev;
                    sp->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = sp;
                    pSet->pEnemyShotHead->prev = sp;
                }
                break;
            }
            p = p->next;
        }
    }

    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// --------------------------------------------------
//  段降下時：最下段の水平ばら撒き【白・鱗弾】
// --------------------------------------------------
static void ShotInvader_HorizontalSpread(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    if (pSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = pSet->muki + (GetRand(60) - 30) / 180.0 * DX_PI;
            p->speed = 2.0 + GetRand(150) / 100.0;
            p->kind = img_enemyShotScale[6];
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// --------------------------------------------------
//  UFO：貫通レーザー照射【白・レーザー】
// --------------------------------------------------
static void ShotUFO_Laser(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    if (pSet->count == 0) {
        pSet->param_i[0] = (pSet->x < 240.0) ? 1 : -1;
    }

    pSet->x += 3.0 * pSet->param_i[0];

    if (pSet->count % 30 == 0 && pSet->count < 120) {
        if (!CheckSoundMem(sound_enemyShot_heavy)) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = DX_PI / 2.0;
        p->speed = 10.0;
        p->kind = img_enemyShotLaser[6];
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// --------------------------------------------------
//  フォールバック：敵本体からの直接発射（aliveCount=0時）
// --------------------------------------------------
static void ShotBossFallback(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    if (pSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16; i++) {
            p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = (DX_PI / 8.0) * i;
            p->speed = 2.0 + (i % 2) * 1.5;
            p->kind = img_enemyShotSmallBall[6];
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  敵本体パターン：インベーダー群「侵攻の階梯」
// ============================================================
void EnemyPat_Invader_Kimi()
{
    static double invaderOffsetX[INVADER_NUM];
    static double invaderOffsetY[INVADER_NUM];
    static int    invaderAlive[INVADER_NUM];
    static int    groupDir;
    static double groupSpeed;
    static int    stepDownTimer;
    static int    shotTimer;
    static int    aliveCount;
    static int    prevHp;
    static int    ufoTimer;
    static double damageAccum;

    int i, row, col;

    // --- 初期化 ---
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        groupDir = 1;
        groupSpeed = 0.8;
        stepDownTimer = 0;
        shotTimer = 0;
        aliveCount = INVADER_NUM;
        prevHp = enemy.hp;
        ufoTimer = 0;
        damageAccum = 0.0;

        for (row = 0; row < INVADER_ROWS; row++) {
            for (col = 0; col < INVADER_COLS; col++) {
                i = row * INVADER_COLS + col;
                invaderOffsetX[i] = (col - (INVADER_COLS - 1) / 2.0) * 40.0;
                invaderOffsetY[i] = (row - (INVADER_ROWS - 1) / 2.0) * 30.0;
                invaderAlive[i] = 1;
            }
        }
        return;
    }

    // --- HP変動から生存数を更新（ダメージ蓄積方式）---
    if (enemy.hp < prevHp) {
        double damage = (double)(prevHp - enemy.hp);
        damageAccum += damage;
        double hpPerInvader = 200.0 / INVADER_NUM;

        while (damageAccum >= hpPerInvader && aliveCount > 0) {
            int idx = GetRand(INVADER_NUM - 1);
            int safety = 0;
            while (!invaderAlive[idx] && safety < 200) {
                idx = GetRand(INVADER_NUM - 1);
                safety++;
            }
            if (invaderAlive[idx]) {
                invaderAlive[idx] = 0;
                aliveCount--;
                damageAccum -= hpPerInvader;
            }
            else {
                damageAccum = 0.0;
                break;
            }
        }
        prevHp = enemy.hp;
    }

    // --- 群の移動 ---
    if (stepDownTimer > 0) {
        stepDownTimer--;
        enemy.y += 0.5;
    }
    else {
        double baseSpeed = 0.8 + (INVADER_NUM - aliveCount) * 0.15;
        if (groupSpeed < baseSpeed) groupSpeed = baseSpeed;

        enemy.x += groupSpeed * groupDir;

        double maxOffsetX = (INVADER_COLS - 1) / 2.0 * 40.0;
        if ((groupDir > 0 && enemy.x + maxOffsetX >= 460.0) ||
            (groupDir < 0 && enemy.x - maxOffsetX <= 20.0)) {
            groupDir *= -1;
            stepDownTimer = 5;
            groupSpeed += 0.4;

            for (col = 0; col < INVADER_COLS; col++) {
                i = 3 * INVADER_COLS + col;
                if (invaderAlive[i]) {
                    sEnemyShotSet* pSet = new sEnemyShotSet;
                    pSet->count = 0;
                    pSet->patternFunc = ShotInvader_HorizontalSpread;
                    pSet->x = enemy.x + invaderOffsetX[i];
                    pSet->y = enemy.y + invaderOffsetY[i];
                    pSet->muki = (groupDir > 0) ? 0.0 : DX_PI;
                    pSet->kind = col;

                    pSet->pEnemyShotHead = new sEnemyShot;
                    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                    pSet->prev = enemyShotSetHead.prev;
                    pSet->next = &enemyShotSetHead;
                    enemyShotSetHead.prev->next = pSet;
                    enemyShotSetHead.prev = pSet;
                }
            }
        }
    }

    // --- 通常発射 ---
    shotTimer++;
    int fireInterval = (aliveCount > 16) ? 50 : (aliveCount > 8) ? 35 : (aliveCount > 4) ? 20 : 10;
    if (shotTimer >= fireInterval) {
        shotTimer = 0;

        if (aliveCount > 0) {
            for (row = 0; row < INVADER_ROWS; row++) {
                int candidates[INVADER_COLS];
                int candCount = 0;
                for (col = 0; col < INVADER_COLS; col++) {
                    i = row * INVADER_COLS + col;
                    if (invaderAlive[i]) candidates[candCount++] = i;
                }
                if (candCount > 0) {
                    int idx = candidates[GetRand(candCount - 1)];
                    sEnemyShotSet* pSet = new sEnemyShotSet;
                    pSet->count = 0;
                    pSet->x = enemy.x + invaderOffsetX[idx];
                    pSet->y = enemy.y + invaderOffsetY[idx];
                    pSet->kind = idx;

                    switch (row) {
                    case 0: pSet->patternFunc = ShotInvader_Green;  break;
                    case 1: pSet->patternFunc = ShotInvader_Blue;   break;
                    case 2: pSet->patternFunc = ShotInvader_Purple; break;
                    case 3: pSet->patternFunc = ShotInvader_Red;    break;
                    }

                    pSet->pEnemyShotHead = new sEnemyShot;
                    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                    pSet->prev = enemyShotSetHead.prev;
                    pSet->next = &enemyShotSetHead;
                    enemyShotSetHead.prev->next = pSet;
                    enemyShotSetHead.prev = pSet;
                }
            }
        }
        else {
            // aliveCount=0 のフォールバック：敵本体から全方向16way弾
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotBossFallback;
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
    }

    // --- UFO出現（600フレーム間隔）---
    ufoTimer++;
    if (ufoTimer >= 600) {
        ufoTimer = 0;
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotUFO_Laser;
        pSet->x = (GetRand(1) == 0) ? -30.0 : 510.0;
        pSet->y = 30.0;
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