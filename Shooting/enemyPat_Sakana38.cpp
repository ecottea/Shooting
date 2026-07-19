// enemyPat_reflectLaserGrid.cpp
// 多重反射レーザー・グリッド弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 反射パネルの位置（画面端を想定）
const double PANEL_LEFT = 0.0;
const double PANEL_RIGHT = 480.0;
const double PANEL_TOP = 0.0;
const double PANEL_BOTTOM = 480.0;

// レーザーの反射回数上限
const int MAX_REFLECT_COUNT = 5;

// レーザーが反射パネルに当たったかどうかを判定し、反射処理を行う
static void ReflectLaser(sEnemyShot* pLaser)
{
    // 反射回数が上限に達していれば何もしない
    if (pLaser->param_i[0] >= MAX_REFLECT_COUNT) return;

    // 画面端（反射パネル）に当たったかどうか
    bool reflected = false;
    double newMuki = pLaser->muki;

    // 左右のパネルに当たった場合：水平方向を反転
    if (pLaser->x <= PANEL_LEFT || pLaser->x >= PANEL_RIGHT) {
        newMuki = DX_PI - pLaser->muki;
        reflected = true;
    }
    // 上下のパネルに当たった場合：垂直方向を反転
    if (pLaser->y <= PANEL_TOP || pLaser->y >= PANEL_BOTTOM) {
        newMuki = -pLaser->muki;
        reflected = true;
    }

    if (reflected) {
        pLaser->muki = newMuki;
        pLaser->param_i[0]++; // 反射回数をカウント
    }
}

// 弾幕パターン：多重反射レーザー・グリッド
static void ShotReflectLaserGrid(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音：中くらいの重さのレーザー発射音
        if (!CheckSoundMem(sound_enemyShot_medium)) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // レーザーを複数本発射（例：4方向）
        const int numLasers = 8;
        const double baseAngle = DX_PI / 4.0; // 90度刻み

        for (int i = 0; i < numLasers; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置は敵の位置から少し下にずらす
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 20.0;

            // 向き：0°, 90°, 180°, 270° の4方向
            pEnemyShot->muki = baseAngle * i;

            // 速度：一定（例：3.0）
            pEnemyShot->speed = 3.0;

            // 弾の種類：レーザー（img_enemyShotLaser[i]）
            // 色は i に応じて変える（例：0:赤, 1:黄, 2:緑, 3:シアン）
            pEnemyShot->kind = img_enemyShotLaser[i];

            // 反射回数を記録するパラメータ（param_i[0]）
            pEnemyShot->param_i[0] = 0;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 発射済みレーザーの移動と反射処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 反射判定
        ReflectLaser(pEnemyShot);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（名前は EnemyPat_Laser_Sakana() に変更）
void EnemyPat_Laser_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の左右移動（サンプルと同じ）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔でレーザー弾幕セットを生成
    if (count % 60 == 0) { // 60フレームごとに発射
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotReflectLaserGrid;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 弾幕関数内で向きを決めるのでここでは 0 でOK
        pEnemyShotSet->kind = shot_count++; // 弾の種類や色のバリエーションに使える

        // 弾リストのダミーヘッドを用意
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}