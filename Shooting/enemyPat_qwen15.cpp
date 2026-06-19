// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  洗濯機をモチーフにした弾幕
//  - 洗濯モード: 回転する洗濯槽(親弾)から、うねる水流(子弾)を発射
//  - 脱水モード: 洗濯槽が弾け飛び、中心から螺旋状の弾が飛び散る
// ============================================================
static void ShotSentakuki(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 洗濯槽の玉（親弾）を生成 (count == 0)
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        int num_tub = 16;
        double radius = 60.0;
        for (int i = 0; i < num_tub; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (double)i / num_tub * DX_PI * 2.0; // 初期角度をmukiに格納
            pEnemyShot->speed = radius;                           // 半径をspeedに格納
            pEnemyShot->kind = img_enemyShotMediumBall[4];        // 青の中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 脱水モードへの移行 (count == 300)
    if (pEnemyShotSet->count == 300) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->kind == img_enemyShotMediumBall[4]) {
                pEnemyShot->speed = 12.0;             // 半径だったspeedを速度に変更
                pEnemyShot->muki += DX_PI / 6.0;      // 角度をずらして飛び散らせる
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // 脱水モードの弾発射 (count >= 300 && count < 360)
    if (pEnemyShotSet->count >= 300 && pEnemyShotSet->count < 360 && pEnemyShotSet->count % 4 == 0) {
        int num_spin = 12;
        double base_angle = (pEnemyShotSet->count - 300) * 0.4;
        for (int i = 0; i < num_spin; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_angle + (double)i / num_spin * DX_PI * 2.0;
            pEnemyShot->speed = 2.5 + (pEnemyShotSet->count - 300) * 0.05;
            pEnemyShot->kind = img_enemyShotScale[1]; // 黄色の鱗弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の更新処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->kind == img_enemyShotMediumBall[4]) {
            // --- 親弾（洗濯槽）の動き ---
            if (pEnemyShotSet->count < 300) {
                // 洗濯モード: 円運動
                double angle = pEnemyShot->muki + pEnemyShot->count * 0.03;
                pEnemyShot->x = pEnemyShotSet->x + cos(angle) * pEnemyShot->speed;
                pEnemyShot->y = pEnemyShotSet->y + sin(angle) * pEnemyShot->speed;

                // 水流（子弾）の発射
                if (pEnemyShot->count % 45 == 20) {
                    sEnemyShot* pChild = new sEnemyShot;
                    pChild->x = pEnemyShot->x;
                    pChild->y = pEnemyShot->y;
                    pChild->muki = atan2(player.y - pChild->y, player.x - pChild->x);
                    pChild->speed = 2.2;
                    pChild->kind = img_enemyShotSmallBall[3]; // シアンの小玉

                    pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pChild->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
                    pEnemyShotSet->pEnemyShotHead->prev = pChild;
                }
            }
            else {
                // 脱水モード: 直進
                pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
                pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            }
        }
        else {
            // --- 子弾（水流）の動き ---
            // サインカーブでうねる
            pEnemyShot->muki += cos(pEnemyShot->count * 0.15) * 0.08;
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_WashingMachine_Qwen()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
        enemy.y = 80.0 + sin(count * 0.02) * 20.0; // ゆらゆらと上下に動く
    }

    // 弾幕セットの生成 (count == 10)
    if (count % 200 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSentakuki;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 弾幕セットの座標を敵に追従させる
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        if (pSet->patternFunc == ShotSentakuki) {
            pSet->x = enemy.x;
            pSet->y = enemy.y;
        }
        pSet = pSet->next;
    }
}