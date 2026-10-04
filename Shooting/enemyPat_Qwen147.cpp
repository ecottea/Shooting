#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：旋回・蛇行する三節棍
// ============================================================
static void ShotSansetsukon(sEnemyShotSet* pEnemyShotSet)
{
    // 発射初回のみ弾を生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base_muki = pEnemyShotSet->muki;

        // 5つのパーツを生成 (0:先端棍, 1:鎖1, 2:中央棍, 3:鎖2, 4:根元棍)
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = base_muki;
            pShot->speed = 3.5; // 棍全体の基本速度
            pShot->count = 0;

            // 素材の割り当て
            if (i == 0 || i == 2 || i == 4) {
                pShot->kind = img_enemyShotLaser[8]; // 棍：短レーザー(橙色)
            }
            else {
                pShot->kind = img_enemyShotSmallBall[7]; // 鎖：小玉(黒色)
            }

            // パラメータ初期化
            pShot->param_i[0] = i;           // パーツインデックス (0~4)
            pShot->param_i[1] = 0;           // 状態 (0:通常/蛇行中, 1:折れ曲がり中)
            pShot->param_d[0] = base_muki;   // 初期発射角度の保存

            // しなりの設定：後方のパーツほど振幅が大きく、位相が遅れるようにする
            pShot->param_d[1] = (i == 0) ? 0.0 : (i * 12.0);       // 振幅
            pShot->param_d[2] = (i == 0) ? 0.0 : (-i * 0.7);       // 位相オフセット
            pShot->margin = 120;

            // 双方向リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int part_idx = pShot->param_i[0];
        int state = pShot->param_i[1];
        double base_muki = pShot->param_d[0];
        double amp = pShot->param_d[1];
        double phase = pShot->param_d[2];

        if (state == 0) {
            // 【通常時】：全体が回転しながら、後方ほど遅れて大きくしなる
            double rotate_muki = base_muki + pEnemyShotSet->count * 0.04; // 全体回転

            // 蛇行の速度成分（振幅の微分相当）
            double snake_speed = amp * 0.15 * cos(pEnemyShotSet->count * 0.15 + phase);

            // 進行方向ベクトル
            double vx = cos(rotate_muki);
            double vy = sin(rotate_muki);

            // 進行方向に対して垂直なベクトル（しなり方向）
            double nx = -sin(rotate_muki);
            double ny = cos(rotate_muki);

            // 位置更新：基本移動 + しなり
            double move_vx = pShot->speed * vx + snake_speed * nx;
            double move_vy = pShot->speed * vy + snake_speed * ny;

            pShot->x += move_vx;
            pShot->y += move_vy;
            pShot->muki = atan2(move_vy, move_vx); // 見た目の向きを更新

            // 【折れ曲がりトリガー】 (例: 発射から100フレーム経過時)
            if (pEnemyShotSet->count == 100) {
                pShot->param_i[1] = 1; // 状態を「折れ曲がり中」に変更

                // 自機の方向に向かって急激に角度を変える（関節が曲がって自機を捕捉）
                double target_muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->param_d[3] = target_muki; // 目標角度を保存

                // 折れ曲がりの衝撃で、その場から鱗弾を撒き散らす
                for (int j = 0; j < 6*3; j++) {
                    sEnemyShot* pShock = new sEnemyShot;
                    pShock->x = pShot->x;
                    pShock->y = pShot->y;
                    pShock->muki = pShot->muki + (GetRand(120) - 60) / 180.0 * DX_PI;
                    pShock->speed = 2.0 + GetRand(150) / 100.0;
                    pShock->kind = img_enemyShotScale[1]; // 衝撃波：鱗弾(黄色)
                    pShock->param_i[1] = 2;
                    pShock->margin = 120;
                    pShot->count = 0;

                    pShock->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pShock->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pShock;
                    pEnemyShotSet->pEnemyShotHead->prev = pShock;
                }
            }
        }
        else if (state == 1) {
            // 【折れ曲がり中】：目標角度（自機方向）に向かって急旋回
            double target_muki = pShot->param_d[3];
            double diff = target_muki - pShot->muki;

            // 角度差を -PI ~ PI の範囲に正規化
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;

            // 徐々に角度を変更（係数 0.2 で滑らかかつ急な旋回）
            pShot->muki += diff * 0.2;

            // 折れ曲がった後は勢いよく加速して襲いかかる
            pShot->x += pShot->speed * 1.8 * cos(pShot->muki);
            pShot->y += pShot->speed * 1.8 * sin(pShot->muki);
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン：三節棍
// ============================================================
void EnemyPat_ThreeSectionStaff_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の移動：左右に往復
        enemy.x += 1.2 * (double)muki;
        if (count % 150 == 0) {
            muki *= -1;
        }
    }

    // 一定間隔で三節棍を発射
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSansetsukon;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // 自機狙いをベースに、発射ごとに角度を少しずらして広範囲をカバー
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x)
            + (shot_count % 3 - 1) * 0.3;

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