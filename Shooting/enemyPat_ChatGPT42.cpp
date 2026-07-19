// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//------------------------------------------------------------
// 石ブロック生成
//------------------------------------------------------------
static void AddStone(
    sEnemyShotSet* pEnemyShotSet,
    double x,
    double y,
    double tx,
    double ty,
    int color)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->x = x;
    pEnemyShot->y = y;

    double dx = tx - x;
    double dy = ty - y;
    double len = sqrt(dx * dx + dy * dy);
    if (len < 0.001) len = 1.0;

    pEnemyShot->muki = atan2(dy, dx);
    pEnemyShot->speed = 2.2;

    pEnemyShot->kind = img_enemyShotMediumBall[color];

    pEnemyShot->param_d[0] = tx;
    pEnemyShot->param_d[1] = ty;
    pEnemyShot->param_i[0] = 0; // 0:移動中 1:停止

    pEnemyShot->margin = 480;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

//------------------------------------------------------------
// ピラミッド弾幕
//------------------------------------------------------------
static void ShotPyramid(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //--------------------------------------------------------
    // 初期生成
    //--------------------------------------------------------
    if (pEnemyShotSet->count == 0) {

        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const double cx = 240.0;
        const double topY = 130.0;
        const double stepY = 34.0;
        const double stepX = 26.0;

        // 7段のピラミッド
        for (int row = 0; row < 7; row++) {

            int num = 13 - row * 2;
            double y = topY + row * stepY;
            double start = cx - (num - 1) * stepX * 0.5;

            for (int i = 0; i < num; i++) {

                // 通路
                if (row == 0 && i == num / 2) continue;
                if (row == 1 && i == num / 2) continue;
                if (row == 2 && i == num / 2) continue;

                double tx = start + i * stepX;

                // 上空から落下
                double sx = tx;
                double sy = -40.0 - row * 18.0;

                AddStone(
                    pEnemyShotSet,
                    sx,
                    sy,
                    tx,
                    y,
                    7);     // 黒
            }
        }
    }

    //--------------------------------------------------------
    // 更新
    //--------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShot->param_i[0] == 0) {

            double tx = pEnemyShot->param_d[0];
            double ty = pEnemyShot->param_d[1];

            double dx = tx - pEnemyShot->x;
            double dy = ty - pEnemyShot->y;
            double d = sqrt(dx * dx + dy * dy);

            if (d <= pEnemyShot->speed) {

                pEnemyShot->x = tx;
                pEnemyShot->y = ty;

                pEnemyShot->speed = 0.0;
                pEnemyShot->param_i[0] = 1;
            }
            else {

                pEnemyShot->x +=
                    cos(pEnemyShot->muki) * pEnemyShot->speed;
                pEnemyShot->y +=
                    sin(pEnemyShot->muki) * pEnemyShot->speed;
            }
        }

        pEnemyShot = pEnemyShot->next;
    }

    //--------------------------------------------------------
// 頂上の黄金弾生成
//--------------------------------------------------------
    if (pEnemyShotSet->count == 210) {

        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;

        pEnemyShot->x = 240.0;
        pEnemyShot->y = 108.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->muki = 0.0;
        pEnemyShot->kind = img_enemyShotLargeBall[1];   // 黄
        pEnemyShot->param_i[0] = 2;                     // 黄金弾

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    //--------------------------------------------------------
    // 儀式：放射弾
    //--------------------------------------------------------
    if (pEnemyShotSet->count == 270) {

        if (CheckSoundMem(sound_enemyShot_extreme))
            StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 36; i++) {

            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = 240.0;
            pEnemyShot->y = 108.0;
            pEnemyShot->muki = DX_PI * 2.0 * i / 36.0;
            pEnemyShot->speed = 2.8;
            pEnemyShot->kind = img_enemyShotSmallBall[1];

            pEnemyShot->param_i[0] = 3;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    //--------------------------------------------------------
    // 崩壊
    //--------------------------------------------------------
    if (pEnemyShotSet->count >= 340 &&
        (pEnemyShotSet->count - 340) % 8 == 0) {

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;

        while (p != pEnemyShotSet->pEnemyShotHead) {

            if (p->param_i[0] == 1) {

                p->param_i[0] = 4;

                double bx = p->x;
                double by = p->y;

                for (int k = 0; k < 3; k++) {

                    sEnemyShot* b = new sEnemyShot;

                    b->x = bx;
                    b->y = by;
                    b->muki = DX_PI * 2.0 * (k + GetRand(20) / 20.0) / 3.0;
                    b->speed = 1.6 + GetRand(12) / 10.0;
                    b->kind = img_enemyShotSmallBall[7];
                    b->param_i[0] = 5;

                    b->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    b->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = b;
                    pEnemyShotSet->pEnemyShotHead->prev = b;
                }

                break;
            }

            p = p->next;
        }
    }

    //--------------------------------------------------------
    // 各種弾更新
    //--------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        switch (pEnemyShot->param_i[0]) {

            // 黄金弾
        case 2:
            break;

            // 放射弾
        case 3:
            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;
            break;

            // 崩壊開始
        case 4:
            pEnemyShot->speed = 2.3;
            pEnemyShot->muki = DX_PI / 2.0;
            pEnemyShot->param_i[0] = 6;
            break;

            // 破片
        case 5:
            pEnemyShot->x += cos(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->y += sin(pEnemyShot->muki) * pEnemyShot->speed;
            pEnemyShot->speed *= 0.99;
            break;

            // 落下する石
        case 6:
            pEnemyShot->y += pEnemyShot->speed;
            pEnemyShot->speed += 0.05;
            break;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_Pyramid_ChatGPT()
{
    static int dir;
    static int wave;

    if (count == 1) {

        enemy.x = 240.0;
        enemy.y = 150.0;
        enemy.maxHp = enemy.hp = 200;

        dir = 1;
        wave = 0;
    }
    else {

        enemy.x += dir * 0.8;

        if (enemy.x > 360.0) dir = -1;
        if (enemy.x < 120.0) dir = 1;
    }

    // 480フレーム毎にピラミッドを建設
    if (count % 480 == 1) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPyramid;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = wave++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}