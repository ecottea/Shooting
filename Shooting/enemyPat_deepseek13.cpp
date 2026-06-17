// enemyPat_LeafCutter.cpp
// はっぱカッターをモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：回転する刃（葉っぱカッター）
static void LeafCutterShot(sEnemyShotSet* pEnemyShotSet)
{
    const int    NUM_BULLETS = 8;          // 刃の枚数
    const double RADIUS = 50.0;       // 回転半径
    const double CENTER_SPEED = 4.0;        // 中心の移動速度
    const double ROT_SPEED = 0.15;       // 回転速度 [rad/frame]

    // 初回のみ弾を生成
    if (pEnemyShotSet->count == 0) {        
        for (int i = 0; i < NUM_BULLETS; ++i) {
            sEnemyShot* p = new sEnemyShot;

            // 初期座標はセットの中心（後で回転させる）
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;

            // muki に各弾の初期角度オフセットを保存（移動方向としては使わない）
            p->muki = (2.0 * DX_PI) * i / NUM_BULLETS;
            p->speed = 0.0;

            // 緑色の鱗弾（葉っぱをイメージ）
            p->kind = img_enemyShotScale[2];

            // 双方向リストに追加
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 中心を設定方向へ移動
    pEnemyShotSet->x += CENTER_SPEED * cos(pEnemyShotSet->muki);
    pEnemyShotSet->y += CENTER_SPEED * sin(pEnemyShotSet->muki);

    // 各弾を中心の周りで回転させる
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        double angle = p->muki + ROT_SPEED * pEnemyShotSet->count;
        p->x = pEnemyShotSet->x + RADIUS * cos(angle);
        p->y = pEnemyShotSet->y + RADIUS * sin(angle);
        p = p->next;
    }
}

static void LeafCutterShot2(sEnemyShotSet* pEnemyShotSet)
{
    const int    NUM_BULLETS = 8;          // 刃の枚数
    const double RADIUS = 25.0;       // 回転半径
    const double CENTER_SPEED = 2.0;        // 中心の移動速度
    const double ROT_SPEED = 0.15;       // 回転速度 [rad/frame]

    // 初回のみ弾を生成
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < NUM_BULLETS; ++i) {
            sEnemyShot* p = new sEnemyShot;

            // 初期座標はセットの中心（後で回転させる）
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;

            // muki に各弾の初期角度オフセットを保存（移動方向としては使わない）
            p->muki = (2.0 * DX_PI) * i / NUM_BULLETS;
            p->speed = 0.0;

            // 緑色の鱗弾（葉っぱをイメージ）
            p->kind = img_enemyShotScale[2];

            // 双方向リストに追加
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 中心を設定方向へ移動
    pEnemyShotSet->x += CENTER_SPEED * cos(pEnemyShotSet->muki);
    pEnemyShotSet->y += CENTER_SPEED * sin(pEnemyShotSet->muki);

    // 各弾を中心の周りで回転させる
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        double angle = p->muki + ROT_SPEED * pEnemyShotSet->count;
        p->x = pEnemyShotSet->x + RADIUS * cos(angle);
        p->y = pEnemyShotSet->y + RADIUS * sin(angle);
        p = p->next;
    }
}

// 敵本体のパターン
void EnemyPat_RazorLeaf_DeepSeek()
{
    static int muki;

    // 初期化（count == 1 のフレームのみ実行）
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.8 * muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 40 フレームごとに葉っぱカッターを発射
    if (count % 40 == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = LeafCutterShot;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        // プレイヤーの方向を向く
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

        // 弾リストのダミーヘッドを作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 敵弾セットリストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    if (count % 5 == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = LeafCutterShot2;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        // プレイヤーの方向を向く
        pSet->muki = GetRand(359) / 360. * DX_PI;

        // 弾リストのダミーヘッドを作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 敵弾セットリストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}