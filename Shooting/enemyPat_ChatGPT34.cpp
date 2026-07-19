// enemyPat_Match.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void AddShot(
    sEnemyShotSet* set,
    double x,
    double y,
    double muki,
    double speed,
    int kind)
{
    sEnemyShot* s = new sEnemyShot;

    s->x = x;
    s->y = y;
    s->muki = muki;
    s->speed = speed;
    s->kind = kind;

    s->prev = set->pEnemyShotHead->prev;
    s->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = s;
    set->pEnemyShotHead->prev = s;
}

static void ShotMatch(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //----------------------------------------------------------
    // 生成
    //----------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double dir = pEnemyShotSet->muki;

        // 棒
        for (int i = 0; i < 10; i++)
        {
            double d = -54.0 + i * 6.0;

            AddShot(
                pEnemyShotSet,
                pEnemyShotSet->x + cos(dir) * d,
                pEnemyShotSet->y + sin(dir) * d,
                dir,
                2.0,
                img_enemyShotBullet[6]);
        }

        // マッチ頭
        AddShot(
            pEnemyShotSet,
            pEnemyShotSet->x + cos(dir) * 8.0,
            pEnemyShotSet->y + sin(dir) * 8.0,
            dir,
            2.0,
            img_enemyShotLargeBall[0]);
    }

    //----------------------------------------------------------
    // 更新
    //----------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
        pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;

        // 棒を少し回転させる
        if (pEnemyShot->kind == img_enemyShotBullet[6])
        {
            pEnemyShot->muki +=
                sin((pEnemyShotSet->count + pEnemyShot->count) * 0.03) * 0.0015;
        }

        // マッチ頭
        if (pEnemyShot->kind == img_enemyShotLargeBall[0])
        {
            // 擦る位置
            if (pEnemyShot->count == 60)
            {
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                //--------------------------------------------------
                // 火花
                //--------------------------------------------------
                for (int i = 0; i < 20; i++)
                {
                    double a =
                        i * DX_PI * 2.0 / 20.0;

                    AddShot(
                        pEnemyShotSet,
                        pEnemyShot->x,
                        pEnemyShot->y,
                        a,
                        5.2,
                        img_enemyShotSmallBall[1]);
                }

                //--------------------------------------------------
                // 炎発生フラグ
                //--------------------------------------------------
                pEnemyShot->param_i[0] = 1;
            }

            //------------------------------------------------------
            // 燃焼
            //------------------------------------------------------
            if (pEnemyShot->param_i[0])
            {
                if ((pEnemyShot->count % 4) == 0)
                {
                    double a =
                        pEnemyShot->muki
                        + sin(pEnemyShot->count * 0.35) * 0.45;

                    AddShot(
                        pEnemyShotSet,
                        pEnemyShot->x,
                        pEnemyShot->y,
                        a,
                        2.6,
                        img_enemyShotScale[8]);
                }

                if ((pEnemyShot->count % 8) == 0)
                {
                    AddShot(
                        pEnemyShotSet,
                        pEnemyShot->x,
                        pEnemyShot->y,
                        -DX_PI / 2,
                        0.9,
                        img_enemyShotSmallBall[6]);
                }
            }
        }

        pEnemyShot = pEnemyShot->next;
    }
}

//--------------------------------------------------------------
// 敵本体
//--------------------------------------------------------------
void EnemyPat_Match_ChatGPT()
{
    static int dir;
    static int shot;

    if (count == 1)
    {
        enemy.x = 240;
        enemy.y = 48;
        enemy.maxHp = enemy.hp = 200;

        dir = 1;
        shot = 0;
    }
    else
    {
        enemy.x += dir * 0.9;

        if (count % 150 == 75)
            dir = -dir;
    }

    if (count % 80 == 0)
    {
        sEnemyShotSet* set = new sEnemyShotSet;

        set->count = 0;
        set->patternFunc = ShotMatch;

        set->x = enemy.x;
        set->y = enemy.y;

        set->muki =
            atan2(
                player.y - enemy.y,
                player.x - enemy.x);

        set->kind = shot++;

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }
}