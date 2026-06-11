// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include <math.h>

// 彗星弾幕パターン
static void ShotComet(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 核となる弾を生成
        sEnemyShot* pCore = new sEnemyShot;
        pCore->x = pEnemyShotSet->x;
        pCore->y = pEnemyShotSet->y;
        pCore->muki = pEnemyShotSet->muki;
        pCore->speed = 2.5;
        pCore->kind = img_enemyShotMediumBall[3]; // シアンの中玉

        pCore->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pCore->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pCore;
        pEnemyShotSet->pEnemyShotHead->prev = pCore;
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;

        // 画面外判定
        if (p->x < -50 || p->x > 530 || p->y < -50 || p->y > 530) {
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
            p = next;
            continue;
        }

        // 核の弾の場合
        if (p->kind == img_enemyShotMediumBall[3]) {
            // 重力で下に曲げる
            p->muki += 0.015;
            // 加速
            p->speed += 0.02;

            // 座標更新
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);

            // 尾の弾を生成
            sEnemyShot* pTail = new sEnemyShot;
            pTail->x = p->x;
            pTail->y = p->y;
            pTail->muki = p->muki;
            pTail->speed = p->speed * 0.9; // 核より少し遅い
            pTail->kind = img_enemyShotSmallBall[6]; // 白の小玉

            pTail->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pTail->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pTail;
            pEnemyShotSet->pEnemyShotHead->prev = pTail;
        }
        // 尾の弾の場合
        else {
            // 減速
            p->speed *= 0.95;
            if (p->speed < 0.1) {
                p->prev->next = p->next;
                p->next->prev = p->prev;
                delete p;
                p = next;
                continue;
            }

            // 座標更新
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }

        p = next;
    }
}

// 敵本体のパターン
void EnemyPat_Comet_Qwen()
{
    static int muki_x;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 150;
        muki_x = 1;
    }
    else {
        enemy.x += 1.2 * (double)muki_x;
        if (enemy.x < 60 || enemy.x > 420) {
            muki_x *= -1;
        }
    }

    // 30フレームごとに彗星を発射
    if (count % 15 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotComet;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // 自機狙い + ランダムなばらつき（重力で落ちるため少し上を狙う）
        double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->muki = base_muki - 1.0 + (GetRand(100) - 50) / 180.0 * DX_PI;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}