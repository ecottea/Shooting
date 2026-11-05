#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ペンデュラムウェーブ弾幕
//  長さ（周期）の異なる複数の振り子が同時に振れ、
//  位相が揃ったり崩れたりする様子を弾幕で表現する。
//  おもり：中玉（左右に振れる）
//  発射弾：小玉（おもりから定期的に放出）
// ============================================================
static void ShotPendulumWave(sEnemyShotSet* pEnemyShotSet)
{
    const int NUM_PENDULUM = 7+2;          // 振り子の本数
    const double PIVOT_Y = 55.0;         // 支点のY座標
    const double BASE_LENGTH = 75.0;     // 基準の長さ
    const double LENGTH_STEP = 14.0;     // 長さの差
    const double BASE_OMEGA = 0.038;     // 基準角速度（rad/frame）
    const double OMEGA_STEP = 0.0028;    // 角速度の差（周期を少しずつずらす）
    const double AMPLITUDE = 0.85;       // 振れ幅（ラジアン）

    sEnemyShot* pEnemyShot;

    // ----- 初期化：振り子のおもり（中玉）を生成 -----
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < NUM_PENDULUM; i++) {
            pEnemyShot = new sEnemyShot;

            // 支点Xを画面横に均等配置（左右に余白を残す）
            double pivotX = 50.0 + i * (380.0 / (NUM_PENDULUM - 1));

            // パラメータ格納
            // param_i[0] : 1=おもり弾（特殊移動対象）
            // param_i[1] : 色番号（0～8）
            // param_d[0] : 支点X
            // param_d[1] : 支点Y
            // param_d[2] : 長さ L
            // param_d[3] : 振幅 A
            // param_d[4] : 角速度 ω
            // param_d[5] : 初期位相（全員0で同期スタート）
            int color = i % 9;
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_i[1] = color;
            pEnemyShot->param_d[0] = pivotX;
            pEnemyShot->param_d[1] = PIVOT_Y;
            pEnemyShot->param_d[2] = BASE_LENGTH + i * LENGTH_STEP;
            pEnemyShot->param_d[3] = AMPLITUDE;
            pEnemyShot->param_d[4] = BASE_OMEGA + i * OMEGA_STEP;
            pEnemyShot->param_d[5] = 0.0;

            // 初期位置（位相0の最下点付近）
            double theta = pEnemyShot->param_d[3] * sin(pEnemyShot->param_d[5]);
            pEnemyShot->x = pEnemyShot->param_d[0] + pEnemyShot->param_d[2] * sin(theta);
            pEnemyShot->y = pEnemyShot->param_d[1] + pEnemyShot->param_d[2] * cos(theta);

            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;     // 速度は使わず毎フレーム位置を再計算

            // 色を少しずつ変えて見分けやすくする（0:赤 ～ 8:橙）
            pEnemyShot->kind = img_enemyShotMediumBall[color];
            pEnemyShot->margin = 120;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----- 毎フレーム更新 -----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // おもり弾（param_i[0]==1）は振り子運動で位置を更新
        if (pShot->param_i[0] == 1) {
            double t = (double)pEnemyShotSet->count;
            double A = pShot->param_d[3];
            double omega = pShot->param_d[4];
            double phase = pShot->param_d[5];
            double L = pShot->param_d[2];
            double px = pShot->param_d[0];
            double py = pShot->param_d[1];

            double theta = A * sin(omega * t + phase);
            pShot->x = px + L * sin(theta);
            pShot->y = py + L * cos(theta);

            // おもりから小玉を定期的に放出
            // 周期をずらしているため、位相が揃った瞬間に横一列の波が生まれる
            if (pEnemyShotSet->count % 7 == 0) {
                // 効果音（軽め）
                if (pEnemyShotSet->count % 21 == 0) {  // 音の重複を少し抑える
                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }

                // 1発の小玉を下方向へ（現在の振れ角度を少し反映）
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = pShot->x;
                pNew->y = pShot->y + 4.0;

                // 基本は真下、振れに応じて少し左右に傾ける
                double baseMuki = DX_PI * 0.5;          // 下方向
                double swingBias = theta * 0.45;        // 振れ角を反映
                pNew->muki = baseMuki + swingBias;

                // 速度にわずかなばらつきを持たせる
                pNew->speed = 1.8 + GetRand(40) / 100.0;

                // 小玉・色はおもりに合わせる（param_i[1]に保存してある色番号を使用）
                int color = pShot->param_i[1];
                if (color < 0 || color > 8) color = 0;
                pNew->kind = img_enemyShotSmallBall[color];

                pNew->param_i[0] = 0;   // 通常弾フラグ
                pNew->margin = 120;

                // リストに追加
                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }
        // 通常弾（小玉）は等速直線運動
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_PendulumWave_Grok()
{
    // 最初のフレームで敵の初期化と弾幕セットの生成を行う
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 36.0;
        enemy.maxHp = enemy.hp = 200;   // 200から変更しないこと

        // ペンデュラムウェーブ用の弾幕セットを1つ生成（以降ずっと動き続ける）
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPendulumWave;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵本体はゆるやかに左右に往復（目立たせすぎない程度）
        // 120フレームごとに方向転換
        static int dir = 1;
        enemy.x += 0.55 * (double)dir;
        if (count % 150 == 75) {
            dir *= -1;
        }
        // 画面端で跳ね返らせる安全策
        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            dir = 1;
        }
        if (enemy.x > 400.0) {
            enemy.x = 400.0;
            dir = -1;
        }
    }
}