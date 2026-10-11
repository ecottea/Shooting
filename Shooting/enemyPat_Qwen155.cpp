#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 平安京の悪夢パターンの弾更新用関数
static void HeiankyoNightmare_Update(sEnemyShotSet* pSet)
{
    int state = pSet->param_i[0];     // 0:初期待機, 1:移動中, 2:移動後待機, 3:完了
    int t = pSet->param_i[1];         // 現在のサブ状態での経過フレーム
    int T = pSet->param_i[2];         // 加速/減速時間(フレーム)

    if (state == 0) {
        // 初期待機 (1秒 = 60フレーム)
        if (pSet->count >= 60) {
            pSet->param_i[0] = 1;
            pSet->param_i[1] = 0;

            // レーザーを中玉に変更
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                pShot->kind = img_enemyShotMediumBall[0]; // 0:赤中玉
                pShot = pShot->next;
            }

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }
    else if (state == 1) {
        // 移動中 (時間Tで加速、時間Tで減速)
        pSet->param_i[1]++;

        double f = 0.0;
        if (t <= T) {
            f = 0.5 * (t / (double)T) * (t / (double)T);
        }
        else if (t <= 2 * T) {
            double t2 = 2.0 * T - t;
            f = 1.0 - 0.5 * (t2 / (double)T) * (t2 / (double)T);
        }
        else {
            f = 1.0;
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x = pShot->param_d[0] + (pShot->param_d[2] - pShot->param_d[0]) * f;
            pShot->y = pShot->param_d[1] + (pShot->param_d[3] - pShot->param_d[1]) * f;
            pShot = pShot->next;
        }

        if (t >= 2 * T) {
            pSet->param_i[0] = 2;
            pSet->param_i[1] = 0;

            // 中玉をレーザーに戻す
            pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                pShot->kind = img_enemyShotLaser[0]; // 0:赤レーザー
                pShot = pShot->next;
            }
        }
    }
    else if (state == 2) {
        // 移動後待機 (1秒 = 60フレーム)
        pSet->param_i[1]++;
        if (t >= 60) {
            pSet->param_i[0] = 3; // 完了状態へ
        }
    }
    else if (state == 3) {
        // 完了状態。終了フラグを立てて EnemyPat_Heiankyo_Qwen 側に破棄を通知する。
        pSet->param_i[15] = 1;
    }
}

// 自機狙い大玉列パターンの弾更新用関数
static void BigBallAim_Update(sEnemyShotSet* pSet)
{
    // 等速直進処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }

    // 一定間隔で大玉の列を追加発射 (60フレームごと、最大4回まで)
    if (pSet->count % 80 == 0 && pSet->param_i[1] < 4) {
        pSet->param_i[1]++;

        double start_x = pSet->param_d[0];
        double start_y = pSet->param_d[1];
        double muki = pSet->param_d[2];
        bool is_vertical = (pSet->param_i[2] == 1); // 1なら上下方向, 0なら左右方向

        for (int k = 0; k < 5; ++k) {
            sEnemyShot* pShotNew = new sEnemyShot;
            pShotNew->kind = img_enemyShotLargeBall[0]; // 0:赤大玉
            pShotNew->count = 0;
            pShotNew->speed = 3.5-0.5;
            pShotNew->muki = muki;
            pShotNew->margin = 160;

            if (is_vertical) {
                pShotNew->x = player.x;
                pShotNew->y = start_y + -k * 40.0 * (start_y < 0 ? 1.0 : -1.0);
            }
            else {
                pShotNew->x = start_x + -k * 40.0 * (start_x < 0 ? 1.0 : -1.0);
                pShotNew->y = player.y;
            }

            pShotNew->prev = pSet->pEnemyShotHead->prev;
            pShotNew->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShotNew;
            pSet->pEnemyShotHead->prev = pShotNew;
        }
    }

    // パターン終了判定 (240フレーム経過で終了)
    if (pSet->count >= 240) {
        pSet->param_i[15] = 1;
    }
}

