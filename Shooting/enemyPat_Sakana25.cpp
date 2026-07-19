// enemyPat_fan_complex.cpp
// 扇風機モチーフの複雑な弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 扇風機風の弾幕（中心からの風＋外周の回転羽根）
static void ShotFanComplex(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回フレームのみセットアップ
    if (pEnemyShotSet->count == 0) {
        // 扇風機らしい中程度の効果音
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 中心からの「風」の弾（鱗弾）
        const int NUM_WIND_SHOTS = 5;
        const double WIND_SPREAD = DX_PI / 6.0; // 風の広がり角度
        const double WIND_SPEED = 2.5;

        for (int i = 0; i < NUM_WIND_SHOTS; i++) {
            pEnemyShot = new sEnemyShot;

            // 中心からプレイヤー方向へ少し広がるように発射
            double baseAngle = pEnemyShotSet->muki; // プレイヤー方向
            double offset = (i - (NUM_WIND_SHOTS - 1) / 2.0) * (WIND_SPREAD / (NUM_WIND_SHOTS - 1));
            pEnemyShot->muki = baseAngle + offset;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->speed = WIND_SPEED;

            // 弾の種類：鱗弾（img_enemyShotScale）
            // 色：シアン系（3）を中心に少しずつ変える
            int colorIndex = 3 + i % 2; // 3〜7,0,1... と循環
            pEnemyShot->kind = img_enemyShotScale[colorIndex];

            pEnemyShot->margin = 480;

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 外周の「羽根」の弾（菱形弾）
        const int NUM_BLADES = 8;
        const double BLADE_RADIUS = 60.0;
        const double BLADE_SPEED = 1.2;
        const double BLADE_ROT_SPEED = 0.02; // 羽根の回転速度

        for (int i = 0; i < NUM_BLADES; i++) {
            pEnemyShot = new sEnemyShot;

            // 初期位置：円周上に等間隔
            double angle = (2.0 * DX_PI * i) / NUM_BLADES;
            pEnemyShot->x = pEnemyShotSet->x + BLADE_RADIUS * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + BLADE_RADIUS * sin(angle);

            // 弾の向き：円の外側へ向かう方向（少しランダムで変化をつける）
            pEnemyShot->muki = angle + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = BLADE_SPEED;

            // 弾の種類：菱形弾（img_enemyShotDiamond）
            // 色：青系（4）を中心に少しずつ変える
            int colorIndex = 4 + i % 2; // 4〜7,0,1... と循環
            pEnemyShot->kind = img_enemyShotDiamond[colorIndex];

            // 回転速度と初期角度をパラメータとして保存
            pEnemyShot->param_d[0] = BLADE_ROT_SPEED; // 回転角速度
            pEnemyShot->param_d[1] = angle;           // 初期角度

            pEnemyShot->margin = 480;

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの更新処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 中心からの風の弾：まっすぐ進む
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 外周の羽根の弾：中心を回りながら外側へ広がる
        if (pEnemyShot->param_d[0] != 0.0) {
            // 中心からの相対位置を計算
            double dx = pEnemyShot->x - pEnemyShotSet->x;
            double dy = pEnemyShot->y - pEnemyShotSet->y;

            // 回転行列で少しずつ回す
            double rot = pEnemyShot->param_d[0];
            double cosRot = cos(rot);
            double sinRot = sin(rot);

            double newDx = dx * cosRot - dy * sinRot;
            double newDy = dx * sinRot + dy * cosRot;

            pEnemyShot->x = pEnemyShotSet->x + newDx;
            pEnemyShot->y = pEnemyShotSet->y + newDy;

            // 弾の向きも回転に合わせて更新
            pEnemyShot->muki += rot;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（扇風機ボス風）
void EnemyPat_ElectricFan_Sakana()
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
        // 左右にゆっくり往復
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で扇風機弾幕セットを生成
    if (count % 40 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFanComplex;
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