// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：へにょりスラローム（毎回ランダム・メタモルフォーゼ：修正版）
static void ShotHenyoriLabyrinth(sEnemyShotSet* pEnemyShotSet)
{
    // セットの開始（0フレーム目）の瞬間だけ、今回のへにょり方をランダム決定し、
    // そのセット自身の変数 (pEnemyShotSet->param_d) に記憶させる
    if (pEnemyShotSet->count == 0) {
        double sign1 = (GetRand(1) == 0) ? 1.0 : -1.0;
        double sign2 = (GetRand(1) == 0) ? 1.0 : -1.0;

        pEnemyShotSet->param_d[0] = (40.0 + (double)GetRand(300) / 10.0) * sign1; // A1
        pEnemyShotSet->param_d[1] = 0.025 + (double)GetRand(200) / 10000.0;       // omega1
        pEnemyShotSet->param_d[2] = (15.0 + (double)GetRand(200) / 10.0) * sign2; // A2
        pEnemyShotSet->param_d[3] = 0.070 + (double)GetRand(400) / 10000.0;       // omega2
    }

    // 記憶したセット固有のパラメータを読み出す
    double set_A1 = pEnemyShotSet->param_d[0];
    double set_omega1 = pEnemyShotSet->param_d[1];
    double set_A2 = pEnemyShotSet->param_d[2];
    double set_omega2 = pEnemyShotSet->param_d[3];

    // 【弾の生成フェーズ】
    if (pEnemyShotSet->count >= 0 && pEnemyShotSet->count < 120) {

        if (pEnemyShotSet->count % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->param_d[0] = enemy.x + (i - 2.0) * 85.0; // 基準X座標
            pEnemyShot->param_d[1] = enemy.y + 10.0;             // 基準Y座標
            pEnemyShot->param_d[2] = pEnemyShotSet->count;       // 発射タイミング

            // 弾自身にもパラメータを渡す（セットが消滅しても弾単体で独立して動けるようにするため）
            pEnemyShot->param_d[3] = set_A1;
            pEnemyShot->param_d[4] = set_omega1;
            pEnemyShot->param_d[5] = set_A2;
            pEnemyShot->param_d[6] = set_omega2;

            pEnemyShot->speed = 2.5;
            pEnemyShot->count = 0;

            pEnemyShot->kind = img_enemyShotScale[(i % 2 == 0) ? 4 : 0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 【軌道更新フェーズ】
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double startX = pShot->param_d[0];
        double startY = pShot->param_d[1];
        double birth = pShot->param_d[2];

        // 画面内の各弾が、自分が生まれた時に割り振られた固有パラメータを読み出す
        double A1 = pShot->param_d[3];
        double omega1 = pShot->param_d[4];
        double A2 = pShot->param_d[5];
        double omega2 = pShot->param_d[6];

        double t = pShot->count;
        double v = pShot->speed;

        double angle1 = omega1 * t + 0.006 * birth;
        double angle2 = omega2 * t + 0.012 * birth;

        // この弾固有の波形から、X方向のうねりオフセットを計算
        double offsetX = A1 * sin(angle1) + A2 * sin(angle2);

        pShot->x = startX + offsetX;
        pShot->y = startY + v * t;

        // 弾の向き（接線ベクトル）の計算
        double dx_dt = A1 * omega1 * cos(angle1) + A2 * omega2 * cos(angle2);
        double dy_dt = v;

        pShot->muki = atan2(dy_dt, dx_dt);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_HenyoriLaser_Gemini()
{
    // 登場時の初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // ワインダーになってしまうのを防ぐため、ボスのX座標は完全に固定する
        enemy.x = 240.0;
    }

    // 300フレーム（約5秒）周期で、へにょり迷路を展開
    if (count % 80 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHenyoriLabyrinth;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = DX_PI / 2.0; // 真下
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