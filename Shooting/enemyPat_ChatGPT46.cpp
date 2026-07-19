// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#ifndef DX_PI
#define DX_PI 3.14159265358979323846
#endif

//============================================================
// 大砂嵐
//============================================================
static void ShotSandStorm(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //--------------------------------------------------------
    // 生成
    //--------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        //----------------------------------------------------
        // kind==0 : 流れる砂
        //----------------------------------------------------
        if (pEnemyShotSet->kind == 0)
        {
            double dir = pEnemyShotSet->param_d[0];

            for (int i = 0; i < 10; i++)
            {
                pEnemyShot = new sEnemyShot;

                while (true) {
                    pEnemyShot->x =
                        pEnemyShotSet->x + GetRand(520) - 260;

                    pEnemyShot->y =
                        pEnemyShotSet->y + GetRand(520) - 260;

                    double dx = pEnemyShot->x - player.x;
                    double dy = pEnemyShot->y - player.y;
                    if (hypot(dx, dy) > 50) break;
                }

                pEnemyShot->muki =
                    dir +
                    (GetRand(40) - 20) / 180.0 * DX_PI;

                pEnemyShot->speed =
                    1.6 + GetRand(80) / 100.0;

                // 黄・橙だけ使用
                if (GetRand(1) == 0)
                    pEnemyShot->kind = img_enemyShotSmallBall[1];
                else
                    pEnemyShot->kind = img_enemyShotSmallBall[8];

                pEnemyShot->param_d[0] =
                    (GetRand(628) / 100.0);

                pEnemyShot->param_d[1] =
                    0.15 + GetRand(20) / 100.0;

                pEnemyShot->param_i[0] = 0;

                pEnemyShot->prev =
                    pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next =
                    pEnemyShotSet->pEnemyShotHead;

                pEnemyShotSet->pEnemyShotHead
                    ->prev->next = pEnemyShot;

                pEnemyShotSet->pEnemyShotHead
                    ->prev = pEnemyShot;
            }
        }

        //----------------------------------------------------
        // kind==1 : 砂嵐の目
        //----------------------------------------------------
        else
        {
            //------------------------------------------------
            // 中心
            //------------------------------------------------
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->speed = 1.2;

            pEnemyShot->kind =
                img_enemyShotLargeBall[8];

            // 中心フラグ
            pEnemyShot->param_i[0] = 1;

            // 回転方向
            pEnemyShot->param_i[1] =
                pEnemyShotSet->param_i[0];

            // 蛇行位相
            pEnemyShot->param_d[0] = 0.0;

            pEnemyShot->prev =
                pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next =
                pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->pEnemyShotHead
                ->prev->next = pEnemyShot;

            pEnemyShotSet->pEnemyShotHead
                ->prev = pEnemyShot;

            //------------------------------------------------
            // 渦
            //------------------------------------------------
            for (int i = 0; i < 24; i++)
            {
                pEnemyShot = new sEnemyShot;

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                pEnemyShot->speed = 0.0;
                pEnemyShot->muki = 0.0;

                if (i & 1)
                    pEnemyShot->kind =
                    img_enemyShotSmallBall[1];
                else
                    pEnemyShot->kind =
                    img_enemyShotSmallBall[8];

                // 渦弾
                pEnemyShot->param_i[0] = 2;

                pEnemyShot->param_i[1] =
                    pEnemyShotSet->param_i[0];

                pEnemyShot->param_d[0] =
                    i * DX_PI * 2.0 / 24.0;

                pEnemyShot->param_d[1] = 16.0;

                pEnemyShot->param_d[2] =
                    pEnemyShotSet->x;

                pEnemyShot->param_d[3] =
                    pEnemyShotSet->y;

                pEnemyShot->prev =
                    pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next =
                    pEnemyShotSet->pEnemyShotHead;

                pEnemyShotSet->pEnemyShotHead
                    ->prev->next = pEnemyShot;

                pEnemyShotSet->pEnemyShotHead
                    ->prev = pEnemyShot;
            }
        }
    }

    //--------------------------------------------------------
    // 更新
    //--------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        sEnemyShot* next = pEnemyShot->next;

        //----------------------------------------------------
        // 流れる砂
        //----------------------------------------------------
        if (pEnemyShot->param_i[0] == 0)
        {
            pEnemyShot->param_d[0] +=
                pEnemyShot->param_d[1];

            double dir =
                pEnemyShot->muki +
                sin(pEnemyShot->param_d[0]) * 0.12;

            pEnemyShot->x +=
                cos(dir) * pEnemyShot->speed;

            pEnemyShot->y +=
                sin(dir) * pEnemyShot->speed;
        }

        //----------------------------------------------------
        // 砂嵐の中心
        //----------------------------------------------------
        else if (pEnemyShot->param_i[0] == 1)
        {
            // 蛇行しながら前進
            pEnemyShot->param_d[0] += 0.055;

            double dir =
                pEnemyShot->muki +
                sin(pEnemyShot->param_d[0]) * 0.45;

            pEnemyShot->x +=
                cos(dir) * pEnemyShot->speed;

            pEnemyShot->y +=
                sin(dir) * pEnemyShot->speed;

            //------------------------------------------------
            // 一定間隔で渦の中心座標を更新
            //------------------------------------------------
            if ((pEnemyShot->count % 2) == 0)
            {
                sEnemyShot* p =
                    pEnemyShotSet->pEnemyShotHead->next;

                while (p != pEnemyShotSet->pEnemyShotHead)
                {
                    if (p->param_i[0] == 2)
                    {
                        p->param_d[2] = pEnemyShot->x;
                        p->param_d[3] = pEnemyShot->y;
                    }

                    p = p->next;
                }
            }
        }

        //----------------------------------------------------
        // 渦
        //----------------------------------------------------
        else if (pEnemyShot->param_i[0] == 2)
        {
            double sign =
                (pEnemyShot->param_i[1] == 0) ? 1.0 : -1.0;

            // 回転
            pEnemyShot->param_d[0] +=
                sign * 0.16;

            // 徐々に半径を拡大
            pEnemyShot->param_d[1] += 0.22;

            double r = pEnemyShot->param_d[1];

            pEnemyShot->x =
                pEnemyShot->param_d[2] +
                cos(pEnemyShot->param_d[0]) * r;

            pEnemyShot->y =
                pEnemyShot->param_d[3] +
                sin(pEnemyShot->param_d[0]) * r;

            // 外周まで広がったら自力で飛び出す
            if (r > 85.0)
            {
                pEnemyShot->param_i[0] = 3;

                pEnemyShot->muki =
                    pEnemyShot->param_d[0];

                pEnemyShot->speed = 2.4;
            }
        }

        //----------------------------------------------------
        // 拡散砂粒
        //----------------------------------------------------
        else
        {
            pEnemyShot->x +=
                cos(pEnemyShot->muki) *
                pEnemyShot->speed;

            pEnemyShot->y +=
                sin(pEnemyShot->muki) *
                pEnemyShot->speed;
        }

        pEnemyShot = next;
    }
}

