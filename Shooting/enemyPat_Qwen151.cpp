#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：サブタレイニアンローズ
static void ShotSubterraneanRose(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 発射予告音・発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 基準角をランダムに決定 (0～359度)
        double base_angle = GetRand(359) * DX_PI / 180.0;
        double V = pEnemyShotSet->param_d[0];
        int shot_type = pEnemyShotSet->kind; // 0: 青, 1: 赤

        // N=60 の全方位弾を生成
        for (int i = 0; i < 60; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_angle + 2.0 * DX_PI * i / 60.0;
            pEnemyShot->speed = V;
            pEnemyShot->count = 0; // メインルーチンでインクリメントされるため初期化

            // 初期状態は中玉として生成（毎フレームの判定で大玉に変化させる）
            if (shot_type == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青中玉
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤中玉
            }

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動と、毎フレームの形状変化処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 移動処理
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // ボス中心から見た弾の偏角を計算 (-PI ～ PI)
        double theta = atan2(pShot->y - enemy.y, pShot->x - enemy.x);

        // 角度を 0 ～ 2*PI の範囲に正規化
        while (theta < 0.0) theta += 2.0 * DX_PI;
        while (theta >= 2.0 * DX_PI) theta -= 2.0 * DX_PI;

        double A = pEnemyShotSet->param_d[1];
        double B = pEnemyShotSet->param_d[2];
        int shot_type = pEnemyShotSet->kind;

        // 目標となる基準角度の計算
        double target = (shot_type == 0) ? (count * A) : -(count * A);

        // 目標角度も 0 ～ 2*PI の範囲に正規化
        while (target < 0.0) target += 2.0 * DX_PI;
        while (target >= 2.0 * DX_PI) target -= 2.0 * DX_PI;

        // 偏角の差分を計算し、0 ～ 2*PI に正規化
        double diff = theta - target;
        if (diff < 0.0) diff += 2.0 * DX_PI;

        // 範囲 B 以内であれば大玉、それ以外は中玉に変化させる
        if (shot_type == 0) {
            if (diff <= B) {
                pShot->kind = img_enemyShotLargeBall[4]; // 青大玉
            }
            else {
                pShot->kind = img_enemyShotMediumBall[4]; // 青中玉
            }
        }
        else {
            if (diff <= B) {
                pShot->kind = img_enemyShotLargeBall[0]; // 赤大玉
            }
            else {
                pShot->kind = img_enemyShotMediumBall[0]; // 赤中玉
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SubterraneanRose_Qwen()
{
    static int phase = 0;
    static bool is_paused = false;
    static int pause_timer = 0;
    static int shot_timer = 0;
    static int shot_type = 0; // 0: 青, 1: 赤 (交互に撃つため)

    // パラメータ初期値
    static double T = 30.0; // 発射間隔(フレーム)
    static double V = 2.0;  // 弾速
    static double A = 0.03; // 角度変化率(ラジアン/フレーム)
    static double B = 0.8;  // 大玉となる角度範囲(ラジアン)

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = 200;
        enemy.hp = 200;

        phase = 0;
        is_paused = false;
        pause_timer = 0;
        shot_timer = 0;
        shot_type = 0;
        T = 30.0;
        V = 2.0;
        A = 0.03;
        B = 0.8;
    }

    // HPが1/4削れる度(300, 200, 100を切る度)にフェーズ遷移
    int current_phase = (200 - enemy.hp) / 50;
    if (current_phase > phase && current_phase <= 3) {
        phase = current_phase;
        is_paused = true;
        pause_timer = 90; // 約1.5秒発射を停止

        // パラメータを変化させて難易度を上昇させる
        switch (phase) {
        case 1:
            T = 25.0; V = 2.5; A = 0.04; B = 0.7;
            break;
        case 2:
            T = 20.0; V = 3.0; A = 0.05; B = 0.6;
            break;
        case 3:
            T = 15.0; V = 3.5; A = 0.06; B = 0.5;
            break;
        }
    }

    // 停止中の処理
    if (is_paused) {
        pause_timer--;
        if (pause_timer <= 0) {
            is_paused = false;
            shot_timer = 0; // 停止明けは即座に発射サイクルを開始
        }
    }
    // 発射処理
    else {
        shot_timer++;
        if (shot_timer >= T) {
            shot_timer = 0;

            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSubterraneanRose;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->kind = shot_type;

            // パラメータを弾幕関数に渡す
            pEnemyShotSet->param_d[0] = V;
            pEnemyShotSet->param_d[1] = A;
            pEnemyShotSet->param_d[2] = B;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            // 次は別の色の弾を撃つように切り替え
            shot_type = 1 - shot_type;
        }
    }
}