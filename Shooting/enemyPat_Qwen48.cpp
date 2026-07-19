// enemyPat_Tmp.cpp
// 潮吹きをモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕パターン関数
// ============================================================

// 泡弾: 圧力上昇を表現する小さな泡。ゆっくり上に浮かび上がる。
static void ShotBubble(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x + GetRand(40) - 20;
            pShot->y = pSet->y + GetRand(20) - 10;
            // ほぼ真上へ、少しばら撒く
            pShot->muki = -DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
            pShot->speed = 1.0 + GetRand(10) / 10.0;

            // 画像: 小玉、色: 6(白)
            pShot->kind = img_enemyShotSmallBall[6];
            pShot->margin = 240;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 飛沫弾: 水流が衝突した際に飛散する水滴。重力で落下する。
static void ShotSplash(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        int num = 12;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;

            // 扇状にばら撒く (上方向を起点に左右60度ずつ)
            double base_muki = -DX_PI / 2.0;
            pShot->muki = base_muki + (i - num / 2.0) * (DX_PI / 12.0);
            pShot->speed = 3.0 + GetRand(20) / 10.0;

            // 画像: 中玉、色: 4(青)
            pShot->kind = img_enemyShotMediumBall[4];
            pShot->margin = 20.0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 速度ベクトルに重力を加算
        double vx = pShot->speed * cos(pShot->muki);
        double vy = pShot->speed * sin(pShot->muki) + 0.2;

        // 新しい速度と向きを計算
        pShot->muki = atan2(vy, vx);
        pShot->speed = sqrt(vx * vx + vy * vy);

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 水流弾: 高圧で脈動しながら噴射されるメインの水流。
static void ShotWaterStream(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = pSet->muki; // 初期は自機狙い
        pShot->speed = 8.0;

        // 画像: 短レーザー、色: 3(シアン)
        pShot->kind = img_enemyShotLaser[3];
        pShot->margin = 20.0;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 弱い自機誘導 (勢いよく直進するが、少しだけ曲がる)
        double target_muki = atan2(player.y - pShot->y, player.x - pShot->x);
        double diff = target_muki - pShot->muki;
        while (diff > DX_PI) diff -= 2 * DX_PI;
        while (diff < -DX_PI) diff += 2 * DX_PI;
        pShot->muki += diff * 0.05;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 寿命(30フレーム)または画面外に到達したら飛散
        if (pShot->count > 25 || pShot->x < 0 || pShot->x > 480 || pShot->y > 480) {
            // 飛沫弾パターンを新たに生成
            sEnemyShotSet* pSplash = new sEnemyShotSet;
            pSplash->count = 0;
            pSplash->patternFunc = ShotSplash;
            pSplash->x = pShot->x;
            pSplash->y = pShot->y;
            pSplash->muki = 0;
            pSplash->kind = 0;

            pSplash->pEnemyShotHead = new sEnemyShot;
            pSplash->pEnemyShotHead->prev = pSplash->pEnemyShotHead;
            pSplash->pEnemyShotHead->next = pSplash->pEnemyShotHead;

            pSplash->prev = enemyShotSetHead.prev;
            pSplash->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSplash;
            enemyShotSetHead.prev = pSplash;

            // 水流弾をリストから外して削除
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            break; // リスト構造が変わったのでループ中断
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Spout_Qwen()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右に移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 【フェーズ1: 圧力上昇】 (0~60フレーム)
    // 予告音を鳴らし、泡弾を発生させる
    const int CYCLE = 300;
    if (count % CYCLE == 10) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count % CYCLE > 0 && count % CYCLE <= 60 && count % CYCLE % 10 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBubble;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 【フェーズ2: 脈動噴射】 (60フレーム以降)
    // 6フレーム噴射 -> 3フレーム停止 のリズムで水流弾を撃つ
    if (count % CYCLE >= 60) {
        if (count % CYCLE % 9 < 6) { // 0,1,2,3,4,5フレーム目で発射
            if (count % CYCLE % 9 == 0) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotWaterStream;
                pSet->x = enemy.x;
                pSet->y = enemy.y + 10.0;
                // 自機狙い
                pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
                pSet->kind = 0;

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
    }
}