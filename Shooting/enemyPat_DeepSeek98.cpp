// enemyPat_tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾をショットセットのリストへ挿入するヘルパー関数
static void InsertBullet(sEnemyShotSet* pSet, sEnemyShot* pShot)
{
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 弾幕パターン関数（ショットセット用）
static void PatternFunc_Tmp(sEnemyShotSet* pSet)
{
    // 画面中央（固定）
    const double centerX = 240.0;
    const double centerY = 240.0;
    const double maxRadius = 240.0;          // 半径の上限（画面幅の半分）
    const double expandRate = 4.0;           // 半径拡大速度（ピクセル/フレーム）

    // 周期ごとのパラメータセット（順番に切り替える）
    struct ParamSet {
        int    k;      // 青・紫の大玉の個数（各色）
        double a;      // 回転角速度（ラジアン/フレーム）
        double v;      // 内向き鱗弾の速さ
        int    t1;     // 連続発射フレーム数
        int    t2;     // 休止フレーム数
        double th0;    // 内向き角度オフセットの最大値（ラジアン）
        double l0;     // 最初の停止距離
        double l;      // 停止距離の増分
        int    T;      // 発射フェーズの総フレーム数
    };
    static const ParamSet params[] = {
        { 3, 0.020, 2.0, 10, 5, 0.30, 50.0, 2.0, 60 },
        { 4, 0.030, 2.5,  8, 4, 0.40, 40.0, 3.0, 50 },
        { 2, 0.015, 1.8, 12, 6, 0.20, 60.0, 1.5, 70 }
    };
    const int numParams = sizeof(params) / sizeof(params[0]);

    // ショットセットのパラメータを参照で扱う
    int& cycleIndex = pSet->param_i[0];  // 現在のパラメータセット番号
    int& cycleFrame = pSet->param_i[1];  // 周期内フレームカウンタ
    int& inwardCount = pSet->param_i[2];  // 内向き鱗弾の発射数（停止距離計算用）
    int& expansionFrames = pSet->param_i[3];  // 半径拡大に要するフレーム数（固定）
    int& shootStartFrame = pSet->param_i[4];  // 発射開始フレーム（拡大終了後）
    int& T_cur = pSet->param_i[5];  // 現在のTの値

    // 周期開始時の処理
    if (cycleFrame == 0) {
        // 前の周期の大玉を削除（param_i[0]==0 の弾を大玉とみなす）
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            sEnemyShot* next = pShot->next;
            if (pShot->param_i[0] == 0) {   // 大玉
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
            pShot = next;
        }

        // 現在のパラメータを取得
        int idx = cycleIndex % numParams;
        int k = params[idx].k;
        double a = params[idx].a;
        double v = params[idx].v;
        int t1 = params[idx].t1;
        int t2 = params[idx].t2;
        double th0 = params[idx].th0;
        double l0 = params[idx].l0;
        double l = params[idx].l;
        T_cur = params[idx].T;

        // 拡大フェーズの長さ（固定：60フレームで半径240に到達）
        expansionFrames = (int)(maxRadius / expandRate);
        shootStartFrame = expansionFrames;   // 拡大終了と同時に発射開始

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // k個の青大玉と k個の紫大玉を生成
        for (int i = 0; i < k; i++) {
            // 青大玉
            sEnemyShot* pBlue = new sEnemyShot;
            pBlue->x = centerX;
            pBlue->y = centerY;
            pBlue->muki = 0.0;
            pBlue->speed = 0.0;
            pBlue->kind = img_enemyShotLargeBall[4];   // 青
            pBlue->param_i[0] = 0;                     // 大玉マーカー
            pBlue->param_d[0] = i * (2.0 * DX_PI / k); // 初期角度（均等配置）
            pBlue->param_d[1] = 0.0;                   // 初期半径
            InsertBullet(pSet, pBlue);

            // 紫大玉（マゼンタで代用）
            sEnemyShot* pMagenta = new sEnemyShot;
            pMagenta->x = centerX;
            pMagenta->y = centerY;
            pMagenta->muki = 0.0;
            pMagenta->speed = 0.0;
            pMagenta->kind = img_enemyShotLargeBall[5]; // マゼンタ
            pMagenta->param_i[0] = 0;
            pMagenta->param_d[0] = i * (2.0 * DX_PI / k) + DX_PI / k; // 青と交互
            pMagenta->param_d[1] = 0.0;
            InsertBullet(pSet, pMagenta);
        }

        inwardCount = 0;   // 内向き発射数をリセット
    }

    // 現在のパラメータを再取得（後で使用）
    int idx = cycleIndex % numParams;
    double a = params[idx].a;
    double v = params[idx].v;
    int t1 = params[idx].t1;
    int t2 = params[idx].t2;
    double th0 = params[idx].th0;
    double l0 = params[idx].l0;
    double l = params[idx].l;

    // 現在の発射フェーズかどうか
    bool isShootingPhase = (cycleFrame >= shootStartFrame && cycleFrame < shootStartFrame + T_cur);
    // 発射リズム内で実際に撃つフレームかどうか（各フレームで判定）
    bool isShootingFrame = false;
    if (isShootingPhase) {
        int localFrame = cycleFrame - shootStartFrame;  // 発射フェーズ内での経過フレーム
        int rhythmPos = localFrame % (t1 + t2);
        isShootingFrame = (rhythmPos < t1) && (pSet->count % 2 == 0);
    }

    // 全弾の更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        if (pShot->param_i[0] == 0) {
            // ---------- 大玉の処理 ----------
            // 半径拡大
            if (pShot->param_d[1] < maxRadius) {
                pShot->param_d[1] += expandRate;
                if (pShot->param_d[1] > maxRadius) pShot->param_d[1] = maxRadius;
            }
            // 回転
            pShot->param_d[0] += a;
            // 座標更新
            pShot->x = centerX + pShot->param_d[1] * cos(pShot->param_d[0]);
            pShot->y = centerY + pShot->param_d[1] * sin(pShot->param_d[0]);

            // 発射
            if (isShootingFrame) {
                if (pSet->count % 6 == 0) {
                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }

                // --- 内向き鱗弾 ---
                double angleToCenter = atan2(centerY - pShot->y, centerX - pShot->x);
                // オフセット角度：-th0 から +th0 まで線形変化
                double offset;
                if (T_cur <= 1) {
                    offset = 0.0;  // T=1ならオフセットなし
                }
                else {
                    int localFrame = cycleFrame - shootStartFrame;
                    offset = -th0 + (2.0 * th0) * localFrame / (T_cur - 1);
                }
                double inwardAngle = angleToCenter + offset;

                sEnemyShot* pIn = new sEnemyShot;
                pIn->x = pShot->x;
                pIn->y = pShot->y;
                pIn->muki = inwardAngle;
                pIn->speed = v;
                // 鱗弾の色は親玉と同色
                if (pShot->kind == img_enemyShotLargeBall[4]) {
                    pIn->kind = img_enemyShotScale[4];   // 青
                }
                else {
                    pIn->kind = img_enemyShotScale[5];   // マゼンタ
                }
                pIn->param_i[0] = 1;                // 内向き鱗弾マーカー
                pIn->param_i[1] = 0;                // 0:移動中, 1:停止中
                pIn->param_d[0] = 0.0;              // 移動距離
                pIn->param_d[1] = l0 + l * inwardCount; // 停止距離
                InsertBullet(pSet, pIn);
                inwardCount++;

                // --- 外向き低速鱗弾（3発）---
                double angleAway = angleToCenter + DX_PI; // 中心から離れる方向
                const double spread = 0.2;               // 広がり角
                const double lowSpeed = 1.0;              // 低速
                for (int s = -1; s <= 1; ++s) {
                    double outAngle = angleAway + s * spread;
                    sEnemyShot* pOut = new sEnemyShot;
                    pOut->x = pShot->x;
                    pOut->y = pShot->y;
                    pOut->muki = outAngle;
                    pOut->speed = lowSpeed;
                    if (pShot->kind == img_enemyShotLargeBall[4]) {
                        pOut->kind = img_enemyShotScale[4];
                    }
                    else {
                        pOut->kind = img_enemyShotScale[5];
                    }
                    pOut->param_i[0] = 2;   // 外向き鱗弾マーカー
                    InsertBullet(pSet, pOut);
                }
            }
        }
        else if (pShot->param_i[0] == 1) {
            // ---------- 内向き鱗弾の処理 ----------
            if (pShot->param_i[1] == 0) {
                // 移動中
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                pShot->param_d[0] += pShot->speed; // 累積距離
                if (pShot->param_d[0] >= pShot->param_d[1]) {
                    pShot->speed = 0.0;            // 停止
                    pShot->param_i[1] = 1;         // 停止状態へ
                }
            }
            // 停止中は何もしない
        }
        else if (pShot->param_i[0] == 2) {
            // ---------- 外向き鱗弾の処理 ----------
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = next;
    }

    // 発射フェーズ終了時に停止中の内向き鱗弾を再移動させる
    if (cycleFrame == shootStartFrame + T_cur) {
        sEnemyShot* pResume = pSet->pEnemyShotHead->next;
        while (pResume != pSet->pEnemyShotHead) {
            if (pResume->param_i[0] == 1 && pResume->param_i[1] == 1) {
                pResume->speed = v;
                pResume->param_i[0] = 2;   // 移動再開
            }
            pResume = pResume->next;
        }
    }

    // 周期の終了判定と次周期への移行
    if (cycleFrame >= shootStartFrame + T_cur + 1) {
        cycleIndex++;       // パラメータセットを切り替え
        cycleFrame = 0;     // フレームカウンタをリセット
    }
    else {
        cycleFrame++;       // フレームを進める
    }
}

// 敵本体のパターン関数（メインから呼ばれる）
void EnemyPat_DanmakuKekkai_DeepSeek()
{
    // 初期化（最初のフレームのみ）
    if (count == 1) {
        // 敵の位置は中央に固定
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;

        // ショットセットを1つだけ生成（この中で全弾を管理）
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = PatternFunc_Tmp;
        pSet->x = 240.0;
        pSet->y = 240.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // 弾リストの初期化
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // ショットセットをグローバルリストに登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        // ショットセット内部の周期管理用パラメータを初期化
        pSet->param_i[0] = 0;   // cycleIndex
        pSet->param_i[1] = 0;   // cycleFrame
        pSet->param_i[2] = 0;   // inwardCount
        pSet->param_i[3] = 60;  // expansionFrames（仮、PatternFunc内で再設定）
        pSet->param_i[4] = 60;  // shootStartFrame（仮）
        pSet->param_i[5] = 0;   // T_cur
    }
    // 以降はショットセットのPatternFuncが毎フレーム自動的に呼ばれる
}