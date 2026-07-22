// enemyPat_CapitalA.cpp
// 大文字「A」をモチーフにした弾幕パターン（視認性大幅向上版）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// 弾幕パターン：Capital A（改良版）
// =============================================
static void ShotCapitalA(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double PI = DX_PI;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK); // 重めの音で強調

        // 1. 左斜めライン（Aの左脚）
        for (int i = -28; i < 28; i++) {
            pEnemyShot = new sEnemyShot;
            double t = i / 27.0;
            pEnemyShot->x = pEnemyShotSet->x - 55 * (1.0 - t);
            pEnemyShot->y = pEnemyShotSet->y + 15 + t * 75;
            pEnemyShot->muki = -PI * 0.35;           // 左上方向に固定気味
            pEnemyShot->speed = 2.4;
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄中玉で視認性高め
            pEnemyShot->margin = 200;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 2. 右斜めライン（Aの右脚）
        for (int i = -28; i < 28; i++) {
            pEnemyShot = new sEnemyShot;
            double t = i / 27.0;
            pEnemyShot->x = pEnemyShotSet->x + 55 * (1.0 - t);
            pEnemyShot->y = pEnemyShotSet->y + 15 + t * 75;
            pEnemyShot->muki = -PI * 0.65;           // 右上方向に固定気味
            pEnemyShot->speed = 2.4;
            pEnemyShot->kind = img_enemyShotMediumBall[1];
            pEnemyShot->margin = 200;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 3. 中央横棒（Aのクロスバー） - 太めに2列
        for (int line = 0; line < 2; line++) {
            for (int i = 0; i < 22; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x - 48 + i * 4.5;
                pEnemyShot->y = pEnemyShotSet->y + 48 + line * 6;
                pEnemyShot->muki = -PI / 2;
                pEnemyShot->speed = 2.6;
                pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤中玉
                pEnemyShot->margin = 200;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 4. 頂点部からの落下弾
        for (int i = 0; i < 16; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y - 25;
            pEnemyShot->muki = -PI / 2 + (GetRand(90) - 45) / 180.0 * PI;
            pEnemyShot->speed = 1.6;
            pEnemyShot->kind = img_enemyShotSmallBall[GetRand(7)];
            pEnemyShot->margin = 200;
            pEnemyShot->param_i[0] = 25 + GetRand(15); // 分裂遅延
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->count++;

        // 分裂処理
        if (pShot->param_i[0] > 0) {
            pShot->param_i[0]--;
            if (pShot->param_i[0] == 0) {
                double baseMuki = pShot->muki;
                for (int k = -2; k <= 2; k++) {
                    sEnemyShot* split = new sEnemyShot;
                    split->x = pShot->x;
                    split->y = pShot->y;
                    split->muki = baseMuki + k * 0.28;
                    split->speed = 2.8;
                    split->kind = img_enemyShotSmallBall[GetRand(7)];
                    split->margin = 200;
                    split->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    split->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = split;
                    pEnemyShotSet->pEnemyShotHead->prev = split;
                }
                pShot->speed = 0.1; // ほぼ消滅
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// =============================================
// 敵本体パターン
// =============================================
void EnemyPat_Daimonji_Grok()
{
    static int muki = 1;
    static int shotTimer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 460.0;
        enemy.maxHp = enemy.hp = 60 * 20;
        muki = 1;
        shotTimer = 0;
    }
    else {
        enemy.x += 1.6 * (double)muki;
        if (enemy.x < 140 || enemy.x > 340) muki *= -1;
    }
    enemy.hp--;

    shotTimer++;
    if (shotTimer % 90 == 0) {        // 約1.5秒ごとに発射
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCapitalA;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}