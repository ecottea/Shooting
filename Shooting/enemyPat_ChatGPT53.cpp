// enemyPat_tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

struct LetterPoint {
    short x;
    short y;
};

//----------------------------------------------
// A
//----------------------------------------------
static const LetterPoint LetterA[] = {
    {-24,24},{-20,16},{-16,8},{-12,0},{-8,-8},{-4,-16},{0,-24},
    {24,24},{20,16},{16,8},{12,0},{8,-8},{4,-16},
    {-12,0},{-8,0},{-4,0},{0,0},{4,0},{8,0},{12,0},
};

//----------------------------------------------
// H
//----------------------------------------------
static const LetterPoint LetterH[] = {
    {-20,-20},{-20,-12},{-20,-4},{-20,4},{-20,12},{-20,20},
    {20,-20},{20,-12},{20,-4},{20,4},{20,12},{20,20},
    {-16,0},{-8,0},{0,0},{8,0},{16,0},
};

//----------------------------------------------
// O
//----------------------------------------------
static const LetterPoint LetterO[] = {
    {-16,-20},{0,-24},{16,-20},
    {-24,-8},{24,-8},
    {-24,8},{24,8},
    {-16,20},{0,24},{16,20},
};

//----------------------------------------------
// X
//----------------------------------------------
static const LetterPoint LetterX[] = {
    {-20,-20},{-12,-12},{-4,-4},{4,4},{12,12},{20,20},
    {20,-20},{12,-12},{4,-4},{-4,4},{-12,12},{-20,20},
};

struct LetterInfo {
    const LetterPoint* p;
    int n;
};

static const LetterInfo gLetterTbl[] = {
    {LetterA,sizeof(LetterA) / sizeof(LetterPoint)},
    {LetterH,sizeof(LetterH) / sizeof(LetterPoint)},
    {LetterO,sizeof(LetterO) / sizeof(LetterPoint)},
    {LetterX,sizeof(LetterX) / sizeof(LetterPoint)},
};

//--------------------------------------------------

static void ShotLetter(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {

        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const LetterInfo& info =
            gLetterTbl[pEnemyShotSet->kind %
            (sizeof(gLetterTbl) / sizeof(LetterInfo))];

        for (int i = 0; i < info.n; i++) {

            sEnemyShot* pShot = new sEnemyShot;

            pShot->x = pEnemyShotSet->x + info.p[i].x * 3.0;
            pShot->y = pEnemyShotSet->y + info.p[i].y * 3.0;

            pShot->speed = 0.0;
            pShot->muki = DX_PI / 2.0;

            pShot->kind = img_enemyShotSmallBall[6];

            // 相対位置保存
            pShot->param_d[0] = info.p[i].x * 3.0;
            pShot->param_d[1] = info.p[i].y * 3.0;

            // 爆散フラグ
            pShot->param_i[0] = 0;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    //------------------------------------------------
    // 文字全体を落下
    //------------------------------------------------
    pEnemyShotSet->y += 0.65;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (pShot->param_i[0] == 0) {

            pShot->x = pEnemyShotSet->x + pShot->param_d[0];
            pShot->y = pEnemyShotSet->y + pShot->param_d[1];

            // 約2.8秒後に爆散開始
            if (pEnemyShotSet->count == 170) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                double a = atan2(
                    player.y - pShot->y,
                    player.x - pShot->x);

                a += (GetRand(60) - 30) / 180.0 * DX_PI;

                pShot->muki = a;
                pShot->speed = 2.8;
                pShot->param_i[0] = 1;
                pShot->kind = img_enemyShotMediumBall[0];
            }
        }
        else {

            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;
        }

        pShot = pShot->next;
    }
}

//--------------------------------------------------------------
// 敵本体
//--------------------------------------------------------------
void EnemyPat_Daimonji_ChatGPT()
{
    static int moveDir;
    static int letterIndex;

    if (count == 1) {

        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
        letterIndex = 0;
    }
    else {

        enemy.x += moveDir * 0.90;

        if (enemy.x < 90.0) {
            enemy.x = 90.0;
            moveDir = 1;
        }
        if (enemy.x > 390.0) {
            enemy.x = 390.0;
            moveDir = -1;
        }
    }

    //----------------------------------------------------------
    // 一定間隔で文字を落下
    //----------------------------------------------------------
    if (count % 90 == 1) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLetter;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // A → H → O → X → ...
        pEnemyShotSet->kind = letterIndex++;

        pEnemyShotSet->muki = DX_PI / 2.0;

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

    //----------------------------------------------------------
    // 中盤以降は生成間隔を短縮
    //----------------------------------------------------------
    if (count > 600 && count % 60 == 1) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLetter;

        pEnemyShotSet->x = enemy.x + GetRand(120) - 60;
        pEnemyShotSet->y = enemy.y + 20.0;

        pEnemyShotSet->kind = letterIndex++;
        pEnemyShotSet->muki = DX_PI / 2.0;

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