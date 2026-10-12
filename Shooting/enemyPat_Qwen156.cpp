#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：ルーロー・ローテーション（定幅の回転檻）
// ============================================================
static void ShotReuleauxTriangle(sEnemyShotSet* pSet)
{
    // pSet->count はメインルーチンで自動インクリメントされる

    // パラメータの定義
    // param_d[0]: 現在の回転角度 (rad)
    // param_d[1]: 現在の縮尺 (1.0 が基準)
    // param_i[0]: 弾の種別 (0: 弧の小弾, 1: 頂点の中弾, 2: 独立した自機狙い弾)
    // param_i[1]: 頂点インデックス (0, 1, 2) ※頂点弾のみ使用

    if (pSet->count == 0) {
        // 発射音（重厚な発射音）
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const double BASE_RV = 260.0;          // 中心から頂点までの距離
        const double L = sqrt(3.0) * BASE_RV;  // 頂点を中心とする円弧の半径（一辺の長さ）
        const int NUM_ARC_BULLETS = 16;        // 1つの円弧あたりの小弾の数

        // ルーローの三角形の輪郭を生成
        for (int i = 0; i < 3; i++) {
            double theta_v = (i * 2.0 * DX_PI) / 3.0; // 頂点の基準角度 (0°, 120°, 240°)
            double vx = BASE_RV * cos(theta_v);
            double vy = BASE_RV * sin(theta_v);

            // 1. 頂点に「中弾」を配置（マゼンタ）
            sEnemyShot* vShot = new sEnemyShot;
            vShot->param_d[0] = BASE_RV;       // 中心からの距離
            vShot->param_d[1] = theta_v;       // 中心からの基準角度
            vShot->param_i[0] = 1;             // 頂点フラグ
            vShot->param_i[1] = i;             // 頂点インデックス
            vShot->kind = img_enemyShotMediumBall[5]; // 中玉・マゼンタ
            vShot->speed = 0.0;
            vShot->margin = 420.0;

            // リストの末尾に追加
            vShot->prev = pSet->pEnemyShotHead->prev;
            vShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = vShot;
            pSet->pEnemyShotHead->prev = vShot;

            // 2. 頂点を中心とする円弧上に「小弾」を配置（シアン）
            // 幾何学的に、頂点iを中心とする円弧は、角度(theta_v + 150°) から (theta_v + 210°) の範囲を走る
            for (int j = 0; j < NUM_ARC_BULLETS; j++) {
                double t = (double)j / (double)NUM_ARC_BULLETS;
                double angle = theta_v + (5.0 * DX_PI / 6.0) + t * (DX_PI / 3.0);

                // 円弧上の絶対座標を計算
                double bx = vx + L * cos(angle);
                double by = vy + L * sin(angle);

                // 回転・縮小処理のために、中心(0,0)からの極座標を保存
                double dist = sqrt(bx * bx + by * by);
                double base_angle = atan2(by, bx);

                sEnemyShot* aShot = new sEnemyShot;
                aShot->param_d[0] = dist;
                aShot->param_d[1] = base_angle;
                aShot->param_i[0] = 0;         // 弧の小弾フラグ
                aShot->kind = img_enemyShotSmallBall[3]; // 小玉・シアン
                aShot->speed = 0.0;
                aShot->margin = 420.0;

                // リストの末尾に追加
                aShot->prev = pSet->pEnemyShotHead->prev;
                aShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = aShot;
                pSet->pEnemyShotHead->prev = aShot;
            }
        }
    }
    else {
        // 回転と「ガタつき」のパラメータ更新
        pSet->param_d[0] += 0.025/3; // 回転速度

        // 定幅曲線の特徴である「転がると中心が上下する」挙動をシミュレート
        // 3角形なので 3 * 回転角度 でサイン波を作ると、頂点が下に来た時にガクンと下がる表現になる
        double wobble = 18.0 * sin(3.0 * pSet->param_d[0]);

        // 収縮と膨張（呼吸）を繰り返して、隙間を開閉させる
        // 1.0 ～ 0.5 の間をゆっくり変動
        pSet->param_d[1] = 0.75 + 0.25 * sin(pSet->count * 0.015)*2;

        double cx = pSet->x;
        double cy = pSet->y + wobble; // ガタつきをY座標に適用

        sEnemyShot* shot = pSet->pEnemyShotHead->next;
        while (shot != pSet->pEnemyShotHead) {
            if (shot->param_i[0] == 2) {
                // 【独立した自機狙い弾】の処理
                // 回転行列の影響を受けず、通常の弾として移動する
                shot->x += shot->speed * cos(shot->muki);
                shot->y += shot->speed * sin(shot->muki);
            }
            else {
                // 【回転する枠の弾】の処理
                double current_angle = shot->param_d[1] + pSet->param_d[0];
                double current_dist = shot->param_d[0] * pSet->param_d[1];

                shot->x = cx + current_dist * cos(current_angle);
                shot->y = cy + current_dist * sin(current_angle);

                // 【ギミック】頂点（インデックス0のもののみ）から定期的に自機狙い弾を発射
                // 3つ全てから出すと過剰になるので、代表して1つの頂点から狙わせる
                if (shot->param_i[0] == 1 ) {
                    if (pSet->count % 45 == 0 && pSet->count > 90) {
                        sEnemyShot* aimShot = new sEnemyShot;
                        aimShot->x = shot->x;
                        aimShot->y = shot->y;
                        aimShot->muki = atan2(player.y - shot->y, player.x - shot->x);
                        aimShot->speed = 3.2;
                        aimShot->kind = img_enemyShotMediumOval[1]; // 中楕円弾・黄
                        aimShot->param_i[0] = 2; // 独立弾フラグを立てて、次回以降の回転計算から除外
                        aimShot->margin = 420.0;

                        // リストの末尾に追加
                        aimShot->prev = pSet->pEnemyShotHead->prev;
                        aimShot->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = aimShot;
                        pSet->pEnemyShotHead->prev = aimShot;
                    }
                }
            }
            shot = shot->next;
        }
    }
}

