// enemyPat_wafu.cpp
// 和風弾幕パターン集
#include "DxLib.h"
#include "gv.h"
#include <math.h>

// =============================================
// 1. 桜吹雪 + 扇形 (優雅な和風ばら撒き)
// =============================================
static void ShotSakuraFan(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int bulletCount = 640;
        double baseAngle = pEnemyShotSet->muki;

        for (int i = 0; i < bulletCount; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double offset = (i - bulletCount / 2.0) * 0.22;
            pEnemyShot->muki = baseAngle + offset;
            pEnemyShot->speed = (2.8 + fabs(offset) * 0.6) * 0.1;

            // 和風らしい弾：小玉（桜色）＋菱形を多めに
            if (GetRand(3) == 0)
                pEnemyShot->kind = img_enemyShotDiamond[0];     // 赤
            else
                pEnemyShot->kind = img_enemyShotSmallBall[5];   // 桜色（赤系）

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動 + 少し加速（桜が舞うような感じ）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot->speed += 0.008;        // 徐々に加速
        pEnemyShot = pEnemyShot->next;
    }
}

// =============================================
// 敵本体パターン例
// =============================================
void EnemyPat_Japanese_Grok()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = 150;
        enemy.hp = enemy.maxHp;
    }

    // 左右ゆったり移動
    enemy.x += sin(count * 0.018) * 1.1;

    if (count % 180 == 30) {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = ShotSakuraFan;
        p->x = enemy.x;
        p->y = enemy.y + 20;
        p->muki = atan2(player.y - p->y, player.x - p->x);

        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }
}
