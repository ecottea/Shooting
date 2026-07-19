// enemyPat_Tmp.cpp
// インベーディング・レイン（侵略の雨）
// スペースインベーダーをモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 隊列の更新を行うパターン関数
static void InvadingRainUpdate(sEnemyShotSet* pSet)
{
    // 定数
    const double SPACING_X = 50.0;           // 横間隔
    const double SPACING_Y = 40.0;           // 縦間隔
    const double INIT_SPEED_X = 1.5;            // 初期横速度
    const double DESCENT_STEP = 50.0;           // 折り返し時の降下量
    const double SPEED_INCREASE = 1.05;           // 加速率
    const double DESCENT_TRIGGER_Y = 480.0 * 0.4;    // 隊列の削減を始めるY座標
    const double DANGER_LINE_Y = 480.0 * 0.8;    // 危険ライン
    const int    SHOT_INTERVAL_INIT = 90;             // 狙い撃ちの初期間隔(フレーム)
    const int    SHOT_INTERVAL_MIN = 20;             // 最低間隔
    const int    REMOVAL_INTERVAL = 30;             // 隊列弾の削減間隔
    const int    FINAL_FLASH_TIME = 30;             // 総攻撃前の点滅時間

    // 初回処理：隊列の生成とパラメータ初期化
    if (pSet->count == 0) {
        pSet->param_i[0] = 1;                        // state = 1 (移動中)
        pSet->param_i[1] = 1;                        // 方向 1:右 -1:左
        pSet->param_i[2] = 0;                        // shot_timer
        pSet->param_i[3] = 0;                        // 削減開始フラグ
        pSet->param_i[4] = 15;                       // 生存弾数
        pSet->param_i[5] = (1 << 15) - 1;            // 生存ビットマスク (0～14)
        pSet->param_i[6] = 0;                        // removal_timer
        pSet->param_i[7] = 0;                        // 降下回数
        pSet->param_d[0] = 240.0 - 2.0 * SPACING_X;  // formation_x (左端)
        pSet->param_d[1] = 10.0;                   // formation_y (上端)
        pSet->param_d[2] = INIT_SPEED_X;             // speed_x
        pSet->param_d[3] = 0.0;                      // 総攻撃時の目標X
        pSet->param_d[4] = 0.0;                      // 総攻撃時の目標Y

        // 5列×3行 = 15個の弾を生成
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 5; ++col) {
                sEnemyShot* bullet = new sEnemyShot;
                bullet->kind = img_enemyShotMediumBall[6];  // 白い正方形弾
                bullet->x = pSet->param_d[0] + col * SPACING_X;
                bullet->y = pSet->param_d[1] + row * SPACING_Y;
                bullet->speed = 0.0;
                bullet->muki = 0.0;
                bullet->param_i[0] = row * 5 + col;          // 識別インデックス
                bullet->margin = 480;

                // 双方向リストへ追加
                bullet->prev = pSet->pEnemyShotHead->prev;
                bullet->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = bullet;
                pSet->pEnemyShotHead->prev = bullet;
            }
        }
        return;
    }

    // 状態変数の参照を取得
    int& state = pSet->param_i[0];
    int& dir = pSet->param_i[1];
    int& shot_timer = pSet->param_i[2];
    int& removal_active = pSet->param_i[3];
    int& alive_count = pSet->param_i[4];
    int& alive_bits = pSet->param_i[5];
    int& removal_timer = pSet->param_i[6];
    int& descent_count = pSet->param_i[7];
    double& form_x = pSet->param_d[0];
    double& form_y = pSet->param_d[1];
    double& speed_x = pSet->param_d[2];
    double& target_x = pSet->param_d[3];
    double& target_y = pSet->param_d[4];

    // 状態1: 通常移動フェーズ
    if (state == 1) {
        // 隊列の横移動
        form_x += dir * speed_x;

        // 画面端での折り返し処理
        bool hit_edge = false;
        if (dir == 1 && form_x + 4.0 * SPACING_X >= 480.0) {
            form_x = 480.0 - 4.0 * SPACING_X;
            hit_edge = true;
        }
        else if (dir == -1 && form_x <= 0.0) {
            form_x = 0.0;
            hit_edge = true;
        }

        if (hit_edge) {
            dir *= -1;
            speed_x *= SPEED_INCREASE;
            form_y += DESCENT_STEP;
            ++descent_count;
        }

        // 一定ラインまで降下したら、上列の弾を1つずつ削除
        if (!removal_active && form_y >= DESCENT_TRIGGER_Y) {
            removal_active = 1;
            removal_timer = 0;
        }

        if (removal_active) {
            ++removal_timer;
            if (removal_timer >= REMOVAL_INTERVAL && alive_count > 0) {
                // 最も上の行の右端から削除
                int rm_row = -1, rm_col = -1;
                for (int r = 2; r >= 0; --r) {
                    for (int c = 4; c >= 0; --c) {
                        int idx = r * 5 + c;
                        if (alive_bits & (1 << idx)) {
                            rm_row = r;
                            rm_col = c;
                            goto FOUND;
                        }
                    }
                }
            FOUND:
                if (rm_row != -1) {
                    int idx = rm_row * 5 + rm_col;
                    alive_bits &= ~(1 << idx);
                    --alive_count;
                    removal_timer = 0;
                }
            }
        }

        // 生存している隊列弾の位置を更新（画面外のものは遠くへ）
        sEnemyShot* bullet = pSet->pEnemyShotHead->next;
        while (bullet != pSet->pEnemyShotHead) {
            int idx = bullet->param_i[0];

            // 隊列インデックス 0～14 を持ち、かつ生存している弾 → 隊列位置に更新
            if (idx >= 0 && idx < 15 && (alive_bits & (1 << idx))) {
                int col = idx % 5;
                int row = idx / 5;
                bullet->x = form_x + col * SPACING_X;
                bullet->y = form_y + row * SPACING_Y;
            }
            // それ以外の弾（レーザーなど）は自力で移動
            else if (idx == -1) {   // レーザー弾の目印
                bullet->x += bullet->speed * cos(bullet->muki);
                bullet->y += bullet->speed * sin(bullet->muki);
            }
            // idx が範囲外で生きている隊列弾はすでに削除扱い → 画面外へ
            else if (idx >= 0 && idx < 15) {
                bullet->x = -2000.0;
                bullet->y = -2000.0;
            }

            bullet = bullet->next;
        }

        // 自機狙いレーザー（前列のランダムな位置から発射）
        int shot_interval = SHOT_INTERVAL_INIT - descent_count * 5;
        if (shot_interval < SHOT_INTERVAL_MIN) shot_interval = SHOT_INTERVAL_MIN;
        ++shot_timer;
        if (shot_timer >= shot_interval) {
            shot_timer = 0;

            // 前列(行0)で生存している列をリストアップ
            int front_cols[5];
            int front_cnt = 0;
            for (int c = 0; c < 5; ++c) {
                int idx = c; // row=0
                if (alive_bits & (1 << idx))
                    front_cols[front_cnt++] = c;
            }

            if (front_cnt > 0) {
                int chosen = front_cols[GetRand(front_cnt - 1)];
                double sx = form_x + chosen * SPACING_X;
                double sy = form_y; // row 0
                double angle = atan2(player.y - sy, player.x - sx);

                sEnemyShot* laser = new sEnemyShot;
                laser->kind = img_enemyShotLaser[0];      // 赤いレーザー
                laser->x = sx;
                laser->y = sy;
                laser->muki = angle;
                laser->speed = 6.0;
                laser->param_i[0] = -1;
                laser->margin = 480;

                laser->prev = pSet->pEnemyShotHead->prev;
                laser->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = laser;
                pSet->pEnemyShotHead->prev = laser;

                if (!CheckSoundMem(sound_enemyShot_medium))
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
        }

        // 総攻撃突入条件チェック
        double bottom_y = form_y + 2.0 * SPACING_Y; // 最下列のY座標
        if (alive_count <= 3 || bottom_y >= DANGER_LINE_Y) {
            state = 2;
            target_x = player.x;
            target_y = player.y;
            pSet->param_i[2] = 0; // final_timer をリセット
        }
    }
    // 状態2: 総攻撃フェーズ
    else if (state == 2) {
        int& final_timer = pSet->param_i[2];
        ++final_timer;

        if (final_timer <= FINAL_FLASH_TIME) {
            // 点滅演出: 全生存弾を赤くし、その場に停止させる（位置は最後に更新されたまま）
            sEnemyShot* bullet = pSet->pEnemyShotHead->next;
            while (bullet != pSet->pEnemyShotHead) {
                int idx = bullet->param_i[0];
                if (alive_bits & (1 << idx)) {
                    bullet->kind = img_enemyShotMediumBall[0]; // 赤
                }
                else {
                    bullet->x += bullet->speed * cos(bullet->muki);
                    bullet->y += bullet->speed * sin(bullet->muki);
                }
                bullet = bullet->next;
            }
        }
        else if (final_timer == FINAL_FLASH_TIME + 1) {
            // 一斉発射: 生存弾すべてを記憶した自機位置へ飛ばす
            sEnemyShot* bullet = pSet->pEnemyShotHead->next;
            while (bullet != pSet->pEnemyShotHead) {
                int idx = bullet->param_i[0];
                if (alive_bits & (1 << idx)) {
                    double angle = atan2(target_y - bullet->y, target_x - bullet->x);
                    bullet->muki = angle;
                    bullet->speed = 5.0;
                    // 色は赤のまま
                }
                else {
                    bullet->x += bullet->speed * cos(bullet->muki);
                    bullet->y += bullet->speed * sin(bullet->muki);
                }
                bullet = bullet->next;
            }
        }
        else {
            // それ以降は各弾を speed/muki で移動させる
            sEnemyShot* bullet = pSet->pEnemyShotHead->next;
            while (bullet != pSet->pEnemyShotHead) {
                int idx = bullet->param_i[0];
                if (alive_bits & (1 << idx)) {
                    bullet->x += bullet->speed * cos(bullet->muki);
                    bullet->y += bullet->speed * sin(bullet->muki);
                }
                else {
                    bullet->x += bullet->speed * cos(bullet->muki);
                    bullet->y += bullet->speed * sin(bullet->muki);
                }
                bullet = bullet->next;
            }
        }
    }
}

// 敵本体のパターン
void EnemyPat_Invader_DeepSeek()
{
     if (count == 1) {
        // 敵本体は画面上部に留まり、HPは標準値
        enemy.x = 240.0;
        enemy.y = -50.0;
        enemy.maxHp = enemy.hp = 200;
    }

    if (count % 120 == 1) {
        // 隊列を管理する ShotSet を1つだけ生成
        sEnemyShotSet* pFormationSet = new sEnemyShotSet;
        pFormationSet->count = 0;
        pFormationSet->patternFunc = InvadingRainUpdate;
        pFormationSet->x = enemy.x;
        pFormationSet->y = enemy.y;
        pFormationSet->muki = 0.0;
        pFormationSet->kind = 0;
        pFormationSet->pEnemyShotHead = new sEnemyShot;
        pFormationSet->pEnemyShotHead->prev = pFormationSet->pEnemyShotHead;
        pFormationSet->pEnemyShotHead->next = pFormationSet->pEnemyShotHead;

        // グローバルリストへ追加
        pFormationSet->prev = enemyShotSetHead.prev;
        pFormationSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pFormationSet;
        enemyShotSetHead.prev = pFormationSet;
    }
}