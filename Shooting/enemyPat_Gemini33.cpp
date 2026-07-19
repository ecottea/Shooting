// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：全方位格子展開＆絶対回避不能の面制圧
static void ShotAbsoluteDeath(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 1フレーム目（count == 0）：絶対的な殺意を込めた極太音と弾の生成
    if (pEnemyShotSet->count == 0) {
        // 重低音のSEを選択してプレイヤーに絶望を与える
        if (CheckSoundMem(sound_enemyShot_extreme) == 1) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 1. 縦・横の格子状に配置して逃げ場を狭める（全画面格子弾）
        // 画面サイズ 480x480 に対し、40ピクセル刻みで隙間なく配置
        for (int i = 0; i <= 12; i++) {
            double pos = i * 40.0;

            // 上から下へ進む弾（縦ライン）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pos;
            pEnemyShot->y = 0.0;
            pEnemyShot->muki = DX_PI / 2.0; // 真下
            pEnemyShot->speed = 3.5;
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤い銃弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 左から右へ進む弾（横ライン）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 0.0;
            pEnemyShot->y = pos;
            pEnemyShot->muki = 0.0; // 真右
            pEnemyShot->speed = 3.5;
            pEnemyShot->kind = img_enemyShotBullet[4]; // 青い銃弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 2. プレイヤーの現座標を中心に、全方位から収縮する「大玉」の輪（逃げ場を完全に潰す）
        // プレイヤーの周囲360度、15度刻みで24発の大玉を配置
        for (int i = 0; i < 24; i++) {
            double angle = (i * 15.0) / 180.0 * DX_PI;
            pEnemyShot = new sEnemyShot;
            // プレイヤーから250ピクセル離れた円周上から、プレイヤーの座標へ向けて放つ
            pEnemyShot->x = player.x + cos(angle) * 250.0;
            pEnemyShot->y = player.y + sin(angle) * 250.0;
            pEnemyShot->muki = angle + DX_PI; // 中心（プレイヤー方向）に向かう
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotLargeBall[1]; // 黄色の大玉（判定が大きい）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_Unavoidable_Gemini()
{
    static int muki;

    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0; // 少し下げて画面中央寄りに
        enemy.maxHp = enemy.hp = 100; // 仕様通り200固定
        muki = 1;
    }
    else {
        // 敵本体は激しく左右に往復して弾幕の発射位置を狂わせる
        enemy.x += 4.5 * (double)muki;
        if (count % 60 == 0) muki *= -1;
    }

    // 25フレームに1回、容赦ない全画面制圧弾幕を展開
    if (count % 25 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAbsoluteDeath;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 双方向リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体リストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}