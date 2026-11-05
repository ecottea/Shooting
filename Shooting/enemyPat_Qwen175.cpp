#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：七色のシンクロ・ペンデュラム
// ============================================================
static void ShotPendulumWave(sEnemyShotSet* pEnemyShotSet)
{
    // 収束・解放フェーズの判定 (840フレーム周期で全振り子が一点に揃う)
    int cycle = count % 840;

    // 収束フェーズ(800~839フレーム目)では、強制的に角度を0(真下)に近づける
    double convergence_factor = 1.0;
    if (cycle >= 800) {
        convergence_factor = 1.0 - ((cycle - 800) / 40.0); // 1.0 -> 0.0 へ線形補間
    }

    // 収束予告音 (収束が完了する約1秒前 = 60フレーム前に鳴らす)
    if (cycle == 780) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // param_i[15] == 0 は発射済みの弾（移動のみ処理）
        if (pShot->param_i[15] == 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        // param_i[15] == 1 はペンデュラム管理下のパーツ（おもり or 糸）
        else {
            double j = pShot->param_d[4];       // 振り子のインデックス (0~6)
            double pivot_x = pShot->param_d[0]; // 支点X
            double pivot_y = pShot->param_d[1]; // 支点Y
            double L = pShot->param_d[2];       // 糸の長さ
            double w = pShot->param_d[3];       // 角速度
            double ratio = pShot->param_d[5];   // 支点からの距離比率 (0.33, 0.66, 0.99, 1.0)

            // 単振動の角度計算 (振幅60度 = DX_PI/3.0)
            double theta = (DX_PI / 3.0) * cos(w * count) * convergence_factor;

            // おもりの現在位置
            double wx = pivot_x + L * sin(theta);
            double wy = pivot_y + L * cos(theta);

            // このパーツの現在位置 (比率に応じて支点とおもりの間を補間)
            pShot->x = pivot_x + (wx - pivot_x) * ratio;
            pShot->y = pivot_y + (wy - pivot_y) * ratio;

            if (ratio == 1.0) {
                // ==========================================
                // 【おもり】の処理
                // ==========================================
                pShot->kind = img_enemyShotLargeBall[(int)j]; // 虹色グラデーション(0:赤 ~ 6:白)

                // 1. 中央通過時（速度最大）：自機狙い弾を発射
                if (fabs(theta) < 0.15 && pShot->param_i[0] == 0) {
                    pShot->param_i[0] = 1; // クールダウンフラグON

                    // 7個同時発音は耳障りなため、中央(j=3)の振り子のみ代表して効果音を再生
                    if (j == 3) {
                        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                    }

                    sEnemyShot* aim = new sEnemyShot;
                    aim->param_i[15] = 0; // 発射済み弾としてマーク
                    aim->x = pShot->x;
                    aim->y = pShot->y;
                    aim->muki = atan2(player.y - aim->y, player.x - aim->x);
                    aim->speed = 3.5;
                    aim->kind = img_enemyShotBullet[(int)j]; // 銃弾でスピード感を出す

                    aim->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    aim->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = aim;
                    pEnemyShotSet->pEnemyShotHead->prev = aim;
                }
                if (fabs(theta) > 0.4) {
                    pShot->param_i[0] = 0; // 範囲外に出たらクールダウン解除
                }

                // 2. 両端到達時（速度ゼロ）：全方位拡散弾を発射
                if (fabs(theta) > 1.0 && pShot->param_i[1] == 0) {
                    pShot->param_i[1] = 1; // クールダウンフラグON

                    // 両端(j=0, j=6)の振り子のみ代表して効果音を再生
                    if (j == 0 || j == 6) {
                        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                    }

                    for (int i = 0; i < 8; i++) {
                        sEnemyShot* omni = new sEnemyShot;
                        omni->param_i[15] = 0;
                        omni->x = pShot->x;
                        omni->y = pShot->y;
                        omni->muki = i * DX_PI / 4.0;
                        omni->speed = 1.5;
                        omni->kind = img_enemyShotSmallBall[(int)j]; // 小玉で着弾予測を補助

                        omni->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        omni->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = omni;
                        pEnemyShotSet->pEnemyShotHead->prev = omni;
                    }
                }
                if (fabs(theta) < 0.8) {
                    pShot->param_i[1] = 0; // 範囲内に戻ったらクールダウン解除
                }

                // 3. 収束完了瞬間（サビ）：おもりが弾けるようにリング状に発射
                if (cycle == 0) {
                    if (j == 0) {
                        // 極大音を再生して解放のインパクトを強調
                        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
                    }
                    for (int i = 0; i < 16; i++) {
                        sEnemyShot* ring = new sEnemyShot;
                        ring->param_i[15] = 0;
                        ring->x = pShot->x;
                        ring->y = pShot->y;
                        ring->muki = i * DX_PI / 8.0;
                        ring->speed = 4.0;
                        ring->kind = img_enemyShotMediumOval[(int)j]; // 中楕円弾で視認性の高いリングを形成

                        ring->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        ring->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = ring;
                        pEnemyShotSet->pEnemyShotHead->prev = ring;
                    }
                }
            }
            else {
                // ==========================================
                // 【糸】の処理
                // ==========================================
                // 銃弾(5.0x2.0)を回転させることで、点線状の「糸」を表現する
                pShot->kind = img_enemyShotBullet[(int)j];
                // 糸の向きは、支点からおもりへのベクトル角度 (DxLibは0が右、PI/2が下)
                pShot->muki = DX_PI / 2.0 - theta;
            }
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_PendulumWave_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと

        // ペンデュラムウェーブ管理用のShotSetを1つ作成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPendulumWave;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // リストの番兵初期化
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 7個の振り子を生成
        for (int j = 0; j < 7; j++) {
            double pivot_x = 120.0 + j * 40.0; // 横一列に等間隔 (120, 160, 200, 240, 280, 320, 360)
            double pivot_y = 40.0;
            double L = 140.0; // 糸の長さ

            // 周期をずらすための角速度計算
            // T_j = 120 * 7 / (7 + j) フレーム周期とする
            // w_j = 2 * PI / T_j = 2 * PI * (7 + j) / (120 * 7)
            // これにより、count = 840 の倍数で全振り子の位相が完全に一致する
            double w = 2.0 * DX_PI * (7.0 + j) / 840.0;

            // 1つの振り子につき4つのパーツを作成（糸の区切り3つ + おもり1つ）
            double ratios[] = { 0.33, 0.66, 0.99, 1.0 };
            for (int k = 0; k < 4; k++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->param_i[15] = 1;         // 1: このShotSetが管理する構造パーツ
                pShot->param_d[0] = pivot_x;    // 支点X
                pShot->param_d[1] = pivot_y;    // 支点Y
                pShot->param_d[2] = L;          // 長さ
                pShot->param_d[3] = w;          // 角速度
                pShot->param_d[4] = (double)j;  // カラーインデックス (0~6)
                pShot->param_d[5] = ratios[k];  // 支点からの距離比率
                pShot->param_i[0] = 0;          // 中央通過発射のクールダウン管理
                pShot->param_i[1] = 0;          // 両端到達発射のクールダウン管理

                // リストに追加
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }

        // 作成したShotSetをグローバル管理リストに登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // 敵本体の動き（左右にゆっくり揺れながら、ペンデュラムの動きを邪魔しない位置取り）
        enemy.x = 240.0 + 30.0 * sin(count * 0.02);
        enemy.y = 40.0 + 10.0 * cos(count * 0.03);
    }
}