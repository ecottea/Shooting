// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

/*
 * 利用可能な素材一覧 (enemyPat_sampleForAI.cpp より把握)
 *
 * 【画像】
 * img_enemyShotSmallBall[8]   : 小玉
 * img_enemyShotMediumBall[8]  : 中玉
 * img_enemyShotLargeBall[8]   : 大玉
 * img_enemyShotBullet[8]      : 銃弾
 * img_enemyShotScale[8]       : 鱗弾
 * img_enemyShotDiamond[8]     : 菱形弾
 * ※ [8] のインデックスは 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒
 *
 * 【効果音】
 * sound_enemyShot_light
 * sound_enemyShot_medium
 * sound_enemyShot_heavy
 * sound_enemyShot_extreme
 */

 // 弾幕：囲碁「星打ち」
static void ShotIgo(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 一定間隔で碁盤の交点めがけて弾（碁石）を打つ
    if (pEnemyShotSet->count % 40 == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 碁盤の交点座標をランダムに決定 (48ピッチのグリッド、8x8マス)
        // GetRand(x) は 0～x を返すため、GetRand(8) * 48 で 0～384 (48ピッチ)
        double target_x = 48.0 + GetRand(8) * 48.0;
        double target_y = 48.0 + GetRand(8) * 48.0;

        double dx = target_x - pEnemyShotSet->x;
        double dy = target_y - pEnemyShotSet->y;
        double dist = sqrt(dx * dx + dy * dy);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = atan2(dy, dx);

        // 100フレームで交点に到達するように速度を調整
        pEnemyShot->speed = dist / 100.0;

        // 黒(7)と白(6)の中玉を交互に生成
        int color = (pEnemyShotSet->count / 40) % 2 == 0 ? 7 : 6;
        pEnemyShot->kind = img_enemyShotMediumBall[color];

        // リストに追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 弾の移動と状態遷移
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pEnemyShot->next;

        if (pEnemyShot->count < 100) {
            // 交点に向かって飛ぶ
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        else if (pEnemyShot->count == 100) {
            if (pEnemyShot->kind == img_enemyShotMediumBall[7] || pEnemyShot->kind == img_enemyShotMediumBall[6]) {
                // 交点に到達、着地（停止）
                pEnemyShot->speed = 0.0;
            }
            else {
                pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
                pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            }
        }
        else if (pEnemyShot->count == 160) {
            if (pEnemyShot->kind == img_enemyShotMediumBall[7] || pEnemyShot->kind == img_enemyShotMediumBall[6]) {
                // 着地から60フレーム後、周囲に弾を放出して消滅（領地を広げる）
                for (int i = 0; i < 8; i++) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    pNewShot->x = pEnemyShot->x;
                    pNewShot->y = pEnemyShot->y;
                    pNewShot->muki = (DX_PI * 2.0 / 8.0) * i;
                    pNewShot->speed = 1.5;

                    // 黒なら赤(0)、白なら青(4)の小玉に変化
                    int color = (pEnemyShot->kind == img_enemyShotMediumBall[7]) ? 0 : 4;
                    pNewShot->kind = img_enemyShotSmallBall[color];

                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }

                // 自身をリストから外して削除
                pEnemyShot->prev->next = pEnemyShot->next;
                pEnemyShot->next->prev = pEnemyShot->prev;
                delete pEnemyShot;
            }
            else {
                pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
                pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            }
        }
        else {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_Go_Qwen()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.2 * (double)muki;
        if (count % 150 == 100) muki *= -1;
    }

    // 一定間隔で弾幕パターンを生成
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotIgo;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
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