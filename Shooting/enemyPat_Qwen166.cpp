#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：シンクロ・モーフィング・フィールド（マスゲーム風）
// ============================================================
static void Shot_MassGame(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;

    // フェーズ0: 整列（count 0 ~ 59）
    if (pSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 12x12 のグリッドで 144 発の弾を配置
        int idx = 0;
        for (int r = 0; r < 12; r++) {
            for (int c = 0; c < 12; c++) {
                pShot = new sEnemyShot;

                // グリッド上の基本位置
                double base_x = 60.0 + c * 30.0;
                double base_y = 60.0 + r * 30.0;
                pShot->x = base_x;
                pShot->y = base_y;

                // パラメータにグリッド座標と、変形先の目標座標（円形）を保存
                pShot->param_i[1] = c; // グリッドX
                pShot->param_i[2] = r; // グリッドY

                // 円形の目標座標 (中心: 240, 240, 半径: 150)
                double angle = (double)idx / 144.0 * 2.0 * DX_PI;
                pShot->param_d[0] = 240.0 + 150.0 * cos(angle);
                pShot->param_d[1] = 240.0 + 150.0 * sin(angle);

                // 初期色はグリッド位置に応じたグラデーション（0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ）
                pShot->kind = img_enemyShotSmallBall[idx % 6];
                pShot->speed = 0.0;
                pShot->muki = 0.0;

                // リストに追加
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;

                idx++;
            }
        }
    }
    // フェーズ1: ウェーブ（count 60 ~ 239）
    else if (pSet->count >= 60 && pSet->count < 240) {
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            // 横方向(c)と時間で正弦波を生成
            double wave = sin(pSet->count * 0.05 + pShot->param_i[1] * 0.4) * 40.0;

            // 基本Y座標に波を加算
            double base_y = 60.0 + pShot->param_i[2] * 30.0;
            pShot->y = base_y + wave;

            // 波の高さに応じて色を変化させる (-40.0 ~ 40.0 を 0 ~ 5 にマッピング)
            int color_idx = (int)((wave + 40.0) / 80.0 * 6.0);
            if (color_idx < 0) color_idx = 0;
            if (color_idx > 5) color_idx = 5;
            pShot->kind = img_enemyShotSmallBall[color_idx];

            pShot = pShot->next;
        }
    }
    // フェーズ2: 造形（count 240 ~ 399）
    else if (pSet->count >= 240 && pSet->count < 400) {
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            // 目標座標に向かって線形補間（イージング）
            pShot->x += (pShot->param_d[0] - pShot->x) * 0.08;
            pShot->y += (pShot->param_d[1] - pShot->y) * 0.08;

            // 形が完成するにつれて「白い中玉」に統一して輝きを表現
            pShot->kind = img_enemyShotMediumBall[6]; // 6:白

            pShot = pShot->next;
        }
    }
    // フェーズ3: 崩壊（count 400）
    else if (pSet->count == 400) {
        // 炸裂音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            // 中心(240, 240)から外向きに放射状 + 少量のランダム拡散
            pShot->muki = atan2(pShot->y - 240.0, pShot->x - 240.0) + (GetRand(40) - 20) / 100.0;
            // 速度は 3.5 〜 6.5 のランダム
            pShot->speed = 3.5 + GetRand(30) / 10.0;
            // 危険な「赤い菱形弾」に変化
            pShot->kind = img_enemyShotDiamond[0]; // 0:赤

            pShot = pShot->next;
        }
    }
    // フェーズ4: 崩壊の継続（count 401 ~ 499）
    else if (pSet->count >= 401 && pSet->count < 500) {
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
    // フェーズ5: フィナーレ（count 500）
    else if (pSet->count == 500) {
        // 中心から短レーザーを12方向に放射（花火の仕上げ）
        for (int i = 0; i < 12; i++) {
            pShot = new sEnemyShot;
            pShot->x = 240.0;
            pShot->y = 240.0;
            pShot->muki = i * DX_PI / 6.0;
            pShot->speed = 0.0; // 速度0でその場に留める（またはメイン処理で消去されるまで表示）
            pShot->kind = img_enemyShotLaser[6]; // 6:白 の短レーザー

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    // フェーズ6: フィナーレ継続（count 501 以降）
    else if (pSet->count >= 501) {
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            // レーザーは速度0なので動かず、既存の菱形弾は画面外へ飛び去る
            // ※画面外消去はメインルーチンが担当するため、ここでは位置更新のみ（速度0の弾は止まったまま）
            if (pShot->speed > 0.0) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_MassGame_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    else {
        // 敵は画面内で穏やかに左右往復
        enemy.x = 240.0 + sin(count * 0.02) * 100.0;
    }

    // count 60 でマスゲーム弾幕を開始
    if (count % 360 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = Shot_MassGame;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // ダミーヘッドノードの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}