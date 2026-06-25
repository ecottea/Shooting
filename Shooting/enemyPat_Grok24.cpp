// enemyPat_Mobius.cpp
// メビウスの帯をモチーフにした弾幕パターン
// 特徴: ねじれたループ状の軌道（パラメトリック曲線 + 回転 + ひねり）を表現
// 弾は円環状に展開しつつ、時間とともに「帯のねじれ」を模した方向反転・位相シフトで動き、
// プレイヤーを包み込むような非自明なループ軌道を描く。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// メビウス帯風弾幕用更新関数
static void ShotMobius(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int NUM_BULLETS = 120;        // リングの弾数（多めで帯の密度感）
    const double BASE_RADIUS = 260.0;   // 初期リング半径
    const double TWIST_SPEED = 0.04;   // ねじれの回転速度
    const double DOWN_SPEED = 2.8;     // 全体下降速度

    if (pEnemyShotSet->count == 0) {
        // 効果音（中くらいの連射感）
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 初期弾生成：円環状に配置（メビウスの「帯」の輪郭）
        for (int i = 0; i < NUM_BULLETS; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (double)i / NUM_BULLETS * DX_PI * 2.0;

            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * BASE_RADIUS;
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * BASE_RADIUS * 0.3; // やや楕円

            // 初期向き：外向き + ねじれのヒント
            pEnemyShot->muki = angle + DX_PI / 2.0;

            pEnemyShot->speed = 2.2 + (i % 3) * 0.3; // 微妙な速度差で帯の厚み

            // 弾種：鱗弾や菱形弾で「帯」の滑らかな印象
            if (i % 3 == 0) {
                pEnemyShot->kind = img_enemyShotScale[i % 9];
            }
            else {
                pEnemyShot->kind = img_enemyShotDiamond[i % 9];
            }

            // パラメータ使用（メビウスねじれ用）
            pEnemyShot->param_i[0] = i;                    // 位相オフセット
            pEnemyShot->param_d[0] = angle;                // 基本角度
            pEnemyShot->param_d[1] = BASE_RADIUS;          // 半径
            pEnemyShot->param_d[2] = 0.0;                  // 時間累積（ねじれ用）

            pEnemyShot->margin = 480;

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム更新：メビウス的ねじれ軌道
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->count++; // 個別カウント（必要なら）

        double t = pEnemyShotSet->count * TWIST_SPEED + pEnemyShot->param_i[0] * 0.1;
        double baseAngle = pEnemyShot->param_d[0];

        // メビウス風パラメトリック運動
        // 半径を時間で変化させつつ、方向を180°ひねるような位相シフト
        double radius = pEnemyShot->param_d[1] * (1.0 - pEnemyShotSet->count * 0.0035); // 徐々に縮小
        if (radius < 10.0) radius = 10.0;

        double twist = sin(t) * DX_PI; // ねじれによる向きの反転

        pEnemyShot->x = pEnemyShotSet->x + cos(baseAngle + t * 1.5) * radius * (0.8 + 0.2 * sin(t * 2));
        pEnemyShot->y = pEnemyShotSet->y + pEnemyShotSet->count * DOWN_SPEED * 0.6
            + sin(baseAngle + t) * radius * 0.5
            + sin(twist) * 35.0; // 垂直方向のひねり

        // 進行方向更新（常に進行方向を向く + ねじれ）
        double dx = cos(baseAngle + twist + t * 3.0) * 1.8;
        double dy = sin(baseAngle + twist + t * 2.5) * 1.8 + DOWN_SPEED;
        pEnemyShot->muki = atan2(dy, dx);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（メビウス帯モチーフ）
void EnemyPat_Mobius_Grok()
{
    static int muki = 1;
    static int phase = 0;

    if (count == 1) {
        // 初期配置（画面上部中央）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
    }
    else {
        // ゆったり左右移動（観察しやすい位置に留まる）
        enemy.x += 1.1 * (double)muki;
        if (enemy.x > 380.0 || enemy.x < 100.0) muki *= -1;

        // 軽い上下揺れで動きにリズム
        enemy.y = 60.0 + sin(count * 0.03) * 12.0;
    }

    // 定期的にメビウス弾幕セットを発射（約1.2秒間隔）
    if (count % 72 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMobius;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 下向き基準
        pEnemyShotSet->kind = phase++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 双方向リストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}