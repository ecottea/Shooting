// enemyPat_Tmp.cpp
// 囲碁をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕パターン：囲碁の打ち込みと捕獲
static void ShotGoCapture(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 黒石を 5 つ打つ (ばらまき気味に横一列)
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i - 2) * 32.0;  // 横に並べる
            pEnemyShot->y = pEnemyShotSet->y;
            // ほぼ真下へゆっくり (角度にわずかなランダムを加える)
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = 4.0;  // 遅め

            // 黒 (インデックス7) の小玉
            pEnemyShot->kind = img_enemyShotSmallBall[7];

            // 双方向リンクへ挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 一定時間後に黒石を停止させ、白石を四方に放つ
        if (pEnemyShot->count == 50 && pEnemyShot->speed >= 4.0 - 1.1 * 1 - 0.1) {
            if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // 白石 (インデックス6) を斜め四方向に生成
            const double angles[4] = {
                DX_PI / 4.0,        // 右下
                3.0 * DX_PI / 4.0,  // 左下
                5.0 * DX_PI / 4.0,  // 左上
                7.0 * DX_PI / 4.0   // 右上
            };
            for (int i = 0; i < 4; i++) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = pEnemyShot->x;
                pNew->y = pEnemyShot->y;
                pNew->muki = angles[i];
                pNew->speed = pEnemyShot->speed - 1.1;
                pNew->kind = pEnemyShot->kind ^ img_enemyShotSmallBall[6] ^ img_enemyShotSmallBall[7];  // 白石

                // 末尾に追加（循環リストの head の直前）
                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }

            pEnemyShot->x = 9999;
        }

        // 移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン（囲碁打ち）
void EnemyPat_Go_DeepSeek()
{
    static int muki;   // 左右移動の向き

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆっくり左右に動く
        enemy.x += 0.7 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 30フレームごとに新しい“打ち込み”セットを生成
    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGoCapture;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

        // 弾の双方向循環リストヘッダを初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体の ShotSet リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}