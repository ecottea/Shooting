// EnemyPat_Laser_ChatGPT.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void AddShot(
    sEnemyShotSet* pSet,
    double x,
    double y,
    double muki,
    double speed,
    int kind)
{
    sEnemyShot* pShot = new sEnemyShot;

    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ============================================================
// レーザー格子
// ============================================================
static void ShotLaserGrid(sEnemyShotSet* pSet)
{
    // param_i[0]
    // 0 : 縦
    // 1 : 横

    // param_d[0] : 初期座標
    // param_d[1] : 振幅
    // param_d[2] : 位相

    if (pSet->count == 0)
    {
        if (!CheckSoundMem(sound_enemyShot_heavy))
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int color = 4; // 青レーザー

        for (int i = 0; i < 18; i++)
        {
            AddShot(
                pSet,
                pSet->x,
                pSet->y,
                0.0,
                0.0,
                img_enemyShotLaser[color]);
        }
    }

    double ofs =
        sin((pSet->count + pSet->param_d[2]) * 0.02)
        * pSet->param_d[1];

    bool enable;

    if (pSet->param_i[0] == 0)
    {
        // 縦
        enable = ((pSet->count / 60) % 2) == 0;
    }
    else
    {
        // 横
        enable = ((pSet->count / 60 ) % 2) == 0;
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;

    int i = 0;

    while (pShot != pSet->pEnemyShotHead)
    {
        if (enable)
        {
            if (pSet->param_i[0] == 0)
            {
                pShot->x = pSet->param_d[0] + ofs;
                pShot->y = i * 28.0 - 8.0;
                pShot->muki = DX_PI / 2.0;
            }
            else
            {
                pShot->x = i * 28.0 - 8.0;
                pShot->y = pSet->param_d[0] + ofs;
                pShot->muki = 0.0;
            }
        }
        else
        {
            // OFF中は画面外へ退避
            pShot->x = -200.0;
            pShot->y = -200.0;
        }

        ++i;
        pShot = pShot->next;
    }
}

// ============================================================
// 補助弾
// ============================================================
static void ShotSupport(sEnemyShotSet* pSet)
{
    if (pSet->count == 0)
    {
        if (!CheckSoundMem(sound_enemyShot_medium))
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    if (pSet->count % 24 == 0 && pSet->count <= 180)
    {
        double base =
            atan2(
                player.y - pSet->y,
                player.x - pSet->x);

        for (int i = -1; i <= 1; i++)
        {
            AddShot(
                pSet,
                pSet->x,
                pSet->y,
                base + i * DX_PI / 24.0,
                2.4,
                img_enemyShotMediumBall[6]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;

    while (pShot != pSet->pEnemyShotHead)
    {
        pShot->x += cos(pShot->muki) * pShot->speed;
        pShot->y += sin(pShot->muki) * pShot->speed;

        pShot = pShot->next;
    }
}

static void CreateShotSet(
    sEnemyShotSet::PatternFunc func,
    double x,
    double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;

    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

//============================================================
// 敵本体
//============================================================
void EnemyPat_Laser_ChatGPT()
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
        enemy.x += dir * 0.8;

        if (enemy.x < 120.0)
        {
            enemy.x = 120.0;
            dir = 1;
        }
        if (enemy.x > 360.0)
        {
            enemy.x = 360.0;
            dir = -1;
        }
    }

    //--------------------------------------------------------
    // 縦レーザー
    //--------------------------------------------------------
    if (count % 360 == 1) PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    if (count % 360 == 61)
    {
        const double pos[] =
        {
            70.0,
            150.0,
            240.0,
            330.0,
            410.0
        };

        for (int i = 0; i < 5; i++)
        {
            CreateShotSet(ShotLaserGrid, 0.0, 0.0);

            sEnemyShotSet* p =
                enemyShotSetHead.prev;

            p->param_i[0] = 0;
            p->param_d[0] = pos[i];
            p->param_d[1] = 18.0;
            p->param_d[2] = i * 45.0;
        }
    }

    //--------------------------------------------------------
    // 横レーザー
    //--------------------------------------------------------
    if (count % 360 == 180) PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    if (count % 360 == 240)
    {
        const double pos[] =
        {
            80.0,
            150.0,
            240.0,
            330.0,
            400.0
        };

        for (int i = 0; i < 5; i++)
        {
            CreateShotSet(ShotLaserGrid, 0.0, 0.0);

            sEnemyShotSet* p =
                enemyShotSetHead.prev;

            p->param_i[0] = 1;
            p->param_d[0] = pos[i];
            p->param_d[1] = 18.0;
            p->param_d[2] = i * 60.0;
        }
    }

    //--------------------------------------------------------
    // 補助弾
    //--------------------------------------------------------
    if (count % 90 == 0)
    {
        CreateShotSet(
            ShotSupport,
            enemy.x,
            enemy.y + 10.0);
    }
}