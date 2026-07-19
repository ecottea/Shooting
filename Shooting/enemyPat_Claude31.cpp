// enemyPat_Tmp.cpp
// パターン名：蜻蛉返り（とんぼがえり）
//
// 扇状の弾列（5列×4発）を自機方向へ射出し、
// 65フレーム後に全弾が一斉反転してホーミングしながら戻ってくる。
//
// ── sEnemyShot のパラメータ使用割り当て ──────────────
//   param_d[0] : vx（速度 X 成分）
//   param_d[1] : vy（速度 Y 成分）
//   param_i[0] : フェーズ（0=前進中、1=反転後）
//
// ── sEnemyShotSet のパラメータ使用割り当て ──────────
//   param_i[0] : 反転 SE 再生済みフラグ（0=未、1=済）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ── 定数 ──────────────────────────────────────────────
static const int    COL = 7;                        // 扇の列数
static const int    ROW = 4;                        // 1列あたりの弾数
static const double SPREAD = 120.0 / 180.0 * DX_PI;   // 扇の広がり（左右 ±30°）
static const double FWD_SPEED = 3.8;                     // 前進基本速度（pixel/frame）
static const double ROW_SPEED_INC = 0.35;                    // 行ごとの速度加算（縦列の間隔を作る）
static const int    WARNING_FRAME = 55;                      // 色変化で反転を予告するフレーム数
static const int    RETURN_FRAME = 65;                      // 反転するフレーム数
static const double HOMING_ACCEL = 0.07;                    // 反転後ホーミング加速度（pixel/frame²）

// ── 弾幕パターン関数：蜻蛉返り ────────────────────────
static void ShotTonboGaeri(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ── 弾生成（count == 0 のみ）──────────────────────
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        pEnemyShotSet->param_i[0] = 0; // 反転 SE 未再生

        double baseAngle = pEnemyShotSet->muki; // 自機方向

        for (int col = 0; col < COL; col++) {
            // 扇を均等分割した各列の角度
            double colAngle = baseAngle + SPREAD * ((double)col / (COL - 1) - 0.5);

            for (int row = 0; row < ROW; row++) {
                // row が大きいほど速く、前に出ることで縦列の間隔を作る
                double spd = FWD_SPEED + row * ROW_SPEED_INC;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = colAngle;
                pEnemyShot->speed = spd;
                pEnemyShot->param_d[0] = spd * cos(colAngle); // vx
                pEnemyShot->param_d[1] = spd * sin(colAngle); // vy
                pEnemyShot->param_i[0] = 0;                   // 前進フェーズ
                pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青の小玉
                pEnemyShot->margin = 400;

                // センチネルリストの末尾に追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ── 毎フレームの移動処理 ──────────────────────────────
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShot->param_i[0] == 0) {
            // ── フェーズ 0：前進 ──────────────────────────

            pEnemyShot->x += pEnemyShot->param_d[0];
            pEnemyShot->y += pEnemyShot->param_d[1];

            // WARNING_FRAME から色を橙に変えて反転を予告
            if (pEnemyShot->count >= WARNING_FRAME) {
                pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙の小玉
            }

            // RETURN_FRAME で反転
            if (pEnemyShot->count >= RETURN_FRAME) {
                // 反転 SE は弾幕セット内で 1 回だけ鳴らす
                if (pEnemyShotSet->param_i[0] == 0) {
                    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
                    pEnemyShotSet->param_i[0] = 1;
                }

                // y 成分を反転して上向きにし、1.15 倍加速
                // x 成分は絞って軌道を収束させる（ホーミングに転移しやすくする）
                pEnemyShot->param_d[0] *= 0.4;
                pEnemyShot->param_d[1] = -fabs(pEnemyShot->param_d[1]) * 1.15;

                pEnemyShot->param_i[0] = 1;                    // 反転フェーズへ
                pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙の中玉に格上げ
            }

        }
        else {
            // ── フェーズ 1：反転後ホーミング ──────────────

            // 自機方向へ緩やかに加速
            double dx = player.x - pEnemyShot->x;
            double dy = player.y - pEnemyShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 1.0) {
                pEnemyShot->param_d[0] += (dx / dist) * HOMING_ACCEL;
                pEnemyShot->param_d[1] += (dy / dist) * HOMING_ACCEL;
            }

            pEnemyShot->x += pEnemyShot->param_d[0];
            pEnemyShot->y += pEnemyShot->param_d[1];

            if (pEnemyShot->y >= 500) pEnemyShot->y = 99999;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ── 敵本体のパターン ──────────────────────────────────
void EnemyPat_Reverse_Claude()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右に往復移動
        enemy.x += 1.2 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 30f 後から 100f ごとに弾幕を 1 波射出
    if (count % 70 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTonboGaeri;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        // センチネルノードの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // メインの弾幕セットリストの末尾に追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}