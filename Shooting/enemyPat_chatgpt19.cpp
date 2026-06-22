// EnemyPat_Hyperbola_ChatGPT.cpp
//
// テーマ：双曲線弾幕
//
// 発想:
// 敵の左右から放たれた2本の弾流が、時間経過とともに
// y = a/x 型の双曲線を描くように湾曲。
// 左枝と右枝が中央付近へ接近し、その後再び離れていくため、
// 双曲線の2つの枝そのものが弾幕になる。
// さらに周期的に係数aを変化させ、双曲線の開き具合が脈動する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

struct HyperParam
{
    double side;      // -1:左枝 +1:右枝
    double baseX;     // 生成時のx
    double phase;     // 位相
};

// ----------------------------------------------------------
// 双曲線の枝
// ----------------------------------------------------------
static void ShotHyperbola(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {

        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int sideIdx = 0; sideIdx < 2; sideIdx++) {

            double side = (sideIdx == 0) ? -1.0 : 1.0;

            for (int i = 0; i < 120; i++) {

                sEnemyShot* pEnemyShot = new sEnemyShot;

                pEnemyShot->x = pEnemyShotSet->x + side * 10.0;
                pEnemyShot->y = pEnemyShotSet->y;

                pEnemyShot->count = 0;

                // side/baseX/phase を count に圧縮して保存
                // countは自由に使えるためパラメータ格納用に利用
                int encodedSide = (side > 0.0) ? 1 : 0;
                pEnemyShot->count =
                    encodedSide |
                    (i << 1);

                pEnemyShot->speed = 1.0;

                int color = (side > 0.0) ? 4 : 1; // 青・黄

                if (i & 1)
                    pEnemyShot->kind = img_enemyShotScale[color];
                else
                    pEnemyShot->kind = img_enemyShotSmallBall[color];

                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->margin = 100;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    double pulse =
        70.0 +
        35.0 * sin(pEnemyShotSet->count * 0.05);

    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->count--;

        int sideBit = pEnemyShot->count & 1;
        int index = pEnemyShot->count >> 1;

        double side = sideBit ? 1.0 : -1.0;

        double t =
            pEnemyShotSet->count * 0.035 +
            index * 0.18;

        // 双曲線 x = ±u
        double u = 0.7 + t;
        u *= 0.5;

        double x = side * (u * 42.0);
        double y = pulse / u * 7.0;

        pEnemyShot->x = pEnemyShotSet->x + x;
        pEnemyShot->y = pEnemyShotSet->y + y;

        if (pEnemyShot->x < -20 || pEnemyShot->x > 480 + 20) pEnemyShot->x = 99999.;

        pEnemyShot = pEnemyShot->next;
    }
}

static void ShotSingle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 1; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x) + (GetRand(100) - 50) / 50.0;
            pEnemyShot->speed = 1.0;

            pEnemyShot->kind = img_enemyShotDiamond[2];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}


// ----------------------------------------------------------
// 敵本体
// ----------------------------------------------------------
void EnemyPat_Hyperbola_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0 + 50;

        enemy.maxHp = 200;
        enemy.hp = enemy.maxHp;
    }

    // リサジュー風のゆったりした揺れ
    enemy.x = 240.0 + 120.0 * sin(count * 0.020);
    enemy.y = 70.0 + 20.0 * sin(count * 0.030) + 50;

    if (count % 10 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHyperbola;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0 - 50;

        pEnemyShotSet->muki = 0.0;

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

    if (count % 10 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSingle;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20;
        pEnemyShotSet->muki = 0.0;

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