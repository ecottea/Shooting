#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotSnakeLaser(sEnemyShotSet* pEnemyShotSet);

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_HenyoriLaser_ChatGPT()
{
    static int moveDir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
    }

    enemy.x += moveDir * 0.8;

    if (enemy.x < 120.0 + 70) moveDir = 1;
    if (enemy.x > 360.0 - 70) moveDir = -1;

    // 4本同時発射
    if (count == 1)
    {
        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = -4; i < 8; i++)
        {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSnakeLaser;

            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 12.0;

            pEnemyShotSet->kind = i;

            // 基準角
            pEnemyShotSet->param_d[0] =
                DX_PI / 2.0 +
                (-0.33 + i * 0.22);

            // 位相
            pEnemyShotSet->param_d[1] =
                i * DX_PI * 0.5;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev =
                pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next =
                pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}

//------------------------------------------------------------
// へにょりレーザー
//------------------------------------------------------------
static void ShotSnakeLaser(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 約160フレームかけてレーザーを伸ばす
    if (pEnemyShotSet->count < 140)
    {
        sEnemyShot* pEnemyShot = new sEnemyShot;

        pEnemyShot->kind = img_enemyShotMediumOval[3];

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        pEnemyShot->speed = 0.0;
        pEnemyShot->muki = 0.0;

        pEnemyShot->margin = 120.0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // レーザー全体を一本の曲線として再配置
    const double baseAngle = pEnemyShotSet->param_d[0];
    const double phase = pEnemyShotSet->param_d[1];

    // 呼吸するように振幅が変化
    const double amplitude =
        20.0 + 15.0 * sin(pEnemyShotSet->count * 0.04);

    int index = 0;

    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        // レーザーに沿った距離
        double forward = index * 3.0;

        // 波が先端方向へ流れていく
        double side =
            sin(forward * 0.08
                - pEnemyShotSet->count * 0.18
                + phase)
            * amplitude;

        pEnemyShot->x =
            pEnemyShotSet->x
            + forward * cos(baseAngle)
            + side * cos(baseAngle + DX_PI / 2.0);

        pEnemyShot->y =
            pEnemyShotSet->y
            + forward * sin(baseAngle)
            + side * sin(baseAngle + DX_PI / 2.0);

        index++;
        pEnemyShot = pEnemyShot->next;
    }
}