// enemyPat_Invader.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace
{

    // インベーダー(8x8)
    const int Invader[8][8] =
    {
        {0,0,1,1,1,1,0,0},
        {0,1,1,1,1,1,1,0},
        {1,1,0,1,1,0,1,1},
        {1,1,1,1,1,1,1,1},
        {1,0,1,1,1,1,0,1},
        {1,1,1,0,0,1,1,1},
        {0,1,0,0,0,0,1,0},
        {1,0,1,0,0,1,0,1},
    };

    const double DOT = 6.0;
    const double INV_W = 8.0 * DOT;
    const double INV_H = 8.0 * DOT;

}

//---------------------------------------------------------
// 編隊弾幕
//---------------------------------------------------------
static void ShotInvader(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //-----------------------------------------------------
    // 初期生成
    //-----------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int COL = 6;
        const double GAP = 18.0;

        for (int c = 0; c < COL; c++)
        {
            double baseX =
                pEnemyShotSet->x
                + (c - (COL - 1) / 2.0) * (INV_W + GAP);

            double baseY = pEnemyShotSet->y;

            for (int iy = 0; iy < 8; iy++)
            {
                for (int ix = 0; ix < 8; ix++)
                {
                    if (!Invader[iy][ix]) continue;

                    pEnemyShot = new sEnemyShot;

                    pEnemyShot->x = baseX + (ix - 3.5) * DOT;
                    pEnemyShot->y = baseY + iy * DOT;

                    pEnemyShot->speed = 0.0;
                    pEnemyShot->muki = 0.0;

                    // 白い小玉でドット絵を表現
                    pEnemyShot->kind = img_enemyShotSmallBall[6];

                    // 所属インベーダー
                    pEnemyShot->param_i[0] = c;

                    // ドット座標
                    pEnemyShot->param_i[1] = ix;
                    pEnemyShot->param_i[2] = iy;

                    // 射撃担当候補（最下段）
                    pEnemyShot->param_i[3] = 1;
                    for (int yy = iy + 1; yy < 8; yy++)
                    {
                        if (Invader[yy][ix])
                        {
                            pEnemyShot->param_i[3] = 0;
                            break;
                        }
                    }

                    pEnemyShot->margin = 100;

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }

        // 編隊全体の状態
        pEnemyShotSet->param_d[0] = 0.0;   // 横オフセット
        pEnemyShotSet->param_d[1] = 0.0;   // 縦オフセット
        pEnemyShotSet->param_i[0] = 1;     // 移動方向
    }

    //-----------------------------------------------------
    // 編隊移動
    //-----------------------------------------------------
    const double LIMIT = 120.0;
    const double SPEED = 1.2;

    pEnemyShotSet->param_d[0] +=
        SPEED * pEnemyShotSet->param_i[0];

    if (fabs(pEnemyShotSet->param_d[0]) > LIMIT)
    {
        pEnemyShotSet->param_i[0] *= -1;
        pEnemyShotSet->param_d[1] += 24.0;
    }

    //-----------------------------------------------------
    // 各ドットを隊列位置へ配置
    //-----------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (pEnemyShot->kind == img_enemyShotSmallBall[6]) {
            int c = pEnemyShot->param_i[0];
            int ix = pEnemyShot->param_i[1];
            int iy = pEnemyShot->param_i[2];

            double baseX =
                pEnemyShotSet->x
                + (c - 2.5) * (INV_W + 18.0);

            pEnemyShot->x =
                baseX
                + (ix - 3.5) * DOT
                + pEnemyShotSet->param_d[0];

            pEnemyShot->y =
                pEnemyShotSet->y
                + iy * DOT
                + pEnemyShotSet->param_d[1];
        }

        pEnemyShot = pEnemyShot->next;
    }

    //-----------------------------------------------------
    // 侵略射撃
    //-----------------------------------------------------
    if (pEnemyShotSet->count % 10 == 0)
    {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int targetCol = GetRand(5);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
        {
            if (pEnemyShot->param_i[0] == targetCol &&
                pEnemyShot->param_i[3])
            {
                sEnemyShot* shot = new sEnemyShot;

                shot->x = pEnemyShot->x;
                shot->y = pEnemyShot->y + 4.0;

                shot->muki = DX_PI / 2.0;
                shot->speed = 4.8;

                // 赤色レーザー
                shot->kind = img_enemyShotLaser[0];
                shot->margin = 100;

                shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                shot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = shot;
                pEnemyShotSet->pEnemyShotHead->prev = shot;

                break;
            }

            pEnemyShot = pEnemyShot->next;
        }
    }

    //-----------------------------------------------------
    // 爆散開始
    //-----------------------------------------------------
    if (pEnemyShotSet->count == 300)
    {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
        {
            // レーザーはそのまま落下させる
            if (pEnemyShot->kind != img_enemyShotLaser[0])
            {
                double dx = pEnemyShot->x - enemy.x;
                double dy = pEnemyShot->y - enemy.y;

                pEnemyShot->muki = atan2(dy, dx);
                pEnemyShot->speed = 2.8;
                pEnemyShot->param_i[7] = 1;      // 爆散モード
            }

            pEnemyShot = pEnemyShot->next;
        }
    }

    //-----------------------------------------------------
    // 爆散後の移動
    //-----------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (pEnemyShot->param_i[7])
        {
            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;
        }
        else if (pEnemyShot->kind == img_enemyShotLaser[0])
        {
            // 縦レーザーのみ通常移動
            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

//---------------------------------------------------------
// 敵本体
//---------------------------------------------------------
void EnemyPat_Invader_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 40.0;

        enemy.maxHp = enemy.hp = 200;

        dir = 1;
    }
    else
    {
        // 緩やかな左右移動
        enemy.x += dir * 0.8;

        if (enemy.x > 340.0) dir = -1;
        if (enemy.x < 140.0) dir = 1;
    }

    // 編隊は一度だけ生成
    if (count == 30)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotInvader;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 40.0;
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

    // 約6秒ごとに新しい編隊を投入
    if (count > 30 && count % 360 == 30)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotInvader;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 40.0;
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