// enemyPat_Tmp_Goban.cpp
// 囲碁モチーフ弾幕パターン「碁盤の劫（Goban no Ko）」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ========================
// 囲碁モチーフ弾幕：黒白の石を交互に配置し、囲むような動き
// ========================
static void ShotGoban(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int phase = pEnemyShotSet->count;

    // 発射タイミング
    if (phase == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 碁盤をイメージした19方向（角度）＋中心
        for (int i = -9; i <= 9; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double baseAngle = pEnemyShotSet->muki + (i * 12.0) / 180.0 * DX_PI;
            pEnemyShot->muki = baseAngle;
            pEnemyShot->speed = 1.8 + (abs(i) % 3) * 0.3;

            // 黒石・白石を交互に（囲碁らしい色分け）
            bool isBlack = (i % 2 == 0);
            int color = isBlack ? 7 : 6; // 7:黒, 6:白

            // 大玉で「石」感を強調
            if (abs(i) % 4 == 0) {
                pEnemyShot->kind = img_enemyShotLargeBall[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[color];
            }

            // 少しランダム性を持たせて自然に
            pEnemyShot->muki += (GetRand(20) - 10) / 180.0 * DX_PI * 0.5;

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    else if (phase == 25 || phase == 55) {
        // 2回目の波：少し遅れて「眼」を作るような追加弾
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + cos(i * DX_PI / 4) * 25;
            pEnemyShot->y = pEnemyShotSet->y + sin(i * DX_PI / 4) * 25;

            pEnemyShot->muki = pEnemyShotSet->muki + (i % 2 == 0 ? 0.8 : -0.8);
            pEnemyShot->speed = 2.2;

            int color = (phase == 25) ? 7 : 6; // 黒白交互
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理（全弾共通）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本直進＋時間経過で少し曲がる（囲むような動き）
        double curve = sin(phase * 0.08 + pEnemyShot->x) * 0.015;
        pEnemyShot->muki += curve;

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ========================
// 敵本体パターン
// ========================
void EnemyPat_Tmp()
{
    static int muki = 1;

    if (count == 1) {
        // 初期位置（上部中央）
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆったり左右往復移動
        enemy.x += 1.35 * (double)muki;
        if (count % 95 == 0) muki *= -1;

        // 時々軽く上下移動
        if (count % 140 < 30) {
            enemy.y += 0.6;
        }
        else if (count % 140 < 70) {
            enemy.y -= 0.6;
        }
    }

    // 弾幕発動（約1.2秒間隔）
    if (count % 24 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGoban;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;

        // プレイヤー方向を基本角度に
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

        // 双方向リスト初期化
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