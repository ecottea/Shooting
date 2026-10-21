#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：純白の情景「牛乳を注ぐ女」
static void ShotMilkPour(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 効果音：液体を注ぐような軽い音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1. コップ役の弾を生成 (画面左下に固定配置)
        sEnemyShot* pCup = new sEnemyShot;
        pCup->x = 120.0;
        pCup->y = 360.0;
        pCup->muki = 0.0;
        pCup->speed = 0.0;
        pCup->kind = img_enemyShotLargeBall[3]; // シアン大玉 (透明なコップ風)
        pCup->param_d[0] = 1.0; // コップとしてマーク

        pCup->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pCup->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pCup;
        pEnemyShotSet->pEnemyShotHead->prev = pCup;

        // 2. 牛乳を注ぐ弾を生成 (放物線軌道)
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            // コップの位置にランダムなばらつきを持たせて着弾させる
            double target_x = 120.0 + GetRand(40) - 20;
            double target_y = 360.0 + GetRand(20) - 10;
            double T = 60.0;       // 到達までのフレーム数
            double g = 0.15;       // 重力加速度

            // 放物線運動の初速度計算
            pShot->param_d[0] = (target_x - pShot->x) / T; // vx
            pShot->param_d[1] = (target_y - pShot->y - 0.5 * g * T * T) / T; // vy
            pShot->param_d[2] = g; // ay (重力)
            pShot->param_d[3] = 0.0; // 状態 (0:注ぎ中, 1:溢れ)

            pShot->kind = img_enemyShotSmallBall[6]; // 白 小玉 (牛乳)
            pShot->speed = 0.0; // param_dで速度を管理するため0

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next; // 状態変更時の安全なイテレーション用

        if (pShot->param_d[3] == 0.0) {
            // 注ぎ中の牛乳：放物線更新
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            pShot->param_d[1] += pShot->param_d[2]; // 重力加速度を加算

            // コップ付近に到達したら溢れ出す演出へ移行
            double dx = pShot->x - 120.0;
            double dy = pShot->y - 360.0;
            if (dx * dx + dy * dy < 400.0) { // 距離20以内
                pShot->param_d[3] = 1.0; // 状態を「溢れ」に変更
                pShot->kind = img_enemyShotMediumBall[6]; // 白 中玉に変化

                // 溢れ出す方向：自機狙い + ランダムばらつき
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x) + (GetRand(60) - 30) / 180.0 * DX_PI;
                pShot->speed = 1.5 + GetRand(100) / 100.0;
            }
        }
        else {
            // 溢れ出した牛乳：通常の弾として更新
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = nextShot;
    }
}

// 弾幕：テーブルのパン
static void ShotBread(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pShot->y = pEnemyShotSet->y + 10.0;
            // 自機狙い + ばらつき
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x) + (GetRand(40) - 20) / 180.0 * DX_PI;
            pShot->speed = 1.5 + GetRand(100) / 100.0;
            pShot->kind = img_enemyShotMediumOval[8]; // 橙 中楕円弾 (パン風)

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Milkmaid_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0; // やや上めに配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // ボスの左右移動 (画面端から80ドット内側で折り返し)
        enemy.x += 0.8 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    // 牛乳を注ぐ攻撃 (20フレームごと)
    if (count % 20 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMilkPour;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0; // 手元から注ぐ演出
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // パン攻撃 (count 180以降、120フレームごとに発動)
    if (count > 180 && count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBread;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
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