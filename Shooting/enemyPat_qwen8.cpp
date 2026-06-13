// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ジグザグ落雷
static void ShotZigzagThunder(sEnemyShotSet* pEnemyShotSet)
{
    // 初期弾生成（雷の起点）
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = DX_PI / 2.0; // 下向き
        pEnemyShot->speed = 10.0;       // 速めに設定
        pEnemyShot->kind = img_enemyShotBullet[6]; // 白の銃弾（雷光をイメージ）

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
    // 一定フレームごとに新しい弾を生成し、雷をジグザグに伸ばす
    else if (pEnemyShotSet->count % 2 == 0 && pEnemyShotSet->count < 40) {
        sEnemyShot* pTail = pEnemyShotSet->pEnemyShotHead->prev; // 現在の先端

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pTail->x;
        pEnemyShot->y = pTail->y;
        // 下向きをベースに、左右にランダムな角度をつける
        pEnemyShot->muki = DX_PI / 2.0 + (GetRand(80) - 40) / 180.0 * DX_PI;
        pEnemyShot->speed = 10.0;
        pEnemyShot->kind = img_enemyShotBullet[6]; // 白の銃弾

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 先端だった弾は速度を0にして、その場に固定する（軌跡を残すため）
        pTail->speed = 0.0;
    }
    else if (pEnemyShotSet->count == 40) {
        pEnemyShotSet->pEnemyShotHead->prev->speed = 5.0;
    }
    // 雷が伸びきったら、先端で爆発（放射状に弾を撒く）
    else if (pEnemyShotSet->count == 60) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        sEnemyShot* pTail = pEnemyShotSet->pEnemyShotHead->prev;
        for (int i = 0; i < 12; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pTail->x;
            pEnemyShot->y = pTail->y;
            pEnemyShot->muki = (DX_PI * 2.0 / 12) * i;
            pEnemyShot->speed = 2.0;
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアンの小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全ての弾の座標を更新
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot->count++;
        if (pEnemyShot->count >= 300) {
            pEnemyShot->x = 9999;
            pEnemyShot->y = 9999;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Thunder_Qwen()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.5 * (double)muki;
        if (enemy.x < 60.0 || enemy.x > 420.0) muki *= -1;
    }

    // 一定間隔で新しい雷（sEnemyShotSet）を生成
    if (count % 45 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotZigzagThunder;

        // プレイヤーのX座標付近に落ちるように調整
        pEnemyShotSet->x = player.x + (GetRand(80) - 40);
        pEnemyShotSet->y = 10;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}