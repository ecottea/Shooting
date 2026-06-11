#include "DxLib.h"
#include "gv.h"
#include <math.h>

// ------------------------------------------------------------
// 波と粒の境界
// ------------------------------------------------------------

static void ShotWaveParticleBoundary(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;

    // 発射時
    if (pSet->count == 0)
    {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int NUM = 41;

        for (int i = 0; i < NUM; i++)
        {
            pShot = new sEnemyShot;

            double ofs = (i - (NUM - 1) / 2.0);

            pShot->x = pSet->x;
            pShot->y = pSet->y;

            pShot->muki = pSet->muki;
            pShot->speed = 0.0;

            if (abs((int)ofs) <= 4)
                pShot->kind = img_enemyShotMediumBall[6]; // 白
            else
                pShot->kind = img_enemyShotSmallBall[4];  // 青

            // x に初期位置を保存
            pShot->x += ofs * 12.0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    int idx = 0;

    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead)
    {
        double phase = idx * 0.32;

        // -------------------------
        // 波の領域
        // -------------------------
        if (pSet->count < 120)
        {
            pShot->y += 1.8;

            double wave =
                sin(pSet->count * 0.09 + phase) *
                (8.0 + pSet->count * 0.12);

            pShot->x +=
                cos(pSet->count * 0.03 + phase) * 0.4;

            pShot->x += wave * 0.08;
        }
        // -------------------------
        // 境界領域
        // -------------------------
        else if (pSet->count < 180)
        {
            double t = (pSet->count - 120) / 60.0;

            double angle =
                phase +
                t * t * 6.0;

            pShot->x += cos(angle) * (1.0 + t * 2.0);
            pShot->y += 1.0 + sin(angle) * 1.5;
        }
        // -------------------------
        // 粒の領域
        // -------------------------
        else
        {
            double angle =
                phase * 2.0 +
                pSet->count * 0.015;

            pShot->x += cos(angle) * 2.4;
            pShot->y += sin(angle) * 2.4 + 1.2;
        }

        idx++;
        pShot = pShot->next;
    }
}


void EnemyPat_Namistubu_ChatGPT()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 80;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

if (count % 120 == 30)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;

    pSet->count = 0;
    pSet->patternFunc = ShotWaveParticleBoundary;

    pSet->x = enemy.x;
    pSet->y = enemy.y + 20;

    pSet->muki = DX_PI / 2.0;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}
}