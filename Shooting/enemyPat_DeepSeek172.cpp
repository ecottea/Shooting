// 弾幕：ヘリンボーン・ステッチ
// 小弾3個で1ステッチ（±45°の短い斜線）を作り、
// 行ごとに方向と位置を交互に変えて織物模様を流す。
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotHerringbone(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium,
        //                  sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // param_i[0] : 行インデックス
        // param_i[1] : 1 なら中弾（結び目）を配置
        int row = pEnemyShotSet->param_i[0];
        int putKnot = pEnemyShotSet->param_i[1];

        const double P = 36.0;             // 横ピッチ
        const double L = 18.0;             // ステッチ長
        const double ux = 0.70710678;       // cos45° = sin45°
        const double stepLen = L * 0.5*0.8;     // 中心〜端点まで 9px
        const double baseY = pEnemyShotSet->y;

        // 偶数行：13列（x=24..456）
        // 奇数行：12列（x=42..438）※左右端からはみ出さないように
        int colStart = -6;
        int colEnd = (row % 2 == 0) ? 6 : 5;
        double baseX = (row % 2 == 0) ? 24.0 : 42.0;

        for (int c = colStart; c <= colEnd; c++) {
            double cx = baseX + (c - colStart) * P;
            double cy = baseY;

            // (row + c) mod 2 で +45° / -45° を交互に
            int dir = ((row + c) % 2 == 0) ? 1 : -1;
            double uy = dir * ux;

            // ─── 小弾3個で1ステッチ ───
            // k = -1, 0, +1 が中心から ±9px の斜線上の3点
            for (int k = -1; k <= 1; k++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx + k * stepLen * ux;
                pEnemyShot->y = cy + k * stepLen * uy;
                pEnemyShot->muki = DX_PI / 2.0; // 真下
                pEnemyShot->speed = 45.0/60;        // 45px/s = 0.75px/フレーム
                // 小弾・白（色: 0:赤,1:黄,2:緑,3:シアン,4:青,5:マゼンタ,6:白,7:黒,8:橙）
                pEnemyShot->kind = img_enemyShotSmallBall[6];
                pEnemyShot->margin = 20.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // ─── 結び目：偶数行のステッチ中央に中弾 ───
            if (putKnot) {
                sEnemyShot* pKnot = new sEnemyShot;
                pKnot->x = cx;
                pKnot->y = cy;
                pKnot->muki = DX_PI / 2.0;
                pKnot->speed = 45.0/60;
                // 中弾・赤（結び目が映えるよう白ステッチと対比）
                pKnot->kind = img_enemyShotMediumBall[0];
                pKnot->margin = 20.0;

                pKnot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pKnot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pKnot;
                pEnemyShotSet->pEnemyShotHead->prev = pKnot;
            }
        }
    }

    // 既存弾の更新：全弾を下方向へ等速移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ─────────────────────────────────────────────
// 敵本体のパターン
// ─────────────────────────────────────────────
void EnemyPat_Herringbone_DeepSeek()
{
    static int muki;
    static int row_counter;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        row_counter = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 0.4秒（=24フレーム）ごとに 1 行発射
    // → 速度45px/s と組み合わせると行間隔が Q=18px になり織り目が噛み合う
    if (count % 24 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHerringbone;
        pEnemyShotSet->x = 240.0;             // 実際は未使用（絶対座標で配置）
        pEnemyShotSet->y = enemy.y + 20.0;    // 敵の直下から
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = row_counter;
        // param_i[0]: 行番号（方向と列数を決定）
        // param_i[1]: 偶数行のみ中弾を配置（2行に1回程度）
        pEnemyShotSet->param_i[0] = row_counter;
        pEnemyShotSet->param_i[1] = (row_counter % 2 == 0) ? 1 : 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        row_counter++;
    }
}