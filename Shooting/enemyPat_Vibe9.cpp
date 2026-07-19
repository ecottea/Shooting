// enemyPat_blizzard.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：吹雪（雪片を模した弾を放射状に発射）
static void ShotBlizzard(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 吹雪を表現：雪片を模した弾を放射状に発射
        for (int i = 0; i < 100; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 方向をランダムにばらつかせる（吹雪の広がりを表現）
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(360) - 180) / 180.0 * DX_PI;
            // 速度をランダムに設定（雪片の速度差を表現）
            pEnemyShot->speed = (50 + GetRand(250)) / 100.0;

            // 弾の種類：小玉、中玉、大玉をランダムに選択（雪片のサイズ差を表現）
            int type = GetRand(2); // 0:小玉, 1:中玉, 2:大玉
            // 弾の色：白、シアン、青をランダムに選択（雪片の冷たいイメージ）
            int color = GetRand(1) + 6; // 6:白, 7:黒

            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotLargeBall[color];
                break;
            }

            // リストに追加
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

// 敵本体のパターン：吹雪
void EnemyPat_Blizzard_Vibe()
{
    static int muki = 1;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵を左右にゆっくり移動させる
        enemy.x += 0.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 吹雪弾幕を定期的に発射
    if (count % 45 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBlizzard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // プレーヤー方向を基準に弾幕を発射
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