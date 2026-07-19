// enemyPat_Tmp.cpp
// 「反射レーザー式・戻り弾幕」の実装例

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 反射レーザー弾幕本体
static void ShotReflectLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回呼び出し時にレーザーを生成
    if (pEnemyShotSet->count == 0) {
        // 効果音: 中くらいの重さの弾発射音を使う
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // レーザー本体の弾を1つ生成
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki; // プレイヤー方向を向く
        pEnemyShot->speed = 3.0;                // レーザーは速め

        // 弾の種類: 銃弾（細長い見た目でレーザーっぽい）
        // 弾の色: シアン（3）でレーザーらしく
        pEnemyShot->kind = img_enemyShotBullet[3];

        // 反射回数と戻り弾幕フラグを初期化
        pEnemyShot->param_i[0] = 0; // 反射回数
        pEnemyShot->param_i[1] = 0; // 戻り弾幕発生フラグ（0=未発生, 1=発生済）

        // リストに追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // レーザー弾の移動と反射処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 画面端で反射（簡易版）
        if (pEnemyShot->x < 0.0 || pEnemyShot->x > 480.0) {
            pEnemyShot->muki = DX_PI - pEnemyShot->muki; // 左右反射
            pEnemyShot->param_i[0]++;                    // 反射回数+1
        }
        if (pEnemyShot->y < 0.0 || pEnemyShot->y > 480.0) {
            pEnemyShot->muki = -pEnemyShot->muki;        // 上下反射
            pEnemyShot->param_i[0]++;                    // 反射回数+1
        }

        // 3回以上反射したら戻り弾幕を発生（未発生の場合のみ）
        if (pEnemyShot->param_i[0] >= 3 && pEnemyShot->param_i[1] == 0) {
            // 戻り弾幕発生フラグを立てる
            pEnemyShot->param_i[1] = 1;

            // 戻り弾幕の効果音: 重めの弾発射音
            if (CheckSoundMem(sound_enemyShot_heavy) == 1)
                StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 戻り弾幕: レーザーの現在位置から周囲に弾をばら撒く
            for (int i = 0; i < 12; i++) {
                sEnemyShot* pReturnShot = new sEnemyShot;
                pReturnShot->x = pEnemyShot->x * 0.99 + 240 * 0.01;
                pReturnShot->y = pEnemyShot->y * 0.99 + 240 * 0.01;
                // 全方位に均等に広がるように角度を設定
                pReturnShot->muki = (i * DX_PI / 6.0) + (GetRand(30) - 15) / 180.0 * DX_PI;
                pReturnShot->speed = (150 + GetRand(100)) / 100.0; // 中速

                // 戻り弾幕は小玉（赤）で目立たせる
                pReturnShot->kind = img_enemyShotSmallBall[0]; // 色0=赤

                // 戻り弾幕用パラメータ（必要なら）
                pReturnShot->param_i[1] = 1; // 戻り弾幕であることを示すフラグ

                // リストに追加
                pReturnShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pReturnShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pReturnShot;
                pEnemyShotSet->pEnemyShotHead->prev = pReturnShot;
            }
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Reverse_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で反射レーザー弾幕を発射
    if (count % 80 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotReflectLaser;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}