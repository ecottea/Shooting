// enemyPat_danmakuBarrier.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 紫奥義「弾幕結界」
static void ShotDanmakuBarrier(sEnemyShotSet* pSet)
{
    // 周期ごとのパラメータテーブル
    static const int phase_k[] = { 3, 4, 5, 4, 3 };
    static const double phase_a[] = { 0.015, 0.020, 0.025, 0.020, 0.015 };
    static const double phase_v[] = { 2.5, 3.0, 2.0, 2.5, 3.5 };
    static const int phase_t1[] = { 4, 3, 5, 4, 3 };
    static const int phase_t2[] = { 3, 4, 2, 3, 4 };
    static const double phase_th0[] = { 0.4, 0.6, 0.5, 0.7, 0.4 };
    static const double phase_l0[] = { 40.0, 50.0, 35.0, 45.0, 40.0 };
    static const double phase_l[] = { 0.5, 0.8, 0.6, 0.7, 0.5 };
    static const int phase_T[] = { 150, 180, 200, 160, 150 };
    static const int NUM_PHASES = sizeof(phase_k) / sizeof(phase_k[0]);

    // === sEnemyShotSet param 使用一覧 ===
    // param_i[0] : 現在の周期番号
    // param_i[1] : 周期内フレームカウント (0〜)
    // param_i[2] : 大玉生成済みフラグ (0:未生成, 1:生成済み)
    // param_i[3] : 停止弾再開処理済みフラグ (0:未処理, 1:処理済み)

    int phase = pSet->param_i[0] % NUM_PHASES;
    int localCount = pSet->param_i[1];
    int k = phase_k[phase];
    double a = phase_a[phase];
    double v = phase_v[phase];
    int t1 = phase_t1[phase];
    int t2 = phase_t2[phase];
    double th0 = phase_th0[phase];
    double l0_base = phase_l0[phase];
    double l_inc = phase_l[phase];
    int T = phase_T[phase];

    // --------------------------------------------------
    // 1. 周期開始時：大玉生成
    // --------------------------------------------------
    if (pSet->param_i[2] == 0) {
        pSet->param_i[2] = 1;

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < k; i++) {
            double baseAngle = (2.0 * DX_PI / k) * i;

            // 青大玉
            sEnemyShot* pBlue = new sEnemyShot;
            pBlue->x = 240.0;
            pBlue->y = 240.0;
            pBlue->muki = 0.0;
            pBlue->speed = 0.0;
            pBlue->kind = img_enemyShotLargeBall[4]; // 青
            pBlue->param_i[0] = 1;   // 大玉フラグ
            pBlue->param_i[1] = 4;   // 色：青
            pBlue->param_i[2] = i;   // インデックス
            pBlue->param_d[0] = 0.0; // 現在半径
            pBlue->param_d[1] = baseAngle; // 基準角度
            pBlue->param_d[2] = a;   // 角速度
            pBlue->prev = pSet->pEnemyShotHead->prev;
            pBlue->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pBlue;
            pSet->pEnemyShotHead->prev = pBlue;

            // 紫大玉（角度を半分ずらして交互に配置）
            sEnemyShot* pPurple = new sEnemyShot;
            double baseAngleP = baseAngle + DX_PI / k;
            pPurple->x = 240.0;
            pPurple->y = 240.0;
            pPurple->muki = 0.0;
            pPurple->speed = 0.0;
            pPurple->kind = img_enemyShotLargeBall[5]; // 紫
            pPurple->param_i[0] = 1;   // 大玉フラグ
            pPurple->param_i[1] = 5;   // 色：紫
            pPurple->param_i[2] = i;
            pPurple->param_d[0] = 0.0;
            pPurple->param_d[1] = baseAngleP;
            pPurple->param_d[2] = a;
            pPurple->prev = pSet->pEnemyShotHead->prev;
            pPurple->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pPurple;
            pSet->pEnemyShotHead->prev = pPurple;
        }
    }

    // --------------------------------------------------
    // 2. 大玉の位置更新（回転＋半径拡大）
    // --------------------------------------------------
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) { // 大玉
            double radius = pShot->param_d[0];
            double baseAngle = pShot->param_d[1];
            double angVel = pShot->param_d[2];

            // 半径拡大（画面半分=240 に達するまで）
            if (radius < 240.0) {
                radius += 4.0;
                if (radius >= 240.0) {
                    pSet->param_i[2] = 2;
                    radius = 240.0;
                }
                pShot->param_d[0] = radius;
            }

            double theta = baseAngle + angVel * localCount;
            pShot->x = 240.0 + radius * cos(theta);
            pShot->y = 240.0 + radius * sin(theta);
        }
        pShot = pShot->next;
    }

    // --------------------------------------------------
    // 3. 鱗弾射出（Tフレームの間）
    // --------------------------------------------------
    if (localCount < T && pSet->param_i[2] == 2) {
        int rhythmCycle = t1 + t2;
        bool isShootingFrame = (rhythmCycle > 0) && ((localCount % rhythmCycle) < t1);

        if (isShootingFrame && localCount % 2 == 0) {
            // ズレ角：-th0 から +th0 まで等差数列的に変化
            double offsetAngle = -th0 + (2.0 * th0 / T) * localCount;

            pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                if (pShot->param_i[0] == 1) { // 大玉から射出
                    double toCenter = atan2(240.0 - pShot->y, 240.0 - pShot->x);
                    int color = pShot->param_i[1]; // 4:青 or 5:紫

                    // --- 内側鱗弾 1個（中央へ向かう）---
                    sEnemyShot* pIn = new sEnemyShot;
                    pIn->x = pShot->x;
                    pIn->y = pShot->y;
                    pIn->muki = toCenter + offsetAngle;
                    pIn->speed = v;
                    pIn->kind = img_enemyShotScale[color];
                    pIn->param_i[0] = 2; // 内側鱗弾
                    pIn->param_i[1] = 0; // 0:移動中, 1:停止中, 2:再開済み
                    pIn->param_d[0] = l0_base + l_inc * localCount; // 停止距離
                    pIn->param_d[1] = 0.0; // 累積移動距離
                    pIn->param_d[2] = v;   // 保存用速さ
                    pIn->prev = pSet->pEnemyShotHead->prev;
                    pIn->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pIn;
                    pSet->pEnemyShotHead->prev = pIn;

                    // --- 外側鱗弾 3個（外側へ低速）---
                    double outDir = toCenter + offsetAngle + DX_PI;
                    for (int j = 0; j < 3; j++) {
                        sEnemyShot* pOut = new sEnemyShot;
                        pOut->x = pShot->x;
                        pOut->y = pShot->y;
                        pOut->muki = outDir + (j - 1) * 0.08;
                        pOut->speed = v * 0.35;
                        pOut->kind = img_enemyShotScale[color];
                        pOut->param_i[0] = 3; // 外側鱗弾
                        pOut->prev = pSet->pEnemyShotHead->prev;
                        pOut->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = pOut;
                        pSet->pEnemyShotHead->prev = pOut;
                    }
                }
                pShot = pShot->next;
            }

            // 射出音（連続で鳴りすぎないよう間引き）
            if (localCount % 6 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }
    }

    // --------------------------------------------------
    // 4. 鱗弾の移動・停止処理
    // --------------------------------------------------
    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 2) { // 内側鱗弾
            // === 修正箇所 ===
            // 移動中(0) と 再開済み(2) の両方で移動処理を実行
            if (pShot->param_i[1] == 0 || pShot->param_i[1] == 2) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);

                // 停止距離のチェックは「移動中」のみ行う
                if (pShot->param_i[1] == 0) {
                    pShot->param_d[1] += pShot->speed;

                    // 停止距離に達したら停止
                    if (pShot->param_d[1] >= pShot->param_d[0]) {
                        pShot->param_i[1] = 1; // 停止中
                        pShot->speed = 0.0;
                    }
                }
            }
            // 停止中(param_i[1] == 1) は speed=0 なので自動的に止まる
        }
        else if (pShot->param_i[0] == 3) { // 外側鱗弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // --------------------------------------------------
    // 5. Tフレーム経過：停止していた鱗弾を再開
    // --------------------------------------------------
    if (localCount == T && pSet->param_i[3] == 0) {
        pSet->param_i[3] = 1;

        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 2 && pShot->param_i[1] == 1) {
                pShot->speed = pShot->param_d[2]; // 保存した速さvで再開
                pShot->param_i[1] = 2; // 再開済み
            }
            pShot = pShot->next;
        }

        // 停止弾再開の効果音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------
    // 6. 周期管理
    // --------------------------------------------------
    if (localCount == T + 90) {
        // 次の周期へ
        pSet->param_i[0]++;    // 周期番号
        pSet->param_i[1] = 0;  // ローカルカウントリセット
        pSet->param_i[2] = 0;  // 大玉未生成
        pSet->param_i[3] = 0;  // 再開処理未実行

        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) pShot->margin = -999;
            pShot = pShot->next;
        }
    }
    else {
        pSet->param_i[1]++;    // ローカルカウント増加
    }
}

// 敵本体のパターン：紫奥義「弾幕結界」
void EnemyPat_DanmakuKekkai_Kimi()
{
    static int shot_count = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }
    else {
        // 中央上部でゆっくり左右に揺れる
        enemy.x = 240.0 + sin(count * 0.02) * 10.0;
    }

    // 弾幕セットは1回だけ生成
    if (count == 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDanmakuBarrier;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;

        // 周期管理パラメータ初期化
        pSet->param_i[0] = 0; // 周期番号
        pSet->param_i[1] = 0; // ローカルカウント
        pSet->param_i[2] = 0; // 大玉未生成
        pSet->param_i[3] = 0; // 再開処理未実行

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}