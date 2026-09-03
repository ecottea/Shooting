// enemyPat_Tmp.cpp
// 紫奥義「弾幕結界」の実装（周期ごとに新規セット作成、回転継続、再移動修正版）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  パラメータ定義 (周期ごとの変化)
// ============================================================
const int MAX_CYCLE = 3;

// k: 大玉の個数(各色), a: 角速度, v: 速さ, t1: 連射フレーム, t2: 休止フレーム
// th0: 最大ズレ量(ラジアン), l0: 初期停止距離, l: 停止距離増加量, T: 射出総フレーム数
const int   PARAM_K[MAX_CYCLE] = { 3, 4, 5 };
const double PARAM_A[MAX_CYCLE] = { 0.02, 0.03, 0.04 };
const double PARAM_V[MAX_CYCLE] = { 3.0, 3.5, 4.0 };
const int   PARAM_T1[MAX_CYCLE] = { 10, 15, 20 };
const int   PARAM_T2[MAX_CYCLE] = { 20, 15, 10 };
const double PARAM_TH0[MAX_CYCLE] = { 0.5, 0.8, 1.0 };
const double PARAM_L0[MAX_CYCLE] = { 100.0, 80.0, 60.0 };
const double PARAM_L[MAX_CYCLE] = { 2.0, 3.0, 4.0 };
const int   PARAM_T[MAX_CYCLE] = { 300, 360, 420 };

