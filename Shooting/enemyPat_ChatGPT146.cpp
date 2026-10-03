#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// プリズム分光：入射光がプリズムで虹色に分光される弾幕

static void ShotPrism(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // param_i[0]:
    // 0 = 入射光
    // 1 = 分光後の虹色レーザー
    // 2 = プリズム本体
    // 3 = 分光時の閃光
    int phase = pEnemyShotSet->param_i[0];

    // 入射光
    if (phase == 0) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_medium))
                StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotLaser[6];

            pEnemyShot->param_i[0] = 0;
            pEnemyShot->param_i[1] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 入射光をプリズム位置まで進ませる
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            if (pShot->param_i[1] == 0) {
                double dx = pEnemyShotSet->param_d[0] - pShot->x;
                double dy = pEnemyShotSet->param_d[1] - pShot->y;

                if (dx * dx + dy * dy < 18.0 * 18.0) {
                    pShot->param_i[1] = 1;

                    // 入射光を消して分光開始
                    pShot->x = -1000.0;
                    pShot->y = -1000.0;

                    pEnemyShotSet->param_i[0] = 1;
                    pEnemyShotSet->param_i[1] = 0;
                    pEnemyShotSet->param_i[2] = 0;
                    pEnemyShotSet->param_i[3] = 1;
                }
            }

            pShot = pShot->next;
        }
    }

    // 分光後の虹色レーザー
    else if (phase == 1) {
        if (pEnemyShotSet->param_i[3] == 1) {
            pEnemyShotSet->param_i[3] = 0;

            double prismX = pEnemyShotSet->param_d[0];
            double prismY = pEnemyShotSet->param_d[1];

            // 赤・橙・黄・緑・シアン・青・マゼンタ
            const int colors[] = { 0, 8, 1, 2, 3, 4, 5 };

            // 上から下へ少しずつ角度を変えて分光
            for (int i = 0; i < 7; i++) {
                pEnemyShot = new sEnemyShot;

                double angleOffset = (i - 3) * 0.075;

                pEnemyShot->x = prismX;
                pEnemyShot->y = prismY;
                pEnemyShot->muki =
                    pEnemyShotSet->muki + angleOffset;
                pEnemyShot->speed = 2.2;
                pEnemyShot->kind = img_enemyShotLaser[colors[i]];

                pEnemyShot->param_i[0] = i;
                pEnemyShot->param_d[0] = prismX;
                pEnemyShot->param_d[1] = prismY;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // 分光時の白い光
            for (int i = 0; i < 8; i++) {
                pEnemyShot = new sEnemyShot;

                double angle =
                    DX_PI * 2.0 * i / 8.0;

                pEnemyShot->x = prismX;
                pEnemyShot->y = prismY;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 1.2;
                pEnemyShot->kind = img_enemyShotLargeBall[6];

                pEnemyShot->param_i[0] = 100;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }

        // 一定時間後に次の入射光へ
        if (pEnemyShotSet->count > 100) {
            pEnemyShotSet->param_i[0] = 2;
            pEnemyShotSet->param_i[1] = 0;
        }
    }

    // プリズム位置を変更して次の光を準備
    else if (phase == 2) {
        if (pEnemyShotSet->count == 101) {
            double side =
                pEnemyShotSet->param_i[3] == 0 ? -1.0 : 1.0;

            pEnemyShotSet->param_d[0] =
                240.0 + side * 90.0;

            pEnemyShotSet->param_d[1] = 190.0;

            pEnemyShotSet->muki =
                atan2(
                    pEnemyShotSet->param_d[1] - pEnemyShotSet->y,
                    pEnemyShotSet->param_d[0] - pEnemyShotSet->x
                );

            pEnemyShotSet->param_i[3] ^= 1;
            pEnemyShotSet->param_i[0] = 0;

            // 既存の弾は残したまま、新しい入射光を生成
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotLaser[6];

            pEnemyShot->param_i[0] = 0;
            pEnemyShot->param_i[1] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 次の入射光が生成されたら通常フェーズへ戻す
        if (pEnemyShotSet->count > 102) {
            pEnemyShotSet->param_i[0] = 0;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Prism_ChatGPT()
{
    static int prismSide;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200;

        prismSide = 0;
    }

    // 敵を左右にゆっくり移動
    //enemy.x += 0.9;

    //if (enemy.x > 390.0) {
    //    enemy.x = 390.0;
    //}

    //if (enemy.x < 90.0) {
    //    enemy.x = 90.0;
    //}
    enemy.x = 240.0 + 150.0 * sin(count * 0.02);

    // 一定間隔で新しいプリズム分光を開始
    if (count % 50 == 1) {
        sEnemyShotSet* pEnemyShotSet =
            new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPrism;
        pEnemyShotSet->alive = 999;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;

        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;
        pEnemyShotSet->param_i[2] = 0;
        pEnemyShotSet->param_i[3] = prismSide;

        prismSide ^= 1;

        pEnemyShotSet->param_d[0] =
            prismSide == 0 ? 150.0 : 330.0;
        pEnemyShotSet->param_d[1] = 190.0;

        pEnemyShotSet->muki =
            atan2(
                pEnemyShotSet->param_d[1] -
                pEnemyShotSet->y,
                pEnemyShotSet->param_d[0] -
                pEnemyShotSet->x
            );

        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead =
            new sEnemyShot;

        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev =
            enemyShotSetHead.prev;

        pEnemyShotSet->next =
            &enemyShotSetHead;

        enemyShotSetHead.prev->next =
            pEnemyShotSet;

        enemyShotSetHead.prev =
            pEnemyShotSet;
    }
}