// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>


static void ShotChargeRush(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    //==============================
    // 発生
    //==============================
    if (pEnemyShotSet->count == 0) {

        // 予告音
        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const int NUM = 48;

        for (int i = 0; i < NUM; i++) {

            pShot = new sEnemyShot;

            double a = DX_PI * 2.0 * i / NUM;

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            pShot->muki = a;
            pShot->speed = 0.0;

            pShot->kind = img_enemyShotMediumBall[6];

            // param_d[0] = 半径
            // param_d[1] = 角度
            pShot->param_d[0] = 10.0;
            pShot->param_d[1] = a;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    //--------------------------------------------------------
    // リング展開
    //--------------------------------------------------------
    if (pEnemyShotSet->count < 60) {

        pShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pShot != pEnemyShotSet->pEnemyShotHead) {

            pShot->param_d[0] += 0.7;
            pShot->param_d[1] += 0.05;

            pShot->x =
                enemy.x +
                cos(pShot->param_d[1]) * pShot->param_d[0];

            pShot->y =
                enemy.y +
                sin(pShot->param_d[1]) * pShot->param_d[0];

            pShot = pShot->next;
        }

        return;
    }

    //--------------------------------------------------------
    // リング解放
    //--------------------------------------------------------
    if (pEnemyShotSet->count == 60) {

        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pShot != pEnemyShotSet->pEnemyShotHead) {

            pShot->muki = pShot->param_d[1];
            pShot->speed = 2.8;

            pShot = pShot->next;
        }
    }

    //--------------------------------------------------------
    // 通常移動
    //--------------------------------------------------------
    pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        pShot->x += cos(pShot->muki) * pShot->speed;
        pShot->y += sin(pShot->muki) * pShot->speed;

        pShot = pShot->next;
    }
}

//-----------------------------------------
// 衝撃波
//-----------------------------------------
static void ShotImpactWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    if (pEnemyShotSet->count == 0) {

        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int NUM = 19;

        for (int i = 0; i < NUM; i++) {

            pShot = new sEnemyShot;

            double a = pEnemyShotSet->muki
                + (i - (NUM - 1) / 2.0) * DX_PI / 24.0;

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = a;
            pShot->speed = 3.8;

            pShot->kind = img_enemyShotLaser[1];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        pShot->x += cos(pShot->muki) * pShot->speed;
        pShot->y += sin(pShot->muki) * pShot->speed;

        pShot = pShot->next;
    }
}

//-----------------------------------------
// 突進終了時の衝撃波
//-----------------------------------------
static void CreateImpactWave(double x, double y, double dir)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotImpactWave;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = dir;
    pEnemyShotSet->kind = 7;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

//======================================================
// 敵本体
//======================================================
void EnemyPat_Tackle_ChatGPT()
{
    enum
    {
        MOVE,
        CHARGE,
        RUSH,
        RETURN
    };

    static int state;
    static int timer;

    static double targetX;
    static double targetY;

    static double vx;
    static double vy;

    static double rushDir;
    if (count == 1) {

        enemy.x = 240.0;
        enemy.y = 50.0;

        enemy.maxHp = enemy.hp = 200;

        state = MOVE;
        timer = 0;

        targetX = targetY = 0.0;
        vx = vy = 0.0;
        rushDir = 0.0;
    }

    timer++;

    switch (state) {

        //--------------------------------------------------
        // 通常移動
        //--------------------------------------------------
    case MOVE:

        enemy.x = 240.0 + sin(count / 90.0) * 120.0;
        enemy.y = 50.0 + sin(count / 45.0) * 12.0;

        if (timer >= 150) {

            timer = 0;
            state = CHARGE;

            targetX = player.x;
            targetY = player.y;

            rushDir = atan2(
                targetY - enemy.y,
                targetX - enemy.x);

            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotChargeRush;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = 0;

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

        break;

        //--------------------------------------------------
        // 溜め
        //--------------------------------------------------
    case CHARGE:

        if (timer >= 60) {

            timer = 0;
            state = RUSH;

            vx = cos(rushDir) * 8.5;
            vy = sin(rushDir) * 8.5;
        }

        break;

        //--------------------------------------------------
        // 突進
        //--------------------------------------------------
    case RUSH:

        enemy.x += vx;
        enemy.y += vy;

        if (timer >= 45) {

            timer = 0;
            state = RETURN;

            CreateImpactWave(
                enemy.x,
                enemy.y,
                rushDir + DX_PI / 2.0);

            CreateImpactWave(
                enemy.x,
                enemy.y,
                rushDir - DX_PI / 2.0);
        }

        break;

        //--------------------------------------------------
        // 中央へ復帰
        //--------------------------------------------------
    case RETURN:
    {
        double tx = 240.0;
        double ty = 50.0;

        double dx = tx - enemy.x;
        double dy = ty - enemy.y;

        double d = sqrt(dx * dx + dy * dy);

        if (d < 4.0) {

            enemy.x = tx;
            enemy.y = ty;

            timer = 0;
            state = MOVE;
        }
        else {

            enemy.x += dx / d * 3.2;
            enemy.y += dy / d * 3.2;
        }

        break;
    }

    }
}