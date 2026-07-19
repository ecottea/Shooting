// enemyPat_kinkakuji.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 金閣寺の一枚天井：黄色い横一列の弾（金閣寺の天井をイメージ）
static void ShotGoldCeiling(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 効果音を再生
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 7発の黄色い小玉を横一列に配置
        for (int i = 0; i < 7; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // 中央から左右に広がるように配置
            pEnemyShot->x = pEnemyShotSet->x - 120 + i * 40 + (GetRand(20) - 10); // ※せめて完全安置は無くす
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = -DX_PI * 1.5; // 下方向（※上方向になってたので修正）
            pEnemyShot->speed = 0.8; // 遅い速度

            // 黄色の小玉
            pEnemyShot->kind = img_enemyShotSmallBall[1];

            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 金閣寺の一枚天井：緑色のX字状回転弾（時計回りに回転）
static void ShotGreenX(sEnemyShotSet* pEnemyShotSet)
{
    // 5フレームごとに新しい弾を生成
    if (pEnemyShotSet->count % 5 == 0) {
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // X字状の角度（時計回りに回転 + ランダム性）
            double base_angle = i * DX_PI / 2; // 0°, 90°, 180°, 270°
            double rotation = (pEnemyShotSet->count / 5) * 0.05;
            double angle = base_angle + rotation + (GetRand(20) - 10) * 0.01;

            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * (80 + GetRand(20));
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * (80 + GetRand(20));
            pEnemyShot->muki = angle + DX_PI / 2 + (GetRand(20) - 10) * 0.01;
            pEnemyShot->speed = 2.0 + GetRand(10) * 0.1;

            // 緑色の中玉
            pEnemyShot->kind = img_enemyShotMediumBall[2];

            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Kinkakuji_Vibe()
{
    static int phase = 0;
    static int shot_count = 0;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        shot_count = 0;
    }

    // 120フレームごとに金閣寺の一枚天井弾幕を発動
    if (count % 120 == 0) {
        // 黄色い横一列弾
        sEnemyShotSet* pGold = new sEnemyShotSet;
        pGold->count = 0;
        pGold->patternFunc = ShotGoldCeiling;
        pGold->x = enemy.x;
        pGold->y = enemy.y;
        pGold->muki = 0;
        pGold->kind = 0;
        pGold->pEnemyShotHead = new sEnemyShot;
        pGold->pEnemyShotHead->prev = pGold->pEnemyShotHead;
        pGold->pEnemyShotHead->next = pGold->pEnemyShotHead;

        pGold->prev = enemyShotSetHead.prev;
        pGold->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pGold;
        enemyShotSetHead.prev = pGold;

        // 緑色のX字状回転弾
        sEnemyShotSet* pGreen = new sEnemyShotSet;
        pGreen->count = 0;
        pGreen->patternFunc = ShotGreenX;
        pGreen->x = enemy.x;
        pGreen->y = enemy.y;
        pGreen->muki = 0;
        pGreen->kind = 0;
        pGreen->pEnemyShotHead = new sEnemyShot;
        pGreen->pEnemyShotHead->prev = pGreen->pEnemyShotHead;
        pGreen->pEnemyShotHead->next = pGreen->pEnemyShotHead;

        pGreen->prev = enemyShotSetHead.prev;
        pGreen->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pGreen;
        enemyShotSetHead.prev = pGreen;
    }
}