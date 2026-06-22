// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 双曲線弾幕：双曲線上に弾を配置し、それぞれを双曲線の接線方向に発射
static void ShotHyperbola(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音を再生
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 双曲線のパラメータ（画面内に収まるように調整）
        const double a = 30.0;  // 双曲線のスケール
        const double b = 30.0;  // 双曲線のスケール
        const int shotCount = 35; // 24発の弾を双曲線上に配置

        for (int i = 0; i < shotCount; i++) {
            double t = (double)i / shotCount * 2.0 * DX_PI - DX_PI; // -π ~ π
            double x = a * cosh(t); // 双曲線のx座標
            double y = b * sinh(t); // 双曲線のy座標

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + x;
            pEnemyShot->y = pEnemyShotSet->y + y;

            // 双曲線の接線方向に弾を発射
            double dx = sinh(t); // 双曲線のx方向微分
            double dy = cosh(t); // 双曲線のy方向微分
            pEnemyShot->muki = atan2(dy, dx);

            pEnemyShot->speed = 0; // 速度を固定

            // 弾の種類と色を固定（大玉、青）
            pEnemyShot->kind = img_enemyShotLargeBall[4]; // 4:青
            pEnemyShot->margin = 999;

            // 弾をリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        for (int i = 0; i < shotCount; i++) {
            double t = (double)i / shotCount * 2.0 * DX_PI - DX_PI; // -π ~ π
            double x = -a * cosh(t); // 双曲線のx座標
            double y = b * sinh(t); // 双曲線のy座標

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + x;
            pEnemyShot->y = pEnemyShotSet->y + y;

            // 双曲線の接線方向に弾を発射
            double dx = -sinh(t); // 双曲線のx方向微分
            double dy = cosh(t); // 双曲線のy方向微分
            pEnemyShot->muki = atan2(dy, dx);

            pEnemyShot->speed = 0; // 速度を固定

            // 弾の種類と色を固定（大玉、青）
            pEnemyShot->kind = img_enemyShotLargeBall[4]; // 4:青
            pEnemyShot->margin = 999;

            // 弾をリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->count >= 50 && pEnemyShot->count <= 50 + 35) {
            pEnemyShot->speed += 0.1;
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 双曲線弾幕：敵を中心に双曲線を2つ重ねて発射
static void ShotDoubleHyperbola(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音を再生
        if (CheckSoundMem(sound_enemyShot_extreme) == 1) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 双曲線のパラメータ（画面内に収まるように調整）
        const double a1 = 20.0;  // 1つ目の双曲線のスケール
        const double b1 = 30.0;  // 1つ目の双曲線のスケール
        const double a2 = 30.0;  // 2つ目の双曲線のスケール
        const double b2 = 20.0;  // 2つ目の双曲線のスケール
        const int shotCount = 35; // 20発の弾を配置

        for (int i = 0; i < shotCount; i++) {
            double t = (double)i / shotCount * 2.0 * DX_PI - DX_PI; // -π ~ π

            // 1つ目の双曲線
            double x1 = a1 * cosh(t);
            double y1 = b1 * sinh(t);

            // 2つ目の双曲線（90度回転）
            double x2 = a2 * sinh(t);
            double y2 = b2 * cosh(t);

            // 1つ目の双曲線上に弾を配置
            //pEnemyShot = new sEnemyShot;
            //pEnemyShot->x = pEnemyShotSet->x + x1;
            //pEnemyShot->y = pEnemyShotSet->y + y1;
            //pEnemyShot->muki = atan2(y1, x1);
            //pEnemyShot->speed = 0; // 速度を固定
            //pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青
            //pEnemyShot->margin = 999;
            //pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            //pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            //pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            //pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 2つ目の双曲線上に弾を配置
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + x2;
            pEnemyShot->y = pEnemyShotSet->y + y2;
            pEnemyShot->muki = atan2(y2, x2);
            pEnemyShot->speed = 0; // 速度を固定
            pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青
            pEnemyShot->margin = 999;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        for (int i = 0; i < shotCount; i++) {
            double t = (double)i / shotCount * 2.0 * DX_PI - DX_PI; // -π ~ π

            // 1つ目の双曲線
            double x1 = -a1 * cosh(t);
            double y1 = -b1 * sinh(t);

            // 2つ目の双曲線（90度回転）
            double x2 = -a2 * sinh(t);
            double y2 = -b2 * cosh(t);

            // 1つ目の双曲線上に弾を配置
            //pEnemyShot = new sEnemyShot;
            //pEnemyShot->x = pEnemyShotSet->x + x1;
            //pEnemyShot->y = pEnemyShotSet->y + y1;
            //pEnemyShot->muki = atan2(y1, x1);
            //pEnemyShot->speed = 0; // 速度を固定
            //pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青
            //pEnemyShot->margin = 999;
            //pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            //pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            //pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            //pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 2つ目の双曲線上に弾を配置
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + x2;
            pEnemyShot->y = pEnemyShotSet->y + y2;
            pEnemyShot->muki = atan2(y2, x2);
            pEnemyShot->speed = 0; // 速度を固定
            pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青
            pEnemyShot->margin = 999;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->count >= 50 && pEnemyShot->count <= 50 + 40) {
            pEnemyShot->speed += 0.1;
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Hyperbola_Vibe()
{
    static int muki = 1;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵を左右に移動
        enemy.x += 0.5 * (double)muki;
        if (enemy.x < 100.0) muki = 1;
        if (enemy.x > 380.0) muki = -1;
    }

    // 90フレームごとに双曲線弾幕を発射
    if (count % 100 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHyperbola;
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

    // 180フレームごとにダブル双曲線弾幕を発射
    if (count % 100 == 80) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDoubleHyperbola;
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