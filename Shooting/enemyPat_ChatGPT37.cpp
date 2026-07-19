// EnemyPat_3D_ChatGPT.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static constexpr double PI2 = DX_PI * 2.0;
static constexpr int SPIRAL_NUM = 16;

//------------------------------------------------------------
// 螺旋回廊
//------------------------------------------------------------
static void ShotSpiralCorridor(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //--------------------------------------------------------
    // 初回生成
    //--------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (!CheckSoundMem(sound_enemyShot_medium))
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < SPIRAL_NUM; i++)
        {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            //------------------------------------
            // 仮想奥行き
            //------------------------------------
            pEnemyShot->param_d[0] = i / (double)SPIRAL_NUM;

            //------------------------------------
            // 螺旋角度
            //------------------------------------
            pEnemyShot->param_d[1] =
                PI2 * i / (double)SPIRAL_NUM;

            //------------------------------------
            // 回転速度
            //------------------------------------
            pEnemyShot->param_d[2] = 0.045;

            //------------------------------------
            // 半径
            //------------------------------------
            pEnemyShot->param_d[3] = 6.0;

            //------------------------------------
            // 中心座標保存
            //------------------------------------
            pEnemyShot->param_d[4] = pEnemyShotSet->x;
            pEnemyShot->param_d[5] = pEnemyShotSet->y;

            //------------------------------------
            // 初期画像
            //------------------------------------
            pEnemyShot->kind = img_enemyShotSmallBall[4];

            pEnemyShot->margin = 480.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    //--------------------------------------------------------
    // 更新
    //--------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (pEnemyShotSet->count <= 310) {
            double px = pEnemyShot->x;
            double py = pEnemyShot->y;

            //------------------------------------
            // depth更新
            //------------------------------------
            pEnemyShot->param_d[0] += 0.020;

            if (pEnemyShot->param_d[0] > 1.0)
                pEnemyShot->param_d[0] -= 1.0;

            //------------------------------------
            // 回転
            //------------------------------------
            pEnemyShot->param_d[1] += pEnemyShot->param_d[2];

            double depth = pEnemyShot->param_d[0];

            //------------------------------------
            // 半径
            //------------------------------------
            double radius =
                pEnemyShot->param_d[3] +
                depth * 170.0;

            //------------------------------------
            // 螺旋位置
            //------------------------------------
            pEnemyShot->x =
                pEnemyShot->param_d[4] +
                cos(pEnemyShot->param_d[1] * 0.8) * radius * 2;

            pEnemyShot->y =
                pEnemyShot->param_d[5] +
                sin(pEnemyShot->param_d[1] * 0.8) * radius;

            //------------------------------------
            // 手前ほど高速
            //------------------------------------
            double speed =
                0.3 +
                depth * depth * 7.0;

            pEnemyShot->y += speed * 2.2;

            //------------------------------------
            // 見た目切替
            //------------------------------------
            if (depth < 0.25)
            {
                pEnemyShot->kind = img_enemyShotSmallBall[4];
            }
            else if (depth < 0.55)
            {
                pEnemyShot->kind = img_enemyShotMediumBall[4];
            }
            else
            {
                pEnemyShot->kind = img_enemyShotLargeBall[4];
            }

            double dx = pEnemyShot->x - px;
            double dy = pEnemyShot->y - py;
            pEnemyShot->speed = sqrt(dx * dx + dy * dy);
            pEnemyShot->muki = atan2(dy, dx);
        }
        else {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_3D_ChatGPT()
{
    static int moveDir;
    static int shotTimer;
    static double spiralAngle;

    //--------------------------------------------------------
    // 初期化
    //--------------------------------------------------------
    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 140.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        moveDir = 1;
        shotTimer = 0;
        spiralAngle = 0.0;
    }
    else
    {
        //----------------------------------------------------
        // 左右へゆっくり移動
        //----------------------------------------------------
        enemy.x += moveDir * 0.8;

        if (enemy.x < 120.0)
            moveDir = 1;
        else if (enemy.x > 360.0)
            moveDir = -1;
    }

    //--------------------------------------------------------
    // ShotSet生成
    //--------------------------------------------------------
    if ((count % 50) == 1)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiralCorridor;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // この弾幕では使用しないが初期化しておく
        pEnemyShotSet->muki = 0.0;

        // 発射毎に螺旋開始角を少しずつずらす
        pEnemyShotSet->param_d[0] = spiralAngle;

        spiralAngle += DX_PI / 18.0;
        if (spiralAngle >= PI2)
            spiralAngle -= PI2;

        pEnemyShotSet->kind = shotTimer++;

        //----------------------------------------------------
        // ダミーヘッド生成
        //----------------------------------------------------
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;

        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        //----------------------------------------------------
        // ShotSetをリストへ追加
        //----------------------------------------------------
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}