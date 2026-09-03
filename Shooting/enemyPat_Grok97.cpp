// enemyPat_PerfectFreeze.cpp
// 凍符「パーフェクトフリーズ」風弾幕（東方紅魔郷 チルノ）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン関数
// サイクル:
//   0〜179F  : カラフル小玉を放射状にばら撒き
//   180F     : 全弾フリーズ（速度0・白化）＋チャージ音
//   181〜239F: 自機狙い青弾を複数wayで発射
//   240F     : 凍結弾をランダム方向に再加速
//   241F〜   : 弾が飛び続ける（画面外でメイン側が消去）
// ============================================================
static void ShotPerfectFreeze(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int PHASE_SCATTER_END = 180;
    const int PHASE_AIMED_END = 240;
    const int PHASE_UNFREEZE = 240;

    // ---- フェーズ1: ばら撒き（毎フレーム1〜2発） ----
    if (pEnemyShotSet->count < PHASE_SCATTER_END) {
        // 発射音（軽め）
        if (pEnemyShotSet->count % 8 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        int fireNum = 1 + (GetRand(1)); // 1 or 2
        for (int i = 0; i < fireNum; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 全方向ランダム（0〜359度）
            pEnemyShot->muki = GetRand(359) / 180.0 * DX_PI;
            // 速度 2.0〜4.5 程度
            pEnemyShot->speed = 2.0 + GetRand(25) / 10.0;
            // 色: 0〜5（赤〜マゼンタ）のカラフル小玉
            int col = GetRand(5);
            pEnemyShot->kind = img_enemyShotSmallBall[col];
            // param_i[0] = 0: 通常移動中
            pEnemyShot->param_i[0] = 0;
            pEnemyShot->count = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- フリーズ瞬間（180F） ----
    if (pEnemyShotSet->count == PHASE_SCATTER_END) {
        // チャージ音でフリーズ演出
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 速度0にして凍結
            pShot->speed = 0.0;
            // 白に変色
            pShot->kind = img_enemyShotSmallBall[6]; // 白
            // param_i[0] = 1: 凍結中
            pShot->param_i[0] = 1;
            pShot = pShot->next;
        }
    }

    // ---- フェーズ2: 自機狙い青弾（181〜239F） ----
    if (pEnemyShotSet->count > PHASE_SCATTER_END &&
        pEnemyShotSet->count < PHASE_AIMED_END) {
        // 数フレームおきに発射
        if ((pEnemyShotSet->count - PHASE_SCATTER_END) % 12 == 1) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // 自機への基準角度
            double baseMuki = atan2(player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

            // 奇数way（5-way）: 中央が自機狙い
            // 偶数wayにしたい場合は way=6, step を調整
            const int way = 5;
            const double step = 12.0 / 180.0 * DX_PI; // 約12度間隔

            for (int i = 0; i < way; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // 中央(i=2)が自機方向、左右対称
                pEnemyShot->muki = baseMuki + (i - (way / 2)) * step;
                pEnemyShot->speed = 3.5;
                // 青の小玉
                pEnemyShot->kind = img_enemyShotSmallBall[4];
                // param_i[0] = 2: 自機狙い弾（フリーズ対象外）
                pEnemyShot->param_i[0] = 2;
                pEnemyShot->count = 0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ---- 解凍瞬間（240F）: 凍結弾をランダム方向へ再加速 ----
    if (pEnemyShotSet->count == PHASE_UNFREEZE) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) { // 凍結中の弾のみ
                // ランダム方向
                pShot->muki = GetRand(359) / 180.0 * DX_PI;
                // やや遅めの速度で再開
                pShot->speed = 1.8 + GetRand(20) / 10.0;
                // 色はそのまま白のままでもよいが、少し青みを帯びさせても可
                // ここでは白のまま
                pShot->param_i[0] = 0; // 通常移動に戻す
            }
            pShot = pShot->next;
        }
    }

    // ---- 全弾の移動処理（毎フレーム） ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // speed が 0 の凍結弾はそのまま、それ以外は移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_PerfectFreeze_Grok()
{
    static int mukiDir;       // 移動方向（±1）
    static int cycleCount;    // 次のサイクル開始までのカウンタ

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        mukiDir = 1;
        cycleCount = 0;
    }
    else {
        // 緩やかに左右移動
        enemy.x += 0.7 * (double)mukiDir;
        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            mukiDir = 1;
        }
        else if (enemy.x > 400.0) {
            enemy.x = 400.0;
            mukiDir = -1;
        }
        // 定期的に方向転換の揺らぎ
        if (count % 150 == 75) {
            if (GetRand(1) == 0) mukiDir *= -1;
        }
    }

    // 一定間隔で新しい「パーフェクトフリーズ」サイクルを開始
    // 1サイクル約 300F 程度を想定し、少し余裕を持たせて発行
    if (cycleCount <= 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPerfectFreeze;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        // パラメータ初期化
        for (int i = 0; i < 16; i++) {
            pEnemyShotSet->param_i[i] = 0;
            pEnemyShotSet->param_d[i] = 0.0;
        }

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 次のサイクルまで待つ（ばら撒き180 + 狙い60 + 余韻 ≈ 320F）
        cycleCount = 320;
    }
    else {
        cycleCount--;
    }
}