// ============================================================
//  新規弾幕セット作成関数
// ============================================================
static sEnemyShotSet* CreateNewShotSet(int cycle)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = nullptr; // 後で設定または直接使用
    pSet->x = 240.0;
    pSet->y = 240.0;
    pSet->muki = 0.0;
    pSet->kind = 0;

    // パラメータ初期化
    pSet->param_i[0] = 0;  // phase (0:拡大, 1:射出, 2:再移動・待機)
    pSet->param_i[1] = cycle; // 現在の周期
    pSet->param_i[2] = 0;  // shoot_timer
    pSet->param_i[7] = 0;  // phase2_timer
    pSet->param_i[8] = 0;  // 初期化フラグ
    pSet->param_i[15] = 0; // 削除フラグ (0:有効, 1:削除対象)

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    // リストに追加
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// ============================================================
//  弾幕パターン関数
// ============================================================
static void ShotDanmakuKekkai(sEnemyShotSet* pSet)
{
    int phase = pSet->param_i[0];
    int cycle = pSet->param_i[1];

    // 現在の周期のパラメータを取得
    int k = PARAM_K[cycle];
    double a = PARAM_A[cycle];
    double v = PARAM_V[cycle];
    int t1 = PARAM_T1[cycle];
    int t2 = PARAM_T2[cycle];
    double th0 = PARAM_TH0[cycle];
    double l0 = PARAM_L0[cycle];
    double l = PARAM_L[cycle];
    int T = PARAM_T[cycle];

    // --------------------------------------------------------
    // Phase 0: 半径拡大
    // --------------------------------------------------------
    if (phase == 0) {
        if (pSet->param_i[8] == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            for (int i = 0; i < k; i++) {
                // 青の大玉
                sEnemyShot* pBlue = new sEnemyShot;
                pBlue->kind = img_enemyShotLargeBall[4];
                pBlue->x = 240.0;
                pBlue->y = 240.0;
                pBlue->param_i[0] = 0; // 0:青
                pBlue->param_i[1] = i;
                pBlue->param_d[0] = (2.0 * DX_PI / k) * i;
                pBlue->prev = pSet->pEnemyShotHead->prev;
                pBlue->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pBlue;
                pSet->pEnemyShotHead->prev = pBlue;

                // 紫の大玉
                sEnemyShot* pPurple = new sEnemyShot;
                pPurple->kind = img_enemyShotLargeBall[5];
                pPurple->x = 240.0;
                pPurple->y = 240.0;
                pPurple->param_i[0] = 1; // 1:紫
                pPurple->param_i[1] = i;
                pPurple->param_d[0] = (2.0 * DX_PI / k) * i + (DX_PI / k);
                pPurple->prev = pSet->pEnemyShotHead->prev;
                pPurple->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pPurple;
                pSet->pEnemyShotHead->prev = pPurple;
            }

            pSet->param_d[5] = 0.0; // current_r
            pSet->param_d[6] = 0.0; // current_theta
            pSet->param_i[8] = 1;   // 初期化済み
        }

        double r = pSet->param_d[5] + 1.5;
        if (r > 240.0) r = 240.0;
        pSet->param_d[5] = r;

        double theta = pSet->param_d[6] + a;
        pSet->param_d[6] = theta;

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->kind == img_enemyShotLargeBall[4] || pShot->kind == img_enemyShotLargeBall[5]) {
                double angle = theta + pShot->param_d[0];
                pShot->x = 240.0 + r * cos(angle);
                pShot->y = 240.0 + r * sin(angle);
            }
            pShot = pShot->next;
        }

        if (r >= 240.0 && pSet->count > 60) {
            pSet->param_i[0] = 1; // phase 1
            pSet->param_i[2] = 0; // shoot_timer
        }
    }
    // --------------------------------------------------------
    // Phase 1: 鱗弾の射出と停止制御
    // --------------------------------------------------------
    else if (phase == 1) {
        // 大玉の回転継続
        double theta = pSet->param_d[6] + a;
        pSet->param_d[6] = theta;
        double r = pSet->param_d[5]; // 240.0 で固定

        sEnemyShot* pShotRotate = pSet->pEnemyShotHead->next;
        while (pShotRotate != pSet->pEnemyShotHead) {
            if (pShotRotate->kind == img_enemyShotLargeBall[4] || pShotRotate->kind == img_enemyShotLargeBall[5]) {
                double angle = theta + pShotRotate->param_d[0];
                pShotRotate->x = 240.0 + r * cos(angle);
                pShotRotate->y = 240.0 + r * sin(angle);
            }
            pShotRotate = pShotRotate->next;
        }

        int shoot_timer = pSet->param_i[2];
        if (shoot_timer < T) {
            int rhythm = shoot_timer % (t1 + t2);

            if (rhythm < t1) {
                double offset = -th0 + (2.0 * th0 / T) * shoot_timer;

                sEnemyShot* pShot = pSet->pEnemyShotHead->next;
                while (pShot != pSet->pEnemyShotHead) {
                    if ((pShot->kind == img_enemyShotLargeBall[4] || pShot->kind == img_enemyShotLargeBall[5]) && pSet->count % 2 == 0) {
                        int color = (pShot->kind == img_enemyShotLargeBall[4]) ? 4 : 5;
                        int imgScale = (color == 4) ? img_enemyShotScale[4] : img_enemyShotScale[5];

                        double dx = 240.0 - pShot->x;
                        double dy = 240.0 - pShot->y;
                        double base_muki = atan2(dy, dx);

                        // 1. 内側に向かって速さvで1個
                        sEnemyShot* pIn = new sEnemyShot;
                        pIn->kind = imgScale;
                        pIn->x = pShot->x;
                        pIn->y = pShot->y;
                        pIn->muki = base_muki + offset;
                        pIn->speed = v;
                        pIn->param_i[0] = 0;       // 0:内側移動中
                        pIn->param_d[0] = 0.0;     // 移動距離
                        pIn->param_d[1] = l0;      // 停止閾値
                        pIn->param_d[2] = l;       // 停止距離増加量
                        pIn->param_d[3] = v;       // 再移動用速さ

                        pIn->prev = pSet->pEnemyShotHead->prev;
                        pIn->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = pIn;
                        pSet->pEnemyShotHead->prev = pIn;

                        // 2. 外側に向かって低速で3個
                        double base_muki_out = base_muki + DX_PI;
                        double v_out = v * 0.4;
                        for (int j = -1; j <= 1; j++) {
                            sEnemyShot* pOut = new sEnemyShot;
                            pOut->kind = imgScale;
                            pOut->x = pShot->x;
                            pOut->y = pShot->y;
                            pOut->muki = base_muki_out + offset + j * 0.3;
                            pOut->speed = v_out;
                            pOut->param_i[0] = 3; // 3:外側移動中

                            pOut->prev = pSet->pEnemyShotHead->prev;
                            pOut->next = pSet->pEnemyShotHead;
                            pSet->pEnemyShotHead->prev->next = pOut;
                            pSet->pEnemyShotHead->prev = pOut;
                        }
                    }
                    pShot = pShot->next;
                }
            }
            pSet->param_i[2]++;
        }

        // 鱗弾の移動・状態更新処理
        sEnemyShot* pShotMove = pSet->pEnemyShotHead->next;
        while (pShotMove != pSet->pEnemyShotHead) {
            int state = pShotMove->param_i[0];

            if (state == 0) { // 内側移動中
                pShotMove->x += pShotMove->speed * cos(pShotMove->muki);
                pShotMove->y += pShotMove->speed * sin(pShotMove->muki);
                pShotMove->param_d[0] += pShotMove->speed;

                if (pShotMove->param_d[0] >= pShotMove->param_d[1]) {
                    pShotMove->param_i[0] = 1; // 1:停止中へ
                    pShotMove->speed = 0.0;
                }
            }
            else if (state == 1) { // 停止中
                pShotMove->param_d[1] += pShotMove->param_d[2]; // 停止距離を増加
            }
            else if (state == 2) { // 再移動中
                pShotMove->x += pShotMove->speed * cos(pShotMove->muki);
                pShotMove->y += pShotMove->speed * sin(pShotMove->muki);
            }
            else if (state == 3) { // 外側移動中
                pShotMove->x += pShotMove->speed * cos(pShotMove->muki);
                pShotMove->y += pShotMove->speed * sin(pShotMove->muki);
            }
            pShotMove = pShotMove->next;
        }

        if (shoot_timer >= T) {
            pSet->param_i[0] = 2; // phase 2
            pSet->param_i[7] = 0; // phase2_timer

            // 停止させていた鱗弾を再度速さvで動かす
            sEnemyShot* pShotRestart = pSet->pEnemyShotHead->next;
            while (pShotRestart != pSet->pEnemyShotHead) {
                if (pShotRestart->param_i[0] == 1) {
                    pShotRestart->param_i[0] = 2; // 2:再移動中へ
                    pShotRestart->speed = pShotRestart->param_d[3]; // 速さを明示的に再設定
                }
                pShotRestart = pShotRestart->next;
            }
        }
    }
    // --------------------------------------------------------
    // Phase 2: 再移動・待機 (次の周期へ)
    // --------------------------------------------------------
    else if (phase == 2) {
        // 大玉の回転継続
        double theta = pSet->param_d[6] + a;
        pSet->param_d[6] = theta;
        double r = pSet->param_d[5];

        sEnemyShot* pShotRotate = pSet->pEnemyShotHead->next;
        while (pShotRotate != pSet->pEnemyShotHead) {
            if (pShotRotate->kind == img_enemyShotLargeBall[4] || pShotRotate->kind == img_enemyShotLargeBall[5]) {
                double angle = theta + pShotRotate->param_d[0];
                pShotRotate->x = 240.0 + r * cos(angle);
                pShotRotate->y = 240.0 + r * sin(angle);
            }
            pShotRotate = pShotRotate->next;
        }

        // 再移動中の弾の処理 (state == 2 は上記の移動ブロックで処理される)
        sEnemyShot* pShotMove = pSet->pEnemyShotHead->next;
        while (pShotMove != pSet->pEnemyShotHead) {
            if (pShotMove->param_i[0] == 2) {
                pShotMove->x += pShotMove->speed * cos(pShotMove->muki);
                pShotMove->y += pShotMove->speed * sin(pShotMove->muki);
            }
            pShotMove = pShotMove->next;
        }

        pSet->param_i[7]++;
        if (pSet->param_i[7] > 120) {
            // 削除フラグを立てる (EnemyPat_DanmakuKekkai_Qwen で検知して新規作成・削除を行う)
            pSet->param_i[15] = 1;
        }
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_DanmakuKekkai_Qwen()
{
    static int current_cycle = 0;
    static sEnemyShotSet* pCurrentSet = nullptr;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
        current_cycle = 0;

        pCurrentSet = CreateNewShotSet(current_cycle);
        pCurrentSet->patternFunc = ShotDanmakuKekkai;
    }
    else {
        enemy.x = 240.0 + 80.0 * sin(count * 0.015);
        enemy.y = 240.0 + 40.0 * cos(count * 0.02);

        // 周期終了の検知と新規セット作成
        if (pCurrentSet != nullptr && pCurrentSet->param_i[15] == 1) {
            // 古いセットの弾をすべて削除
            sEnemyShot* pShot = pCurrentSet->pEnemyShotHead->next;
            while (pShot != pCurrentSet->pEnemyShotHead) {
                sEnemyShot* next = pShot->next;
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = next;
            }
            delete pCurrentSet->pEnemyShotHead;

            // リストから外して削除
            pCurrentSet->prev->next = pCurrentSet->next;
            pCurrentSet->next->prev = pCurrentSet->prev;
            delete pCurrentSet;

            // 新しい周期へ
            current_cycle = (current_cycle + 1) % MAX_CYCLE;
            pCurrentSet = CreateNewShotSet(current_cycle);
            pCurrentSet->patternFunc = ShotDanmakuKekkai;
        }

        if (pCurrentSet != nullptr) {
            pCurrentSet->x = enemy.x;
            pCurrentSet->y = enemy.y;
        }
    }
}