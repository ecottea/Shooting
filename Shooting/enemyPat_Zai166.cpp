#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：マスゲーム「集団演技・四季の波」（難易度アップ版）
//
//  改訂ポイント：
//    1. グリッドを 11列×4行×2組=88発 に拡張（旧 7×2×2=28発）
//       → 画面横幅ほぼ全面を覆盖、サイドの安全地帯をほぼ消滅
//    2. 全フェーズを通じてボスから自機方向へ3弾ファンを定期発射
//       → 20フレームごと（約3回/秒）、計213発の自機狙い弾
//    3. 第3演技の交差ピークで大弾3発発射（既存、時刻だけ更新）
//    4. 総弾数 約305発/演技、画面内同時 約110発前後
//
//  構成（約24.7秒＝1480フレーム）：
//    第1演技 入場行進 (0-240)     : 上下端から進軍 ＋ ボスからのファン
//    第2演技 第一波   (240-640)   : 左→右→左 5往復 ＋ ファン
//    第3演技 交差波   (640-940)   : 双方向同時波、中央ピークで大弾 ＋ ファン
//    第4演技 分裂回転 (940-1300)  : 左右半分が逆回転 ＋ ファン
//    第5演技 退場行進 (1300-1480) : 整列退場 ＋ ボスからのファン（お見舞い）
// ============================================================

// --- グリッド定義 ---
static const int GRID_COLUMNS = 13;
static const int GRID_ROWS = 10;
static const double HOME_X[GRID_COLUMNS] = {
    0.0, 40.0, 80.0, 120.0, 160.0, 200.0, 240.0, 280.0, 320.0, 360.0, 400.0, 440.0, 480.0
};
static const double HOME_RED_Y[GRID_ROWS] = { 220.0, 200.0, 180.0, 160.0, 140.0, 120.0, 100.0, 80.0, 60.0, 40.0 };
static const double HOME_BLUE_Y[GRID_ROWS] = { 260.0, 280.0, 300.0, 320.0, 340.0, 360.0, 380.0, 400.0, 420.0, 440.0 };

// 旗手（中央・大玉）のホーム位置
static const double FLAG_HOME_X = 240.0;
static const double FLAG_HOME_Y = 240.0;

// ボス位置（ファン発射元）
static const double BOSS_X = 240.0;
static const double BOSS_Y = 40.0;

// フェーズ境界（フレーム）
static const int PHASE1_END = 240;     // 入場行進
static const int PHASE2_END = 640;     // 第一波（5 legs × 80）
static const int PHASE3_END = 940;     // 交差波（3 rounds × 100）
static const int PHASE4_END = 1300;   // 分裂回転（360）
static const int PHASE5_END = 1480;   // 退場行進（180）

// 波のパラメータ
static const double JUMP_H_WAVE = 20.0;   // 第2演技の跳ね幅
static const double JUMP_H_CROSS = 15.0;   // 第3演技の跳ね幅（1波あたり）
static const int    WAVE_JUMP_FRAMES = 30;  // 1弾の跳ね持続時間
static const int    WAVE_COL_OFFSET = 4;   // 列ごとの伝播オフセット
static const int    WAVE_LEG_FRAMES = 80;  // 1 leg の長さ（11列を伝播しきるため拡張）

// 交差ピーク時刻（ローカル）
static const int CROSS_PEAK_LOCAL =
(GRID_COLUMNS / 2) * WAVE_COL_OFFSET + WAVE_JUMP_FRAMES / 2;
//   11列の場合: 5*4 + 15 = 35

// ファン発射パラメータ
static const int    FAN_INTERVAL = 20;        // 発射間隔（フレーム）
static const int    FAN_BULLETS = 3;         // 1回の弾数
static const double FAN_SPREAD = 0.1745;    // ±10度（ラジアン）
static const double FAN_SPEED = 3.0;

// ループ周期
static const int LOOP_FRAMES = 1600;          // 1480演技 + 120休止

// グループID（pShot->param_i[0] に格納）
enum GroupID {
    GRP_RED = 0,
    GRP_BLUE = 1,
    GRP_FLAG = 2,   // 中央旗手（静止）
    GRP_FREE = 3,   // 自由飛翔弾（速度で動く）
};

