// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 双方向リストへの追加を簡略化するヘルパー関数
static void AddShotToList(sEnemyShotSet* pSet, sEnemyShot* pShot) {
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 大玉を生成するヘルパー関数
static void CreateBigShot(sEnemyShotSet* pSet, int color, double initAngle, double angularVel) {
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = 240.0;
    pShot->y = 240.0;
    pShot->speed = 0.0;
    pShot->muki = 0.0;
    pShot->kind = img_enemyShotLargeBall[color]; // 青(4)または紫(5)

    // param_d[0]: 現在の角度, param_d[1]: 角速度, param_d[2]: 現在の半径
    pShot->param_d[0] = initAngle;
    pShot->param_d[1] = angularVel;
    pShot->param_d[2] = 0.0;

    pShot->param_i[0] = color;
    pShot->param_i[10] = 0; // 消去フラグ (0:有効, 1:消去予定)
    AddShotToList(pSet, pShot);
}

// 鱗弾を生成するヘルパー関数
static void CreateScaleShot(sEnemyShotSet* pSet, double x, double y, double angle, double speed, int color, int isInner, int cycleNum, double stopDist) {
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = angle;
    pShot->speed = speed;
    pShot->kind = img_enemyShotScale[color]; // 青(4)または紫(5)

    // param_d[2]: 進んだ距離, param_d[3]: 停止距離
    pShot->param_d[2] = 0.0;
    pShot->param_d[3] = stopDist;

    // param_i[1]: 内側弾フラグ, param_i[2]: 何周期目に撃たれた弾か
    pShot->param_i[1] = isInner;
    pShot->param_i[2] = cycleNum;

    AddShotToList(pSet, pShot);
}

// 弾幕結界のメイン処理
static void ShotBarricade(sEnemyShotSet* pEnemyShotSet) {
    // 周期ごとのパラメータ配列
    // k, a, v, t1, t2, th0, l0, l, T
    const int NUM_CYCLES = 3;
    static const double cycleParams[NUM_CYCLES][9] = {
        { 4,  0.020, 3.0, 30, 15, DX_PI / 6.0, 30.0, 2.0,  90 },
        { 5, -0.025, 2.5, 20, 20, DX_PI / 4.0, 50.0, 1.5, 120 },
        { 6,  0.015, 4.0, 40, 10, DX_PI / 8.0, 60.0, 1.5, 100 },
    };

    int cycle = pEnemyShotSet->param_i[0] % NUM_CYCLES;
    int k = (int)cycleParams[cycle][0];
    double a = cycleParams[cycle][1];
    double v = cycleParams[cycle][2];
    int t1 = (int)cycleParams[cycle][3];
    int t2 = (int)cycleParams[cycle][4];
    double th0 = cycleParams[cycle][5];
    double l0 = cycleParams[cycle][6];
    double l_inc = cycleParams[cycle][7];
    int T = (int)cycleParams[cycle][8];

    // ==========================================
    // 1. 大玉の生成（拡大フェーズでかつ未生成の場合）
    // ==========================================
    // param_i[1] == 0 : 拡大フェーズ
    // param_i[3] == 0 : 未生成
    if (pEnemyShotSet->param_i[1] == 0 && pEnemyShotSet->param_i[3] == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < k; i++) {
            double angle = 2.0 * DX_PI * i / k;
            CreateBigShot(pEnemyShotSet, 4, angle, a);               // 青
            CreateBigShot(pEnemyShotSet, 5, angle + DX_PI / k, a);   // 紫 (青と半分ズラす)
        }
        pEnemyShotSet->param_i[3] = 1; // 生成済みフラグを立てる
    }

    // ==========================================
    // 2. 移動・拡大処理
    // ==========================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 大玉の処理
        if (pShot->kind == img_enemyShotLargeBall[4] || pShot->kind == img_enemyShotLargeBall[5]) {
            if (pShot->param_i[10] == 0) { // 有効な大玉のみ
                pShot->param_d[0] += pShot->param_d[1]; // 角度更新
                if (pShot->param_d[2] < 240.0) {        // 半径拡大 (画面幅の半分 = 240)
                    pShot->param_d[2] += 2.0;           // 拡大速度は 2.0/フレーム
                    if (pShot->param_d[2] > 240.0) pShot->param_d[2] = 240.0;
                }
                pShot->x = 240.0 + pShot->param_d[2] * cos(pShot->param_d[0]);
                pShot->y = 240.0 + pShot->param_d[2] * sin(pShot->param_d[0]);
            }
        }
        else {
            // 鱗弾の移動と停止処理
            if (pShot->speed > 0.0) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            // 内側弾の停止判定 (param_i[1] == 1 が内側弾のフラグ)
            if (pShot->param_i[1] == 1 && pShot->speed > 0.0) {
                pShot->param_d[2] += pShot->speed; // 進んだ距離を累積
                if (pShot->param_d[2] >= pShot->param_d[3]) { // 停止距離に達したら
                    pShot->speed = 0.0;
                }
            }
        }
        pShot = pShot->next;
    }

    // ==========================================
    // 3. フェーズ管理と射出処理
    // ==========================================
    if (pEnemyShotSet->param_i[1] == 0) {
        // 拡大完了チェック (全ての大玉の半径が240に達したか)
        bool allExpanded = true;
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if ((pShot->kind == img_enemyShotLargeBall[4] || pShot->kind == img_enemyShotLargeBall[5])
                && pShot->param_i[10] == 0 && pShot->param_d[2] < 240.0) {
                allExpanded = false;
                break;
            }
            pShot = pShot->next;
        }

        if (allExpanded) {
            pEnemyShotSet->param_i[1] = 1; // 射出フェーズへ移行
            pEnemyShotSet->param_i[2] = 0; // 射出用フレームカウンタをリセット
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
    else {
        // 射出フェーズ
        int t = pEnemyShotSet->param_i[2]; // 現在の周期内での経過フレーム
        if (t < T) {
            // 撃つリズムの判定 (t1フレーム撃つ、t2フレーム撃たない)
            int rhythm = t % (t1 + t2);
            if (rhythm < t1 && rhythm % 2 == 0) {
                // ズレ角の計算 (最初 -th0、最後 th0、1フレーム毎に加算)
                double delta_th = (T > 1) ? (2.0 * th0 / (T - 1.0)) : 0.0;
                double th_offset = -th0 + delta_th * t;

                pShot = pEnemyShotSet->pEnemyShotHead->next;
                while (pShot != pEnemyShotSet->pEnemyShotHead) {
                    // 有効な大玉から発射
                    if ((pShot->kind == img_enemyShotLargeBall[4] || pShot->kind == img_enemyShotLargeBall[5]) && pShot->param_i[10] == 0) {
                        // 大玉から画面中央に向かうベクトルを基準角度とする
                        double base_angle = atan2(240.0 - pShot->y, 240.0 - pShot->x);

                        // 内側に向かって1個
                        double stopDist = l0 + l_inc * t;
                        CreateScaleShot(pEnemyShotSet, pShot->x, pShot->y, base_angle + th_offset, v, pShot->param_i[0], 1, pEnemyShotSet->param_i[0], stopDist);

                        // 外側に向かって低速で3個
                        double outer_angle = base_angle + DX_PI + th_offset;
                        double outer_speed = v * 0.35; // 低速
                        for (int j = -1; j <= 1; j++) {
                            // 3発が重なって見えないのを防ぐためごくわずかに角度を散らす
                            CreateScaleShot(pEnemyShotSet, pShot->x, pShot->y, outer_angle + j * 0.04, outer_speed, pShot->param_i[0], 0, pEnemyShotSet->param_i[0], 0.0);
                        }
                    }
                    pShot = pShot->next;
                }
            }
            pEnemyShotSet->param_i[2]++; // 射出フレームカウンタを進める
        }
        else {
            // ==========================================
            // 4. 周期終了処理
            // ==========================================
            // 停止させていた内側弾を再度速さvで動かす
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[1] == 1 && pShot->param_i[2] == pEnemyShotSet->param_i[0] && pShot->speed == 0.0) {
                    pShot->speed = v;
                    pShot->param_i[1] = 2;
                }
                pShot = pShot->next;
            }

            // 大玉に消去フラグを立てる
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->kind == img_enemyShotLargeBall[4] || pShot->kind == img_enemyShotLargeBall[5]) {
                    pShot->param_i[10] = 1;
                }
                pShot = pShot->next;
            }

            // 次の周期へ移行
            pEnemyShotSet->param_i[0]++;     // 周期番号をインクリメント
            pEnemyShotSet->param_i[1] = 0;   // 拡大フェーズに戻す
            pEnemyShotSet->param_i[2] = 0;   // 射出フレームカウンタをリセット
            pEnemyShotSet->param_i[3] = 0;   // 生成済みフラグをリセット
        }
    }

    // ==========================================
    // 5. 消去フラグが立った大玉をリストから外して削除
    // ==========================================
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;
        if (pShot->param_i[10] == 1) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot; // プールに返却
        }
        else if (pShot->param_i[1] == 2) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = nextShot;
    }
}


// 敵本体のパターン
void EnemyPat_DanmakuKekkai_Zai()
{
    static int muki;
    static int initialized;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        initialized = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 弾幕セットの初回生成
    if (!initialized) {
        initialized = 1;
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBarricade;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 240.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // param_i はゼロ初期化されているため、以下の通りとして扱う
        // param_i[0]: 周期番号 (0, 1, 2...)
        // param_i[1]: フェーズ (0:拡大, 1:射出)
        // param_i[2]: 射出フレームカウンタ
        // param_i[3]: 大玉生成済みフラグ

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}