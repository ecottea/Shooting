#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：天枢光陣「北斗七星の注ぎ口」
static void ShotBigDipper(sEnemyShotSet* pEnemyShotSet)
{
    // 初回時の効果音再生
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 北斗七星の相対座標データ（勺4個、柄3個）
    const double rel_x[7] = { -40.0, -40.0, 20.0, 20.0, 60.0, 100.0, 140.0 };
    const double rel_y[7] = { -25.0,  25.0, 20.0, -20.0, -15.0, -5.0,  10.0 };

    double cx = pEnemyShotSet->x;
    double cy = pEnemyShotSet->y;
    double theta = pEnemyShotSet->count * 0.012; // 星座全体の緩やかな回転角

    // 一定間隔（例: 3フレームごと）で、柄杓の先端（天枢）から星屑をこぼす
    if (pEnemyShotSet->count % 3 == 0 && pEnemyShotSet->count < 350) {
        // 先端（インデックス0）のワールド座標を計算
        double tip_x = cx + (rel_x[0] * cos(theta) - rel_y[0] * sin(theta));
        double tip_y = cy + (rel_x[0] * sin(theta) + rel_y[0] * cos(theta));

        // 星屑（黄色い小玉と白い小玉）を2つ生成
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = tip_x;
            pEnemyShot->y = tip_y;

            // 下方向へ流れ落ちるような初速と角度
            double scatter_angle = DX_PI/2 + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.8 + GetRand(120) / 100.0;
            pEnemyShot->muki = scatter_angle;

            // 自由パラメータ(param_d[0])に重力加速度を持たせる
            pEnemyShot->param_d[0] = 0.04;

            // 色と種類の選択：黄色(1)または白(6)の小玉
            int color = (i == 0) ? 1 : 6;
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 最初のフレームで北斗七星の形を形作る7つの星（青い中玉）を配置
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 7; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + (rel_x[i] * cos(theta) - rel_y[i] * sin(theta));
            pEnemyShot->y = cy + (rel_x[i] * sin(theta) + rel_y[i] * cos(theta));
            pEnemyShot->muki = atan2(rel_y[i], rel_x[i]) + DX_PI/2;
            pEnemyShot->speed = 0.5;
            pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青の中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動および重力（落下加速）の処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 重力加速度が設定されている場合は下方向へ加速を加算
        if (pShot->param_d[0] > 0.0) {
            pShot->y += pShot->param_d[0] * (pEnemyShotSet->count % 40);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_BigDipper_Gemini()
{
    static int muki;
    static int shot_count;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 一定周期で北斗七星の弾幕セットを生成
    if (count % 260 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBigDipper;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}