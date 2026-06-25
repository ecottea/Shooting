// EnemyPat_Sierpinski_ChatGPT.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotSierpinski(sEnemyShotSet* pEnemyShotSet)
{
    // --------------------------------------------------
    // 初期生成
    // --------------------------------------------------
    if (pEnemyShotSet->count == 0) {

        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double R = 240.0;

        for (int i = 0; i < 3; i++) {

            sEnemyShot* pEnemyShot = new sEnemyShot;

            double ang = -DX_PI / 2.0 + DX_PI * 2.0 / 3.0 * i;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = ang;
            pEnemyShot->speed = 2.5;

            pEnemyShot->kind = img_enemyShotLargeBall[4];

            // param_i[0] = 頂点番号
            // param_i[1] = 深さ
            pEnemyShot->param_i[0] = i;
            pEnemyShot->param_i[1] = 0;

            // 目標位置
            pEnemyShot->param_d[0] = pEnemyShotSet->x + cos(ang) * R;
            pEnemyShot->param_d[1] = pEnemyShotSet->y + sin(ang) * R;

            pEnemyShot->margin = 999;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --------------------------------------------------
    // 更新
    // --------------------------------------------------
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // -------------------------------
        // 停止前
        // -------------------------------
        if (pEnemyShot->param_i[2] == 0) {

            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            double dx = pEnemyShot->param_d[0] - pEnemyShot->x;
            double dy = pEnemyShot->param_d[1] - pEnemyShot->y;

            if (dx * dx + dy * dy < 16.0) {

                pEnemyShot->x = pEnemyShot->param_d[0];
                pEnemyShot->y = pEnemyShot->param_d[1];

                pEnemyShot->speed = 0.0;
                pEnemyShot->param_i[2] = 1;
                pEnemyShot->count = 0;
            }
        }
        // -------------------------------
        // 停止後
        // -------------------------------
        else {

            int depth = pEnemyShot->param_i[1];

            if (depth < 5 && pEnemyShot->count == 20) {

                double scale = 200.0 / pow(1.5, depth);

                for (int k = 0; k < 2; k++) {

                    sEnemyShot* pChild = new sEnemyShot;

                    double ang;

                    if (k == 0)
                        ang = pEnemyShot->muki + DX_PI * 2.0 / 3.0;
                    else
                        ang = pEnemyShot->muki - DX_PI * 2.0 / 3.0;

                    pChild->x = pEnemyShot->x;
                    pChild->y = pEnemyShot->y;

                    pChild->muki = ang;
                    pChild->speed = 2.0;

                    pChild->kind = img_enemyShotMediumBall[
                        (depth + 5) % 6
                    ];

                    pChild->param_i[0] = k;
                    pChild->param_i[1] = depth + 1;

                    pChild->param_d[0] =
                        pChild->x + cos(ang) * scale;
                    pChild->param_d[1] =
                        pChild->y + sin(ang) * scale;

                    pChild->margin = 999;

                    pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pChild->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
                    pEnemyShotSet->pEnemyShotHead->prev = pChild;
                }
            }
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// --------------------------------------------------
// 敵本体
// --------------------------------------------------
void EnemyPat_Sierpinski_ChatGPT()
{
    static int dir_x;
    static int dir_y;

    if (count == 1) {

        enemy.x = 240.0;
        enemy.y = 160.0;

        enemy.maxHp = 200;
        enemy.hp = enemy.maxHp;

        dir_x = 1;
        dir_y = 1;
    }
    else {

        enemy.x += dir_x * 0.8;
        enemy.y += dir_y * 0.1;

        if (enemy.x > 360.0) dir_x = -1;
        if (enemy.x < 120.0) dir_x = 1;
        if (enemy.y > 240.0) dir_y = -1;
        if (enemy.y < 120.0) dir_y = 1;
    }

    if (count % 290 == 1) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSierpinski;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

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
}