// enemyPat_Tmp_LeafCutter.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// はっぱカッター風弾幕パターン
// 特徴：鱗弾（葉っぱ風）を使い、扇状に広がりながら徐々にプレイヤー方向へ曲がる「カッター」挙動
static void ShotLeafCutter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 出現時（count == 0）に弾を生成
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int num = 9;                    // 9枚の葉っぱカッター
        const double baseAngle = pEnemyShotSet->muki;
        const double spread = DX_PI / 3.0;    // ±約60度に扇状展開

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;

            // 敵本体位置から少し下に生成
            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y + 15.0;

            // 角度：中央に向かって密集し、外側に向かって広がる
            double offset = (i - (num - 1) / 2.0) * spread / (num - 1);
            pEnemyShot->muki = baseAngle + offset;

            // 初速は控えめ（カッターらしいゆったりした動き）
            pEnemyShot->speed = 1.8 + (GetRand(60) / 100.0);

            // 葉っぱモチーフとして鱗弾を使用（色は緑～黄系）
            int color = (GetRand(2) == 0) ? 2 : 1; // 緑 or 黄
            pEnemyShot->kind = img_enemyShotScale[color];

            pEnemyShot->margin = 9999;

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの更新処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 緩やかにプレイヤー方向へ曲がる（カッターが追尾するような挙動）
        double targetAngle = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        double diff = targetAngle - pEnemyShot->muki;

        // -PI〜PIに正規化
        while (diff > DX_PI) diff -= DX_PI * 2;
        while (diff < -DX_PI) diff += DX_PI * 2;

        pEnemyShot->muki += diff * 0.035;   // 曲がり具合（0.035程度が自然）

        // 徐々に加速（後半に鋭くなる）
        if (pEnemyShotSet->count > 40) {
            pEnemyShot->speed += 0.018;
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（はっぱカッター弾幕用）
void EnemyPat_RazorLeaf_Grok()
{
    static int muki = 1;
    static int wave = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        wave = 0;
    }
    else {
        // ゆったりとした波状移動（葉っぱが風に揺れるイメージ）
        wave++;
        enemy.x += 1.35 * (double)muki;
        enemy.y = 60.0 + sin(wave * 0.028) * 28.0;

        // 画面端で反転
        if (enemy.x < 90.0 && muki < 0) muki = 1;
        if (enemy.x > 390.0 && muki > 0) muki = -1;
    }

    // 約40フレームごとに葉っぱカッターを発射
    if (count % 40 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLeafCutter;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // プレイヤー方向を基本角度とする
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 双方向リストのヘッドノード作成
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