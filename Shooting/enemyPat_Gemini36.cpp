// enemyPat_popcorn.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：ポップ・コーン・ポッパー
static void ShotPopcorn(sEnemyShotSet* pEnemyShotSet)
{
    // =======================================================
    // 1. 発射フェーズ（一定期間、種を上方向へばら撒き続ける）
    // =======================================================
    if (pEnemyShotSet->count < 150 && pEnemyShotSet->count % 5 == 0) {

        // 連続再生による音割れを防ぐため、再生中なら止めてから鳴らす
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 一度に2〜3個の種を放り投げる
        int seedNum = 2 + GetRand(1);
        for (int i = 0; i < seedNum; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // [ 状態管理（param_i, param_d の使い道） ]
            // param_i[0] : 弾の状態 (0:落下前の種, 1:加熱・停止中, 2:弾けた後)
            // param_i[1] : 停止してから破裂するまでの待機フレーム数
            // param_i[2] : 停止し始めたフレームの記録用
            // param_d[0] : X方向の速度 (vx)
            // param_d[1] : Y方向の速度 (vy)
            // param_d[2] : 弾けるY座標のトリガー位置

            pEnemyShot->param_i[0] = 0; // 状態0: 種
            pEnemyShot->param_d[0] = (GetRand(400) - 200) / 100.0; // vx: -2.0 〜 2.0
            pEnemyShot->param_d[1] = -(150 + GetRand(250)) / 100.0; // vy: -1.5 〜 -4.0 (上方向へ放り投げる)

            pEnemyShot->param_d[2] = 120.0 + GetRand(200); // 弾ける高さ: 120 〜 320
            pEnemyShot->param_i[1] = 20 + GetRand(40);     // 待機時間: 20 〜 60フレーム

            // 種の見た目: 黄色(1)の小玉
            pEnemyShot->kind = img_enemyShotSmallBall[1];

            // リストの末尾(headのprev)に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // =======================================================
    // 2. 弾の更新フェーズ（リストを順回しして各弾の挙動を計算）
    // =======================================================
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShot->param_i[0] == 0) {
            // 【状態0: 種】 重力に従って放物線を描く
            pEnemyShot->param_d[1] += 0.06; // 重力加速度
            pEnemyShot->x += pEnemyShot->param_d[0];
            pEnemyShot->y += pEnemyShot->param_d[1];

            // 落下中 (vy > 0) かつ、指定の高さ (トリガーY座標) を超えたら停止状態へ移行
            if (pEnemyShot->param_d[1] > 0 && pEnemyShot->y >= pEnemyShot->param_d[2]) {
                pEnemyShot->param_i[0] = 1;
                pEnemyShot->param_i[2] = pEnemyShot->count; // 停止した瞬間の時間を記録
            }
        }
        else if (pEnemyShot->param_i[0] == 1) {
            // 【状態1: 加熱停止中】 弾ける直前の震える演出
            if (pEnemyShot->count % 2 == 0) {
                pEnemyShot->x += 1.0;
            }
            else {
                pEnemyShot->x -= 1.0;
            }

            // 待機時間を過ぎたら破裂させる
            if (pEnemyShot->count - pEnemyShot->param_i[2] >= pEnemyShot->param_i[1]) {
                pEnemyShot->param_i[0] = 2; // 状態2: 弾けた後へ移行

                // 見た目を 白(6)の大玉 に変更し、破裂音を鳴らす
                pEnemyShot->kind = img_enemyShotLargeBall[6];
                if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                // ポップコーン本体の移動速度 (ゆっくり下へ拡散)
                pEnemyShot->param_d[0] = (GetRand(200) - 100) / 100.0; // vx: -1.0 〜 1.0
                pEnemyShot->param_d[1] = (50 + GetRand(100)) / 100.0;  // vy: 0.5 〜 1.5

                // 塩(破片)を周囲に散らす
                int fragNum = 3 + GetRand(2); // 3〜5個
                for (int j = 0; j < fragNum; j++) {
                    sEnemyShot* pFrag = new sEnemyShot;
                    pFrag->x = pEnemyShot->x;
                    pFrag->y = pEnemyShot->y;

                    pFrag->param_i[0] = 2; // すでに破裂済みの状態として扱う
                    pFrag->kind = img_enemyShotDiamond[6]; // 白(6)の菱形弾

                    double angle = GetRand(360) * DX_PI / 180.0;
                    double speed = (150 + GetRand(150)) / 100.0; // 1.5 〜 3.0
                    pFrag->param_d[0] = cos(angle) * speed;
                    pFrag->param_d[1] = sin(angle) * speed;

                    // リストの末尾に追加
                    pFrag->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pFrag->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pFrag;
                    pEnemyShotSet->pEnemyShotHead->prev = pFrag;
                }
            }
        }
        else if (pEnemyShot->param_i[0] == 2) {
            // 【状態2: ポップコーン本体 ＆ 塩の破片】 等速直線運動
            pEnemyShot->x += pEnemyShot->param_d[0];
            pEnemyShot->y += pEnemyShot->param_d[1];
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（指定関数名）
void EnemyPat_Popcorn_Gemini()
{
    static int muki;

    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 少し耐久力高め
        muki = 1;
    }
    else {
        // ボス本体は左右にゆっくりと往復移動する
        enemy.x += 0.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 240フレーム（約4秒）ごとにポップコーン弾幕のセットを生成
    if (count % 240 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPopcorn; // 今回作成した関数をセット
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // ダミーヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // EnemyShotSet全体のリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}