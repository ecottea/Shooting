#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 蜂の巣を形成する停止弾
static void ShotHoneycomb(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //=========================================================
    // 初期生成
    //=========================================================
    if (pEnemyShotSet->count == 0) {

        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 六角格子
        const int ROW = 9;
        const int COL = 16;
        const double DX = 42.0;
        const double DY = 36.0;

        for (int y = 0; y < ROW; y++) {
            for (int x = 0; x < COL; x++) {

                pEnemyShot = new sEnemyShot;

                double tx = 240.0
                    + (x - (COL - 1) * 0.5) * DX
                    + ((y & 1) ? DX * 0.5 : 0.0);

                double ty = 90.0 + y * DY;

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                pEnemyShot->muki = atan2(
                    ty - pEnemyShotSet->y,
                    tx - pEnemyShotSet->x);

                pEnemyShot->speed = 2.4;

                // 黄色中玉
                pEnemyShot->kind = img_enemyShotMediumBall[1];

                // 停止位置
                pEnemyShot->param_d[0] = tx;
                pEnemyShot->param_d[1] = ty;

                // 0:移動中 1:停止
                pEnemyShot->param_i[0] = 0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    //=========================================================
    // 更新
    //=========================================================
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShot->param_i[0] == 0) {

            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            double dx = pEnemyShot->param_d[0] - pEnemyShot->x;
            double dy = pEnemyShot->param_d[1] - pEnemyShot->y;

            // 目的地到達
            if (dx * dx + dy * dy < 9.0) {
                pEnemyShot->x = pEnemyShot->param_d[0];
                pEnemyShot->y = pEnemyShot->param_d[1];

                pEnemyShot->speed = 0.0;
                pEnemyShot->param_i[0] = 1;
            }
        }
        else if (pEnemyShot->param_i[0] == 1) {
            if (pEnemyShot->count == 150) {
                pEnemyShot->kind = img_enemyShotMediumBall[8];
                pEnemyShot->param_i[0] = 2;
            }
        }
        else if (pEnemyShot->param_i[0] == 2) {
            pEnemyShot->speed += 0.01;
            if (pEnemyShot->speed > 1) pEnemyShot->speed = 1;

            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}

void EnemyPat_Bee_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.9 * muki;

        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    //----------------------------------------------------------
    // 蜂の巣を展開
    //----------------------------------------------------------
    if (count % 150 == 1) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHoneycomb;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    //----------------------------------------------------------
    // 蜂（働き蜂）3WAY
    //----------------------------------------------------------
    if (count % 24 == 1) {

        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc =
            [](sEnemyShotSet* set)
        {
            if (set->count == 0) {

                double base =
                    atan2(player.y - set->y,
                        player.x - set->x);

                for (int i = -1; i <= 1; i++) {

                    sEnemyShot* p = new sEnemyShot;

                    p->x = set->x;
                    p->y = set->y;

                    p->muki = base + i * (12.0 / 180.0 * DX_PI);
                    p->speed = 4.2;

                    p->kind = img_enemyShotSmallBall[1];

                    p->prev = set->pEnemyShotHead->prev;
                    p->next = set->pEnemyShotHead;
                    set->pEnemyShotHead->prev->next = p;
                    set->pEnemyShotHead->prev = p;
                }
            }

            sEnemyShot* p = set->pEnemyShotHead->next;

            while (p != set->pEnemyShotHead) {

                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);

                p = p->next;
            }
        };

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}