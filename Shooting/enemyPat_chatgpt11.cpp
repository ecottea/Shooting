// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotHakaiBeam(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //=================================================
    // 初回生成
    //=================================================
    if (pEnemyShotSet->count == 0)
    {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // チャージ球
        for (int i = 0; i < 64; i++)
        {
            pEnemyShot = new sEnemyShot;

            double r = 40.0 + GetRand(180);
            double a = DX_PI * 2.0 * i / 32.0;

            pEnemyShot->x = pEnemyShotSet->x + cos(a) * r;
            pEnemyShot->y = pEnemyShotSet->y + sin(a) * r;

            pEnemyShot->muki = a + DX_PI;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotSmallBall[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    //=================================================
    // チャージ演出
    //=================================================
    if (pEnemyShotSet->count < 60)
    {
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
        {
            double dx = pEnemyShotSet->x - pEnemyShot->x;
            double dy = pEnemyShotSet->y - pEnemyShot->y;

            pEnemyShot->x += dx * 0.06;
            pEnemyShot->y += dy * 0.06;            

            pEnemyShot = pEnemyShot->next;
        }
    }

    //=================================================
    // はかいこうせん発射
    //=================================================
    if (pEnemyShotSet->count == 60)
    {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 極太レーザー本体
        for (int d = 0; d < 90; d++)
        {
            for (int w = -3; w <= 3; w++)
            {
                pEnemyShot = new sEnemyShot;

                double px = cos(pEnemyShotSet->muki);
                double py = sin(pEnemyShotSet->muki);

                double nx = -py;
                double ny = px;

                pEnemyShot->x =
                    pEnemyShotSet->x +
                    px * d * 8.0 +
                    nx * w * 10.0;

                pEnemyShot->y =
                    pEnemyShotSet->y +
                    py * d * 8.0 +
                    ny * w * 10.0;

                pEnemyShot->muki = pEnemyShotSet->muki;
                pEnemyShot->speed = 3.0;

                if (abs(w) <= 1)
                    pEnemyShot->kind = img_enemyShotLargeBall[6];
                else
                    pEnemyShot->kind = img_enemyShotLargeBall[3];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    //=================================================
    // 照射中の掃射
    //=================================================
    if (pEnemyShotSet->count >= 60)
    {
        double target =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

        double diff = target - pEnemyShotSet->muki;

        while (diff > DX_PI) diff -= DX_PI * 2.0;
        while (diff < -DX_PI) diff += DX_PI * 2.0;

        pEnemyShotSet->muki += diff * 0.01;

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
        {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShotSet->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShotSet->muki);

            if (pEnemyShot->speed <= 1e-12) {
                pEnemyShot->x = 9999;
                pEnemyShot->y = 9999;
            }

            pEnemyShot = pEnemyShot->next;
        }

        // プラズマ飛沫
        if ((pEnemyShotSet->count % 2) == 0 && pEnemyShotSet->count <= 180)
        {
            for (int i = 0; i < 4; i++)
            {
                double dist = GetRand(700);

                double px = cos(pEnemyShotSet->muki);
                double py = sin(pEnemyShotSet->muki);

                double nx = -py;
                double ny = px;

                double side = GetRand(60) - 30;

                pEnemyShot = new sEnemyShot;

                pEnemyShot->x =
                    pEnemyShotSet->x +
                    px * dist +
                    nx * side;

                pEnemyShot->y =
                    pEnemyShotSet->y +
                    py * dist +
                    ny * side;

                pEnemyShot->muki =
                    pEnemyShotSet->muki +
                    (GetRand(120) - 60) / 180.0 * DX_PI;

                pEnemyShot->speed =
                    1.0 + GetRand(200) / 100.0;

                pEnemyShot->kind = img_enemyShotMediumBall[3];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
}

//=========================================================
// 敵本体
//=========================================================
void EnemyPat_Hakaikousen_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        dir = 1;
    }

    enemy.x += dir * 1.2;

    if (enemy.x > 400) dir = -1;
    if (enemy.x < 80)  dir = 1;

    if (count % 180 == 30)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHakaiBeam;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

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