// enemyPat_BlockBreaker.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const int BLOCK_W = 8;
static const int BLOCK_H = 5;
static const int BLOCK_NUM = BLOCK_W * BLOCK_H;

//------------------------------------------------------------
// 小玉生成
//------------------------------------------------------------
static void SpawnSmall(
    sEnemyShotSet* set,
    double x,
    double y,
    double dir,
    double spd,
    int color)
{
    sEnemyShot* s = new sEnemyShot;

    s->x = x;
    s->y = y;
    s->muki = dir;
    s->speed = spd;
    s->kind = img_enemyShotSmallBall[color];

    s->prev = set->pEnemyShotHead->prev;
    s->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = s;
    set->pEnemyShotHead->prev = s;
}

//------------------------------------------------------------
// ブロック(中玉)
//------------------------------------------------------------
static void SpawnBlock(
    sEnemyShotSet* set,
    double x,
    double y)
{
    sEnemyShot* s = new sEnemyShot;

    s->x = x;
    s->y = y;
    s->speed = 0.0;
    s->kind = img_enemyShotMediumBall[1];

    // 種類
    // 0=ブロック
    // 1=ボール
    s->param_i[0] = 0;

    s->prev = set->pEnemyShotHead->prev;
    s->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = s;
    set->pEnemyShotHead->prev = s;
}

//------------------------------------------------------------
// ボール(大玉)
//------------------------------------------------------------
static void SpawnBall(
    sEnemyShotSet* set,
    double x,
    double y,
    double dir,
    double spd)
{
    sEnemyShot* s = new sEnemyShot;

    s->x = x;
    s->y = y;
    s->muki = dir;
    s->speed = spd;
    s->kind = img_enemyShotLargeBall[6];

    s->param_i[0] = 1;

    s->prev = set->pEnemyShotHead->prev;
    s->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = s;
    set->pEnemyShotHead->prev = s;
}

//------------------------------------------------------------
// ブロック崩し弾幕
//------------------------------------------------------------
static void ShotBlockBreaker(sEnemyShotSet* set)
{
    if (set->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);

        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        //----------------------------------------------------
        // ブロック配置
        //----------------------------------------------------
        for (int y = 0; y < BLOCK_H; y++)
        {
            for (int x = 0; x < BLOCK_W; x++)
            {
                SpawnBlock(
                    set,
                    72.0 + x * 48.0,
                    60.0 + y * 32.0);
            }
        }

        //----------------------------------------------------
        // ボール3個
        //----------------------------------------------------
        SpawnBall(set, 240, 150, DX_PI / 4.0, 2.7);
        SpawnBall(set, 240, 150, DX_PI * 3.0 / 4.0, 2.7);
        SpawnBall(set, 240, 150, DX_PI / 3.0, 2.7);
    }

    //----------------------------------------------------
    // ボール移動・壁反射
    //----------------------------------------------------
    sEnemyShot* ball = set->pEnemyShotHead->next;

    while (ball != set->pEnemyShotHead)
    {
        if (ball->param_i[0] == 1)
        {
            ball->x += cos(ball->muki) * ball->speed;
            ball->y += sin(ball->muki) * ball->speed;

            if (ball->x < 12.0)
            {
                ball->x = 12.0;
                ball->muki = DX_PI - ball->muki;
            }
            else if (ball->x > 468.0)
            {
                ball->x = 468.0;
                ball->muki = DX_PI - ball->muki;
            }

            if (ball->y < 12.0)
            {
                ball->y = 12.0;
                ball->muki = -ball->muki;
            }
        }

        ball = ball->next;
    }

    //----------------------------------------------------
    // ボールとブロックの当たり判定
    //----------------------------------------------------
    ball = set->pEnemyShotHead->next;

    while (ball != set->pEnemyShotHead)
    {
        if (ball->param_i[0] != 1)
        {
            ball = ball->next;
            continue;
        }

        sEnemyShot* block = set->pEnemyShotHead->next;

        while (block != set->pEnemyShotHead)
        {
            sEnemyShot* nextBlock = block->next;

            if (block->param_i[0] == 0)
            {
                double dx = ball->x - block->x;
                double dy = ball->y - block->y;

                if (dx * dx + dy * dy < 18.0 * 18.0)
                {
                    // 反射
                    if (fabs(dx) > fabs(dy))
                        ball->muki = DX_PI - ball->muki;
                    else
                        ball->muki = -ball->muki;

                    if (CheckSoundMem(sound_enemyShot_noize))
                        StopSoundMem(sound_enemyShot_noize);

                    PlaySoundMem(sound_enemyShot_noize, DX_PLAYTYPE_BACK);

                    //------------------------------------------------
                    // ブロック破壊時の十字弾
                    //------------------------------------------------
                    for (int i = 0; i < 4; i++)
                    {
                        SpawnSmall(
                            set,
                            block->x,
                            block->y,
                            DX_PI / 2.0 * i,
                            2.4,
                            8);
                    }

                    //------------------------------------------------
                    // ブロック削除
                    //------------------------------------------------
                    block->prev->next = block->next;
                    block->next->prev = block->prev;
                    delete block;

                    break;
                }
            }

            block = nextBlock;
        }

        ball = ball->next;
    }

    //----------------------------------------------------
    // ブロック残数を数える
    //----------------------------------------------------
    int blockCount = 0;

    sEnemyShot* p = set->pEnemyShotHead->next;
    while (p != set->pEnemyShotHead)
    {
        if (p->param_i[0] == 0)
            blockCount++;

        p = p->next;
    }

    //----------------------------------------------------
    // 半分以下になったらボール追加（1回だけ）
    //----------------------------------------------------
    if (blockCount <= BLOCK_NUM / 2 && set->param_i[0] == 0)
    {
        set->param_i[0] = 1;

        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);

        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        SpawnBall(set, 240.0, 150.0, DX_PI * 5.0 / 6.0, 3.6);
        SpawnBall(set, 240.0, 150.0, DX_PI * 1.0 / 6.0, 3.6);

        sEnemyShot* q = set->pEnemyShotHead->next;
        while (q != set->pEnemyShotHead)
        {
            if (q->param_i[0] == 1)
                q->speed += 0.8;

            q = q->next;
        }
    }

    sEnemyShot* pShot = set->pEnemyShotHead->next;
    while (pShot != set->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_BlockBreak_ChatGPT()
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
        enemy.x += dir * 1.0;

        if (enemy.x < 80.0)
            dir = 1;
        else if (enemy.x > 400.0)
            dir = -1;
    }

    //----------------------------------------------------
    // 弾幕生成（1回だけ）
    //----------------------------------------------------
    if (count == 1)
    {
        sEnemyShotSet* set = new sEnemyShotSet;

        set->count = 0;
        set->patternFunc = ShotBlockBreaker;
        set->x = enemy.x;
        set->y = enemy.y;
        set->muki = 0.0;
        set->kind = 0;

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }
}