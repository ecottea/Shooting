// SierpinskiGasket.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// シェルピンスキーのギャスケット風弾幕パターン
static void ShotSierpinski(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 移動処理 (countや画面外判定はメインルーチン側で処理される)
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 分裂処理
        // pEnemyShot->count はメインルーチンでインクリメントされる
        if (pEnemyShot->count > 0 && pEnemyShot->count % 20 == 0) {
            // param_i[0] は分裂レベル (初期値4, 0になるまで分裂)
            if (pEnemyShot->param_i[0] > 0) {
                // 左右120度の方向に2つの子弾を生成
                double angles[2] = { DX_PI * 2.0 / 3.0, -DX_PI * 2.0 / 3.0 };

                for (int i = 0; i < 2; i++) {
                    sEnemyShot* newShot = new sEnemyShot;
                    newShot->x = pEnemyShot->x;
                    newShot->y = pEnemyShot->y;
                    newShot->muki = pEnemyShot->muki + angles[i];
                    newShot->speed = pEnemyShot->speed * 0.95; // 少し減速

                    // レベルを1下げる
                    newShot->param_i[0] = pEnemyShot->param_i[0] - 1;

                    // レベルに応じた色と画像を設定
                    int level = newShot->param_i[0];
                    int color = 5 - level; // 4:青, 3:シアン, 2:緑, 1:黄, 0:赤
                    if (color < 0) color = 0;
                    if (color > 8) color = 8;

                    switch (level) {
                    case 4: newShot->kind = img_enemyShotLargeBall[color]; break;
                    case 3: newShot->kind = img_enemyShotMediumBall[color]; break;
                    case 2: newShot->kind = img_enemyShotSmallBall[color]; break;
                    case 1: newShot->kind = img_enemyShotScale[color]; break;
                    default: newShot->kind = img_enemyShotDiamond[color]; break;
                    }

                    // 弾リストに追加
                    newShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    newShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = newShot;
                    pEnemyShotSet->pEnemyShotHead->prev = newShot;
                }

                // 親弾は逆方向(180度)へ加速させて画面外へ捨て去る
                // これにより「中央がくり抜かれる」ギャスケットの特徴を表現する
                pEnemyShot->muki += DX_PI;
                pEnemyShot->speed = 6.0;
                
                if (pEnemyShot->count > 30) pEnemyShot->x = 99999;
            }
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Sierpinski_Qwen()
{
    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // ゆらゆらと移動
        enemy.x += 0.5 * sin(count * 0.02);
        enemy.y += 0.3 * cos(count * 0.015);
    }

    // 60フレームごとに弾幕セットを生成
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSierpinski;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        // 自機狙い
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        // 弾リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 初期弾を3方向(0度, 120度, -120度)に生成
        double angles[3] = { 0.0, DX_PI * 2.0 / 3.0, -DX_PI * 2.0 / 3.0 };
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + angles[i];
            pEnemyShot->speed = 2.5;
            pEnemyShot->param_i[0] = 5; // 最大分裂レベル

            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 最初は赤大玉

            // 弾リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 弾幕セットをリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}