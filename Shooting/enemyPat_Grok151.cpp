#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：サブタレイニアンローズ用リング弾
// param_i[0]: 色 (0=赤, 4=青)
// param_d[0]: 角速度係数 A (rad/frame)
// param_d[1]: 大玉角度幅 B (rad)
static void ShotSubterraneanRose(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int N = 60; // N-way

    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 基準角をランダムに決定 (0 ~ 2π)
        // GetRand(x) は 0以上x以下の整数を返すので、細かくするために大きめの値を使う
        double baseAngle = GetRand(100000) / 100000.0 * 2.0 * DX_PI;

        int color = pEnemyShotSet->param_i[0]; // 0:赤 4:青
        double speed = pEnemyShotSet->param_d[2]; // V

        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + i * (2.0 * DX_PI / N);
            pEnemyShot->speed = speed;
            // 初期は中玉
            pEnemyShot->kind = img_enemyShotMediumBall[color];
            // 後で判定に使うため色とA,Bを弾側にも持たせておく（任意）
            pEnemyShot->param_i[0] = color;
            pEnemyShot->param_d[0] = pEnemyShotSet->param_d[0]; // A
            pEnemyShot->param_d[1] = pEnemyShotSet->param_d[1]; // B

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム、中心からみた偏角に基づいて大玉/中玉を切り替える
    // 弾は放射状に飛ぶので、偏角 ≈ muki（正規化が必要）
    double A = pEnemyShotSet->param_d[0];
    double B = pEnemyShotSet->param_d[1];
    int color = pEnemyShotSet->param_i[0];
    int c = pEnemyShotSet->count; // 発射からの経過フレーム

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 位置更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 偏角を 0～2π に正規化
        double ang = fmod(pShot->muki, 2.0 * DX_PI);
        if (ang < 0.0) ang += 2.0 * DX_PI;

        bool isLarge = false;
        // 共通：回転中心角を計算し、相対角が [0, B] 内なら大玉
        // 青は正回転 (CCW)、赤は負回転 (CW)
        double centerAng;
        if (color == 4) { // 青
            centerAng = c * A;
        }
        else { // 赤
            centerAng = -c * A;
        }
        // 相対角 = ang - centerAng を 0～2π に
        double rel = fmod(ang - centerAng, 2.0 * DX_PI);
        if (rel < 0.0) rel += 2.0 * DX_PI;
        // 青は +方向に B、赤も仕様通り -( ) から -( +B) なので同様に [0,B] でOK
        // （赤の場合 centerAng = -c*A なので、rel が 0～B がちょうどその区間）
        if (rel <= B || B >= 2.0 * DX_PI) {
            isLarge = true;
        }

        if (isLarge) {
            pShot->kind = img_enemyShotLargeBall[color];
        }
        else {
            pShot->kind = img_enemyShotMediumBall[color];
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン：サブタレイニアンローズ
void EnemyPat_SubterraneanRose_Grok()
{
    static int phase;          // 0～3
    static int pauseFrames;    // HP減少時の停止フレーム
    static int shotTimer;      // 次弾発射までのカウンタ
    static int colorToggle;    // 0:青, 1:赤 交互
    static double T;           // 発射間隔
    static double V;           // 弾速
    static double A;           // 回転速度係数
    static double B;           // 大玉の角度幅

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200; // 変更禁止
        phase = 0;
        pauseFrames = 0;
        shotTimer = 0;
        colorToggle = 0; // 最初は青

        // 第1段階パラメータ（適宜調整可能）
        T = 40.0;
        V = 2.2;
        A = 0.015;          // rad/frame
        B = 2.0 * DX_PI / 8.0; // 約45度分（8個程度の大玉イメージ）
    }
    else {
        // ボスはほぼ固定（微動でも可だがシンプルに固定）
        // enemy.x / y はそのまま
    }

    // HPに応じた段階変化（1/4ごとに）
    int targetPhase = 0;
    if (enemy.hp <= 50) targetPhase = 3;
    else if (enemy.hp <= 100) targetPhase = 2;
    else if (enemy.hp <= 150) targetPhase = 1;
    else targetPhase = 0;

    if (targetPhase > phase) {
        phase = targetPhase;
        pauseFrames = 60; // 少し停止
        shotTimer = 0;
        colorToggle = 0;

        // 段階ごとに T, V, A, B を変化
        switch (phase) {
        case 1: // 2段階目：弾速アップ、大玉少し減
            T = 36.0;
            V = 2.8;
            A = 0.018;
            B = 2.0 * DX_PI / 10.0;
            break;
        case 2: // 3段階目：大玉大幅増加、速度やや下げ
            T = 32.0;
            V = 2.0;
            A = 0.012;
            B = 2.0 * DX_PI / 2.0; // 半分近く
            break;
        case 3: // 4段階目：発射頻度アップ、密度高め、大玉少なめ
            T = 24.0;
            V = 2.5;
            A = 0.020;
            B = 2.0 * DX_PI / 15.0;
            break;
        }
    }

    if (pauseFrames > 0) {
        pauseFrames--;
        return; // 弾を撃たない
    }

    // 発射処理
    if (shotTimer <= 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSubterraneanRose;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0; // 未使用
        pEnemyShotSet->kind = 0;

        // パラメータ設定
        if (colorToggle == 0) {
            pEnemyShotSet->param_i[0] = 4; // 青
        }
        else {
            pEnemyShotSet->param_i[0] = 0; // 赤
        }
        pEnemyShotSet->param_d[0] = A;
        pEnemyShotSet->param_d[1] = B;
        pEnemyShotSet->param_d[2] = V;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 交互に色を切り替え
        colorToggle = 1 - colorToggle;
        shotTimer = (int)T;
    }
    else {
        shotTimer--;
    }
}