//============================================================
// 敵本体
//============================================================
void EnemyPat_SandStorm_ChatGPT()
{
    static int moveDir;
    static int wind;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 40.0;

        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
        wind = 0;
    }
    else
    {
        enemy.x += moveDir * 0.9;

        if (enemy.x < 90.0)
            moveDir = 1;
        else if (enemy.x > 390.0)
            moveDir = -1;
    }

    //--------------------------------------------------------
    // 突風（砂の流れる向きを変更）
    //--------------------------------------------------------
    if (count % 240 == 0)
        wind ^= 1;

    //--------------------------------------------------------
    // 流れる砂
    //--------------------------------------------------------
    if ((count % 18) == 0)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSandStorm;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 240.0;

        if (wind == 0)
            pEnemyShotSet->param_d[0] = DX_PI * 0.15;   // 左上→右下
        else
            pEnemyShotSet->param_d[0] = DX_PI * 0.65;   // 左下→右上

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

    //--------------------------------------------------------
    // 砂嵐の目
    //--------------------------------------------------------
    if ((count % 150) == 75)
    {
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSandStorm;
        pEnemyShotSet->kind = 1;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;

        pEnemyShotSet->muki =
            atan2(player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

        // 時計回り・反時計回りを交互に
        pEnemyShotSet->param_i[0] = (count / 150) & 1;

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