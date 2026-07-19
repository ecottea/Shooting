// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// -----------------------------------------------------------
// 弾幕パターン関数: 擬似3Dスパイラルトンネル
// -----------------------------------------------------------
static void ShotTunnel3D(sEnemyShotSet* pEnemyShotSet)
{
    // --- 定数定義 ---
    const double Z_MAX = 50.0;      // 奥行き最大値（初期Z）
    const double Z_SPEED = 0.4;     // Z軸の移動速度（手前へ進む速さ）
    const double RADIUS = 18.0;     // トンネルの半径（画面内に収まるよう小さめに設定）
    const double TWIST = 0.3;       // トンネルのねじれ係数
    const double FOV = 240.0;       // 遠近法の補正値（画角）
    const int NUM_BULLETS = 16;     // 1リングあたりの弾数

    // --- 生成処理 (count == 0) ---
    if (pEnemyShotSet->count == 0) {
        // 効果音（トンネル発生音）
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // リング状に弾を生成
        for (int i = 0; i < NUM_BULLETS; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 基本角度の設定
            double theta = (2.0 * DX_PI / NUM_BULLETS) * i + pEnemyShotSet->param_d[0];

            // param_dの使い方
            // [0]: 基本角度 (theta)
            // [1]: Z座標 (奥行き)
            pEnemyShot->param_d[0] = theta;
            pEnemyShot->param_d[1] = Z_MAX;

            // 【素材選定】
            // 奥（遠景）は「小さく・暗い」弾を使う。
            // 種類: 小玉 (img_enemyShotSmallBall)
            // 色: 4=青 (暗い色で大気遠近法を表現)
            pEnemyShot->kind = img_enemyShotSmallBall[4];

            // 【重要】画面外判定の閾値を大きくする
            // 擬似3D表現上、手前（Zが小さい）になると弾は画面外へ飛び出していく。
            // メインルーチンの画面外消去処理で手前で弾が消えてしまわないよう、
            // margin（画面外へどれだけ出たら弾を自動で消すか）を大きめに設定する。
            pEnemyShot->margin = 1000.0;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 更新・描画座標計算処理 ---
    // トンネル全体の回転 (時間経過によるロール回転)
    double global_rot = pEnemyShotSet->count * 0.04;

    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next_p = pEnemyShot->next; // 削除に備えてnextを保存

        double theta = pEnemyShot->param_d[0];
        double z = pEnemyShot->param_d[1];

        // Z座標の更新（手前へ移動）
        z -= Z_SPEED;
        pEnemyShot->param_d[1] = z;

        // カメラ通過判定（Zが小さすぎたら削除）
        // 画面外判定はメイン側だが、Z<=0は計算不能なのでここで処理する
        if (z < 1.0) {
            pEnemyShot->prev->next = pEnemyShot->next;
            pEnemyShot->next->prev = pEnemyShot->prev;
            delete pEnemyShot;
        }
        else {
            // 3D座標の計算
            // ねじれの適用: Zが小さい（手前）ほど角度がずれる
            double current_theta = theta + global_rot + (Z_MAX - z) * TWIST;

            double x3d = RADIUS * cos(current_theta);
            double y3d = RADIUS * sin(current_theta);

            // 透視投影 (2Dスクリーン座標への変換)
            double scale = FOV / z;
            pEnemyShot->x = pEnemyShotSet->x + x3d * scale;
            pEnemyShot->y = pEnemyShotSet->y + y3d * scale;

            // --- 大気遠近法とサイズ変化の表現 ---
            // Z値に応じて画像（サイズと色）を切り替える
            if (z > 15.0) {
                // 奥: 小さい、暗い（青）
                pEnemyShot->kind = img_enemyShotSmallBall[4];
            }
            else if (z > 12.0) {
                // 中: 中くらい、中間色（シアン）
                pEnemyShot->kind = img_enemyShotMediumBall[3];
            }
            else {
                // 手前: 大きい、明るい（白）
                pEnemyShot->kind = img_enemyShotLargeBall[6];
            }
        }

        pEnemyShot = next_p;
    }
}

// -----------------------------------------------------------
// 敵本体のパターン
// -----------------------------------------------------------
void EnemyPat_3D_Qwen()
{
    static double base_angle;

    if (count == 1) {
        // 敵の初期位置（画面中央上部、消失点）
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        base_angle = 0;
    }
    else {
        // 敵は「裂け目」として中央に鎮座するイメージ（固定）
    }

    // 弾幕生成 (トンネルの生成)
    // 毎フレーム生成すると密度が濃すぎるため、2フレームに1回程度
    if (count % 1 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTunnel3D;

        // 消失点（敵の位置）
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;

        // トンネルの位相をランダムに変化させる
        pEnemyShotSet->param_d[0] = base_angle;

        pEnemyShotSet->kind = 0;

        // リスト登録
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (count % 120 == 0) {
        base_angle = GetRand(360) / 180.0 * DX_PI;
    }
}