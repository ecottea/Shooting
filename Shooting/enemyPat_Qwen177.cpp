#include "gv.h"
#include <cmath>

// ============================================================
//  定数定義
// ============================================================
#define PYTHAGORAS_UNIT 30.0
#define PHASE_PREP        0
#define PHASE_SPAWN       30
#define PHASE_MOVE_START  120
#define PHASE_EXPLODE     240
#define PHASE_END         300

// グループID
#define GROUP_FRAME 99
#define GROUP_A2    1
#define GROUP_B2    2

// ============================================================
//  ヘルパー関数
// ============================================================
static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, int group)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;

    // パラメータ初期化
    pShot->param_i[0] = group;
    pShot->param_i[1] = 0;
    for (int i = 0; i < 16; ++i) pShot->param_d[i] = 0.0;

    // リストへ追加
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 枠(正方形)を描画するヘルパー
static void DrawSquareFrame(sEnemyShotSet* p, double cx, double cy, double size)
{
    double half = size / 2.0;
    int n = (int)(size / PYTHAGORAS_UNIT);

    // 4辺を中玉(白)で点線描画
    for (int i = 0; i <= n; i++) {
        double offset = -half + i * PYTHAGORAS_UNIT;

        // 上下辺
        AddShot(p, cx + offset, cy - half, 0, 0, img_enemyShotMediumBall[6], GROUP_FRAME);
        AddShot(p, cx + offset, cy + half, 0, 0, img_enemyShotMediumBall[6], GROUP_FRAME);

        // 左右辺 (角の重複を避ける)
        if (i > 0 && i < n) {
            AddShot(p, cx - half, cy + offset, 0, 0, img_enemyShotMediumBall[6], GROUP_FRAME);
            AddShot(p, cx + half, cy + offset, 0, 0, img_enemyShotMediumBall[6], GROUP_FRAME);
        }
    }
}

// ============================================================
//  弾幕パターン関数：ピタゴラスの証明
// ============================================================
static void ShotPythagoras(sEnemyShotSet* p)
{
    // 配置座標の定義 (画面 480x480)
    const double CX_A = 120.0, CY_A = 480-200.0, SIZE_A = 3.0 * PYTHAGORAS_UNIT; // 90
    const double CX_B = 360.0, CY_B = 480-200.0, SIZE_B = 4.0 * PYTHAGORAS_UNIT; // 120
    const double CX_C = 240.0, CY_C = 480-380.0, SIZE_C = 5.0 * PYTHAGORAS_UNIT; // 150

    // --- フェーズ進行 ---
    if (p->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 1. 枠の描画
        DrawSquareFrame(p, CX_A, CY_A, SIZE_A);
        DrawSquareFrame(p, CX_B, CY_B, SIZE_B);
        DrawSquareFrame(p, CX_C, CY_C, SIZE_C);
    }
    else if (p->count == PHASE_SPAWN) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 2. a^2 (赤 9発) の生成
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                double x = CX_A - PYTHAGORAS_UNIT + i * PYTHAGORAS_UNIT;
                double y = CY_A - PYTHAGORAS_UNIT + j * PYTHAGORAS_UNIT;
                AddShot(p, x, y, 0, 0, img_enemyShotSmallBall[0], GROUP_A2);
            }
        }

        // 3. b^2 (青 16発) の生成
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                double x = CX_B - 1.5 * PYTHAGORAS_UNIT + i * PYTHAGORAS_UNIT;
                double y = CY_B - 1.5 * PYTHAGORAS_UNIT + j * PYTHAGORAS_UNIT;
                AddShot(p, x, y, 0, 0, img_enemyShotSmallBall[4], GROUP_B2);
            }
        }
    }
    else if (p->count == PHASE_MOVE_START) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 4. c^2 のグリッド座標リストを作成 (25箇所)
        double target_coords[25][2];
        int idx = 0;
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                target_coords[idx][0] = CX_C - 2.0 * PYTHAGORAS_UNIT + i * PYTHAGORAS_UNIT;
                target_coords[idx][1] = CY_C - 2.0 * PYTHAGORAS_UNIT + j * PYTHAGORAS_UNIT;
                idx++;
            }
        }

        // 5. 赤と青の弾を走査し、目標座標を割り当て＆色変更
        sEnemyShot* pShot = p->pEnemyShotHead->next;
        int t_idx = 0;
        while (pShot != p->pEnemyShotHead) {
            if (pShot->param_i[0] == GROUP_A2 || pShot->param_i[0] == GROUP_B2) {
                // 目標座標セット
                pShot->param_d[0] = target_coords[t_idx][0];
                pShot->param_d[1] = target_coords[t_idx][1];
                pShot->param_d[2] = 1.0; // 移動フラグ

                // 色をマゼンタ(5)に変更 (証明完了の統合)
                // 小玉から中玉への変更はサイズが変わるため、ここでは小玉マゼンタを使用
                // もし大玉を使いたい場合は img_enemyShotLargeBall[5] などにする
                pShot->kind = img_enemyShotSmallBall[5];

                t_idx++;
            }
            pShot = pShot->next;
        }
    }
    else if (p->count > PHASE_MOVE_START && p->count < PHASE_EXPLODE) {
        // 6. 移動処理 (線形補間)
        sEnemyShot* pShot = p->pEnemyShotHead->next;
        while (pShot != p->pEnemyShotHead) {
            if (pShot->param_d[2] == 1.0) {
                pShot->x += (pShot->param_d[0] - pShot->x) * 0.06;
                pShot->y += (pShot->param_d[1] - pShot->y) * 0.06;

                // 回転演出
                pShot->muki += 0.1;
            }
            pShot = pShot->next;
        }
    }
    else if (p->count == PHASE_EXPLODE) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 7. 炸裂 (Q.E.D.)
        sEnemyShot* pShot = p->pEnemyShotHead->next;
        while (pShot != p->pEnemyShotHead) {
            if (pShot->param_i[0] == GROUP_A2 || pShot->param_i[0] == GROUP_B2) {
                // 移動完了、発射準備
                pShot->param_d[2] = 0.0;

                // プレイヤー狙い + ランダム偏差
                double angle = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->muki = angle + (GetRand(100) - 50) / 200.0;
                pShot->speed = 3.5 + GetRand(20) / 10.0;
            }
            else if (pShot->param_i[0] == GROUP_FRAME) {
                // 枠は画面外へ消す (下へ落とす)
                pShot->speed = 2.0;
                pShot->muki = DX_PI / 2.0;
            }
            pShot = pShot->next;
        }
    }
    else if (p->count == PHASE_END) {
        // セット終了
        p->alive = 0;
    }

    {
        sEnemyShot* pShot = p->pEnemyShotHead->next;
        while (pShot != p->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_PythagoreanTheorem_Qwen()
{
    if (count == 1) {
        // 敵は中央上部に配置し、証明中は動かさない
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 長めの戦闘を想定
    }

    // 600フレーム (約10秒) ごとに証明弾幕を実行
    if (count % 300 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPythagoras;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->alive = 600;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}