// enemyPat_Tmp.cpp
// 花火モチーフ弾幕「菊花大輪」実装
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕パターン：菊花大輪（きくかたいりん）
// 中心から大玉が放射状に広がり、一定時間後に各弾が二次爆発して
// 小さな火花弾を全周に散らす花火風パターン
// ------------------------------------------------------------
static void ShotHanabi(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- 第1段階：大輪の発射（set生成直後の1回だけ） ---
    if (pEnemyShotSet->count == 0) {
        // 花火らしい重めの発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int PARENT_NUM = 48;          // 大輪の弾数
        const double ANGLE_STEP = 2.0 * DX_PI / PARENT_NUM;

        for (int i = 0; i < PARENT_NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * ANGLE_STEP;           // 全周均等
            pEnemyShot->speed = 1.8;                     // 最初はゆっくり

            // 親弾フラグと爆発タイミング（70〜89フレームでバラけさせる）
            pEnemyShot->param_i[0] = 1;                  // 1 = 親弾（二次爆発可能）
            pEnemyShot->param_i[1] = 65 + GetRand(19);   // 爆発フレーム

            // 親弾は大玉。色は赤・橙・黄をサイクル
            int color;
            switch (i % 3) {
            case 0:  color = 0; break;   // 赤
            case 1:  color = 8; break;   // 橙
            default: color = 1; break;   // 黄
            }
            pEnemyShot->kind = img_enemyShotLargeBall[color];

            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 移動・二次爆発処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 削除時にnextが変わるので先に保存
        sEnemyShot* pNext = pShot->next;

        // 親弾は爆発まで少し加速（花火が開く感じ）
        if (pShot->param_i[0] == 1 && pShot->count < pShot->param_i[1]) {
            pShot->speed += 0.045;
        }

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // --- 第2段階：二次爆発 ---
        if (pShot->param_i[0] == 1 && pShot->count == pShot->param_i[1]) {
            const int CHILD_NUM = 12;
            const double CHILD_ANGLE_STEP = 2.0 * DX_PI / CHILD_NUM;

            for (int j = 0; j < CHILD_NUM; j++) {
                sEnemyShot* pChild = new sEnemyShot;
                pChild->x = pShot->x;
                pChild->y = pShot->y;
                // 親の向きを基準に全周に散らす
                pChild->muki = pShot->muki + j * CHILD_ANGLE_STEP;
                // 速度に少しランダム性を持たせる（3.2〜5.1）
                pChild->speed = 0.7 + GetRand(19) / 10.0;
                pChild->param_i[0] = 0;  // 子弾はこれ以上爆発しない

                // 火花は小玉。色はランダム（黒を避ける）
                int ccol = GetRand(8);
                if (ccol == 7) ccol = 6;  // 黒→白
                pChild->kind = img_enemyShotSmallBall[ccol];

                // リンクリストに追加
                pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pChild->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
                pEnemyShotSet->pEnemyShotHead->prev = pChild;
            }

            // 親弾をリストから外して削除（二次爆発後は消える）
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }

        pShot = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Firework_Grok()
{
    static int dir = 1;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;  // 200で固定
        dir = 1;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += 0.7 * (double)dir;
        if (enemy.x < 120.0 || enemy.x > 360.0) {
            dir *= -1;
        }
    }

    // 約3秒ごと（180フレーム）に花火を1発打ち上げる
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHanabi;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = 0.0;          // このパターンでは未使用
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ショットセットをリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}