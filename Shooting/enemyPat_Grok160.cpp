#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  黄金螺旋の結晶崩壊 (Golden Spiral Crystallization)
//  TAS専用・極限難易度・幾何学的に美しい弾幕
//  人間の反応速度では生存不能。フレーム単位の軌道計算が必須。
// ============================================================

// 黄金比関連定数
static const double PHI = 1.618033988749895;
static const double GOLDEN_ANGLE = 2.399963229728653;           // ≈137.50776°
static const double TWO_PI = 6.283185307179586;
static const double INV_PHI = 0.618033988749895;

// フェーズ時間 (60fps想定) — 遷移を短く急峻に
static const int PHASE1_END = 300;   // 0〜5秒  : 螺旋の芽生え（高速化）
static const int PHASE2_END = 540;   // 5〜9秒  : 結晶化（遷移短縮）
static const int PHASE3_END = 900;   // 9〜15秒 : 崩壊と再帰

// ------------------------------------------------------------
//  弾幕パターン本体
// ------------------------------------------------------------
static void ShotGoldenSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 中心座標を敵位置に追従
    const double cx = enemy.x;
    const double cy = enemy.y + 18.0;

    // --------------------------------------------------------
    //  初期化（count == 0 のときのみ弾を生成）
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 予告音 + 発射音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // param_i[0] : アーム番号
        // param_i[1] : アーム内インデックス
        // param_i[2] : 色インデックス
        // param_i[3] : レイヤー種別 (0:主螺旋 1:カウンター 2:高密度干渉層)
        // param_d[0] : 初期角度 (θ0)
        // param_d[1] : 螺旋成長係数 b
        // param_d[2] : 基準半径スケール a
        // param_d[3] : 回転角速度
        // param_d[4] : 結晶化用ターゲット角度
        // param_d[5] : 結晶化用ターゲット半径
        // param_d[6] : 追加位相オフセット（隙間を極限まで狭める用）

        // ---- レイヤー0: 主螺旋アーム（高密度・黄金角） ----
        const int ARMS = 13;      // フィボナッチ数で密度向上
        const int PER_ARM = 36;      // 1アームあたり大幅増

        for (int arm = 0; arm < ARMS; arm++) {
            double base_theta = arm * GOLDEN_ANGLE;
            int color = (arm * 2 + 3) % 9;

            for (int i = 0; i < PER_ARM; i++) {
                pEnemyShot = new sEnemyShot;

                double t = (double)i / (PER_ARM - 1);
                // 外側ほど極端に密になる指数配置
                double r0 = 12.0 + 110.0 * t * t * t;
                double b = 0.112 + 0.018 * (arm % 4);
                // 角速度を高め、アーム間で微妙にずらして隙間を常時変化させる
                double omega = 0.031 + 0.007 * ((arm % 3) - 1);

                pEnemyShot->param_i[0] = arm;
                pEnemyShot->param_i[1] = i;
                pEnemyShot->param_i[2] = color;
                pEnemyShot->param_i[3] = 0;
                pEnemyShot->param_d[0] = base_theta + i * 0.068;
                pEnemyShot->param_d[1] = b;
                pEnemyShot->param_d[2] = r0;
                pEnemyShot->param_d[3] = omega;
                pEnemyShot->param_d[4] = 0.0;
                pEnemyShot->param_d[5] = 0.0;
                pEnemyShot->param_d[6] = (arm * 0.17 + i * 0.03);

                // 高密度のためほぼ小玉主体。所々菱形で幾何学線を強調
                if (i % 5 == 0) {
                    pEnemyShot->kind = img_enemyShotDiamond[color];
                }
                else {
                    pEnemyShot->kind = img_enemyShotSmallBall[color];
                }

                pEnemyShot->x = cx;
                pEnemyShot->y = cy;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->margin = 999;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // ---- レイヤー1: 反時計回り高密度カウンター ----
        const int COUNTER = 8;
        const int PER_COUNTER = 28;

        for (int arm = 0; arm < COUNTER; arm++) {
            double base_theta = arm * (TWO_PI / COUNTER) + 0.31;
            int color = (arm * 3 + 5) % 9;

            for (int i = 0; i < PER_COUNTER; i++) {
                pEnemyShot = new sEnemyShot;

                double t = (double)i / (PER_COUNTER - 1);
                double r0 = 22.0 + 95.0 * t * t;
                double b = 0.098;
                double omega = -0.038;  // 速い反時計

                pEnemyShot->param_i[0] = 100 + arm;
                pEnemyShot->param_i[1] = i;
                pEnemyShot->param_i[2] = color;
                pEnemyShot->param_i[3] = 1;
                pEnemyShot->param_d[0] = base_theta - i * 0.092;
                pEnemyShot->param_d[1] = b;
                pEnemyShot->param_d[2] = r0;
                pEnemyShot->param_d[3] = omega;
                pEnemyShot->param_d[4] = 0.0;
                pEnemyShot->param_d[5] = 0.0;
                pEnemyShot->param_d[6] = arm * 0.41;

                pEnemyShot->kind = img_enemyShotScale[color];

                pEnemyShot->x = cx;
                pEnemyShot->y = cy;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->margin = 999;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // ---- レイヤー2: 超高密度干渉層（隙間を極限まで潰す） ----
        // 主螺旋の隙間を埋めるように配置。TASでしか抜けられない狭さを生む
        const int INTERF = 21;
        const int PER_INTERF = 20;

        for (int arm = 0; arm < INTERF; arm++) {
            double base_theta = arm * (GOLDEN_ANGLE * 0.5) + 0.07;
            int color = (arm + 6) % 9;

            for (int i = 0; i < PER_INTERF; i++) {
                pEnemyShot = new sEnemyShot;

                double t = (double)i / (PER_INTERF - 1);
                double r0 = 28.0 + 80.0 * t;
                double b = 0.075 + 0.01 * (i % 3);
                double omega = 0.024 * ((arm % 2) * 2 - 1);  // 交互に逆回転

                pEnemyShot->param_i[0] = 200 + arm;
                pEnemyShot->param_i[1] = i;
                pEnemyShot->param_i[2] = color;
                pEnemyShot->param_i[3] = 2;
                pEnemyShot->param_d[0] = base_theta + i * 0.055;
                pEnemyShot->param_d[1] = b;
                pEnemyShot->param_d[2] = r0;
                pEnemyShot->param_d[3] = omega;
                pEnemyShot->param_d[4] = 0.0;
                pEnemyShot->param_d[5] = 0.0;
                pEnemyShot->param_d[6] = i * 0.13 + arm * 0.09;

                pEnemyShot->kind = img_enemyShotSmallBall[color];

                pEnemyShot->x = cx;
                pEnemyShot->y = cy;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->margin = 999;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // --------------------------------------------------------
    //  毎フレーム更新
    // --------------------------------------------------------
    const int t = pEnemyShotSet->count;

    int phase = 0;
    if (t >= PHASE3_END)      phase = 3;
    else if (t >= PHASE2_END) phase = 2;
    else if (t >= PHASE1_END) phase = 1;
    else                      phase = 0;

    // フェーズ遷移音
    if (t == PHASE1_END) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
    if (t == PHASE2_END) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int   arm = pShot->param_i[0];
        const int   idx = pShot->param_i[1];
        const int   layer = pShot->param_i[3];
        const double theta0 = pShot->param_d[0];
        const double b = pShot->param_d[1];
        const double a = pShot->param_d[2];
        const double omega = pShot->param_d[3];
        const double phase_off = pShot->param_d[6];

        double theta, r;

        if (phase == 0) {
            // ===== フェーズ0: 高速黄金螺旋 =====
            // 回転・膨張を速め、呼吸変調の周波数を上げて隙間が高速開閉する
            double spiral_t = t * 1.25;
            theta = theta0 + omega * spiral_t + 0.4 * sin(t * 0.11 + phase_off);
            r = a * exp(b * (spiral_t * 0.052 + idx * 0.055));
            // 高周波の微小振動で人間の予測を不可能に
            r *= (1.0 + 0.09 * sin(t * 0.19 + arm * 0.7 + phase_off));
            // 層ごとにわずかに異なる半径変調を加え干渉を強める
            if (layer == 2) {
                r *= (1.0 + 0.05 * sin(t * 0.27 + idx));
            }
        }
        else if (phase == 1) {
            // ===== フェーズ1: 急峻な結晶化 =====
            if (t == PHASE1_END) {
                double cur_theta = theta0 + omega * (PHASE1_END * 1.25);
                double cur_r = a * exp(b * (PHASE1_END * 1.25 * 0.052 + idx * 0.055));

                // より高次の対称性へスナップ（隙間を極小化）
                int fold;
                if (layer == 0) fold = 13;
                else if (layer == 1) fold = 8;
                else fold = 21;
                double sector = TWO_PI / fold;
                double snapped = floor(cur_theta / sector + 0.5) * sector;

                pShot->param_d[4] = snapped;
                // ターゲット半径を詰めて密度をさらに上げる
                pShot->param_d[5] = cur_r * 0.92;
            }

            // 遷移時間を短く、easeを急峻に（ほぼ即座に格子化）
            double local_t = (double)(t - PHASE1_END) / 90.0;  // 約1.5秒で完了
            if (local_t > 1.0) local_t = 1.0;
            // 非常に急なease（quinticに近い）
            double ease = local_t * local_t * local_t * (local_t * (local_t * 6.0 - 15.0) + 10.0);

            double spiral_t = PHASE1_END * 1.25;
            double spiral_theta = theta0 + omega * spiral_t;
            double spiral_r = a * exp(b * (spiral_t * 0.052 + idx * 0.055));

            theta = spiral_theta + (pShot->param_d[4] - spiral_theta) * ease;
            r = spiral_r + (pShot->param_d[5] - spiral_r) * ease;

            // 結晶化後は高速回転 + 高周波呼吸で隙間が常時変動
            double crystal_rot = (t - PHASE1_END) * 0.014;
            theta += crystal_rot + 0.08 * sin((t - PHASE1_END) * 0.13 + phase_off);
            r *= (1.0 + 0.11 * sin((t - PHASE1_END) * 0.09 + idx * 0.4));
        }
        else {
            // ===== フェーズ2以降: 急速崩壊 + 再帰的密集 =====
            double local_t = (double)(t - PHASE2_END);
            // 収縮を速く
            double collapse = exp(-local_t * 0.014);
            double new_spiral = (1.0 - collapse) * 2.4;

            double base_theta = pShot->param_d[4];
            double base_r = pShot->param_d[5];

            // 回転を加速させ続ける
            theta = base_theta + local_t * (0.019 + 0.0007 * local_t);
            r = base_r * collapse;

            // 新しい螺旋成分を強く混ぜ、フラクタル的に隙間を埋める
            double add_theta = theta0 + omega * (local_t * 1.9) + 0.3 * sin(local_t * 0.15 + phase_off);
            double add_r = a * 0.75 * new_spiral * exp(b * local_t * 0.04);
            double mix = new_spiral * 0.85;
            if (mix > 1.0) mix = 1.0;
            theta = theta * (1.0 - mix) + add_theta * mix;
            r = r + add_r;

            // 層2は特に激しく外側へ拡散しつつ内部を埋める
            if (layer == 2) {
                r *= (1.0 + 0.15 * sin(local_t * 0.21 + arm));
            }

            // 終盤で急加速して画面を埋め尽くす
            if (t > PHASE3_END - 90) {
                double final_push = (t - (PHASE3_END - 90)) * 1.1;
                r += final_push * final_push * 0.22;
            }
        }

        // 位置確定
        pShot->x = cx + r * cos(theta);
        pShot->y = cy + r * sin(theta);
        pShot->muki = theta + DX_PI * 0.5;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体パターン
// ------------------------------------------------------------
void EnemyPat_GeometryTAS_Grok()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 85.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵自身も黄金比周期でやや大きめに動かし、基準点を常にずらす
        double t = (double)count;
        enemy.x = 240.0 + 26.0 * sin(t * 0.013 * INV_PHI);
        enemy.y = 85.0 + 11.0 * sin(t * 0.019 + 0.4);
    }

    // パターン開始
    if (count % 900 == 20) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGoldenSpiral;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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