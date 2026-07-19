// enemyPat_Tmp.cpp
// 潮吹き噴流弾幕の実装例

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 潮吹き噴流弾幕のパターン関数
static void ShotTideSpout(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ発射処理（count == 0 のフレームで潮吹きを開始）
    if (pEnemyShotSet->count == 0) {
        // 効果音: 中くらいの重さの弾発射音を使用
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 潮吹きの「柱」を構成する弾の本数（うねる柱の密度）
        const int NUM_TIDE_SHOTS = 24;

        for (int i = 0; i < NUM_TIDE_SHOTS; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置は敵の位置から少し下にずらす（水面から噴き上がるイメージ）
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 20.0;

            // 潮吹きの「うねり」を表現するための角度
            // 基準角度は真上（-π/2）で、左右にうねる波をサイン波で表現
            double baseAngle = -DX_PI / 2.0; // 真上
            double wavePhase = (i * DX_PI * 2.0) / NUM_TIDE_SHOTS; // 位相
            double waveAmp = DX_PI / 3.0; // うねりの振幅（左右30度程度）

            pEnemyShot->muki = baseAngle + sin(wavePhase) * waveAmp;

            // 速度は一定で、やや速め
            pEnemyShot->speed = 4.0;

            // 弾の種類と色の選択
            // 弾の種類一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
            // 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            // 潮吹き＝水柱なので、水色〜青系の色を選ぶとイメージしやすい
            int colorIndex = 3; // シアン（水色系）
            switch (pEnemyShotSet->kind % 8) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[colorIndex];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[colorIndex];
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotLargeBall[colorIndex];
                break;
            case 3:
                pEnemyShot->kind = img_enemyShotBullet[colorIndex];
                break;
            case 4:
                pEnemyShot->kind = img_enemyShotScale[colorIndex];
                break;
            case 5:
                pEnemyShot->kind = img_enemyShotDiamond[colorIndex];
                break;
            case 6:
                pEnemyShot->kind = img_enemyShotMediumOval[colorIndex];
                break;
            case 7:
                pEnemyShot->kind = img_enemyShotLaser[colorIndex];
                break;
            }

            // 弾をリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 弾を進行方向に進める
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // うねりを追加（時間経過で少しずつ角度を変化させる）
        double timeFactor = pShot->count * 0.05; // 時間経過に応じた位相
        double waveAmp = DX_PI / 12.0; // うねりの振幅（小さめ）
        pShot->muki += cos(timeFactor) * waveAmp * 0.1; // 少しずつ角度を変化

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Spout_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の左右移動（サンプルと同じ）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で潮吹き弾幕セットを生成
    if (count % 40 == 1) { // 60フレームごとに潮吹きを開始
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTideSpout; // 潮吹き噴流パターン
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = 480.0;
        pEnemyShotSet->muki = 0.0; // 潮吹きは真上噴出を基本とするので、ここでは0でも可
        pEnemyShotSet->kind = shot_count++; // 弾の種類切り替え用

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}