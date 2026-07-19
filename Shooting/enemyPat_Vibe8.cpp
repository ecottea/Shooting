// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：雷（稲妻）
static void ShotLightning(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 稲妻の本体（白色の菱形弾をジグザグに配置）
        double currentY = pEnemyShotSet->y;
        double currentX = pEnemyShotSet->x;
        for (int i = 0; i < 15; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = currentX;
            pEnemyShot->y = currentY;
            pEnemyShot->muki = DX_PI / 2.0; // 下方向
            pEnemyShot->speed = 8.0;
            pEnemyShot->kind = img_enemyShotDiamond[6]; // 白菱形弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // ジグザグパターン
            currentY += 15.0;
            if ((i / 2) % 2 == 0) {
                currentX += 10.0;
            }
            else {
                currentX -= 10.0;
            }
        }

        // 分岐（小玉、シアン）
        for (int i = 0; i < 10; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(80) - 40);
            pEnemyShot->y = pEnemyShotSet->y + GetRand(150);
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(80) - 40) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.0 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Thunder_Vibe()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.5 * (double)muki;
        if (enemy.x <= 70 || enemy.x >= 480 - 70) muki *= -1;
    }

    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLightning;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 稲妻は下方向に固定

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}