// enemyPat_Tmp.cpp
// 紅葉（もみじ）をモチーフにした弾幕パターン
// 落ち葉のように漂う菱形弾・鱗弾を中心に、赤・黄・橙系の色味で秋らしい演出

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotMomiji(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);  // 落ち葉らしいやや重めの音

        // 中央から放射状に落ち葉をばらまく（16方向＋ランダム微調整）
        for (int i = 0; i < 16; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 基本方向は下寄り（秋の落ち葉イメージ）
            double baseAngle = DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI * 0.7;
            pEnemyShot->muki = baseAngle + (i * DX_PI * 2.0 / 16.0);

            pEnemyShot->speed = (120 + GetRand(120)) / 100.0;  // ゆったり落ちる速度

            // 紅葉らしい色と形状（Diamond=菱形、Scale=鱗で葉っぱっぽく）
            int leafType = GetRand(1);  // 0: Diamond, 1: Scale
            int color = GetRand(3);     // 0:赤, 1:黄, 2:緑（秋らしい色中心）
            if (leafType == 0) {
                pEnemyShot->kind = img_enemyShotDiamond[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotScale[color];
            }

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾更新（落ち葉らしいゆらゆら漂う動き）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pEnemyShotSet->count / 8.0;  // ゆったりとした周期

        // 基本進行方向 + 横揺れ（sinで葉っぱの舞う感じ）
        double sway = sin(t + pEnemyShot->x) * 1.2;

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki) + sway;
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki) * 0.9 + 0.8;  // 下方向に少し加速感

        // 徐々に速度を落として自然に落ちていく感じ
        if (pEnemyShotSet->count > 30) {
            pEnemyShot->speed *= 0.995;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    static int muki = 1;

    if (count == 1) {
        // 初期位置（画面上部中央）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 250;
        muki = 1;
    }
    else {
        // ゆったり左右移動
        enemy.x += 1.1 * (double)muki;
        if (count % 140 == 70) muki *= -1;

        // 時々少し上下に動きを加える
        if (count % 80 == 0) {
            enemy.y = 50.0 + sin(count / 40.0) * 20.0;
        }
    }

    // 定期的に紅葉弾幕を発射（少し間隔を空けて）
    if (count % 18 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMomiji;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;

        // プレイヤー方向を少し意識しつつ、下寄りのランダム性を持たせる
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y + GetRand(80) - 40,
            player.x - pEnemyShotSet->x + GetRand(60) - 30);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}