// enemyPat_leafCutter.cpp
// 「はっぱカッター」風の弾幕パターン（色・種類を絞った版）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// はっぱカッター風の弾幕パターン
static void ShotLeafCutter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 発射音（軽めの効果音）
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 葉っぱカッターの「刃」をイメージした扇状の弾幕
        const int numBlades = 15;           // 刃の本数
        const double baseAngle = pEnemyShotSet->muki; // プレイヤー方向を基準
        const double spreadAngle = DX_PI / 3.0;       // 左右に60度ずつ広がる

        for (int i = 0; i < numBlades; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置は敵の位置から少し前方に
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 角度を扇状に配置
            double t = (double)i / (numBlades - 1); // 0.0 〜 1.0
            double angleOffset = (t - 0.5) * spreadAngle; // -spread/2 〜 +spread/2
            pEnemyShot->muki = baseAngle + angleOffset;

            // 速度は一定だが、外側ほど少し速くする
            pEnemyShot->speed = 3.0 + 1.0 * fabs(angleOffset) / (spreadAngle * 0.5);

            // 弾の種類と色を絞る
            // 種類: 鱗弾（Scale）か菱形弾（Diamond）のどちらかに固定
            // 色 : 緑系（2:緑）のみ
            int type = (i % 2 == 0) ? 4 : 5; // 4:鱗弾, 5:菱形弾
            int color = 2;                   // 2:緑

            switch (type) {
            case 4:
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            case 5:
                pEnemyShot->kind = img_enemyShotDiamond[color];
                break;
            }

            if (i == numBlades / 2) pEnemyShot->kind = img_enemyShotLargeBall[color];

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存の弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（指定どおりの関数名）
void EnemyPat_RazorLeaf_Sakana()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // はっぱカッター弾幕の発射
    if (count % 20 == 0) { // 20フレームごとに発射
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLeafCutter;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}