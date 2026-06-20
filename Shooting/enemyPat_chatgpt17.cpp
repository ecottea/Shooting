// enemyPat_Kouyou.cpp
// 紅葉モチーフ弾幕
//
// コンセプト:
// ・敵の周囲に「紅葉の木」を形成
// ・葉(菱形弾)が枝から離れ、風に流されながら落下
// ・落下中に左右へ揺れ、秋風で舞う紅葉を表現
//
// count, pEnemyShotSet->count, pEnemyShot->count の加算はメインルーチン側で行われる前提

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotKouyou(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回生成
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int LEAF_NUM = 36;

        for (int i = 0; i < LEAF_NUM; i++) {
            double ang = DX_PI * 2.0 * i / LEAF_NUM;

            pEnemyShot = new sEnemyShot;

            // 木の枝先に葉を配置
            double r =
                35.0 +
                12.0 * sin(ang * 3.0) +
                8.0 * sin(ang * 5.0);

            pEnemyShot->x = pEnemyShotSet->x + cos(ang) * r * 3;
            pEnemyShot->y = pEnemyShotSet->y + sin(ang) * r * 3;

            pEnemyShot->muki = DX_PI / 2.0;
            pEnemyShot->speed = 0.0;

            // 秋らしく赤・黄・橙系
            int color;
            switch (i % 4) {
            case 0: color = 0; break; // 赤
            case 1: color = 1; break; // 黄
            case 2: color = 5; break; // マゼンタ(紅葉色代用)
            default: color = 6; break; // 白(銀杏風)
            }

            pEnemyShot->kind = img_enemyShotDiamond[color];
            pEnemyShot->margin = 999.;

            // 葉ごとに個性を持たせる
            pEnemyShot->count = i * 7;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        int t = pEnemyShotSet->count;

        // 前半は木に留まる
        if (t < 40) {
            double ang = pEnemyShot->count * DX_PI / 126.0;

            double r =
                35.0 +
                12.0 * sin(ang * 3.0) +
                8.0 * sin(ang * 5.0);

            pEnemyShot->x =
                pEnemyShotSet->x +
                cos(ang) * r * 3;

            pEnemyShot->y =
                pEnemyShotSet->y +
                sin(ang) * r * 3;
        }
        else {
            // 紅葉が風で舞い落ちる

            int fallTime = t - 40;

            double wind =
                sin(
                    (fallTime + pEnemyShot->count * 3)
                    * 0.08
                ) * 1.8;

            pEnemyShot->x += wind;

            // 落下加速
            pEnemyShot->speed += 0.02;
            if (pEnemyShot->speed > 2.5)
                pEnemyShot->speed = 2.5;

            pEnemyShot->y += pEnemyShot->speed;

            // ときどき突風
            if ((fallTime + pEnemyShot->count) % 90 == 0) {
                pEnemyShot->x +=
                    (GetRand(80) - 40) * 0.2;
            }
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体
void EnemyPat_Maple_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;

        muki = 1;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.7 * muki;

        if (enemy.x < 120) muki = 1;
        if (enemy.x > 360) muki = -1;
    }

    // 定期的に紅葉を発生
    if (count % 90 == 0) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKouyou;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

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