// ============================================================
// 敵本体のパターン：仮実装用
// ============================================================
void EnemyPat_ReuleauxTriangle_Qwen()
{
    static int phase = 0;
    static sEnemyShotSet* pReuleauxSet = nullptr;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        phase = 0;
        pReuleauxSet = nullptr;
    }
    else {
        // 敵の移動：左右にゆっくり揺れながら、少し上下も動く
        enemy.x = 240.0 + 120.0 * sin(count * 0.015);
        enemy.y = 240.0 + 30.0 * sin(count * 0.025);

        if (phase == 0) {
            // フェーズ0: 予告と準備
            if (count == 60-59) {
                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            }
            if (count == 120-59) {
                // フェーズ1へ移行：弾幕セットの生成
                pReuleauxSet = new sEnemyShotSet;
                pReuleauxSet->count = 0;
                pReuleauxSet->patternFunc = ShotReuleauxTriangle;
                pReuleauxSet->x = enemy.x;
                pReuleauxSet->y = enemy.y;
                pReuleauxSet->muki = 0.0;
                pReuleauxSet->kind = 0;
                pReuleauxSet->param_d[0] = 0.0; // 回転角度初期値
                pReuleauxSet->param_d[1] = 1.0; // 縮尺初期値

                // 循環リストのヘッド初期化
                pReuleauxSet->pEnemyShotHead = new sEnemyShot;
                pReuleauxSet->pEnemyShotHead->prev = pReuleauxSet->pEnemyShotHead;
                pReuleauxSet->pEnemyShotHead->next = pReuleauxSet->pEnemyShotHead;

                // グローバルリストに接続
                pReuleauxSet->prev = enemyShotSetHead.prev;
                pReuleauxSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pReuleauxSet;
                enemyShotSetHead.prev = pReuleauxSet;

                phase = 1;
            }
        }
        else if (phase == 1) {
            // フェーズ1: 弾幕の維持と追従
            // 発生源の中心を敵の現在位置に追従させる（滑らかな動きのため）
            pReuleauxSet->x = enemy.x;
            pReuleauxSet->y = enemy.y;

            // 一定時間経過したら次のフェーズへ（ここではループ継続させるためフェーズ変更はしないが、
            // 必要に応じて pReuleauxSet を削除して phase = 2 などにする）
            // 例: if (count > 600) { phase = 2; }
        }
    }
}