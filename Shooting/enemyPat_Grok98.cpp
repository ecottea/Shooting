// enemyPat_Tmp.cpp
// 紫奥義「弾幕結界」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 画面中心
static const double CENTER_X = 240.0;
static const double CENTER_Y = 240.0;

// 1周期分のパラメータ
struct BarrierParams {
    int    k;      // 青・紫それぞれk個
    double a;      // 角速度 (rad/frame)
    double v;      // 鱗弾の速さ
    int    t1;     // 連続発射フレーム数
    int    t2;     // 連続休止フレーム数
    double th0;    // 角度ズレの最大値 (rad)
    double l0;     // 内側鱗弾の初期停止距離
    double l;      // 停止距離の増加量 / frame
    int    T;      // 射出期間フレーム数
};

// 周期ごとのパラメータテーブル（徐々に難化）
static const BarrierParams PARAMS[] = {
    // k,   a,      v,    t1, t2,  th0,   l0,   l,    T
    {  4,  0.015,  2.8,   3,  5,  0.25,  40.0, 0.4,  90 },
    {  5,  0.020,  3.0,   3,  4,  0.30,  35.0, 0.5, 100 },
    {  6,  0.025,  3.2,   2,  4,  0.35,  30.0, 0.6, 110 },
    {  7,  0.030,  3.4,   2,  3,  0.40,  25.0, 0.7, 120 },
    {  8,  0.035,  3.6,   2,  3,  0.45,  20.0, 0.8, 130 },
};
static const int NUM_CYCLES = sizeof(PARAMS) / sizeof(PARAMS[0]);

// 大玉・鱗弾の色
static const int COLOR_BLUE = 4; // 青
static const int COLOR_PURPLE = 5; // マゼンタ（紫）

