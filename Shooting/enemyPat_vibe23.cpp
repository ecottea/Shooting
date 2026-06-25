// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：シェルピンスキーのギャスケットをモチーフにした高度な弾幕
static void ShotSierpinskiGasket(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- レベル0: 中心に白の大玉を配置 ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme) == 1) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(10) - 5) / 180.0 * DX_PI;
        pEnemyShot->speed = 0.8;
        pEnemyShot->kind = img_enemyShotLargeBall[6]; // 白
        pEnemyShot->param_i[0] = 0;
        pEnemyShot->param_i[1] = 0;
        pEnemyShot->param_d[0] = 0.002;
        pEnemyShot->param_d[1] = 0.0;
        pEnemyShot->margin = 480.0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // --- レベル1: 3方向に赤の鱗弾を分裂（120度間隔）---
    if (pEnemyShotSet->count == 30) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 0 && pEnemyShot->param_i[1] == 0) {
                for (int i = 0; i < 3; i++) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    double angle = pEnemyShot->muki + (i * 2 * DX_PI / 3) + (GetRand(12) - 6) / 180.0 * DX_PI;
                    pNewShot->x = pEnemyShot->x;
                    pNewShot->y = pEnemyShot->y;
                    pNewShot->muki = angle;
                    pNewShot->speed = 1.5 + GetRand(8) / 10.0;
                    pNewShot->kind = img_enemyShotScale[0]; // 赤の鱗弾
                    pNewShot->param_i[0] = 1;
                    pNewShot->param_i[1] = 0;
                    pNewShot->param_d[0] = 0.0015;
                    pNewShot->param_d[1] = (i == 0) ? 0.008 : (i == 1) ? -0.008 : 0.0;
                    pNewShot->margin = 480.0;

                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }
                pEnemyShot->param_i[1] = 1;
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // --- レベル2: 各鱗弾から3方向に緑の銃弾を分裂（60度間隔）---
    if (pEnemyShotSet->count == 60) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 1 && pEnemyShot->param_i[1] == 0) {
                for (int i = 0; i < 3; i++) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    double offset = (i - 1) * DX_PI / 3;
                    double angle = pEnemyShot->muki + offset + (GetRand(8) - 4) / 180.0 * DX_PI;
                    pNewShot->x = pEnemyShot->x;
                    pNewShot->y = pEnemyShot->y;
                    pNewShot->muki = angle;
                    pNewShot->speed = 2.2 + GetRand(10) / 10.0;

                    pNewShot->kind = img_enemyShotBullet[2]; // 緑の銃弾
                    pNewShot->param_i[0] = 2;
                    pNewShot->param_i[1] = 0;
                    pNewShot->param_d[0] = 0.0;
                    pNewShot->param_d[1] = (GetRand(2) == 0) ? 0.012 : -0.012;
                    pNewShot->margin = 480.0;

                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }
                pEnemyShot->param_i[1] = 1;
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // --- レベル3: 各銃弾から3方向に青の菱形弾を分裂（30度間隔）---
    if (pEnemyShotSet->count == 90) {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 2 && pEnemyShot->param_i[1] == 0) {
                for (int i = 0; i < 3; i++) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    double offset = (i - 1) * DX_PI / 6;
                    double angle = pEnemyShot->muki + offset + (GetRand(6) - 3) / 180.0 * DX_PI;
                    pNewShot->x = pEnemyShot->x;
                    pNewShot->y = pEnemyShot->y;
                    pNewShot->muki = angle;
                    pNewShot->speed = 2.8 + GetRand(6) / 10.0;

                    pNewShot->kind = img_enemyShotDiamond[4]; // 青の菱形弾
                    pNewShot->param_i[0] = 3;
                    pNewShot->param_i[1] = 0;
                    pNewShot->param_d[0] = -0.001;
                    pNewShot->param_d[1] = (i == 0) ? 0.006 : (i == 1) ? -0.006 : 0.0;
                    pNewShot->margin = 480.0;

                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }
                pEnemyShot->param_i[1] = 1;
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // --- レベル4: 追加の分裂（プレイヤーを狙う黒の小玉）---
    if (pEnemyShotSet->count == 120) {
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 3 && pEnemyShot->param_i[1] == 0) {
                for (int i = 0; i < 2; i++) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    double targetAngle = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                    double offset = (i == 0) ? -0.08 : 0.08;
                    pNewShot->x = pEnemyShot->x;
                    pNewShot->y = pEnemyShot->y;
                    pNewShot->muki = targetAngle + offset;
                    pNewShot->speed = 3.5;
                    pNewShot->kind = img_enemyShotSmallBall[7]; // 黒
                    pNewShot->param_i[0] = 4;
                    pNewShot->param_i[1] = 1;
                    pNewShot->param_d[0] = 0.0;
                    pNewShot->param_d[1] = 0.01;
                    pNewShot->margin = 480.0;

                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }
                pEnemyShot->param_i[1] = 1;
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // --- 弾の移動（レベルごとに異なる動き）---
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        switch (pEnemyShot->param_i[0]) {
        case 0:
            pEnemyShot->speed += pEnemyShot->param_d[0];
            break;
        case 1:
            pEnemyShot->speed += pEnemyShot->param_d[0];
            pEnemyShot->muki += pEnemyShot->param_d[1] * (1.0 + 0.5 * sin(pEnemyShotSet->count * 0.05));
            break;
        case 2:
            pEnemyShot->muki += pEnemyShot->param_d[1] * sin(pEnemyShotSet->count * 0.1);
            break;
        case 3:
            pEnemyShot->speed += pEnemyShot->param_d[0];
            if (pEnemyShot->speed < 1.0) pEnemyShot->speed = 1.0;
            pEnemyShot->muki += pEnemyShot->param_d[1];
            break;
        case 4:
            if (pEnemyShot->param_i[1] == 1) {
                double targetAngle = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                double diff = targetAngle - pEnemyShot->muki;
                while (diff > DX_PI) diff -= 2 * DX_PI;
                while (diff < -DX_PI) diff += 2 * DX_PI;
                pEnemyShot->muki += diff * pEnemyShot->param_d[1];
            }
            break;
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Sierpinski_Vibe()
{
    static int shot_count;
    static double movePhase = 0.0;
    static double moveAmplitude = 60.0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
        movePhase = 0.0;
        moveAmplitude = 60.0;
    }
    else {
        movePhase += 0.015;
        moveAmplitude += 0.02;
        if (moveAmplitude > 100.0) moveAmplitude = 100.0;
        enemy.x = 240.0 + moveAmplitude * sin(movePhase);
        enemy.y = 60.0 + moveAmplitude * 0.6 * sin(movePhase * 2.0);
    }

    if (count % 100 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSierpinskiGasket;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}