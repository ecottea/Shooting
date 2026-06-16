// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 太陽コロナ弾
// ------------------------------------------------------------
static void ShotSunCorona(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射時のみ生成
    if (pEnemyShotSet->count == 0) {

        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int ringNum = 24;

        for (int i = 0; i < ringNum; i++) {

            pEnemyShot = new sEnemyShot;

            double ang =
                i * DX_PI * 2.0 / ringNum;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = ang;
            pEnemyShot->speed = 0.0;

            pEnemyShot->kind =
                img_enemyShotMediumBall[1];   // 黄

            pEnemyShot->count = i;

            pEnemyShot->margin = 9999;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    double rot =
        pEnemyShotSet->count * 0.03;

    // 方向反転（黒点活動）
//    if ((pEnemyShotSet->count / 180) % 2)
//        rot = -rot;

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        double ang =
            (pEnemyShot->count * DX_PI * 2.0 / 24.0
            + rot) * 0.2 - DX_PI / 2.0;

        // 脈動するコロナ
        double r =
            100.0
            + 25.0 * sin(
                (pEnemyShotSet->count * 0.05
                + pEnemyShot->count * 0.35) * 0.1);

        pEnemyShot->x =
            pEnemyShotSet->x + cos(ang) * r;

        pEnemyShot->y =
            pEnemyShotSet->y + sin(ang) * r;

        if (pEnemyShot->count >= 90 * 5) {
            pEnemyShot->x = 9999;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------------------------------------------------
// 太陽フレア
// ------------------------------------------------------------
static void ShotSolarFlare(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {

        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int flareNum = 7;

        for (int i = 0; i < flareNum; i++) {

            pEnemyShot = new sEnemyShot;

            double ang =
                pEnemyShotSet->muki
                + (i - (flareNum - 1) / 2.0)
                * DX_PI / 30.0;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = ang;
            pEnemyShot->speed = 2.0 + i * 0.18;

            pEnemyShot->kind =
                img_enemyShotScale[1]; // 黄

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 徐々に加速するフレア
        pEnemyShot->speed += 0.02;

        pEnemyShot->x +=
            cos(pEnemyShot->muki) *
            pEnemyShot->speed;

        pEnemyShot->y +=
            sin(pEnemyShot->muki) *
            pEnemyShot->speed;

        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_Sun_ChatGPT()
{
    static int moveDir;

    if (count == 1) {

        enemy.x = 240.0;
        enemy.y = 70.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        moveDir = 1;
    }

    // ゆっくり左右移動
    enemy.x += 0.7 * moveDir;

    if (enemy.x < 120.0)
        moveDir = 1;

    if (enemy.x > 360.0)
        moveDir = -1;

    // --------------------------------------------------------
    // 常時コロナ生成
    // --------------------------------------------------------
    if (count % 90 == 0) {

        sEnemyShotSet* pEnemyShotSet =
            new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSunCorona;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 240 + (GetRand(400) - 200);

        pEnemyShotSet->pEnemyShotHead =
            new sEnemyShot;

        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev =
            enemyShotSetHead.prev;

        pEnemyShotSet->next =
            &enemyShotSetHead;

        enemyShotSetHead.prev->next =
            pEnemyShotSet;

        enemyShotSetHead.prev =
            pEnemyShotSet;
    }

    // --------------------------------------------------------
    // 太陽フレア
    // --------------------------------------------------------
    if (count % 40 == 0) {

        sEnemyShotSet* pEnemyShotSet =
            new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc =
            ShotSolarFlare;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->muki =
            atan2(
                player.y - enemy.y,
                player.x - enemy.x);

        pEnemyShotSet->pEnemyShotHead =
            new sEnemyShot;

        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev =
            enemyShotSetHead.prev;

        pEnemyShotSet->next =
            &enemyShotSetHead;

        enemyShotSetHead.prev->next =
            pEnemyShotSet;

        enemyShotSetHead.prev =
            pEnemyShotSet;
    }
}