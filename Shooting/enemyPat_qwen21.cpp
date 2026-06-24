// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：火山の噴火
// 火山弾（大玉）を放物線描画で発射し、画面下端に到達すると爆発して火の粉（小玉）を撒き散らす
static void ShotVolcano(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    sEnemyShot* pNext;

    if (pEnemyShotSet->count == 0) {
        // 噴火！効果音
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 火山弾（大玉）を生成
        int num_shots = 3;
        for (int i = 0; i < num_shots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20);
            pEnemyShot->y = pEnemyShotSet->y;

            // 初速度（プレイヤー方向へ、少し山なりに打ち上げる）
            double angle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            double spd = 3.5 + GetRand(10) / 10.0;
            pEnemyShot->param_d[0] = cos(angle) * spd;       // vx
            pEnemyShot->param_d[1] = sin(angle) * spd - 1.5; // vy (上に打ち上げるため減算)
            pEnemyShot->param_d[2] = 0.12;                   // 重力加速度
            pEnemyShot->param_i[0] = 0;                      // 0: 火山弾, 1: 火の粉
            pEnemyShot->param_i[1] = 0;                      // 寿命カウント

            // 色は赤(0) または 橙(8)
            int color = GetRand(1) == 0 ? 0 : 8;
            pEnemyShot->kind = img_enemyShotLargeBall[color];
            pEnemyShot->margin = 400.0; // 画面外判定を少し緩くする

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pNext = pEnemyShot->next;

        if (pEnemyShot->param_i[0] == 0) {
            // 火山弾（大玉）の動き
            pEnemyShot->x += pEnemyShot->param_d[0];
            pEnemyShot->y += pEnemyShot->param_d[1];
            pEnemyShot->param_d[1] += pEnemyShot->param_d[2]; // 重力を加算

            pEnemyShot->param_i[1]++;

            // 画面下端に達する前に爆発 (画面サイズ 480x480)
            // メインルーチンで消去される前に処理するため、画面外手前で判定
            // 上に打ち上がりすぎて画面外に出る場合や、寿命が尽きた場合も爆発
            if (pEnemyShot->y > 450.0 || pEnemyShot->y < -10.0 || pEnemyShot->param_i[1] > 150) {
                // 爆発：火の粉を撒き散らす
                if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                int num_explosion = 16;
                for (int i = 0; i < num_explosion; i++) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    pNewShot->x = pEnemyShot->x;
                    pNewShot->y = pEnemyShot->y;

                    // 円周上に配置。少しランダム性を加える
                    double angle = (double)i / num_explosion * 2.0 * DX_PI + GetRand(10) / 100.0;
                    pNewShot->muki = angle;
                    pNewShot->speed = 1.5 + GetRand(15) / 10.0;
                    pNewShot->param_i[0] = 1; // 火の粉フラグ

                    // 色は赤(0), 黄(1), 橙(8) のどれか
                    int color = GetRand(2);
                    if (color == 0) color = 0;      // 赤
                    else if (color == 1) color = 1; // 黄
                    else color = 8;                // 橙
                    pNewShot->kind = img_enemyShotSmallBall[color];
                    pNewShot->margin = 400;

                    // リストに追加
                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }

                // 火山弾を削除
                pEnemyShot->prev->next = pEnemyShot->next;
                pEnemyShot->next->prev = pEnemyShot->prev;
                delete pEnemyShot;
            }
        }
        else {
            // 火の粉（小玉）の動き
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_Volcano_Qwen()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.2 * (double)muki;
        if (count % 100 == 50) muki *= -1;
    }

    // 一定間隔で火山弾幕を発射
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotVolcano;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}