// ============================================================
//  敵パターン：バーンズリーのシダ (Burnsley Fern)
//  既存の弾素材（小玉/中玉/大玉）だけでシダの自己相似構造を描く。
//  生成 → 静止 → 崩壊（胞子の放出）の3フェーズ。
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：バーンズリーのシダ
//   param_d[0] : 現在のシダ座標 fx
//   param_d[1] : 現在のシダ座標 fy
//   param_d[2] : スケール（呼び出し側で設定）
//   param_i[0] : 生成済み点数
//   param_i[1] : フェーズ (0=生成, 1=静止, 2=崩壊)
//   param_i[2] : 左右反転フラグ (0=通常, 1=反転)
static void ShotBurnsleyFern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    const int PHASE_GEN_END = 120-10; // 生成フェーズ終了 (約2秒)
    const int PHASE_HOLD_END = 300-20; // 静止フェーズ終了 (約5秒)

    // ---- 初期化 ----
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        pEnemyShotSet->param_d[0] = 0.0; // fx
        pEnemyShotSet->param_d[1] = 0.0; // fy
        pEnemyShotSet->param_i[0] = 0;   // 生成点数
        pEnemyShotSet->param_i[1] = 0;   // phase
        // param_i[2] (mirror), param_d[2] (scale) は呼び出し側で設定済み
    }

    // ---- フェーズ遷移 ----
    if (pEnemyShotSet->param_i[1] == 0 && pEnemyShotSet->count >= PHASE_GEN_END) {
        pEnemyShotSet->param_i[1] = 1;
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (pEnemyShotSet->param_i[1] == 1 && pEnemyShotSet->count >= PHASE_HOLD_END) {
        pEnemyShotSet->param_i[1] = 2;
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // ---- 生成フェーズ：IFSで自己相似点を量産して弾を配置 ----
    if (pEnemyShotSet->param_i[1] == 0) {
        const int    mirror = pEnemyShotSet->param_i[2];
        const double scale = pEnemyShotSet->param_d[2];
        const double bossX = pEnemyShotSet->x;
        const double bossY = pEnemyShotSet->y;

        // 1フレームあたり3点
        for (int k = 0; k < 3; k++) {
            double fx = pEnemyShotSet->param_d[0];
            double fy = pEnemyShotSet->param_d[1];

            // バーンズリーのシダ IFS
            //   1% : 根元, 85% : 主軸, 7% : 左葉, 7% : 右葉
            int r = GetRand(99);
            double nx, ny;
            int tag; // 0=根元, 1=主軸, 2=左葉, 3=右葉
            if (r < 1) {
                nx = 0.0;
                ny = 0.16 * fy;
                tag = 0;
            }
            else if (r < 86) {
                nx = 0.85 * fx + 0.04 * fy;
                ny = -0.04 * fx + 0.85 * fy + 1.6;
                tag = 1;
            }
            else if (r < 93) {
                nx = 0.2 * fx - 0.26 * fy;
                ny = 0.23 * fx + 0.22 * fy + 1.6;
                tag = 2;
            }
            else {
                nx = -0.15 * fx + 0.28 * fy;
                ny = 0.26 * fx + 0.24 * fy + 0.44;
                tag = 3;
            }
            pEnemyShotSet->param_d[0] = nx;
            pEnemyShotSet->param_d[1] = ny;

            // 最初の30点はアトラクタ収束用のウォームアップ（弾は出さない）
            if (pEnemyShotSet->param_i[0] < 30) {
                pEnemyShotSet->param_i[0]++;
                continue;
            }

            // 画面座標へ変換（ボスを根元として下方向に垂れ下がる）
            double cx = bossX + (nx - 0.25) * scale * (mirror ? -1.0 : 1.0);
            double cy = bossY + 10.0 + ny * scale;

            // 画面外はスキップ（画面外消去はメインルーチンが行うが、
            // 生成時点で見えない点は最初から作らない）
            if (cx < 20 || cx > 460 || cy < 20 || cy > 460) {
                pEnemyShotSet->param_i[0]++;
                continue;
            }

            // タグに応じて弾を配置
            if (tag == 0) {
                // 胞子・根元：大玉1個
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx;
                pEnemyShot->y = cy;
                pEnemyShot->speed = 0.0;
                pEnemyShot->muki = 0.0;
                pEnemyShot->kind = img_enemyShotLargeBall[5]; // マゼンタ
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
            else if (tag == 1) {
                // 主軸・側枝の節：中玉1個
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx;
                pEnemyShot->y = cy;
                pEnemyShot->speed = 0.0;
                pEnemyShot->muki = 0.0;
                pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
            else {
                // 葉：小玉3個を微小なV字に配置して葉片っぽい塊にする
                double outward = atan2(cy - bossY, cx - bossX);
                for (int j = 0; j < 3; j++) {
                    double ang = outward + (j - 1) * 0.5;
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = cx + 2.5 * cos(ang);
                    pEnemyShot->y = cy + 2.5 * sin(ang);
                    pEnemyShot->speed = 0.0;
                    pEnemyShot->muki = ang;
                    pEnemyShot->kind = img_enemyShotSmallBall[(tag == 2) ? 2 : 3]; // 緑/シアン
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
            pEnemyShotSet->param_i[0]++;
        }
    }

    // ---- 静止フェーズ：何もしない（全弾speed=0で停止） ----

    // ---- 崩壊フェーズ：胞子が弾け飛ぶように外側＋下方向へ加速 ----
    if (pEnemyShotSet->param_i[1] == 2) {
        const double bossX = pEnemyShotSet->x;
        const double bossY = pEnemyShotSet->y;
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 初回だけ発射方向・初速を決める
            if (pShot->param_i[0] == 0) {
                pShot->param_i[0] = 1;
                double dx = pShot->x - bossX;
                double dy = pShot->y - bossY;
                // 下向きにバイアスをかけた外向き角度
                pShot->muki = atan2(dy + 60.0, dx) + (GetRand(4)-2)*0.8;
                pShot->speed = 0.5;
            }
            pShot->speed += 0.025;
            if (pShot->speed > 4.0) pShot->speed = 4.0;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// 弾セットを1つ生成するヘルパー
static void SpawnFernShotSet(double x, double y, double scale, int mirror)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotBurnsleyFern;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = 0;
    pEnemyShotSet->param_i[2] = mirror;
    pEnemyShotSet->param_d[2] = scale;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_BarnsleyFern_DeepSeek()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    // メインのシダ（大・反転なし）
    if (count % 500 == 30) {
        SpawnFernShotSet(enemy.x, enemy.y, 32.0, 0);
    }

    // 自己相似の反転シダ（小・左右反転）
    if (count % 500 == 200) {
        SpawnFernShotSet(enemy.x, enemy.y, 22.0, 1);
    }
}