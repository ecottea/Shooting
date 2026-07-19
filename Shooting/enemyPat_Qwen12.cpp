// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：太陽のフレアとコロナ (重層放射状弾幕)
static void ShotSolarFlare(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 1. 初期フレア (count == 0): 低速の赤大玉を全方位に放出
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (DX_PI * 2.0 / 12.0) * i;
            pEnemyShot->speed = 1.2;
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤大玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 2. コロナの回転 (count % 10 == 0): 中速で回転する黄・赤の中玉
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count < 120 && pEnemyShotSet->count % 10 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        int num_shots = 16;
        double angle_offset = pEnemyShotSet->count * 0.08; // 時間とともに角度がずれる(回転)
        for (int i = 0; i < num_shots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (DX_PI * 2.0 / num_shots) * i + angle_offset;
            pEnemyShot->speed = 2.2;
            int color = (i % 2 == 0) ? 1 : 0; // 黄と赤を交互に
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 3. 太陽風 (count % 4 == 0): 高速で逆回転する黄の小玉
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count < 150 && pEnemyShotSet->count % 4 == 0) {
        int num_shots = 8;
        double angle_offset = -pEnemyShotSet->count * 0.05; // 逆方向に回転
        for (int i = 0; i < num_shots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (DX_PI * 2.0 / num_shots) * i + angle_offset;
            pEnemyShot->speed = 3.5;
            pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の座標更新 (メインルーチンでインクリメントされた count に従い動作)
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン (太陽)
void EnemyPat_Sun_Qwen()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 太陽のようにゆっくりと左右に漂う
        enemy.x += 0.6 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 一定間隔で太陽の弾幕セットを生成
    if (count % 90 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSolarFlare;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = 0.0; // 太陽は全方位攻撃のため特定の向きは持たない

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}