// ------------------------------------------------------------
// 弾幕結界パターン本体
// ------------------------------------------------------------
static void ShotDanmakuKekkai(sEnemyShotSet* pEnemyShotSet)
{
    // param_i の使い方
    // [0] : フェーズ (0=展開, 1=射出, 2=再開後待機, 3=終了)
    // [1] : 現在の周期インデックス
    // [2] : フェーズ内経過フレーム
    // [3] : 大玉の総数 (2*k)
    // [4] : 射出リズム用カウンタ
    // [5] : 現在が発射中か (1/0)
    //
    // param_d の使い方
    // [0] : 現在の半径
    // [1] : 現在の基準角度 (全体の回転)
    // [2] : 外側鱗弾の速さ
    // [3] : 展開速度 (半径増加量/frame)

    sEnemyShot* pShot;

    // ---- 初期化 (count==0) ----
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = 0; // フェーズ0: 展開
        pEnemyShotSet->param_i[1] = 0; // 周期0
        pEnemyShotSet->param_i[2] = 0; // フェーズ内カウンタ
        pEnemyShotSet->param_d[0] = 20.0; // 初期半径
        pEnemyShotSet->param_d[1] = 0.0;  // 基準角度
        pEnemyShotSet->param_d[3] = 3.5;  // 半径増加速度

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 最初の周期の大玉を生成
        const BarrierParams& p = PARAMS[0];
        int total = p.k * 2;
        pEnemyShotSet->param_i[3] = total;

        for (int i = 0; i < total; i++) {
            pShot = new sEnemyShot;
            int color = (i % 2 == 0) ? COLOR_BLUE : COLOR_PURPLE;
            pShot->kind = img_enemyShotLargeBall[color];
            pShot->x = CENTER_X;
            pShot->y = CENTER_Y;
            pShot->muki = 0.0;
            pShot->speed = 0.0; // 位置は手動更新
            // param_i[0] : 大玉フラグ (1=大玉)
            // param_i[1] : インデックス
            // param_i[2] : 色
            pShot->param_i[0] = 1;
            pShot->param_i[1] = i;
            pShot->param_i[2] = color;
            // param_d[0] : 初期角度オフセット
            pShot->param_d[0] = (DX_PI * 2.0 * i) / total;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        return;
    }

    int phase = pEnemyShotSet->param_i[0];
    int cycle = pEnemyShotSet->param_i[1];
    int phaseCnt = pEnemyShotSet->param_i[2];
    const BarrierParams& param = PARAMS[cycle < NUM_CYCLES ? cycle : NUM_CYCLES - 1];

    // ---- フェーズ0: 半径拡大＋回転 ----
    if (phase == 0) {
        double& r = pEnemyShotSet->param_d[0];
        double& baseAng = pEnemyShotSet->param_d[1];
        double expandSpd = pEnemyShotSet->param_d[3];

        r += expandSpd;
        if (r >= 240.0) {
            r = 240.0;
            // 展開完了 → 射出フェーズへ
            pEnemyShotSet->param_i[0] = 1;
            pEnemyShotSet->param_i[2] = 0;
            pEnemyShotSet->param_i[4] = 0; // リズムカウンタ
            pEnemyShotSet->param_i[5] = 1; // 発射中
            pEnemyShotSet->param_d[2] = param.v * 0.35; // 外側の低速
        }
        baseAng += param.a;

        // 大玉の位置を更新
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                double ang = baseAng + pShot->param_d[0];
                pShot->x = CENTER_X + r * cos(ang);
                pShot->y = CENTER_Y + r * sin(ang);
                pShot->muki = ang; // 向きも更新（見た目用）
            }
            pShot = pShot->next;
        }
        pEnemyShotSet->param_i[2]++;
        return;
    }

    // ---- フェーズ1: 鱗弾射出 ----
    if (phase == 1) {
        double r = pEnemyShotSet->param_d[0];
        double& baseAng = pEnemyShotSet->param_d[1];
        double outerSpd = pEnemyShotSet->param_d[2];

        // 大玉を回転させ続ける
        baseAng += param.a;

        // 大玉位置更新
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                double ang = baseAng + pShot->param_d[0];
                pShot->x = CENTER_X + r * cos(ang);
                pShot->y = CENTER_Y + r * sin(ang);
                pShot->muki = ang;
            }
            pShot = pShot->next;
        }

        // リズム管理
        int& rhythmCnt = pEnemyShotSet->param_i[4];
        int& isFiring = pEnemyShotSet->param_i[5];

        if (isFiring) {
            if (rhythmCnt >= param.t1) {
                isFiring = 0;
                rhythmCnt = 0;
            }
        }
        else {
            if (rhythmCnt >= param.t2) {
                isFiring = 1;
                rhythmCnt = 0;
            }
        }
        rhythmCnt++;

        // 射出期間中かつ発射フレームなら鱗弾を生成
        if (phaseCnt < param.T && isFiring && phaseCnt % 2 == 0) {
            // 角度ズレ量を線形補間: -th0 → +th0
            double t = (param.T <= 1) ? 0.0 : (double)phaseCnt / (param.T - 1);
            double offset = -param.th0 + 2.0 * param.th0 * t;

            // 効果音（適度に間引く）
            if (phaseCnt % 4 == 0) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }

            // 各大玉から射出
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] == 1) {
                    double ang = baseAng + pShot->param_d[0]; // 大玉の現在角度
                    int color = pShot->param_i[2];

                    // --- 内側へ1発 ---
                    sEnemyShot* pScale = new sEnemyShot;
                    pScale->kind = img_enemyShotScale[color];
                    pScale->x = pShot->x;
                    pScale->y = pShot->y;
                    // 中心方向 + オフセット
                    double inDir = ang + DX_PI + offset;
                    pScale->muki = inDir;
                    pScale->speed = param.v;
                    // 内側鱗弾フラグ
                    pScale->param_i[0] = 2; // 2=内側鱗弾
                    pScale->param_d[0] = 0.0; // 走行距離
                    pScale->param_d[1] = param.l0; // 現在の停止距離
                    pScale->param_d[2] = param.l;  // 増加量

                    pScale->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pScale->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pScale;
                    pEnemyShotSet->pEnemyShotHead->prev = pScale;

                    // --- 外側へ3発（低速）---
                    for (int j = 0; j < 3; j++) {
                        sEnemyShot* pOut = new sEnemyShot;
                        pOut->kind = img_enemyShotScale[color];
                        pOut->x = pShot->x;
                        pOut->y = pShot->y;
                        // 外側方向 + オフセット + 少し散らす
                        double outDir = ang + offset + (j - 1) * 0.12;
                        pOut->muki = outDir;
                        pOut->speed = outerSpd;
                        pOut->param_i[0] = 3; // 3=外側鱗弾

                        pOut->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pOut->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pOut;
                        pEnemyShotSet->pEnemyShotHead->prev = pOut;
                    }
                }
                pShot = pShot->next;
            }
        }

        // 内側鱗弾の停止処理 & 停止距離増加
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 2) { // 内側
                // 停止距離を毎フレーム増加
                pShot->param_d[1] += pShot->param_d[2];

                if (pShot->speed > 0.0) {
                    pShot->param_d[0] += pShot->speed; // 走行距離加算
                    if (pShot->param_d[0] >= pShot->param_d[1]) {
                        pShot->speed = 0.0; // 停止
                    }
                }
            }
            // 通常移動（大玉以外）
            if (pShot->param_i[0] != 1) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }

        phaseCnt++;
        pEnemyShotSet->param_i[2] = phaseCnt;

        // Tフレーム経過 → 再開フェーズへ
        if (phaseCnt >= param.T) {
            pEnemyShotSet->param_i[0] = 2;
            pEnemyShotSet->param_i[2] = 0;

            // 停止中の内側鱗弾を再加速
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] == 2 && pShot->speed == 0.0) {
                    pShot->speed = param.v;
                }
                pShot = pShot->next;
            }
        }
        return;
    }

    // ---- フェーズ2: 再開後の待機 → 次周期へ ----
    if (phase == 2) {
        double r = pEnemyShotSet->param_d[0];
        double& baseAng = pEnemyShotSet->param_d[1];
        baseAng += param.a;

        // 大玉回転継続
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                double ang = baseAng + pShot->param_d[0];
                pShot->x = CENTER_X + r * cos(ang);
                pShot->y = CENTER_Y + r * sin(ang);
                pShot->muki = ang;
            }
            else {
                // 鱗弾は普通に移動
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }

        phaseCnt++;
        pEnemyShotSet->param_i[2] = phaseCnt;

        // 約90フレーム待って次周期
        if (phaseCnt >= 90) {
            cycle++;
            if (cycle >= NUM_CYCLES) {
                // 全周期終了
                pEnemyShotSet->param_i[0] = 3;
                return;
            }
            // 次周期の準備：既存大玉を削除して新規生成
            // （大玉以外の鱗弾はそのまま残す）
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                sEnemyShot* next = pShot->next;
                if (pShot->param_i[0] == 1) {
                    // リストから外す（実際のdeleteはメイン側の管理に任せる想定だが、
                    // ここでは速度0・画面外相当にして自然消滅を待つ）
                    pShot->x = -1000.0;
                    pShot->y = -1000.0;
                    pShot->speed = 0.0;
                }
                pShot = next;
            }

            // 新パラメータで大玉再生成
            const BarrierParams& np = PARAMS[cycle];
            int total = np.k * 2;
            pEnemyShotSet->param_i[1] = cycle;
            pEnemyShotSet->param_i[3] = total;
            pEnemyShotSet->param_d[0] = 20.0; // 半径リセット
            pEnemyShotSet->param_d[1] = 0.0;
            pEnemyShotSet->param_d[3] = 3.5 + cycle * 0.3;

            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            for (int i = 0; i < total; i++) {
                pShot = new sEnemyShot;
                int color = (i % 2 == 0) ? COLOR_BLUE : COLOR_PURPLE;
                pShot->kind = img_enemyShotLargeBall[color];
                pShot->x = CENTER_X;
                pShot->y = CENTER_Y;
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->param_i[0] = 1;
                pShot->param_i[1] = i;
                pShot->param_i[2] = color;
                pShot->param_d[0] = (DX_PI * 2.0 * i) / total;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }

            // 展開フェーズに戻る
            pEnemyShotSet->param_i[0] = 0;
            pEnemyShotSet->param_i[2] = 0;
        }
        return;
    }

    // ---- フェーズ3: 終了後は残弾を動かすだけ ----
    if (phase == 3) {
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] != 1) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_DanmakuKekkai_Grok()
{
    if (count == 1) {
        // 画面中央やや上に配置
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;

        // 弾幕結界ショットセットを1つ生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDanmakuKekkai;
        pEnemyShotSet->x = CENTER_X;
        pEnemyShotSet->y = CENTER_Y;
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
    // 敵本体はほぼ固定（わずかに揺らす程度）
    else {
        enemy.x = 240.0 + 8.0 * sin(count * 0.03);
        enemy.y = 120.0 + 4.0 * cos(count * 0.05);
    }
}