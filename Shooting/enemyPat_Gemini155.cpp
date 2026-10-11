#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <algorithm>
#include <cstdlib>

// ----------------------------------------------------------------
// 補助関数：安全な剰余演算 (負数対応)
// ----------------------------------------------------------------
inline int SafeMod(int a, int b) {
    int res = a % b;
    if (res < 0) res += b;
    return res;
}

// ----------------------------------------------------------------
// パターン1: 基本パターン（下向き赤レーザー ⇔ 中玉移動）
// ----------------------------------------------------------------
static void ShotHeiankyouBase(sEnemyShotSet* pEnemyShotSet)
{
    // param_i[0]: N, param_i[1]: T, param_i[2]: initialized flag
    // param_d[0]: W, param_d[1]: H
    int& count = pEnemyShotSet->count;

    if (count == 0) {
        // 効果音の再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // パラメータのランダム決定
        int N = GetRand(2) + 4; // N = 4, 5, 6 のいずれか
        int T = 30*2;             // 加減速時間 T
        double W = 80.0;        // x方向間隔（固定ベース）
        double H = 16.0;        // y方向間隔（狭め）

        pEnemyShotSet->param_i[0] = N;
        pEnemyShotSet->param_i[1] = T;
        pEnemyShotSet->param_i[2] = 1; // 初期化完了
        pEnemyShotSet->param_d[0] = W;
        pEnemyShotSet->param_d[1] = H;

        // ランダム順列 P の生成
        int P[10];
        for (int k = 0; k < N; k++) P[k] = k;
        for (int k = N - 1; k > 0; k--) {
            int target = GetRand(k);
            int tmp = P[k];
            P[k] = P[target];
            P[target] = tmp;
        }

        // ランダムビット列 R, D の生成
        int R[10], D[10];
        for (int k = 0; k < N; k++) {
            R[k] = GetRand(1);
            D[k] = GetRand(1);
        }

        // グリッド範囲: (-60, -60) から (540, 540) に margin = 60 を考慮
        double startX = -80.0;
        double endX = 560.0;
        double startY = -80.0;
        double endY = 580.0;

        int num_i = (int)((endX - startX) / W) + 1;
        int num_j = (int)((endY - startY) / H) + 1;

        // レーザーの生成
        for (int i = 0; i < num_i; i++) {
            for (int j = 0; j < num_j; j++) {
                int q = j / N;
                int r = j % N;
                int Q_val = SafeMod(P[r] - r, N);

                // 移動先 (i2, j2) の計算
                int i2 = R[r] == 1 ? i + 1 : i - 1;
                int j2 = D[r] == 1 ? j + Q_val : j + Q_val - N;

                double x1 = startX + i * W;
                double y1 = startY + j * H;
                double x2 = startX + i2 * W;
                double y2 = startY + j2 * H;

                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = x1;
                pShot->y = y1;
                pShot->muki = DX_PI / 2; // 下向き
                pShot->speed = 0.0;
                pShot->count = 0;
                // 初期状態：赤の短レーザー (img_enemyShotLaser[0])
                pShot->kind = img_enemyShotLaser[0];
                pShot->margin = 80;

                // パラメータに移動情報を格納
                // param_d[0..3] = x1, y1, x2, y2
                pShot->param_d[0] = x1;
                pShot->param_d[1] = y1;
                pShot->param_d[2] = x2;
                pShot->param_d[3] = y2;

                // リストへ追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    int N = pEnemyShotSet->param_i[0];
    int T = pEnemyShotSet->param_i[1];
    int phase1_end = 60;                   // 1秒待機 (60フレーム)
    int phase2_end = 60 + 2 * T;           // 加減速移動完了
    int phase3_end = 60 + 2 * T + 60;      // 到着後1秒待機
    int phase4_end = 60 + 2 * T + 60 + 60; // 全消去前待機

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double x1 = pShot->param_d[0];
        double y1 = pShot->param_d[1];
        double x2 = pShot->param_d[2];
        double y2 = pShot->param_d[3];

        if (count == phase1_end) {
            // 中玉 (赤: img_enemyShotMediumBall[0]) に変える
            pShot->kind = img_enemyShotMediumBall[0];
        }
        else if (count >= phase1_end && count < phase1_end + 2 * T) {
            // 加減速移動計算 (台形速度プロファイル)
            double t = (double)(count - phase1_end);
            double progress = 0.0;
            if (t <= T) {
                progress = 0.5 * (t / T) * (t / T);
            }
            else {
                double t2 = (2.0 * T - t);
                progress = 1.0 - 0.5 * (t2 / T) * (t2 / T);
            }
            pShot->x = x1 + (x2 - x1) * progress;
            pShot->y = y1 + (y2 - y1) * progress;
        }
        else if (count == phase1_end + 2 * T) {
            // 完全に目的地に配置し、速度0で固定
            pShot->x = x2;
            pShot->y = y2;
        }
        if (count == phase2_end) {
            // 再びレーザーに変える (img_enemyShotLaser[0])
            pShot->kind = img_enemyShotLaser[0];
        }
        pShot = pShot->next;
    }

    // 最後の待機が終わったらこのショットセットを終了（削除はメインまたは自動）
    if (count >= phase3_end) {
        // 次のループや消去のためにリセットまたはフラグ処理

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->margin = -9999;
            pShot = pShot->next;
        }
    }
}

// ----------------------------------------------------------------
// パターン2: 転置パターン（xとyを入れ替えた基本パターン）
// ----------------------------------------------------------------
static void ShotHeiankyouTranspose(sEnemyShotSet* pEnemyShotSet)
{
    int& count = pEnemyShotSet->count;

    if (count == 0) {
        int N = GetRand(2) + 4;
        int T = 30*2;
        double W = 80.0;
        double H = 16.0*2.5;

        pEnemyShotSet->param_i[0] = N;
        pEnemyShotSet->param_i[1] = T;
        pEnemyShotSet->param_i[2] = 1;
        pEnemyShotSet->param_d[0] = W;
        pEnemyShotSet->param_d[1] = H;

        int P[10];
        for (int k = 0; k < N; k++) P[k] = k;
        for (int k = N - 1; k > 0; k--) {
            int target = GetRand(k);
            int tmp = P[k];
            P[k] = P[target];
            P[target] = tmp;
        }

        int R[10], D[10];
        for (int k = 0; k < N; k++) {
            R[k] = GetRand(1);
            D[k] = GetRand(1);
        }

        double startX = -80.0;
        double endX = 560.0;
        double startY = -80.0;
        double endY = 560.0;

        int num_i = (int)((endX - startX) / H) + 1; // 転置によりHとWを入れ替え
        int num_j = (int)((endY - startY) / W) + 1;

        for (int i = 0; i < num_i; i++) {
            for (int j = 0; j < num_j; j++) {
                int q = j / N;
                int r = j % N;
                int Q_val = SafeMod(P[r] - r, N);

                int i2 = R[r] == 1 ? i + 1 : i - 1;
                int j2 = D[r] == 1 ? j + Q_val : j + Q_val - N;

                double x1 = startX + i * H;
                double y1 = startY + j * W;
                double x2 = startX + i2 * H;
                double y2 = startY + j2 * W;

                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = x1;
                pShot->y = y1;
                pShot->muki = 0.0; // 右向き等
                pShot->speed = 0.0;
                pShot->count = 0;
                pShot->kind = img_enemyShotLaser[0];
                pShot->margin = 80;

                pShot->param_d[0] = x1;
                pShot->param_d[1] = y1;
                pShot->param_d[2] = x2;
                pShot->param_d[3] = y2;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    int T = pEnemyShotSet->param_i[1];
    int phase1_end = 60;
    int phase2_end = 60 + 2 * T;
    int phase3_end = 60 + 2 * T + 60;
    int phase4_end = 60 + 2 * T + 60 + 60;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double x1 = pShot->param_d[0];
        double y1 = pShot->param_d[1];
        double x2 = pShot->param_d[2];
        double y2 = pShot->param_d[3];

        if (count == phase1_end) {
            pShot->kind = img_enemyShotMediumBall[0];
        }
        else if (count >= phase1_end && count < phase1_end + 2 * T) {
            double t = (double)(count - phase1_end);
            double progress = 0.0;
            if (t <= T) {
                progress = 0.5 * (t / T) * (t / T);
            }
            else {
                double t2 = (2.0 * T - t);
                progress = 1.0 - 0.5 * (t2 / T) * (t2 / T);
            }
            pShot->x = x1 + (x2 - x1) * progress;
            pShot->y = y1 + (y2 - y1) * progress;
        }
        else if (count == phase1_end + 2 * T) {
            pShot->x = x2;
            pShot->y = y2;
        }
        if (count == phase2_end) {
            pShot->kind = img_enemyShotLaser[0];
        }
        pShot = pShot->next;
    }

    if (count >= phase3_end) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->margin = -9999;
            pShot = pShot->next;
        }
    }
}

// ----------------------------------------------------------------
// 大玉の列を動かすパターン関数
// ----------------------------------------------------------------
static void ShotLargeBallLine(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ----------------------------------------------------------------
// 敵本体のパターン：平安京の悪夢
// ----------------------------------------------------------------
void EnemyPat_Heiankyo_Gemini()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    // ライフフェーズに応じた弾幕の生成・制御
    // 一定周期 (例: 240フレームごと) に各パターンのセットを生成する
    if (count % 240 == 30) {
        if (enemy.hp >= 133) {
            // 【HP 2/3以上】基本パターン ＋ 縦方向大玉の列
            {
                // 基本パターンセット
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotHeiankyouBase;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
            {
                // 縦方向大玉の列（自機と同じ左右位置の画面上下端の一方から）
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotLargeBallLine;
                pSet->x = player.x;
                pSet->y = (GetRand(1) == 0) ? 0.0 : 480.0;

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                double target_y = (pSet->y < 240.0) ? 480.0 : 0.0;
                double angle = atan2(target_y - pSet->y, player.x - pSet->x);

                for (int i = 0; i < 5; i++) {
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = pSet->x;
                    pShot->y = pSet->y - (double)(i * 20) * (pSet->y < 240.0 ? 1.0 : -1.0);
                    pShot->muki = (pSet->y < 240.0) ? DX_PI / 2 : -DX_PI / 2;
                    pShot->speed = 2.5;
                    pShot->kind = img_enemyShotLargeBall[0]; // 赤の大玉

                    pShot->prev = pSet->pEnemyShotHead->prev;
                    pShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pShot;
                    pSet->pEnemyShotHead->prev = pShot;
                }

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        else if (enemy.hp >= 66 && enemy.hp < 133) {
            // 【HP 2/3〜1/3】x/yを転置した基本パターン ＋ 横方向大玉の列
            {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotHeiankyouTranspose;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
            {
                // 横方向大玉の列（自機と同じ上下位置の画面左右端の一方から）
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotLargeBallLine;
                pSet->x = (GetRand(1) == 0) ? 0.0 : 480.0;
                pSet->y = player.y;

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                for (int i = 0; i < 5; i++) {
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = pSet->x - (double)(i * 20) * (pSet->x < 240.0 ? 1.0 : -1.0);
                    pShot->y = pSet->y;
                    pShot->muki = (pSet->x < 240.0) ? 0.0 : DX_PI;
                    pShot->speed = 2.5;
                    pShot->kind = img_enemyShotLargeBall[1]; // 黄の大玉

                    pShot->prev = pSet->pEnemyShotHead->prev;
                    pShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pShot;
                    pSet->pEnemyShotHead->prev = pShot;
                }

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        else {
            // 【HP 1/3以下】基本パターンと転置パターンを同時に発生（大玉なし）
            {
                sEnemyShotSet* pSet1 = new sEnemyShotSet;
                pSet1->count = 0;
                pSet1->patternFunc = ShotHeiankyouBase;
                pSet1->x = enemy.x;
                pSet1->y = enemy.y;
                pSet1->pEnemyShotHead = new sEnemyShot;
                pSet1->pEnemyShotHead->prev = pSet1->pEnemyShotHead;
                pSet1->pEnemyShotHead->next = pSet1->pEnemyShotHead;
                pSet1->prev = enemyShotSetHead.prev;
                pSet1->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet1;
                enemyShotSetHead.prev = pSet1;
            }
            {
                sEnemyShotSet* pSet2 = new sEnemyShotSet;
                pSet2->count = 0;
                pSet2->patternFunc = ShotHeiankyouTranspose;
                pSet2->x = enemy.x;
                pSet2->y = enemy.y;
                pSet2->pEnemyShotHead = new sEnemyShot;
                pSet2->pEnemyShotHead->prev = pSet2->pEnemyShotHead;
                pSet2->pEnemyShotHead->next = pSet2->pEnemyShotHead;
                pSet2->prev = enemyShotSetHead.prev;
                pSet2->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet2;
                enemyShotSetHead.prev = pSet2;
            }
        }
    }
}