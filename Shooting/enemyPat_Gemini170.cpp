#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：転世「一条戻り橋」風の収束する弾の壁
static void ShotIchijoModoribashi(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音の再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double centerX = pEnemyShotSet->x;
        double centerY = pEnemyShotSet->y;
        double spawnRadius = 320.0+100; // 画面外の距離からスタート

        int totalWays = 40; // 40方向（9度刻み）
        for (int i = 0; i < totalWays; i++) {
            // 一定間隔で隙間（スリット）を作るために特定の角度をスキップ
            if (i % 8 == 6 || i % 8 == 7) continue;

            pEnemyShot = new sEnemyShot;
            double baseAngle = i * (DX_PI * 2.0 / totalWays);

            // パラメータに初期状態を格納
            pEnemyShot->param_d[0] = spawnRadius; // 初期半径
            pEnemyShot->param_d[1] = baseAngle;   // 初期角度
            pEnemyShot->param_d[2] = 0.012;       // 回転の角速度（スピン量）
            pEnemyShot->speed = 2.2;              // 収束速度

            // 初期位置の計算（極座標から直交座標へ変換）
            pEnemyShot->x = centerX + spawnRadius * cos(baseAngle);
            pEnemyShot->y = centerY + spawnRadius * sin(baseAngle);
            pEnemyShot->muki = baseAngle + DX_PI;

            // 弾の種類（シアンの中玉）
            pEnemyShot->kind = img_enemyShotMediumBall[3];
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // メインルーチンで pShot->count が毎フレーム自動インクリメントされるため、
    // それを利用して時間経過による位置（渦巻き状の収束）を計算する
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double centerX = pEnemyShotSet->x;
        double centerY = pEnemyShotSet->y;

        // 時間経過とともに半径が縮まり、角度が回転する
        double r = pShot->param_d[0] - pShot->speed * pShot->count;
        double angle = pShot->param_d[1] + pShot->param_d[2] * pShot->count;

        pShot->x = centerX + r * cos(angle);
        pShot->y = centerY + r * sin(angle);

        pShot = pShot->next;
    }
}

// 弾幕：プレッシャーを与える自機狙い弾（3WAY）
static void ShotAiming(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // pEnemyShotSet->muki は生成時に自機方向を向いている
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 1) * 0.15;
            pEnemyShot->speed = 3.5;
            // 白の小玉
            pEnemyShot->kind = img_enemyShotSmallBall[6];

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

// 敵本体のパターン
void EnemyPat_Modoribashi_Gemini()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0+100;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // ボスが左右に移動
        enemy.x += 1.0 * (double)muki;
        if (count % 140 == 70) muki *= -1;
    }

    // 一定間隔で収束弾（一条戻り橋風）を発生
    if (count % 100 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotIchijoModoribashi;
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

    // 収束弾の合間に自機狙い弾を放ち、回避のプレッシャーを高める
    if (count % 50 == 25 && count > 100) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAiming;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
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