// enemyPat_Dragon.cpp
// 竜をモチーフにした弾幕パターン（1パターン）の実装

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 竜の吐息風：扇形に広がる多重リング弾
static void ShotDragonBreath(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 竜の吐息をイメージした多重リング
        const int RING_COUNT = 3;          // リングの数
        const int SHOTS_PER_RING = 16;     // 1リングあたりの弾数
        const double BASE_SPEED = 2.0;     // 基本速度
        const double SPREAD_ANGLE = DX_PI / 3.0; // 扇の広がり角度（60度）

        for (int ring = 0; ring < RING_COUNT; ring++) {
            double ringSpeed = BASE_SPEED + ring * 0.5; // 外側のリングほど速く
            double ringRadius = 10.0 + ring * 15.0;     // 外側ほど半径が大きい

            for (int i = 0; i < SHOTS_PER_RING; i++) {
                pEnemyShot = new sEnemyShot;
                // リング状の初期配置
                double angleOffset = (DX_PI * 2.0 * i) / SHOTS_PER_RING;
                pEnemyShot->x = pEnemyShotSet->x + ringRadius * cos(angleOffset);
                pEnemyShot->y = pEnemyShotSet->y + ringRadius * sin(angleOffset);

                // プレイヤー方向を中心に扇形に広がる
                double baseDir = pEnemyShotSet->muki;
                double spread = (GetRand(100) - 50) / 100.0 * SPREAD_ANGLE;
                pEnemyShot->muki = baseDir + spread;
                pEnemyShot->speed = ringSpeed;

                // 竜らしい色（赤系）を中心にランダムに色を選ぶ
                int color = GetRand(3); // 0:赤, 1:黄, 2:オレンジ寄り（白で代用）など
                switch (color) {
                case 0:
                    pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤
                    break;
                case 1:
                    pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄
                    break;
                case 2:
                default:
                    pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
                    break;
                }

                // リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（関数名は指定どおり）
void EnemyPat_Dragon_Sakana()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で竜の吐息弾幕を発射
    if (count % 40 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDragonBreath;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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