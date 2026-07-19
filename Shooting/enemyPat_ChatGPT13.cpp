// EnemyPat_RazorLeaf_ChatGPT.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ========================================
// はっぱカッター
// ========================================
static void ShotLeafCutter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回生成
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = -40; i <= 40; i++)
        {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki =
                pEnemyShotSet->muki +
                i * (-DX_PI / 36.0);

            pEnemyShot->speed = 1.8;

            // 緑色の鱗弾
            pEnemyShot->kind = img_enemyShotScale[2];

            // 回転方向を記録
            pEnemyShot->count = (i < 0) ? -1 : 1;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        sEnemyShot* next = pEnemyShot->next;

        // 前半：葉が回転しながら飛ぶ
        if (pEnemyShotSet->count < 50)
        {
            pEnemyShot->muki +=
                pEnemyShot->count * 0.025;

            pEnemyShot->speed *= 1.01;
            if (pEnemyShot->speed > 3.0)
                pEnemyShot->speed = 3.0;
        }
        // 後半：カッター化して一直線
        else if (pEnemyShotSet->count == 50)
        {
            pEnemyShot->speed = 5.0;
            pEnemyShot->kind = img_enemyShotBullet[2];

            if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        pEnemyShot->x +=
            cos(pEnemyShot->muki) *
            pEnemyShot->speed;

        pEnemyShot->y +=
            sin(pEnemyShot->muki) *
            pEnemyShot->speed;

        pEnemyShot->count--;

        pEnemyShot = next;
    }
}

// ========================================
// 敵本体
// ========================================
void EnemyPat_RazorLeaf_ChatGPT()
{
    static int moveDir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 60.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        moveDir = 1;
    }

    // 横移動
    enemy.x += moveDir * 1.2;

    if (enemy.x > 420.0)
        moveDir = -1;
    if (enemy.x < 60.0)
        moveDir = 1;

    // はっぱカッター発射
    if (count % 90 == 30)
    {
        sEnemyShotSet* pEnemyShotSet =
            new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc =
            ShotLeafCutter;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

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