// ============================================================
//  マスゲーム演技パターン
// ============================================================
static void ShotMassGame(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    // ---------- 生成 ----------
    if (t == 0) {
        // 予告音（演技開始のファンファーレ）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 赤組 (44発)
        for (int c = 0; c < GRID_COLUMNS; ++c) {
            for (int r = 0; r < GRID_ROWS; ++r) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->param_i[0] = GRP_RED;
                pShot->param_i[1] = c;
                pShot->param_i[2] = r;
                pShot->kind = img_enemyShotSmallBall[0]; // 0:赤
                pShot->x = HOME_X[c];
                pShot->y = 10.0;                       // 画面上端（安全位置）
                pShot->speed = 0.0;
                pShot->muki = 0.0;
                pShot->margin = 480;
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
        // 青組 (44発)
        for (int c = 0; c < GRID_COLUMNS; ++c) {
            for (int r = 0; r < GRID_ROWS; ++r) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->param_i[0] = GRP_BLUE;
                pShot->param_i[1] = c;
                pShot->param_i[2] = r;
                pShot->kind = img_enemyShotSmallBall[4]; // 4:青
                pShot->x = HOME_X[c];
                pShot->y = 470.0;                      // 画面下端（安全位置）
                pShot->speed = 0.0;
                pShot->muki = 0.0;
                pShot->margin = 480;
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
        // 旗手（中央・大玉）
        {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_i[0] = GRP_FLAG;
            pShot->param_i[1] = GRID_COLUMNS / 2;
            pShot->param_i[2] = 0;
            pShot->kind = img_enemyShotLargeBall[6]; // 6:白
            pShot->x = FLAG_HOME_X;
            pShot->y = FLAG_HOME_Y;               // 最初から中央
            pShot->speed = 0.0;
            pShot->muki = 0.0;
            pShot->margin = 480;
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // ---------- ファン発射（全フェーズ共通：t=60〜PHASE5_END） ----------
    // ボス位置から自機方向へ3弾ファンを発射。演技中は常に飛んでくる。
    if (t >= 60 && t < PHASE5_END && (t - 60) % FAN_INTERVAL == 0) {
        double base_muki = atan2(player.y - BOSS_Y, player.x - BOSS_X);
        for (int i = 0; i < FAN_BULLETS; ++i) {
            double offset = ((double)i - (double)(FAN_BULLETS - 1) / 2.0) * FAN_SPREAD;
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_i[0] = GRP_FREE;
            pShot->param_i[1] = 0;
            pShot->param_i[2] = 0;
            pShot->kind = img_enemyShotSmallBall[1]; // 1:黄
            pShot->x = BOSS_X;
            pShot->y = BOSS_Y;
            pShot->muki = base_muki + offset;
            pShot->speed = FAN_SPEED;
            pShot->margin = 480;
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        // 軽発射音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // ---------- 第3演技：中央交差ピークで大弾発射 ----------
    // 各ラウンドの中央列跳ねピーク時刻で、旗手から自機方向へ大弾
    if (t == PHASE2_END + CROSS_PEAK_LOCAL ||
        t == PHASE2_END + CROSS_PEAK_LOCAL + 100 ||
        t == PHASE2_END + CROSS_PEAK_LOCAL + 200) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->param_i[0] = GRP_FREE;
        pShot->param_i[1] = 0;
        pShot->param_i[2] = 0;
        pShot->kind = img_enemyShotLargeBall[8]; // 8:橙
        pShot->x = FLAG_HOME_X;
        pShot->y = FLAG_HOME_Y;
        pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
        pShot->speed = 3.0;
        pShot->margin = 480;
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;

        // 重発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ---------- 配置更新 ----------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int group = pShot->param_i[0];

        if (group == GRP_FREE) {
            // 自由飛翔弾：速度で動かす（画面外でメインルーチンが消去）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 演技弾：フェーズに応じて位置を直接制御
            int col = pShot->param_i[1];
            int row = pShot->param_i[2];

            // ホーム位置
            double hx, hy;
            if (group == GRP_RED) {
                hx = HOME_X[col];
                hy = HOME_RED_Y[row];
            }
            else if (group == GRP_BLUE) {
                hx = HOME_X[col];
                hy = HOME_BLUE_Y[row];
            }
            else { // GRP_FLAG
                hx = FLAG_HOME_X;
                hy = FLAG_HOME_Y;
            }

            if (t < PHASE1_END) {
                // === 第1演技：入場行進 ===
                double progress = (double)t / (double)PHASE1_END;
                double init_y;
                if (group == GRP_RED)        init_y = 10.0;
                else if (group == GRP_BLUE) init_y = 470.0;
                else                         init_y = hy;   // 旗手は最初からホーム
                pShot->x = hx;
                pShot->y = init_y + (hy - init_y) * progress;
            }
            else if (t < PHASE2_END) {
                // === 第2演技：第一波（左→右→左 を5往復） ===
                pShot->x = hx;
                pShot->y = hy;

                if (group != GRP_FLAG) {
                    int local_t = t - PHASE1_END;
                    int leg = local_t / WAVE_LEG_FRAMES;
                    int leg_local = local_t % WAVE_LEG_FRAMES;

                    if (leg < 5) {
                        // 偶数leg=左→右、奇数leg=右→左
                        int col_off;
                        if (leg % 2 == 0) col_off = col * WAVE_COL_OFFSET;
                        else              col_off = (GRID_COLUMNS - 1 - col) * WAVE_COL_OFFSET;

                        if (leg_local >= col_off && leg_local < col_off + WAVE_JUMP_FRAMES) {
                            double jt = (double)(leg_local - col_off) / (double)WAVE_JUMP_FRAMES;
                            double jh = sin(jt * DX_PI) * JUMP_H_WAVE;
                            if (group == GRP_RED)  pShot->y = hy - jh;
                            else                   pShot->y = hy + jh;
                        }
                    }
                }
            }
            else if (t < PHASE3_END) {
                // === 第3演技：交差波（双方向同時、3ラウンド） ===
                pShot->x = hx;
                pShot->y = hy;

                if (group != GRP_FLAG) {
                    int local_t = t - PHASE2_END;
                    int round = local_t / 100;     // 3ラウンド × 100フレーム
                    int r_local = local_t % 100;

                    if (round < 3) {
                        // 双方向の波を足し合わせる（中央列は両波同時到達 → 跳ね幅2倍）
                        double total = 0.0;

                        int offA = col * WAVE_COL_OFFSET;
                        if (r_local >= offA && r_local < offA + WAVE_JUMP_FRAMES) {
                            double jt = (double)(r_local - offA) / (double)WAVE_JUMP_FRAMES;
                            total += sin(jt * DX_PI) * JUMP_H_CROSS;
                        }
                        int offB = (GRID_COLUMNS - 1 - col) * WAVE_COL_OFFSET;
                        if (r_local >= offB && r_local < offB + WAVE_JUMP_FRAMES) {
                            double jt = (double)(r_local - offB) / (double)WAVE_JUMP_FRAMES;
                            total += sin(jt * DX_PI) * JUMP_H_CROSS;
                        }

                        if (group == GRP_RED)  pShot->y = hy - total;
                        else                   pShot->y = hy + total;
                    }
                }
            }
            else if (t < PHASE4_END) {
                // === 第4演技：分裂回転 ===
                int centerCol = GRID_COLUMNS / 2;   // 11列なら5（中央列=静止軸）
                if (group == GRP_FLAG || col == centerCol) {
                    pShot->x = hx;
                    pShot->y = hy;
                }
                else {
                    int local_t = t - PHASE3_END;
                    // 1回転（2π）を第4演技期間で行う。最終フレームで theta=2π となりホームへ戻る
                    double theta = (double)local_t * 2.0 * DX_PI
                        / (double)(PHASE4_END - PHASE3_END - 1);

                    bool   isLeft = (col < centerCol);
                    // 左半分の重心=(40+80+120+160+200)/5=120
                    // 右半分の重心=(280+320+360+400+440)/5=360
                    double pivotX = isLeft ? 120.0 : 360.0;
                    double pivotY = 240.0;

                    // 各グループの回転方向（スクリーン座標系では数学的正回転=画面時計回り）
                    //   赤・左 = +theta (画面 時計回り)
                    //   赤・右 = -theta (画面 反時計回り)
                    //   青・左 = -theta
                    //   青・右 = +theta
                    double sign = 0.0;
                    if (group == GRP_RED)  sign = isLeft ? +1.0 : -1.0;
                    else                    sign = isLeft ? -1.0 : +1.0;
                    double ang = sign * theta;

                    // ピボットからの相対ベクトルを回転
                    double rx = hx - pivotX;
                    double ry = hy - pivotY;
                    double ca = cos(ang);
                    double sa = sin(ang);
                    double nx = ca * rx - sa * ry;
                    double ny = sa * rx + ca * ry;
                    pShot->x = pivotX + nx;
                    pShot->y = pivotY + ny;
                }
            }
            else if (t < PHASE5_END) {
                // === 第5演技：退場行進 ===
                int local_t = t - PHASE4_END;

                if (local_t < 30) {
                    // ホーム位置で一拍静止
                    pShot->x = hx;
                    pShot->y = hy;
                }
                else {
                    // 行進で画面外へ
                    int    march_t = local_t - 30;
                    double march_speed = 3.0;
                    pShot->x = hx;
                    if (group == GRP_RED)        pShot->y = hy - march_speed * (double)march_t;
                    else if (group == GRP_BLUE)  pShot->y = hy + march_speed * (double)march_t;
                    else                          pShot->y = hy - march_speed * (double)march_t;
                }
            }
            else {
                // === 演技終了後：画面外へ流す（メインルーチンが消去） ===
                if (group == GRP_RED)        pShot->y -= 3.0;
                else if (group == GRP_BLUE)  pShot->y += 3.0;
                else                          pShot->y -= 3.0;
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_MassGame_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = BOSS_X;
        enemy.y = BOSS_Y;
        enemy.maxHp = enemy.hp = 300; // 200から変更しないこと
    }

    // 演技を LOOP_FRAMES 周期で生成
    if (count >= 60 && (count - 60) % LOOP_FRAMES == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMassGame;
        pEnemyShotSet->x = BOSS_X;
        pEnemyShotSet->y = BOSS_Y;
        pEnemyShotSet->muki = 0.0;
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