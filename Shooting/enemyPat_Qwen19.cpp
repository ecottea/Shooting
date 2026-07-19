// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：双曲線 (cosh, sinh) の右分支を描く
// ============================================================
static void ShotHyperbolaRight(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // pEnemyShot->count を時間、muki を t の初期オフセットとして利用
        // t が -3.0 ～ 3.0 の範囲で変化し、明確なカーブを描く
        double t = pEnemyShot->count * 0.04 + pEnemyShot->muki;

        double a = 60.0; // X方向のスケール
        double b = 40.0; // Y方向のスケール

        // 双曲線の基本座標 (右分支)
        double tx = a * cosh(t);
        double ty = b * sinh(t);

        // 回転行列を適用
        double cos_m = cos(pEnemyShotSet->muki);
        double sin_m = sin(pEnemyShotSet->muki);
        double rx = tx * cos_m - ty * sin_m;
        double ry = tx * sin_m + ty * cos_m;

        // 位置を直接上書き (speed=0.0 により直進は無効化)
        pEnemyShot->x = pEnemyShotSet->x + rx;
        pEnemyShot->y = pEnemyShotSet->y + ry;

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 弾幕：双曲線 (cosh, sinh) の左分支を描く
// ============================================================
static void ShotHyperbolaLeft(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pEnemyShot->count * 0.04 + pEnemyShot->muki;
        double a = 60.0;
        double b = 40.0;

        // 双曲線の基本座標 (左分支)
        double tx = -a * cosh(t);
        double ty = -b * sinh(t);

        double cos_m = cos(pEnemyShotSet->muki);
        double sin_m = sin(pEnemyShotSet->muki);
        double rx = tx * cos_m - ty * sin_m;
        double ry = tx * sin_m + ty * cos_m;

        pEnemyShot->x = pEnemyShotSet->x + rx;
        pEnemyShot->y = pEnemyShotSet->y + ry;

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Hyperbola_Qwen()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // リサジュー図形のような滑らかな動き
        enemy.x = 240.0 + 70.0 * sin(count * 0.015);
        enemy.y = 120.0 + 40.0 * cos(count * 0.02);
    }

    // 90フレームごとに弾幕を展開
    if (count % 15 == 0) {
        // 双曲線全体をゆっくり回転させる
        double base_muki = count * 0.02;

        // 効果音 (重厚なサウンド)
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // ----------------------------------------------------
        // 右分支の弾幕セット生成
        // ----------------------------------------------------
        sEnemyShotSet* pSetR = new sEnemyShotSet;
        pSetR->count = 0;
        pSetR->patternFunc = ShotHyperbolaRight;
        pSetR->x = enemy.x;
        pSetR->y = enemy.y;
        pSetR->muki = base_muki;

        pSetR->pEnemyShotHead = new sEnemyShot;
        pSetR->pEnemyShotHead->prev = pSetR->pEnemyShotHead;
        pSetR->pEnemyShotHead->next = pSetR->pEnemyShotHead;

        for (int i = 0; i < 80; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->count = 0;
            pShot->speed = 0.0; // 直進を無効化

            // muki メンバを t の初期オフセットとして流用 (-3.0 ～ 3.0 の範囲)
            pShot->muki = i * 0.15 - 3.0 - 3.0 - 11.0;
            pShot->muki *= 0.5;

            pShot->kind = img_enemyShotDiamond[i % 6]; // 菱形弾
            pShot->margin = 19999;

            pShot->prev = pSetR->pEnemyShotHead->prev;
            pShot->next = pSetR->pEnemyShotHead;
            pSetR->pEnemyShotHead->prev->next = pShot;
            pSetR->pEnemyShotHead->prev = pShot;
        }

        pSetR->prev = enemyShotSetHead.prev;
        pSetR->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetR;
        enemyShotSetHead.prev = pSetR;

        // ----------------------------------------------------
        // 左分支の弾幕セット生成
        // ----------------------------------------------------
        sEnemyShotSet* pSetL = new sEnemyShotSet;
        pSetL->count = 0;
        pSetL->patternFunc = ShotHyperbolaLeft;
        pSetL->x = enemy.x;
        pSetL->y = enemy.y;
        pSetL->muki = base_muki;

        pSetL->pEnemyShotHead = new sEnemyShot;
        pSetL->pEnemyShotHead->prev = pSetL->pEnemyShotHead;
        pSetL->pEnemyShotHead->next = pSetL->pEnemyShotHead;

        for (int i = 0; i < 80; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->count = 0;
            pShot->speed = 0.0;
            pShot->muki = i * 0.15 - 3.0 - 3.0 - 11.0; // t の初期オフセット
            pShot->muki *= 0.5;
            pShot->kind = img_enemyShotScale[i % 6]; // 鱗弾
            pShot->margin = 19999;

            pShot->prev = pSetL->pEnemyShotHead->prev;
            pShot->next = pSetL->pEnemyShotHead;
            pSetL->pEnemyShotHead->prev->next = pShot;
            pSetL->pEnemyShotHead->prev = pShot;
        }

        pSetL->prev = enemyShotSetHead.prev;
        pSetL->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetL;
        enemyShotSetHead.prev = pSetL;
    }
}