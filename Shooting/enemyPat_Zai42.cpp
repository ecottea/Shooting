// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <algorithm>

// 弾を生成するヘルパー関数（中心X座標を受け取るように変更）
static void CreatePyramidShot(sEnemyShotSet* pEnemyShotSet, double local_x, double local_y, double y_offset, int is_outer, double center_x) {
    sEnemyShot* pEnemyShot = new sEnemyShot;

    // 初期座標のセット（それぞれのピラミッドの中心X座標を基準にする）
    pEnemyShot->x = center_x + local_x;
    pEnemyShot->y = pEnemyShotSet->param_d[0] + local_y + y_offset; // param_d[0] は発射時の中心Yとして扱う

    // 回転と降下の計算のために、各ピラミッドの中心からの相対座標を保存
    pEnemyShot->param_d[0] = local_x;
    pEnemyShot->param_d[1] = local_y;
    pEnemyShot->param_d[2] = y_offset;
    pEnemyShot->param_d[3] = center_x; // この弾が属するピラミッドの中心X座標を保存

    // 今回は速度と向きのパラメータは使わず、相対座標から直接座標を計算する
    pEnemyShot->speed = 0.0;
    pEnemyShot->muki = 0.0;

    if (is_outer) {
        // 外枠：大玉 黄色(1) -> 金色の枠組み
        pEnemyShot->kind = img_enemyShotMediumBall[1];
    }
    else {
        // 内部ブロック：鱗弾 オレンジ(8) -> 砂の石積み
        pEnemyShot->kind = img_enemyShotScale[8];
    }

    pEnemyShot->margin = 200;

    // 双方向リストへの接続
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 弾幕：創世の三角錐
static void ShotPyramid(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 重厚感のある発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // セットのパラメータ初期化
        // param_d[0]: 発射時の中心Y
        // param_d[1]: 回転角速度 (ラジアン/フレーム)
        // param_d[2]: 降下速度
        pEnemyShotSet->param_d[0] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[1] = 0.008; // ゆっくり回転
        pEnemyShotSet->param_d[2] = 1.2;   // 降下速度

        // 3つのピラミッドの中心X座標をランダムに決定
        // シャッフルしたい配列を用意
        double center_xs[3] = { 60.0, 240.0, 420.0 };

        // フィッシャーイェーツ法でシャッフル
        for (int i = 2; i > 0; i--) {
            int j = GetRand(i); // 0 から i までのいずれかのインデックスを取得
            double temp = center_xs[i];
            center_xs[i] = center_xs[j];
            center_xs[j] = temp;
        }

        double sizes[3] = { 320.0, 210.0, 100.0 };
        double y_offsets[3] = { -90.0, -45.0, 0.0 };
        double rot_offsets[3] = { 0.0, 0.3, 0.6 }; // 少しずつ回転をずらして立体的に見せる

        for (int t = 0; t < 3; t++) {
            double L = sizes[t];
            double y_off = y_offsets[t];
            double base_rot = rot_offsets[t];
            double current_cx = center_xs[t]; // 今回のピラミッドの中心X

            // 正三角形の頂点座標（中心からの相対座標）を計算
            double px[3], py[3];
            for (int i = 0; i < 3; i++) {
                double angle = base_rot - DX_PI / 2.0 + (2.0 * DX_PI / 3.0) * i;
                px[i] = (L / 2.0) * cos(angle);
                py[i] = (L / 2.0) * sin(angle);
            }

            // 1. 外枠（辺）の弾配置
            int edge_interval = 26;
            for (int e = 0; e < 3; e++) {
                int n1 = e, n2 = (e + 1) % 3;
                double ex = px[n2] - px[n1];
                double ey = py[n2] - py[n1];
                double dist = sqrt(ex * ex + ey * ey);
                int num = (int)(dist / edge_interval);
                for (int i = 0; i <= num; i++) {
                    double ratio = (num == 0) ? 0.0 : (double)i / num;
                    double sx = px[n1] + ex * ratio;
                    double sy = py[n1] + ey * ratio;
                    CreatePyramidShot(pEnemyShotSet, sx, sy, y_off, 1, current_cx);
                }
            }

            // 2. 内部ブロックの弾配置（水平スキャンで階段状にする）
            double min_y = min(py[0], min(py[1], py[2]));
            double max_y = max(py[0], max(py[1], py[2]));
            int block_interval = 28; // 外枠より広い間隔にして、避ける隙間を確保

            for (double sy = min_y + block_interval; sy < max_y; sy += block_interval) {
                double cross_x[2];
                int c_idx = 0;

                // 水平線が三角形の辺と交差するX座標を求める
                for (int e = 0; e < 3; e++) {
                    int n1 = e, n2 = (e + 1) % 3;
                    double y1 = py[n1], y2 = py[n2];
                    // 厳密な交差判定（頂点ですれ違った際の重複を防ぐ）
                    if ((y1 < sy && sy <= y2) || (y2 < sy && sy <= y1)) {
                        double ratio = (sy - y1) / (y2 - y1);
                        cross_x[c_idx++] = px[n1] + (px[n2] - px[n1]) * ratio;
                    }
                }

                // 交差点が2つあれば、その間にブロックを配置
                if (c_idx == 2) {
                    double left_x = min(cross_x[0], cross_x[1]);
                    double right_x = max(cross_x[0], cross_x[1]);
                    // 左右に少し余白を持たせて石積みを配置
                    for (double sx = left_x + block_interval * 0.6; sx < right_x; sx += block_interval) {
                        CreatePyramidShot(pEnemyShotSet, sx, sy, y_off, 0, current_cx);
                    }
                }
            }
        }
    }

    // 毎フレームの移動処理（回転しながら降下）
    double rot_speed = pEnemyShotSet->param_d[1];
    double fall_speed = pEnemyShotSet->param_d[2];
    double base_cy = pEnemyShotSet->param_d[0]; // 発射時のボスY座標
    double rot_angle = rot_speed * pEnemyShotSet->count;

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 保存しておいた相対座標と、この弾専用の中心X座標を取り出す
        double lx = pEnemyShot->param_d[0];
        double ly = pEnemyShot->param_d[1];
        double y_off = pEnemyShot->param_d[2];
        double cx = pEnemyShot->param_d[3]; // 個別の中心X座標

        // 回転行列を適用して相対座標を回転させる
        double rx = lx * cos(rot_angle) - ly * sin(rot_angle);
        double ry = lx * sin(rot_angle) + ly * cos(rot_angle);

        // 実際の座標に変換（回転後の座標 + 自身の中心X座標 + 共通の中心Y座標 + 降下量）
        pEnemyShot->x = cx + rx;
        pEnemyShot->y = base_cy + ry + y_off + fall_speed * pEnemyShotSet->count;

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Pyramid_Zai()
{
    static int shot_count;

    if (count == 1) {
        // 弾幕全体が見えやすいように、画面上部の中央で固定
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;

        // 出現時に予告音を鳴らす
        //if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        //PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 敵は動かない（ピラミッドの頂点としての役割のため）

    // 150フレームごとにピラミッドを生成
    if (count % 150 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPyramid;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0; // 今回のギミックでは使用しない
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}