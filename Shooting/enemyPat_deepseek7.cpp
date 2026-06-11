// enemyPat_comet.cpp
// 彗星をモチーフにした弾幕パターン
// 尾の出現を遅延させ、画面外に一気に伸びないようにする

#include "DxLib.h"
#include "gv.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕：彗星（頭部＋遅延出現する尾）
// ------------------------------------------------------------
static void ShotComet(sEnemyShotSet* pEnemyShotSet)
{
    const double speed = 3.0;           // 頭部・尾部共通の速度
    const int    tailNum = 100;            // 尾の数
    const double tailSpacing = 12.0;          // 尾同士の間隔 [pixel]
    const int    spawnInterval = 3;            // 尾を1つ出現させる間隔 [フレーム]
    const int    color = 6;            // 白

    // ----- 初回（count == 0）で頭部を生成 -----
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pHead = new sEnemyShot;
        pHead->x = pEnemyShotSet->x;
        pHead->y = pEnemyShotSet->y;
        pHead->muki = pEnemyShotSet->muki;
        pHead->speed = speed;
        pHead->kind = img_enemyShotLargeBall[color];

        pHead->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pHead;
        pEnemyShotSet->pEnemyShotHead->prev = pHead;
    }
    // ----- 尾を count % spawnInterval == 0 のタイミングで1つずつ追加 -----
    else {
        const int tailIndex = pEnemyShotSet->count / spawnInterval;   // 1, 2, …, tailNum
        if (pEnemyShotSet->count % spawnInterval == 0 &&
            tailIndex >= 1 && tailIndex <= tailNum)
        {
            // 現在の頭部位置を計算（等速直線運動なので簡単に逆算できる）
            const double headX = pEnemyShotSet->x + pEnemyShotSet->count * speed * cos(pEnemyShotSet->muki);
            const double headY = pEnemyShotSet->y + pEnemyShotSet->count * speed * sin(pEnemyShotSet->muki);
            const double tailDir = pEnemyShotSet->muki + DX_PI;   // 進行方向の逆

            const double offsetX = tailIndex * tailSpacing * cos(tailDir);
            const double offsetY = tailIndex * tailSpacing * sin(tailDir);

            sEnemyShot* pTail = new sEnemyShot;
            pTail->x = headX + offsetX;
            pTail->y = headY + offsetY;
            pTail->muki = pEnemyShotSet->muki;
            pTail->speed = speed;
            pTail->kind = img_enemyShotSmallBall[color];

            pTail->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pTail->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pTail;
            pEnemyShotSet->pEnemyShotHead->prev = pTail;
        }
    }

    // ----- この弾幕セットに属する全弾を移動 -----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン（彗星使い）
// ------------------------------------------------------------
void EnemyPat_Comet_DeepSeek()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 150;
        muki = 1;
    }
    else {
        enemy.x += 1.2 * (double)muki;
        if (count % 90 == 45) muki *= -1;
    }

    // 一定間隔で彗星を生成
    if (count % 30 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotComet;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 8.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}