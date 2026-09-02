// enemyPat_sunflower.cpp
// 弾幕：日輪の舞（ひまわりモチーフ）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --------------------------------------------------
// 弾幕パターン：日輪の舞
// --------------------------------------------------
//  ・中心の「種」を大玉（黒）で表現
//  ・「花弁」を中楕円弾（黄）で表現し、カーブしながら広がる
//  ・種がプレイヤー方向をゆるやかに追従
//  ・一定時間後、種が「満開」となり全方位に小玉（黄・橙）を飛散
// --------------------------------------------------
static void ShotSunflowerDance(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double PI = DX_PI;

    // --- Phase 0: 初期生成（count == 0 のみ実行） ---
    if (pEnemyShotSet->count == 0) {
        // 予告＋生成音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 【種】大玉・黒色。中心に1つ。
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShot->speed = 1.0;
        pEnemyShot->kind = img_enemyShotLargeBall[7]; // 7:黒
        pEnemyShot->param_i[0] = 0; // 0=種
        pEnemyShot->param_i[1] = 0; // 状態管理用リザーブ

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 【花弁】中楕円弾・黄色。8方向に放射状。
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            double baseAngle = (PI * 2.0 / 8.0) * i;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle;
            pEnemyShot->speed = 1.6;
            pEnemyShot->kind = img_enemyShotMediumOval[1]; // 1:黄
            pEnemyShot->param_i[0] = 1;     // 1=花弁
            pEnemyShot->param_d[0] = baseAngle;         // 基準角度
            pEnemyShot->param_d[1] = 0.006;             // 回転速度（時計回り）
            if (i % 2 == 1) pEnemyShot->param_d[1] = -0.006; // 反時計回りを交互に

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 種の位置を追跡（ShotSet にも記録しておく） ---
    sEnemyShot* pSeed = nullptr;
    double seedX = pEnemyShotSet->x;
    double seedY = pEnemyShotSet->y;

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 0) {
            pSeed = pEnemyShot;
            seedX = pEnemyShot->x;
            seedY = pEnemyShot->y;
            break;
        }
        pEnemyShot = pEnemyShot->next;
    }

    // 種の現在位置を ShotSet にバックアップ（満開時に使う）
    if (pSeed != nullptr) {
        pEnemyShotSet->param_d[0] = seedX;
        pEnemyShotSet->param_d[1] = seedY;
    }

    // --- 追加花弁：種が生存中、20フレームごとに回転しながら生成 ---
    if (pSeed != nullptr && pEnemyShotSet->count > 0 && pEnemyShotSet->count < 200 && pEnemyShotSet->count % 20 == 0) {
        double rot = pEnemyShotSet->count * 0.08; // 回転角
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = rot + PI * i; // 2方向

            pEnemyShot->x = seedX;
            pEnemyShot->y = seedY;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 1.8;
            pEnemyShot->kind = img_enemyShotMediumOval[1]; // 黄
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = (pEnemyShotSet->count % 40 == 0) ? 0.008 : -0.008;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 満開：count == 200 で種が爆発し、全方位に小玉を飛散 ---
    if (pSeed != nullptr && pEnemyShotSet->count == 200) {
        // 種を画面外へ（メインルーチンが消去）
        pSeed->x = -1000.0;
        pSeed->y = -1000.0;

        // 爆発音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 種の最後の位置を ShotSet から復元
        double bx = pEnemyShotSet->param_d[0];
        double by = pEnemyShotSet->param_d[1];

        for (int i = 0; i < 24 * 5; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (PI * 2.0 / 24.0 / 5) * i;

            pEnemyShot->x = bx;
            pEnemyShot->y = by;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 1.8 + (i % 6) * 0.4; // 少しずつ速度差をつける
            int color = (i % 2 == 0) ? 1 : 8; // 1:黄, 8:橙
            pEnemyShot->kind = img_enemyShotSmallBall[color];
            pEnemyShot->param_i[0] = 2; // 2=飛散弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 弾の更新 ---
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 0) {
            // 【種】プレイヤー方向をゆるやかに向きながら追従
            double targetMuki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            double diff = targetMuki - pEnemyShot->muki;
            while (diff > PI)  diff -= PI * 2.0;
            while (diff < -PI) diff += PI * 2.0;
            pEnemyShot->muki += diff * 0.025; // 追従率（小さいほどゆるやか）
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        else if (pEnemyShot->param_i[0] == 1) {
            // 【花弁】カーブしながら外側へ広がる
            pEnemyShot->muki += pEnemyShot->param_d[1];
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        else if (pEnemyShot->param_i[0] == 2) {
            // 【飛散弾】直進
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// --------------------------------------------------
// 敵本体のパターン
// --------------------------------------------------
void EnemyPat_Sunflower_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり揺れる
        enemy.x += 0.6 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }

    // 90フレーム（1.5秒）ごとに弾幕セットを生成
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSunflowerDance;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
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