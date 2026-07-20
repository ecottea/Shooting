// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 鉄線（壁）のX座標を正弦波で計算する関数
// y: Y座標, offset: 通路の中心からの距離（プラスマイナスで壁の幅を決める）
static double GetCoilX(int y, double offset, double base_x) {
    // 画面幅480の中心(240)を基準に、周期240、振幅80のS字カーブを描く
    return base_x + 80.0 * sin(y * DX_PI / 120.0) + offset;
}

// 弾幕：イライラ・コイル（動く鉄線の壁）
static void ShotIrairaCoil(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初期化時のみ、壁を構成する弾を一気に生成する
    if (pEnemyShotSet->count == 0) {
        // 予告音を鳴らす
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 通路の片側のオフセット幅（自機がギリギリ通れる狭さ）
        double offset = 15.0;

        // Y座標 -100 から 600 まで、8ピクセル間隔で弾を敷き詰める
        for (int y = -100; y <= 600; y += 8) {

            // --- 壁1（赤い小玉） ---
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = GetCoilX(y, -offset, pEnemyShotSet->x);
            pEnemyShot->y = (double)y;

            // 壁の形状を保つため、次の座標への接線方向に進むように速度と向きを設定
            double dx1 = GetCoilX(y + 1, -offset, pEnemyShotSet->x) - pEnemyShot->x;
            double dy1 = 1.0;
            double mag1 = sqrt(dx1 * dx1 + dy1 * dy1);
            pEnemyShot->speed = 1.5 * mag1; // Y方向に毎フレーム1.5ピクセル進む速さ
            pEnemyShot->muki = atan2(dy1, dx1);

            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤の小玉

            // 双方向リストへの追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;


            // --- 壁2（青い小玉） ---
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = GetCoilX(y, offset, pEnemyShotSet->x);
            pEnemyShot->y = (double)y;

            double dx2 = GetCoilX(y + 1, offset, pEnemyShotSet->x) - pEnemyShot->x;
            double dy2 = 1.0;
            double mag2 = sqrt(dx2 * dx2 + dy2 * dy2);
            pEnemyShot->speed = 1.5 * mag2;
            pEnemyShot->muki = atan2(dy2, dx2);

            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青の小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- 通路内の障害物（手ブレ誘発弾：黄色い中玉） ---
        // 通路の真ん中に、80ピクセル間隔で配置
        for (int y = 0; y <= 480; y += 80) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = GetCoilX(y, 0.0, pEnemyShotSet->x); // 通路の中央
            pEnemyShot->y = (double)y;

            // 障害物も壁に合わせて流れるようにする
            double dx3 = GetCoilX(y + 1, 0.0, pEnemyShotSet->x) - pEnemyShot->x;
            double dy3 = 1.0;
            double mag3 = sqrt(dx3 * dx3 + dy3 * dy3);
            pEnemyShot->speed = 1.5 * mag3;
            pEnemyShot->muki = atan2(dy3, dx3);

            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄色の中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Irairabou_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 60フレーム（約1秒）ごとに次の鉄線を発射
    // 連続して流れてくることで、画面上に常時イライラ棒の状態を維持する
    if (count % 60 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotIrairaCoil;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}