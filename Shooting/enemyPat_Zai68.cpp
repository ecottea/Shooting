// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：二重咲き・昇華スターマイン
static void ShotFirework(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 打ち上げフェーズ
    if (pEnemyShotSet->count == 0) {
        // 予告音としてチャージ音を使用
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;

        // 発射位置
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        pEnemyShot->speed = 4.5; // 打ち上げ速度

        // 赤色の中玉を花火玉に見立てる
        pEnemyShot->kind = img_enemyShotMediumBall[0];

        // param_i[0] = 0 : 打ち上げ玉
        pEnemyShot->param_i[0] = 0;
        // param_d[1], [2] : 開花目標地点（プレイヤーの現在位置）
        pEnemyShot->param_d[1] = player.x;
        pEnemyShot->param_d[2] = player.y;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 弾の移動と状態遷移
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // === 打ち上げ玉の処理 ===
        if (pShot->param_i[0] == 0) {
            double dx = pShot->param_d[1] - pShot->x;
            double dy = pShot->param_d[2] - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);

            // 目標地点に到達したら開花
            if (dist < 15.0) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK); // 開花音

                int numBurst = 20; // 花弾の数
                for (int i = 0; i < numBurst; i++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pShot->x;
                    pEnemyShot->y = pShot->y;
                    pEnemyShot->muki = (2.0 * DX_PI / numBurst) * i;
                    pEnemyShot->speed = 6.0;

                    // 黄色の菱形弾を星型に見立てる
                    pEnemyShot->kind = img_enemyShotDiamond[1];

                    // param_i[0] = 1 : 開花弾
                    pEnemyShot->param_i[0] = 1;
                    // param_d[0] : 減速率
                    pEnemyShot->param_d[0] = 0.96;

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }

                // 打ち上げ玉を画面外へ飛ばしてメインルーチンの消去判定に任せる
                pShot->x = -100.0;
                pShot->y = -100.0;
                pShot->speed = 0.0;
            }
        }
        // === 開花弾の処理 ===
        else if (pShot->param_i[0] == 1) {
            // 徐々に減速させる
            pShot->speed *= pShot->param_d[0];

            // ある程度减速したら二重咲き（残滓の雨）
            if (pShot->speed < 0.5) {
                int numSpark = 5; // 火花の数
                for (int i = 0; i < numSpark; i++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pShot->x;
                    pEnemyShot->y = pShot->y;

                    // GetRand(3) は 0〜3 の4種類。少し角度を散らす
                    pEnemyShot->muki = (2.0 * DX_PI / numSpark) * i + GetRand(3) / 10.0;
                    // GetRand(10) は 0〜10 の11種類。初速にばらつきを持たせる
                    pEnemyShot->speed = 1.0 + GetRand(10) / 10.0;

                    // 白色の小玉を細かい火花に見立てる
                    pEnemyShot->kind = img_enemyShotSmallBall[6];
                    pEnemyShot->margin = 10;

                    // param_i[0] = 2 : 極小弾(火花)
                    pEnemyShot->param_i[0] = 2;
                    // param_d[0] : 重力（下方向への加速度）
                    pEnemyShot->param_d[0] = 0.01;
                    pEnemyShot->param_d[1] = 0.0;

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }

                // 開花弾を画面外へ飛ばして消去
                pShot->x = -100.0;
                pShot->y = -100.0;
                pShot->speed = 0.0;
            }
        }
        // === 極小弾(火花)の処理 ===
        else if (pShot->param_i[0] == 2) {
            // 重力を加算して下に落ちていく
            pShot->y += pShot->param_d[1];
            pShot->param_d[1] += pShot->param_d[0];
        }

        // 共通の移動処理（消去対象になった弾はspeedが0なので動かない）
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Firework_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 460.0;
        enemy.maxHp = enemy.hp = 20 * 60; // 200で固定
        muki = 1;
        shot_count = 0;

        player.y = 240;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    enemy.hp--;

    // スターマインの連続発射（30フレームごと）
    if (count % 30 == 0 && count >= 90) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFirework;

        // 発射位置を少しランダムにずらして花火らしさを出す
        // GetRand(40) は 0〜40 の41種類
        pEnemyShotSet->x = enemy.x + GetRand(40) - 20;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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