// 敵本体のパターン：平安京の悪夢風
void EnemyPat_Heiankyo_Qwen()
{
    static int current_hp_phase = 0;        // 0:HP 2/3以上, 1:HP 1/3～2/3, 2:HP 1/3以下
    static sEnemyShotSet* pSetNormal = nullptr;
    static sEnemyShotSet* pSetTransposed = nullptr;
    static sEnemyShotSet* pSetBigBall = nullptr;

    // count == 1 での初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        current_hp_phase = 0;
        pSetNormal = nullptr;
        pSetTransposed = nullptr;
        pSetBigBall = nullptr;
    }

    // HPフェーズの判定
    int new_hp_phase = 0;
    if (enemy.hp < enemy.maxHp / 3) {
        new_hp_phase = 2;
    }
    else if (enemy.hp < enemy.maxHp * 2 / 3) {
        new_hp_phase = 1;
    }

    // フェーズが変更された場合は現在のパターンを強制終了させてリセット
    if (new_hp_phase != current_hp_phase) {
        current_hp_phase = new_hp_phase;
        if (pSetNormal) pSetNormal->param_i[15] = 1;
        if (pSetTransposed) pSetTransposed->param_i[15] = 1;
        if (pSetBigBall) pSetBigBall->param_i[15] = 1;
    }

    // パターンの生成と破棄の管理
    bool need_new_pattern = false;

    // 【修正点】初期状態（全てnullptr）の場合も生成対象とする
    if (pSetNormal == nullptr && pSetTransposed == nullptr && pSetBigBall == nullptr && count >= 30) {
        need_new_pattern = true;
    }
    else {
        // いずれかが終了フラグを立てた場合
        if (pSetNormal && pSetNormal->param_i[15] == 1) need_new_pattern = true;
        if (pSetTransposed && pSetTransposed->param_i[15] == 1) need_new_pattern = true;
        if (pSetBigBall && pSetBigBall->param_i[15] == 1) need_new_pattern = true;
    }

    // 初期状態や強制終了後にクリアと新規生成
    if (need_new_pattern) {
        // クリーンアップ用ラムダ
        auto cleanup = [](sEnemyShotSet*& pSet) {
            if (pSet) {
                pSet->prev->next = pSet->next;
                pSet->next->prev = pSet->prev;
                sEnemyShot* pShot = pSet->pEnemyShotHead->next;
                while (pShot != pSet->pEnemyShotHead) {
                    sEnemyShot* next = pShot->next;
                    delete pShot;
                    pShot = next;
                }
                delete pSet->pEnemyShotHead;
                delete pSet;
                pSet = nullptr;
            }
        };

        cleanup(pSetNormal);
        cleanup(pSetTransposed);
        cleanup(pSetBigBall);

        // 新しいパラメータをランダム生成
        int N = 6 + GetRand(4);             // 6 ～ 10
        int H = 30 + GetRand(20);           // 30 ～ 50 (狭め)
        int T = 45 + GetRand(30)+20;           // 45 ～ 75 フレーム
        const int W = 80;                   // 広め（固定）

        int P[16], Q[16], R_arr[16], D_arr[16];

        // 順列 P の生成 (0 から N-1)
        for (int i = 0; i < N; ++i) P[i] = i;
        for (int i = N - 1; i > 0; --i) {
            int j = GetRand(i);
            int tmp = P[i]; P[i] = P[j]; P[j] = tmp;
        }

        // Q の計算
        for (int k = 0; k < N; ++k) {
            Q[k] = (P[k] - k + N) % N;
        }

        // R, D のランダムビット列生成
        for (int i = 0; i < N; ++i) {
            R_arr[i] = GetRand(1);
            D_arr[i] = GetRand(1);
        }

        // ShotSet 生成用ラムダ
        auto create_shot_set = [&]() {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->param_i[0] = 0;    // state
            pSet->param_i[1] = 0;    // sub_count / 発射カウンター
            pSet->param_i[15] = 0;   // 終了フラグ
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
            return pSet;
        };

        // 格子パターン生成用ラムダ
        auto generate_grid = [&](sEnemyShotSet* pSet, bool transposed) {
            pSet->patternFunc = HeiankyoNightmare_Update;
            pSet->param_i[2] = T;    // 加速/減速時間

            int g_max_i = 480 / W;
            int g_max_j = 480 / H;
            if (transposed) {
                g_max_i = 480 / H;
                g_max_j = 480 / W;
            }

            for (int i = 0; i <= g_max_i; ++i) {
                for (int j = 0; j <= g_max_j; ++j) {
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->kind = img_enemyShotLaser[0];
                    pShot->speed = 0.0;
                    pShot->count = 0;
                    pShot->muki = DX_PI / 2.0;

                    int r;
                    double x_start, y_start, x_target, y_target;

                    if (!transposed) {
                        r = j % N;
                        x_start = i * W;
                        y_start = j * H;
                        int i2 = (R_arr[r] == 1) ? (i + 1) : (i - 1);
                        int j2 = (D_arr[r] == 1) ? (j + Q[r]) : (j + Q[r] - N);
                        x_target = i2 * W;
                        y_target = j2 * H;
                    }
                    else {
                        r = i % N;
                        x_start = j * W;
                        y_start = i * H;
                        int j2 = (R_arr[r] == 1) ? (j + 1) : (j - 1);
                        int i2 = (D_arr[r] == 1) ? (i + Q[r]) : (i + Q[r] - N);
                        x_target = i2 * H;
                        y_target = j2 * W;
                    }

                    pShot->x = x_start;
                    pShot->y = y_start;
                    pShot->param_d[0] = x_start;
                    pShot->param_d[1] = y_start;
                    pShot->param_d[2] = x_target;
                    pShot->param_d[3] = y_target;
                    pShot->margin = 100;

                    pShot->prev = pSet->pEnemyShotHead->prev;
                    pShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pShot;
                    pSet->pEnemyShotHead->prev = pShot;
                }
            }
        };

        // フェーズに応じたパターン構成
        if (current_hp_phase == 0) {
            pSetNormal = create_shot_set();
            generate_grid(pSetNormal, false);
        }
        else if (current_hp_phase == 1) {
            pSetTransposed = create_shot_set();
            generate_grid(pSetTransposed, true);
        }
        else if (current_hp_phase == 2) {
            pSetNormal = create_shot_set();
            generate_grid(pSetNormal, false);
            pSetTransposed = create_shot_set();
            generate_grid(pSetTransposed, true);
        }

        // 大玉パターン生成 (phase 0, 1 のみ)
        if (current_hp_phase == 0 || current_hp_phase == 1) {
            pSetBigBall = create_shot_set();
            pSetBigBall->patternFunc = BigBallAim_Update;

            if (current_hp_phase == 0) {
                // 上下方向
                double start_y = (GetRand(1) == 0) ? -20.0 : 500.0;
                double dy = player.y - start_y;
                double muki = atan2(dy, 0.0);

                pSetBigBall->param_d[0] = 0.0;       // 未使用
                pSetBigBall->param_d[1] = start_y;   // 開始Y座標
                pSetBigBall->param_d[2] = muki;      // 向き
                pSetBigBall->param_i[2] = 1;         // 上下方向フラグ
            }
            else {
                // 左右方向
                double start_x = (GetRand(1) == 0) ? -20.0 : 500.0;
                double dx = player.x - start_x;
                double muki = atan2(0.0, dx);

                pSetBigBall->param_d[0] = start_x;   // 開始X座標
                pSetBigBall->param_d[1] = 0.0;       // 未使用
                pSetBigBall->param_d[2] = muki;      // 向き
                pSetBigBall->param_i[2] = 0;         // 左右方向フラグ
            }
        }

        // 予告音再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}