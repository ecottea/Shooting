// EnemyPat_Blizzard_DeepSeek.cpp
// 吹雪をモチーフにした弾幕パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 吹雪弾幕の動作関数（1セットの弾を生成＆移動）
static void ShotBlizzard(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;

    // 最初のフレームだけ弾を生成
    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12; i++) {
            pShot = new sEnemyShot;

            // 出現位置はセットの座標を中心に少しばらつかせる
            pShot->x = pSet->x + GetRand(60) - 30;
            pShot->y = pSet->y + GetRand(20) - 10;

            // 向き：真下（PI/2）を中心に ±20度
            pShot->muki = (90.0 + GetRand(80) - 40) / 180.0 * DX_PI;

            // 速度：1.2 ～ 2.0
            pShot->speed = (120 + GetRand(80)) / 100.0;

            // 弾の種類：小玉(0), 鱗弾(1), 菱形弾(2)
            int type = GetRand(2);
            // 色：白(6), シアン(3), 青(4) をランダムに選ぶ（吹雪っぽい寒色系）
            int color = 6; // 白
            if (GetRand(100) < 40) color = 3;      // シアン
            else if (GetRand(100) < 40) color = 4; // 青

            switch (type) {
            case 0: pShot->kind = img_enemyShotSmallBall[color]; break;
            case 1: pShot->kind = img_enemyShotScale[color];     break;
            case 2: pShot->kind = img_enemyShotDiamond[color];   break;
            }

            // 循環リストに挿入
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 全弾の移動（毎フレーム）
    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 基本移動に加え、横方向の揺らぎ（風による舞い）を追加
        pShot->x += pShot->speed * cos(pShot->muki) + 0.4 * sin(pShot->count * 0.1);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->count++; // 弾ごとの経過フレーム（揺らぎの位相に利用）

        pShot = pShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_Blizzard_DeepSeek()
{
    static int muki;

    // 初回フレームのみ初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;          // 画面中央上部
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.5 * (double)muki;
        if (count % 120 == 60) {
            muki *= -1;          // 一定周期で反転
        }
    }

    // 8フレームごとに吹雪弾幕セットを発射
    if (count % 8 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBlizzard;
        pSet->x = enemy.x;
        pSet->y = enemy.y - 10.0;
        // 向きは一応プレイヤー方向を指しておく（利用は任意）
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

        // 自機弾との接触判定用に循環リストのヘッドを作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 全体の敵弾セットリストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}