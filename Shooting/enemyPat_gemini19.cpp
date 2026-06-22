// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：双曲線の波面 (Hyperbolic Wave)
static void ShotHyperbolaWave(sEnemyShotSet* pEnemyShotSet)
{
    // 10フレーム間隔で波面を生成（合計5波）
    if (pEnemyShotSet->count % 10 == 0 && pEnemyShotSet->count < 10 * 6) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 双曲線のパラメータ
        double a = 1.0;
        // 波面ごとに漸近線の開き具合を微調整し、立体感を持たせる
        double b = 1.2 + 0.1 * (pEnemyShotSet->count / 10);

        // 生成する角度の範囲（片側）。漸近線の角度を超えないように設定
        double max_phi = 45.0 * DX_PI / 180.0;
        int num_shots = 24; // 1つの波面を構成する弾数（片側）
        double base_speed = 1.5;

        // ウェーブごとに色を変化させる（0〜7の範囲）
        int color = (pEnemyShotSet->count / 10) % 8;

        // プレイヤーの方向を双曲線の中心軸の角度とする
        double base_angle = pEnemyShotSet->muki + pEnemyShotSet->kind * pEnemyShotSet->count * 0.0015;

        // 前方（プレイヤー方向）の波面を生成
        for (int i = 0; i <= num_shots; i++) {
            // 角度 phi を -max_phi から +max_phi まで等間隔に配置
            double phi = -max_phi + (2.0 * max_phi * i) / num_shots;

            // 極方程式に基づく動径 r の計算
            double denom = b * b * cos(phi) * cos(phi) - a * a * sin(phi) * sin(phi);
            if (denom <= 0.001) denom = 0.001; // ゼロ除算・負数ルートの回避
            double r = (a * b) / sqrt(denom);

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_angle + phi;
            // 速度を動径 r に比例させることで、移動後の波面が双曲線になる
            pEnemyShot->speed = base_speed * r;
            // 波面が連なって見えるよう菱形弾を採用
            pEnemyShot->kind = img_enemyShotDiamond[color];

            // 双方向リストの末尾へ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 後方の波面も同時に生成（反対側に開く双曲線）
        for (int i = 0; i <= num_shots; i++) {
            double phi = -max_phi + (2.0 * max_phi * i) / num_shots;
            double denom = b * b * cos(phi) * cos(phi) - a * a * sin(phi) * sin(phi);
            if (denom <= 0.001) denom = 0.001;
            double r = (a * b) / sqrt(denom);

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_angle + phi + DX_PI; // 180度反転
            pEnemyShot->speed = base_speed * r;
            pEnemyShot->kind = img_enemyShotDiamond[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存の弾を移動させる処理
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 等速直線運動を維持することで、波面が双曲線の形状を保ちながら拡大する
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Hyperbola_Gemini()
{
    if (count == 1) {
        // ゲーム画面中央上部に配置
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // なめらかに左右に揺れる移動（サイン波）
        enemy.x = 240.0 + 100.0 * sin(count * 2.0 * DX_PI / 240.0);
    }

    // 120フレームごとに弾幕セットを生成
    if (count % 65 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHyperbolaWave;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // 発射セットが生成された瞬間のプレイヤーの方向を計算
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = (count % 130) / 65 * 2 - 1;

        // 弾リストのダミーヘッダの生成と初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セット本体をマネージャーのリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}