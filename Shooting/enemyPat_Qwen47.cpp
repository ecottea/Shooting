// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：干渉する妖蛇リボン (Wavy Ribbon)
// ============================================================
static void ShotWavyRibbon(sEnemyShotSet* pEnemyShotSet)
{
    // 1. レーザーの長さを制限する（countが120を超えるまで弾を生成）
    if (pEnemyShotSet->count < 90) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        double base_muki = pEnemyShotSet->muki;
        double phase = pEnemyShotSet->param_d[0];
        double amp = pEnemyShotSet->param_d[1];
        double omega = pEnemyShotSet->param_d[2];

        // うねりの角度を計算して初期向きとする
        pEnemyShot->muki = base_muki + amp * sin(phase);
        pEnemyShot->speed = 4.5; // 画面外へ早く去らせる速度

        // 弾側にパラメータをコピー
        pEnemyShot->param_d[0] = phase;
        pEnemyShot->param_d[1] = amp;
        pEnemyShot->param_d[2] = omega;
        pEnemyShot->param_d[3] = base_muki;

        // リストに追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 効果音（リボンごとに発射タイミングをずらす）
        if (pEnemyShotSet->count % 24 == pEnemyShotSet->param_i[0]) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // 2. 既存の弾の座標更新と色更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;

        // サインカーブに従って進行方向(muki)をうねらせる
        pShot->muki = pShot->param_d[3] + pShot->param_d[1] * sin(pShot->param_d[2] * t + pShot->param_d[0]);

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 弾の色を時間経過で変化させ、虹色リボンにする
        int color_table[] = { 0, 1, 2, 3, 4, 5, 6, 8 };
        int color_idx = color_table[(int)(t / 2) % 8];
        pShot->kind = img_enemyShotMediumBall[color_idx];

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_HenyoriLaser_Qwen()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵の移動：画面中央を軸に大きく左右揺れ動き、斜め方向へ移動する
        enemy.x = 240.0 + 100.0 * sin((count - 1) * 0.012);
        enemy.y = 60.0 + 30.0 * sin((count - 1) * 0.018);

        // 3. 一定周期（150フレームごと）で新しいへにょりレーザーを生成
        if ((count - 1) % 60 == 0) {
            // ★自機への角度を計算
            double base_muki = atan2(player.y - enemy.y, player.x - enemy.x);

            // 3本同時に生成
            for (int i = 0; i < 3; i++) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotWavyRibbon;
                pSet->x = enemy.x;
                pSet->y = enemy.y + 10.0;

                // ★発射角度は自機狙いベースで、3本で扇状に広がるようにずらす
                // (i-1)*0.15 ラジアン ≒ 約8.6度ずつずらす
                pSet->muki = base_muki + (i - 1) * 0.35;

                // ★パラメータをランダム化（撃つ度に変化する）
                // param_d[0]: 位相 (0 ～ 2π)
                pSet->param_d[0] = GetRand(628) / 100.0;
                // param_d[1]: 振幅 (0.3 ～ 0.8)
                pSet->param_d[1] = 0.3 + GetRand(50) / 100.0;
                // param_d[2]: 周波数 (0.06 ～ 0.15)
                pSet->param_d[2] = 0.06 + GetRand(9) / 100.0;

                // 効果音の発射タイミングオフセット
                pSet->param_i[0] = i * 8;

                // 弾管理用ダミーヘッドの初期化
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                // グローバルリストに追加
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }

        // 4. 寿命が来た pEnemyShotSet を削除する
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            sEnemyShotSet* pNext = pSet->next;
            if (pSet->count > 240) {
                pSet->prev->next = pSet->next;
                pSet->next->prev = pSet->prev;
                delete pSet;
            }
            pSet = pNext;
        }
    }
}