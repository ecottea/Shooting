// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 彗星弾幕：複数の層で弾を放射状に発射し、尾の広がりと長さを表現
static void ShotComet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 4層の彗星の尾を表現
        for (int layer = 0; layer < 4; layer++) {
            for (int i = 0; i < 6; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                // 層ごとに角度を広げ、彗星の尾の広がりを表現
                double baseAngle = DX_PI / 2; // 上方向
                double angle = baseAngle + (GetRand(150) - 75) / 180.0 * DX_PI + layer * 0.3;
                pEnemyShot->muki = angle;

                // 層ごとに速度を変化させ、尾の長さを表現
                pEnemyShot->speed = 0.8 + layer * 0.4 + GetRand(80) / 100.0;

                // 彗星をイメージした青色の鱗弾
                pEnemyShot->kind = img_enemyShotScale[4]; // 4:青

                // リンクリストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
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
void EnemyPat_Comet_Vibe()
{
    static double moveOffset = 0.0;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        moveOffset = 0.0;
    }
    else {
        // 敵を左右に揺れながらゆっくり降下させる
        moveOffset += 0.02;
        enemy.x = 240.0 + 150.0 * sin(moveOffset);
        if (enemy.y < 300) {
            enemy.y += 0.15;
        }
    }

    // 彗星の尾を定期的に発射
    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotComet;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}