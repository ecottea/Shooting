#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：通常時の自機狙い全方位弾
static void ShotRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int way = 16;
        for (int i = 0; i < way; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 基準となる角度は muki に入っている
            pEnemyShot->muki = pEnemyShotSet->muki + (DX_PI * 2.0 / way) * i;
            pEnemyShot->speed = 2.0;
            pEnemyShot->kind = img_enemyShotMediumOval[4]; // 青い中楕円弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾幕：逃走軌跡の種弾と、その後の炸裂
static void ShotTrailSeed(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 種弾を1つ設置 (動かない)
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤い中玉
        pEnemyShot->param_i[0] = 0; // 0:種弾, 1:炸裂後の弾

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    if (pEnemyShotSet->count == 90) { // 1.5秒(90フレーム)経過で炸裂
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 炸裂時の自機狙い角度
        double base_angle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 十字方向に4発発射
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_angle + (DX_PI / 2.0) * i;
            pEnemyShot->speed = 4.0; // 炸裂後は速い
            pEnemyShot->kind = img_enemyShotLaser[0]; // 赤い短レーザー
            pEnemyShot->param_i[0] = 1; // 炸裂後の弾
            pEnemyShot->margin = 80;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 種弾を画面外へ飛ばして消去（メインルーチンの画面外消去に任せる）
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                pShot->y = -9999.0;
            }
            pShot = pShot->next;
        }
    }

    // 弾の移動と演出
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 種弾の演出：炸裂直前にブルブル震える
            if (pEnemyShotSet->count > 60) {
                // GetRand(2) は 0,1,2 を返すので -1 して -1,0,1 の乱数にする
                pShot->x = pEnemyShotSet->x + (GetRand(2) - 1);
                pShot->y = pEnemyShotSet->y + (GetRand(2) - 1);
            }
        }
        else {
            // 炸裂後の弾は直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン（反動ノックバック・軌道爆散）
void EnemyPat_Escape_Gemini()
{
    static int state;       // 0: 通常状態, 1: ノックバック(逃走)中
    static int prev_hp;     // 被弾判定用のHP保存
    static double vx, vy;   // 逃走時の速度
    static int knockback_timer;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 200.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        state = 0;
        prev_hp = enemy.hp;
        knockback_timer = 0;
    }

    // 状態が通常時かつダメージを受けた場合、ノックバック開始
    if (state == 0 && enemy.hp < prev_hp) {
        state = 1;
        knockback_timer = 20; // 20フレーム間逃走する

        // 自機から遠ざかる方向を計算（自機から敵への角度）
        double angle = atan2(enemy.y - player.y, enemy.x - player.x);
        double speed = 6.5; // 逃走速度
        vx = cos(angle) * speed;
        vy = sin(angle) * speed;

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK); // 発動音として予告音を流用
    }

    // ノックバック中の処理
    if (state == 1) {
        enemy.x += vx;
        enemy.y += vy;

        // 画面端でのバウンド（弾幕の壁を作るための画面分断移動）
        if (enemy.x < 20.0) { enemy.x = 20.0;  vx *= -1.0; }
        if (enemy.x > 460.0) { enemy.x = 460.0; vx *= -1.0; }
        if (enemy.y < 20.0) { enemy.y = 20.0;  vy *= -1.0; }
        if (enemy.y > 460.0) { enemy.y = 460.0; vy *= -1.0; }

        // 逃走中、4フレームおきに軌跡上に種弾を設置
        if (knockback_timer % 4 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotTrailSeed;
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

        knockback_timer--;
        if (knockback_timer <= 0) {
            state = 0; // ノックバック終了、通常状態へ
        }
    }
    // 通常状態の処理
    else {
        // 少しずつ画面中央上部（240, 100）へ戻ろうとする
        enemy.x += (240.0 - enemy.x) * 0.02;
        enemy.y += (200.0 - enemy.y) * 0.02;

        // 一定間隔（90フレーム毎）で自機狙いの全方位弾を発射
        if (count % 90 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotRing;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            // 基準となる自機狙い角度をmukiに渡す
            pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    // 次フレームの判定用にHPを保存
    prev_hp = enemy.hp;
}
