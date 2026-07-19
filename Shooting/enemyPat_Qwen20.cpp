// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：砂時計（砂が落ちる動きを表現）
static void ShotHourglass(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 一定フレームごとに弾を生成
    if (pEnemyShotSet->count % 3 == 0) {
        // 効果音 (砂が落ちる軽い音)
        // if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        //PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 砂時計の膨らみを表現する角度 theta
        // sin関数を使って、-PI/4 から PI/4 の間を往復させる
        double theta = sin(pEnemyShotSet->count * 0.04) * DX_PI / 4.0;

        // 上向きと下向きの弾を同時に撃つ
        for (int dir = 0; dir < 2; dir++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // dir=0: 上向き (-PI/2), dir=1: 下向き (PI/2)
            double base_muki = (dir == 0) ? -DX_PI / 2.0 : DX_PI / 2.0;
            pEnemyShot->muki = base_muki + theta;

            // 初期速度
            pEnemyShot->speed = 3.0;

            // 弾の種類と色 (砂っぽい黄色の小玉)
            // 利用可能な色: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒
            int color = 1; // 1:黄
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->margin = 480;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動と速度更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 速度の減衰と逆方向への加速
        // count=0 で speed=3.0, count=100 で speed=0, それ以降は負の速度 (逆方向に加速)
        double v = 3.0 - 0.03 * pEnemyShot->count;
        pEnemyShot->speed = v;

        // 位置の更新
        // speed が負になると、muki の逆方向に進む
        // これにより、上向きの弾が戻ってくるときは下向きの軌道を通り、
        // 砂が上下に行き来する動きを表現する
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Hourglass_Qwen()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 120.0; // 画面中央より少し上 (砂時計のくびれ)
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.8 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 一定時間ごとに弾幕セットを生成
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHourglass;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (count % 3 == 1) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
}