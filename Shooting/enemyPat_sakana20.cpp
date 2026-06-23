// enemyPat_sandglass.cpp
// 砂時計をモチーフにした弾幕パターン（色・種類を絞った版）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 砂時計をイメージした弾幕パターン
static void ShotSandglass(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音再生（軽めの音）
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 砂時計の「上から下へ落ちる砂」をイメージ
        const int NUM_SHOTS = 24; // 砂時計の輪郭をなぞる弾数
        const double RADIUS_UPPER = 80.0; // 上側の半径
        const double RADIUS_LOWER = 80.0; // 下側の半径
        const double Y_OFFSET_UPPER = -40.0; // 上側の中心Yオフセット
        const double Y_OFFSET_LOWER = 40.0; // 下側の中心Yオフセット

        for (int i = 0; i < NUM_SHOTS; i++) {
            pEnemyShot = new sEnemyShot;

            // 上側円弧上の位置
            double angle = (i * 2.0 * DX_PI) / NUM_SHOTS;
            pEnemyShot->x = pEnemyShotSet->x + RADIUS_UPPER * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + Y_OFFSET_UPPER + RADIUS_UPPER * sin(angle);

            // 砂時計の中央（くびれ部分）に向かう向き
            double centerX = pEnemyShotSet->x;
            double centerY = pEnemyShotSet->y;
            pEnemyShot->muki = atan2(centerY - pEnemyShot->y, centerX - pEnemyShot->x);

            // 速度はややゆっくりめ
            pEnemyShot->speed = (100 + GetRand(50)) / 100.0;

            // 弾の種類と色を絞る
            // 0:小玉, 1:鱗弾
            int type = GetRand(1);
            // 色は 1:黄 か 8:橙 のどちらか
            int color = (GetRand(1) == 0) ? 1 : 8;

            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            }

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（関数名はご指定どおり）
void EnemyPat_Hourglass_Sakana()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で砂時計弾幕セットを生成
    if (count % 10 == 0) { // 3フレームごとなど、適宜調整
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSandglass;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 弾幕関数内で個別に設定するのでここでは0でOK

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}