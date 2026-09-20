#include "DxLib.h"
#include "gv.h"
#include <math.h>

// カルマン渦弾幕
static void ShotKarmanVortex(sEnemyShotSet* pEnemyShotSet)
{
    // param_d 割り当て
    // [0]: 右核 相対X [1]: 右核 相対Y
    // [2]: 左核 相対X [3]: 左核 相対Y
    // [4]: 右核 回転角度(rad) [5]: 左核 回転角度(rad)
    // [6]: 全体のうねり用 sinカウンタの補助
    // param_i 割り当て
    // [0]: 次に弾を出す核 0=右,1=左 (交互制御)
    // [1]: 核識別用フラグ(弾側で使用)

    const double CORE_SPEED_Y = 1.2;
    const double CORE_ROT_SPEED = 6.0 * DX_PI / 180.0;
    const int SPAWN_INTERVAL = 6/2; // 0.1秒毎に交互生成
    const double BULLET_SPEED_BASE = 1.5;

    if (pEnemyShotSet->count == 0) {
        // 初期化
        pEnemyShotSet->param_d[0] = 75.0;
        pEnemyShotSet->param_d[1] = 0.0;
        pEnemyShotSet->param_d[2] = -75.0;
        pEnemyShotSet->param_d[3] = 0.0;
        pEnemyShotSet->param_d[4] = 0.0;
        pEnemyShotSet->param_d[5] = DX_PI; // 180度ずらしてスタート
        pEnemyShotSet->param_i[0] = 0;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 核を可視化するための2発の固定弾を生成 (消えない柱)
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pCore = new sEnemyShot;
            pCore->x = pEnemyShotSet->x + (i == 0 ? pEnemyShotSet->param_d[0] : pEnemyShotSet->param_d[2]);
            pCore->y = pEnemyShotSet->y + (i == 0 ? pEnemyShotSet->param_d[1] : pEnemyShotSet->param_d[3]);
            pCore->muki = 0.0;
            pCore->speed = 0.0;
            pCore->param_i[1] = 1; // 1=核弾フラグ
            pCore->param_i[0] = i; // 0=右核, 1=左核
            // 中玉で核を表現 [3]=シアン, [4]=青
            pCore->kind = (i == 0) ? img_enemyShotMediumBall[3] : img_enemyShotMediumBall[4];
            pCore->margin = 140;

            pCore->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pCore->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pCore;
            pEnemyShotSet->pEnemyShotHead->prev = pCore;
        }
    }

    // --- 1. 核の移動と回転更新 ---
    pEnemyShotSet->param_d[1] += CORE_SPEED_Y;
    pEnemyShotSet->param_d[3] += CORE_SPEED_Y;
    pEnemyShotSet->param_d[4] += CORE_ROT_SPEED; // 右は時計回り
    pEnemyShotSet->param_d[5] -= CORE_ROT_SPEED; // 左は反時計回り

    // 全体をゆらゆら揺らす (カルマン渦列のうねり)
    double sway = sin(pEnemyShotSet->count * 0.03) * 15.0;

    // 画面下まで行ったらリセットして上から再出現 (無限列にする)
    if (pEnemyShotSet->param_d[1] > 500.0) {
        pEnemyShotSet->param_d[1] = 0.0;
        pEnemyShotSet->param_d[3] = 0.0;
    }

    // --- 2. 交互に小弾を放出 ---
    if (pEnemyShotSet->count % SPAWN_INTERVAL == 0) {
        int side = pEnemyShotSet->param_i[0]; // 0=右, 1=左

        double coreX = pEnemyShotSet->x + (side == 0 ? pEnemyShotSet->param_d[0] : pEnemyShotSet->param_d[2]) + sway;
        double coreY = pEnemyShotSet->y + (side == 0 ? pEnemyShotSet->param_d[1] : pEnemyShotSet->param_d[3]);
        double coreAngle = (side == 0 ? pEnemyShotSet->param_d[4] : pEnemyShotSet->param_d[5]);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = coreX;
        pEnemyShot->y = coreY;
        // 接線方向に射出
        pEnemyShot->muki = coreAngle + DX_PI / 2.0;
        pEnemyShot->speed = BULLET_SPEED_BASE + GetRand(30) / 100.0; // 1.5 - 1.8

        // 小玉で流線を描く
        pEnemyShot->kind = (side == 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[4];

        // カーブ用のパラメータ
        pEnemyShot->param_d[0] = 60.0; // カーブ残りフレーム
        pEnemyShot->param_d[1] = (side == 0 ? 1.0 : -1.0); // 回転方向
        pEnemyShot->param_i[1] = 0; // 通常弾フラグ

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 次は逆側
        pEnemyShotSet->param_i[0] ^= 1;

        if (pEnemyShotSet->count % 9 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // --- 3. 全弾の移動 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 1) {
            // 核弾は核の座標に追従
            int side = pShot->param_i[0];
            pShot->x = pEnemyShotSet->x + (side == 0 ? pEnemyShotSet->param_d[0] : pEnemyShotSet->param_d[2]) + sway;
            pShot->y = pEnemyShotSet->y + (side == 0 ? pEnemyShotSet->param_d[1] : pEnemyShotSet->param_d[3]);
        }
        else {
            // 通常弾: 直進 + 渦によるカーブ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            if (pShot->param_d[0] > 0) {
                // 回転方向にゆっくり曲げる
                pShot->muki += pShot->param_d[1] * (2.5 * DX_PI / 180.0);
                pShot->param_d[0] -= 1.0;
            }
        }
        pShot = pShot->next;
    }
}

// 敵本体
void EnemyPat_KarmanVortex_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 柱自体はほぼ固定、わずかに揺れるだけにしてカルマン渦の柱感を出す
        enemy.x = 240.0 + sin(count * 0.02) * 10.0;
    }

    // 1つの渦列セットだけを生成し、ずっと維持する
    if (count == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKarmanVortex;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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

    // ShotSetの座標は常に敵に追従
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet = pSet->next;
    }
}