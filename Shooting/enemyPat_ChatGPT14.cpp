#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 毒胞子 → 破裂 → 毒液リング
// ------------------------------------------------------------
static void ShotPoison(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回生成
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base_angle = GetRand(359) / 360.0 * DX_PI;

        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;

            double ang = base_angle + i * DX_PI * 2.0 / 12.0;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = ang;
            pEnemyShot->speed = 2.6;

            // 紫色の毒胞子
            pEnemyShot->kind = img_enemyShotMediumBall[5];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // ----------------------------------------------------
        // 胞子段階
        // ----------------------------------------------------
        if (pEnemyShot->kind == img_enemyShotMediumBall[5]) {

            pEnemyShot->speed *= 0.992;

            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            // 一定時間後に破裂
            if (pEnemyShot->count == 75) {

                if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                for (int i = 0; i < 8; i++) {

                    sEnemyShot* pNew = new sEnemyShot;

                    double ang =
                        i * DX_PI * 2.0 / 8.0 +
                        (GetRand(100) / 100.0) * 0.2;

                    pNew->x = pEnemyShot->x;
                    pNew->y = pEnemyShot->y;

                    pNew->muki = ang;
                    pNew->speed = 0.8;

                    // 毒液
                    pNew->kind = img_enemyShotSmallBall[2];

                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }

                // 破裂後は黒い毒核に変化
                pEnemyShot->kind = img_enemyShotLargeBall[7];
                pEnemyShot->speed = 0.0;
            }
        }
        // ----------------------------------------------------
        // 毒核
        // ----------------------------------------------------
        else if (pEnemyShot->kind == img_enemyShotLargeBall[7]) {

            // ほぼ停止
            pEnemyShot->muki += 0.02;
        }
        // ----------------------------------------------------
        // 毒液
        // ----------------------------------------------------
        else {

            // ゆっくり渦を巻く
            pEnemyShot->muki += 0.01;

            // 徐々に拡散
            if (pEnemyShot->speed < 3.5)
                pEnemyShot->speed += 0.015;

            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        if (pEnemyShot->count == 600) {
            pEnemyShot->x = 9999;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_Poison_ChatGPT()
{
    static int moveDir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
    }
    else {

        enemy.x += moveDir * 1.2;

        if (enemy.x > 400.0)
            moveDir = -1;
        if (enemy.x < 80.0)
            moveDir = 1;
    }

    // 毒胞子を周期的に散布
    if (count % 50 == 30) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPoison;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}