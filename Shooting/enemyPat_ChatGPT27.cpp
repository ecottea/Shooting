// EnemyPat_Shuriken_ChatGPT.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const double SHURIKEN_RADIUS = 14.0;
static const double SHURIKEN_ROTATE = DX_PI / 90.0;
static const double SHURIKEN_SPEED = 2.3;

//------------------------------------------------------------
// 回転手裏剣
//------------------------------------------------------------
static void ShotShuriken(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //--------------------------------------------------------
    // 初回生成
    //--------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 4; i++)
        {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->kind = img_enemyShotDiamond[6];

            pEnemyShot->speed = 0.0;
            pEnemyShot->margin = 40.0;

            pEnemyShot->param_i[0] = i;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[2] = pEnemyShotSet->muki;
        pEnemyShotSet->param_d[3] = 0.0;
    }

    //--------------------------------------------------------
    // 手裏剣中心移動
    //--------------------------------------------------------
    pEnemyShotSet->param_d[0] +=
        cos(pEnemyShotSet->param_d[2]) * SHURIKEN_SPEED;

    pEnemyShotSet->param_d[1] +=
        sin(pEnemyShotSet->param_d[2]) * SHURIKEN_SPEED;

    //--------------------------------------------------------
    // 回転
    //--------------------------------------------------------
    pEnemyShotSet->param_d[3] += SHURIKEN_ROTATE;

    //--------------------------------------------------------
    // 羽根配置
    //--------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        double ang =
            pEnemyShotSet->param_d[3]
            + pEnemyShot->param_i[0] * DX_PI / 2.0;

        pEnemyShot->x =
            pEnemyShotSet->param_d[0]
            + cos(ang) * SHURIKEN_RADIUS;

        pEnemyShot->y =
            pEnemyShotSet->param_d[1]
            + sin(ang) * SHURIKEN_RADIUS;

        //----------------------------------------------------
        // 菱形が進行方向を向くようにする
        //----------------------------------------------------
        pEnemyShot->muki = ang;

        pEnemyShot = pEnemyShot->next;
    }
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_Shuriken_ChatGPT()
{
    static int muki;
    static int shotCount;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 50.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        muki = 1;
        shotCount = 0;
    }
    else
    {
        //----------------------------------------------------
        // 左右移動
        //----------------------------------------------------
        enemy.x += muki * 1.1;

        if (enemy.x > 400.0)
            muki = -1;
        if (enemy.x < 80.0)
            muki = 1;

        //----------------------------------------------------
        // 少し上下にも揺れる
        //----------------------------------------------------
        enemy.y = 50.0 + sin(count * DX_PI / 90.0) * 12.0;
    }

    //--------------------------------------------------------
    // 手裏剣発射
    //--------------------------------------------------------
    if (count % 20 == 0)
    {
        for (int i = -2; i <= 2; i++)
        {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotShuriken;

            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 8.0;

            double aim =
                atan2(
                    player.y - pEnemyShotSet->y,
                    player.x - pEnemyShotSet->x);

            // 少し扇状に投げる
            pEnemyShotSet->muki =
                DX_PI / 2 + i * (DX_PI / 18.0);

            pEnemyShotSet->kind = shotCount++;

            //------------------------------------------------
            // Shotリスト初期化
            //------------------------------------------------
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;

            pEnemyShotSet->pEnemyShotHead->prev =
                pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->pEnemyShotHead->next =
                pEnemyShotSet->pEnemyShotHead;

            //------------------------------------------------
            // ShotSetリストへ接続
            //------------------------------------------------
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;

            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}
