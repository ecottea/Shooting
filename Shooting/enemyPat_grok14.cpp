// enemyPat_Tmp.cpp
// 毒をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------- 毒の霧 -------------------
static void ShotPoisonMist(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int N = 60;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double baseAngle = (double)i / N * DX_PI * 2.0;
            pEnemyShot->muki = baseAngle + (GetRand(50) - 25) / 180.0 * DX_PI;
            pEnemyShot->speed = (55 + GetRand(75)) / 100.0;

            // 毒色（緑・シアン・マゼンタ中心）
            int color = GetRand(3) + 2;
            pEnemyShot->kind = (GetRand(100) < 70)
                ? img_enemyShotSmallBall[color]
                : img_enemyShotMediumBall[color];

            // リンク挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 毒の霧らしいゆらゆら＆沈む挙動
        pEnemyShot->muki += sin((double)pEnemyShotSet->count * 0.13) * 0.022;
        pEnemyShot->y += 0.45;

        if (pEnemyShotSet->count > 35) {
            pEnemyShot->speed += 0.015;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------- 毒の針（菱形弾） -------------------
static void ShotPoisonNeedle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int N = 50;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // プレイヤー方向に集中しつつ少し広がる
            double targetAngle = pEnemyShotSet->muki;
            pEnemyShot->muki = targetAngle + (GetRand(80) - 40) / 180.0 * DX_PI * 0.7;
            pEnemyShot->speed = (180 + GetRand(140)) / 100.0;  // 比較的速め

            int color = GetRand(3) + 2; // 毒色
            pEnemyShot->kind = img_enemyShotDiamond[color];

            // リンク挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 毒の針らしい挙動：徐々に加速
        if (pEnemyShotSet->count > 20) {
            pEnemyShot->speed += 0.035;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------- 敵本体 -------------------
void EnemyPat_Poison_Grok()
{
    static int muki = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆったり左右移動＋徐々に下降
        enemy.x += 1.15 * (double)muki;
        if (count % 105 == 52) muki *= -1;

        if (count % 4 == 0 && enemy.y < 165.0) {
            enemy.y += 0.7;
        }
    }

    // 毒の霧（メイン弾幕）
    if (count % 47 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPoisonMist;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 毒の針（サブ弾幕）
    if (count % 118 == 65) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPoisonNeedle;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}