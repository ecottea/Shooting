// enemyPat_poison.cpp
// 「毒」をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 毒をモチーフにした弾幕パターン
static void ShotPoison(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 最初のフレームで毒弾を一斉に生成
    if (pEnemyShotSet->count == 0) {
        // 毒っぽい効果音（中くらいの重さを想定）
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 毒弾を 8 方向に発射
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 基本は 8 方向
            pEnemyShot->muki = pEnemyShotSet->muki + (i * DX_PI / 4.0);
            // 毒弾はゆっくり広がる
            pEnemyShot->speed = 1.0 + GetRand(50) / 100.0; // 1.0〜1.5 くらい

            // 毒っぽい色: 緑(2), 紫(5), 黒(7) を優先
            int color;
            int r = GetRand(9); // 0〜9
            if (r < 4) color = 2;      // 緑
            else if (r < 7) color = 5; // 紫
            else color = 7;            // 黒

            // 弾種は小玉・中玉・鱗弾・菱形弾からランダム
            int type = GetRand(3); // 0:小玉, 1:中玉, 2:鱗弾, 3:菱形弾
            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            case 3:
                pEnemyShot->kind = img_enemyShotDiamond[color];
                break;
            }

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 一定フレームごとに追加の毒弾を吐き出す
    if (pEnemyShotSet->count % 30 == 1 && pEnemyShotSet->count <= 180) {
        // 追加の毒弾はプレイヤー方向を基準に少しランダム
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0; // 2.0〜3.0 くらい

            // 追加弾は緑か紫
            int color = (GetRand(1) == 0) ? 2 : 5;
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 毒っぽくうねるような動きを加える
        double wobble = cos(pEnemyShot->count * 0.1) * 0.05;
        pEnemyShot->muki += wobble;

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot->count++;
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（関数名は指定通り）
void EnemyPat_Poison_Sakana()
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

    // 一定間隔で毒弾幕セットを生成
    if (count % 20 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPoison;
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