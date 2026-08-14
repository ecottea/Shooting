// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：氷符「絶対零度世界（エターナルフォースブリザード）」
static void ShotEternalForceBlizzard(sEnemyShotSet* pEnemyShotSet)
{
    // =========================================================
    // 1. 吹雪フェーズ（予兆） : 0 ～ 119 フレーム
    // =========================================================
    if (pEnemyShotSet->count < 110) {
        // 2フレームに1回、吹雪のように散発的に弾をばら撒く
        if (pEnemyShotSet->count % 2 == 0) {

            // パラパラという軽い音を断続的に鳴らす
            if (pEnemyShotSet->count % 10 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            // 1回に3発、ランダムな方向・速度で発射
            for (int i = 0; i < 5; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                // GetRand(359) は 0～359 の 360種類の整数を返す
                pEnemyShot->muki = GetRand(359) * DX_PI / 180.0;

                // 初速をランダムに (1.0 ～ 3.0)
                pEnemyShot->speed = (100 + GetRand(200)) / 100.0 + 1;

                // 弾の種類: 小丸弾。色は青(4)か白(6)をランダムで選択し、吹雪感を出す
                if (GetRand(1) == 0) {
                    pEnemyShot->kind = img_enemyShotSmallBall[4];
                }
                else {
                    pEnemyShot->kind = img_enemyShotSmallBall[6];
                }

                // param_i[0] を状態管理に使用。0 = 吹雪（通常移動）
                pEnemyShot->param_i[0] = 0;

                // リストへの追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // =========================================================
    // 2. 凍結フェーズ開始時の SE : 120 フレーム目
    // =========================================================
    if (pEnemyShotSet->count == 120) {
        // 凍結音の代わりに予告音（チャージ音）を使用
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // =========================================================
    // 4. 氷解フェーズ開始時の SE : 210 フレーム目
    // =========================================================
    // ※ 121 ～ 209 フレーム間は「3. 予兆表示（待機）」として何もしない
    if (pEnemyShotSet->count == 210) {
        // 炸裂音として派手な音を使用
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // =========================================================
    // 既存弾の挙動更新（毎フレーム実行）
    // =========================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // --- 状態0: 吹雪フェーズ または 凍結フェーズ ---
        if (pShot->param_i[0] == 0) {

            // 120フレーム目で全ての弾を「空間停止」させる
            if (pEnemyShotSet->count == 120) {
                pShot->speed = 0.0;
            }
            // 210フレーム目で停止していた弾が一斉に「氷解炸裂」する
            else if (pEnemyShotSet->count == 210) {

                pShot->param_i[0] = 1; // 状態1 = 炸裂（針弾）へ移行

                // 元の吹雪弾自身を針弾（シアンの菱形弾）に変化させる
                pShot->kind = img_enemyShotDiamond[3];

                // 炸裂の基準となる角度をランダムに決定
                //double baseAngle = GetRand(359) * DX_PI / 180.0;
                //pShot->muki = baseAngle;
                pShot->speed = 1.5;

                // さらに同じ位置から、別の2方向（+120度、-120度）へ分裂弾を生成する
                // これにより1つの弾が3方向に炸裂する
                for (int i = -1; i <= 1; i += 2) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    pNewShot->x = pShot->x;
                    pNewShot->y = pShot->y;
                    pNewShot->muki = pShot->muki + (i * 60.0 * DX_PI / 180.0);
                    pNewShot->speed = 1.5;
                    pNewShot->kind = img_enemyShotDiamond[6]; // 分裂先は白の菱形弾にして色味を豊かに
                    pNewShot->param_i[0] = 1; // すでに炸裂済みの状態として生成

                    // リストへの追加（末尾に追加されるため、以後のフレームで自然に処理される）
                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }
            }
        }
        // --- 状態1: 氷解炸裂フェーズ（針弾） ---
        else if (pShot->param_i[0] == 1) {
            // 超高速の針弾を表現するため、最大速度（10.0）まで急加速させる
            if (pShot->speed < 10.0) {
                pShot->speed += 0.2;
            }
        }

        // --- 座標更新 ---
        // countや画面外消去はメインルーチンで行われるため、ここでは純粋に移動のみを行う
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}


// 敵本体のパターン
void EnemyPat_EternalForceBlizzard_Gemini()
{
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 長めに魅せるため少し高め
        shot_count = 0;
    }
    else {
        // 待機中は左右に優雅に揺れ動く
        enemy.x = 240.0 + 80.0 * sin(count * DX_PI / 180.0);
    }

    // 300フレーム周期で弾幕を展開
    if (count % 300 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotEternalForceBlizzard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 今回は固定値で使わない
        pEnemyShotSet->kind = shot_count++;

        // ダミーのヘッドノードを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}