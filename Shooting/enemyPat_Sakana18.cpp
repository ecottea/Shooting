// enemyPat_spiderWeb2.cpp
// クモの巣をモチーフにした弾幕パターン（放射状＋渦巻きバージョン）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// クモの巣弾幕パターン（放射状＋渦巻き）
static void ShotSpiderWeb2(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音は軽めのものを使用
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // クモの巣の「放射状の糸」と「渦巻き」を組み合わせる
        const int numRays = 12;          // 放射線の本数（クモの巣の主な糸）
        const int numSpiral = 4;         // 渦巻きの層数
        const double baseSpeed = 1.2;    // 基本的な速度

        // 放射状の糸
        for (int ray = 0; ray < numRays; ++ray) {
            double angle = (DX_PI * 2.0 / numRays) * ray; // 等間隔の角度

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = baseSpeed + (GetRand(30)) / 100.0; // 速度に軽いランダム

            // クモの巣らしい雰囲気の弾種・色を選ぶ
            int type = GetRand(1);       // 0:小玉, 1:中玉
            int color = GetRand(1);     // 0:白, 1:黒

            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                break;
            }

            // ダミーリストに接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 渦巻き状の輪
        for (int spiral = 1; spiral <= numSpiral; ++spiral) {
            double spiralRadius = spiral * 30.0; // 渦の半径
            int numSpiralBullets = 8 + spiral * 2; // 外側ほど弾を増やす

            for (int i = 0; i < numSpiralBullets; ++i) {
                double angle = (DX_PI * 2.0 / numSpiralBullets) * i;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + spiralRadius * cos(angle);
                pEnemyShot->y = pEnemyShotSet->y + spiralRadius * sin(angle);

                // 渦巻きの「外側に広がる」イメージで、少し外側方向へ
                pEnemyShot->muki = angle + (GetRand(15) - 7.5) / 180.0 * DX_PI;
                pEnemyShot->speed = baseSpeed * 0.8 + (GetRand(40)) / 100.0;

                int type = GetRand(2);   // 0:小玉, 1:中玉, 2:大玉
                int color = GetRand(2);  // 0:白, 1:黒, 2:グレー相当

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

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 既存弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（指定どおりの関数名）
void EnemyPat_SpiderWeb_Sakana()
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

    // クモの巣弾幕の発射間隔（例：30フレームごと）
    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiderWeb2;
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