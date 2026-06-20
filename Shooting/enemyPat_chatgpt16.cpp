// enemyPat_Go.cpp
// 囲碁モチーフ弾幕
//
// コンセプト:
// ・黒石と白石が盤面に配置されるように出現
// ・しばらく静止して「囲碁盤」を形成
// ・その後、連結した石列がプレイヤーへ向かって滑るように動き出す
// ・黒→白→黒→白…と交互に展開されるため、盤面が成長していくように見える

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotGoBoard(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発生時
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int BOARD_W = 19;
        const int BOARD_H = 19;
        const double CELL = 28.0;

        // 囲碁盤状に石を配置
        for (int iy = 0; iy < BOARD_H; iy++) {
            for (int ix = 0; ix < BOARD_W; ix++) {

                pEnemyShot = new sEnemyShot;

                pEnemyShot->x =
                    pEnemyShotSet->x +
                    (ix - (BOARD_W - 1) * 0.5) * CELL;

                pEnemyShot->y =
                    pEnemyShotSet->y +
                    iy * CELL;

                // 囲碁の石らしく黒白交互
                bool black = ((ix + iy) & 1);

                pEnemyShot->kind =
                    black ?
                    img_enemyShotSmallBall[7] :   // 黒石
                    img_enemyShotSmallBall[6];    // 白石

                pEnemyShot->muki = atan2(
                    player.y - pEnemyShot->y,
                    player.x - pEnemyShot->x);

                pEnemyShot->speed = 0.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 前半は盤面形成
        if (pEnemyShotSet->count < 60) {

            double t = pEnemyShotSet->count / 60.0;

            // 微妙な脈動で盤面に生命感
            pEnemyShot->x += 0.15 * sin(
                pEnemyShotSet->count * 0.10 +
                pEnemyShot->count * 0.3);

            pEnemyShot->y += 0.15 * cos(
                pEnemyShotSet->count * 0.12 +
                pEnemyShot->count * 0.3);
        }
        else {

            // 囲碁の「連」をイメージして
            // 徐々に加速しながらプレイヤーへ滑走
            if (pEnemyShot->speed < 3.8)
                pEnemyShot->speed += 0.03;

            pEnemyShot->x +=
                cos(pEnemyShot->muki) *
                pEnemyShot->speed;

            pEnemyShot->y +=
                sin(pEnemyShot->muki) *
                pEnemyShot->speed;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体
void EnemyPat_Go_ChatGPT()
{
    static int dir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        dir = 1;
    }
    else {
        enemy.x += dir * 1.1;

        if (enemy.x < 100.0) dir = 1;
        if (enemy.x > 380.0) dir = -1;
    }

    // 黒番・白番をイメージして交互に展開
    if (count % 90 == 0) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGoBoard;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

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