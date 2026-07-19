// enemyPat_moebius.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：メビウスの帯
static void ShotMoebius(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ----------------------------------------------------
    // 初期化：弾の生成
    // ----------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num_theta = 60; // 帯の長手方向の分割数
        int num_w = 5;      // 帯の幅方向の分割数

        for (int i = 0; i < num_theta; i++) {
            for (int j = 0; j < num_w; j++) {
                pEnemyShot = new sEnemyShot;

                // param_d[0] : 帯方向の角度 (theta) メビウスの帯を一周するため 0 ～ 4π
                pEnemyShot->param_d[0] = i * (4.0 * DX_PI / num_theta);
                // param_d[1] : 帯の幅方向の位置 (w) 中心を0として -24 ～ +24
                pEnemyShot->param_d[1] = (j - (num_w - 1) / 2.0) * 12.0;

                // param_i[0] : 弾の状態 (0: 帯の形成中, 1: 飛翔中)
                pEnemyShot->param_i[0] = 0;

                // 帯の幅位置によって色を変え、立体感と構造を強調
                int color;
                if (j == 2) color = 6;                // 中心は白
                else if (j == 1 || j == 3) color = 3; // 中間はシアン
                else color = 4;                       // 外縁は青

                // 流れやねじれが視認しやすい「鱗弾」を使用
                pEnemyShot->kind = img_enemyShotScale[color];

                // 双方向リストへ追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ----------------------------------------------------
    // 発射タイミングの効果音
    // ----------------------------------------------------
    if (pEnemyShotSet->count == 120) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ----------------------------------------------------
    // 弾の更新処理
    // ----------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShot->param_i[0] == 0) {
            // 【状態0】 メビウスの帯の形成・回転中
            double R = 80.0;

            // 毎フレーム少しずつ帯上を移動させ、弾が流れるようにする
            double theta = pEnemyShot->param_d[0] + pEnemyShotSet->count * 0.05;
            double w = pEnemyShot->param_d[1];

            // 3次元的なメビウスの帯の座標を計算
            double base_x = (R + w * cos(theta / 2.0)) * cos(theta);
            double base_y = (R + w * cos(theta / 2.0)) * sin(theta);
            double base_z = w * sin(theta / 2.0);

            // 全体を時間経過で回転させる
            double time_angle = pEnemyShotSet->count * 0.02;

            // Y軸周りの回転
            double rot_x1 = base_x * cos(time_angle) - base_z * sin(time_angle);
            double rot_z1 = base_x * sin(time_angle) + base_z * cos(time_angle);
            double rot_y1 = base_y;

            // X軸周りの傾き (30度傾けて立体感を出す)
            double tilt = DX_PI / 6.0;
            double final_x = rot_x1;
            double final_y = rot_y1 * cos(tilt) - rot_z1 * sin(tilt);

            // 画面座標に変換（敵の座標を中心に配置）
            double next_x = pEnemyShotSet->x + final_x;
            double next_y = pEnemyShotSet->y + final_y;

            // 弾の向きを進行方向に向ける
            if (pEnemyShotSet->count == 0) {
                pEnemyShot->x = next_x;
                pEnemyShot->y = next_y;
                pEnemyShot->muki = theta + DX_PI / 2.0; // 初回は暫定の接線方向
            }
            else {
                pEnemyShot->muki = atan2(next_y - pEnemyShot->y, next_x - pEnemyShot->x);
                pEnemyShot->x = next_x;
                pEnemyShot->y = next_y;
            }

            // 240フレーム経過で飛翔モードに移行
            if (pEnemyShotSet->count >= 120) {
                pEnemyShot->param_i[0] = 1;
                // 中心（セットの位置）から放射状に飛ぶように向きを設定
                pEnemyShot->muki = atan2(final_y, final_x);
                // 弾速に少しばらつきを持たせ、帯が崩れていく美しさを演出
                // GetRand(100) は 0～100 の 101種類の値を返す
                pEnemyShot->speed = (150 + GetRand(100)) / 100.0; // 1.5 ～ 2.5
            }
        }
        else {
            // 【状態1】 飛翔中（直線移動）
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Mobius_Gemini()
{
    static int muki;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0;               // 画面中央より少し上に配置
        enemy.maxHp = enemy.hp = 200; // ボス相当のHP
        muki = 1;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += 1.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 400フレーム周期で弾幕発動
    if (count % 60 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMoebius;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストへの登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}