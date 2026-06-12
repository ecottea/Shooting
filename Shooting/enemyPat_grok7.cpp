// enemyPat_comet.cpp
// 彗星をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：彗星（頭＋尾の流星群）
static void ShotComet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double baseMuki = pEnemyShotSet->muki;
        double baseX = pEnemyShotSet->x;
        double baseY = pEnemyShotSet->y;

        // 彗星の頭（大玉・高速）
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = baseX;
        pEnemyShot->y = baseY;
        pEnemyShot->muki = baseMuki;
        pEnemyShot->speed = 4.2;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤の大玉（頭）

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 彗星の尾（小玉・やや遅め・微 spread）
        for (int i = 1; i <= 16; i++) {
            pEnemyShot = new sEnemyShot;
            double offset = i * 6.0;
            pEnemyShot->x = baseX - offset * cos(baseMuki);
            pEnemyShot->y = baseY - offset * sin(baseMuki);
            pEnemyShot->muki = baseMuki + (GetRand(40) - 20) / 180.0 * DX_PI * 0.15; // 軽い扇状
            pEnemyShot->speed = 3.8 - i * 0.08; // 後方ほどやや遅くして尾を伸ばす効果

            int color = (i < 6) ? 0 : 4; // 頭に近い部分は赤、遠い部分は青
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾更新（直線移動）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（彗星モチーフ）
void EnemyPat_Comet_Grok()
{
    static int muki = 1;
    static int shotPhase = 0;

    if (count == 1) {
        // 初期位置（画面上部中央）
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 100;
        muki = 1;
        shotPhase = 0;
    }
    else {
        // 左右往復＋軽い上下揺れ（彗星が滑るような動き）
        enemy.x += 1.35 * (double)muki;
        enemy.y += sin(count / 25.0) * 0.6;

        if (enemy.x < 80.0 || enemy.x > 400.0) {
            muki *= -1;
        }
    }

    // 定期的に彗星弾を発射（プレイヤー狙い）
    if (count % 25 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotComet;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;

        // プレイヤー方向に発射（やや予測気味）
        double dx = player.x - pEnemyShotSet->x;
        double dy = player.y - pEnemyShotSet->y + 20.0; // 少し先読み
        pEnemyShotSet->muki = atan2(dy, dx);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 時々角度をずらした追加の彗星（密度アップ）
    if (count % 75 == 0) {
        for (int i = -1; i <= 1; i += 2) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotComet;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 18.0;

            double dx = player.x - pEnemyShotSet->x;
            double dy = player.y - pEnemyShotSet->y + 30.0;
            pEnemyShotSet->muki = atan2(dy, dx) + i * 0.35;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}