// enemyPat_Shuriken.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --- 定数定義 ---
const int PHASE_TRANSITION_COUNT = 150; // 停滞から一斉攻撃（解放）に移行するまでのフレーム数
const int BOUND_MAX = 12;                // 分裂した手裏剣の最大反射回数

// 弾幕パターン：旋風・影分身十字
static void ShotShurikenCross(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ==========================================
    // 初期化：巨大手裏剣の生成
    // ==========================================
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // pEnemyShotSetのパラメータを利用して全体の状態を管理
        pEnemyShotSet->param_d[0] = 1.0; // 巨大手裏剣全体の進行スピード
        pEnemyShotSet->param_d[1] = 0.0; // 巨大手裏剣の自転角度(alpha)

        // 十字の4本アーム、各アームに5個の弾を配置して巨大な手裏剣を形成
        for (int arm = 0; arm < 4; arm++) {
            double base_angle = (DX_PI / 2.0) * arm;
            for (int i = 1; i <= 8; i++) {
                pEnemyShot = new sEnemyShot;

                // param_i[0] の役割: 0=手裏剣の刃, 1=停滞弾(影分身), 2=反射弾(解放後の破片)
                pEnemyShot->param_i[0] = 0;

                // 中心からの距離 (r)
                pEnemyShot->param_d[0] = i * 20.0;
                // 中心からの角度オフセット (theta)
                pEnemyShot->param_d[1] = base_angle;

                pEnemyShot->kind = img_enemyShotDiamond[3]; // シアンの菱形弾

                pEnemyShot->margin = 480;

                // リストへの追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ==========================================
    // フェーズ1：投擲と自転、影分身の設置
    // ==========================================
    if (pEnemyShotSet->count < PHASE_TRANSITION_COUNT) {
        // 巨大手裏剣の中心座標を移動
        double setSpeed = pEnemyShotSet->param_d[0];
        pEnemyShotSet->x += setSpeed * cos(pEnemyShotSet->muki);
        pEnemyShotSet->y += setSpeed * sin(pEnemyShotSet->muki);

        // 自転角度の更新
        pEnemyShotSet->param_d[1] += 0.05; // 回転速度
        double alpha = pEnemyShotSet->param_d[1];

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 0) { // 刃の弾の処理
                double r = pEnemyShot->param_d[0];
                double theta = pEnemyShot->param_d[1];

                // 自転を考慮したワールド座標への変換
                pEnemyShot->x = pEnemyShotSet->x + r * cos(alpha + theta);
                pEnemyShot->y = pEnemyShotSet->y + r * sin(alpha + theta);
                pEnemyShot->muki = alpha + theta + DX_PI / 2.0;

                // 一番外側の弾から、一定間隔で停滞弾(影分身)をドロップする
                if (r >= 100.0 && pEnemyShotSet->count % 8 == 0) {
                    sEnemyShot* pDrop = new sEnemyShot;
                    pDrop->x = pEnemyShot->x;
                    pDrop->y = pEnemyShot->y;
                    pDrop->muki = 0.0;
                    pDrop->speed = 0.0;
                    pDrop->param_i[0] = 1; // 役割：停滞弾
                    pDrop->kind = img_enemyShotSmallBall[4]; // 青の小玉

                    pDrop->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pDrop->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pDrop;
                    pEnemyShotSet->pEnemyShotHead->prev = pDrop;
                }
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // ==========================================
    // フェーズ移行：解放（一斉攻撃）
    // ==========================================
    if (pEnemyShotSet->count == PHASE_TRANSITION_COUNT) {
        if (CheckSoundMem(sound_enemyShot_extreme) == 1) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 0) {
                // 手裏剣の刃は四方に散る反射弾（破片）に変化
                pEnemyShot->param_i[0] = 2; // 役割：反射弾
                pEnemyShot->param_i[1] = 0; // 反射回数カウントリセット

                double theta = pEnemyShot->param_d[1];
                double alpha = pEnemyShotSet->param_d[1];
                pEnemyShot->muki = alpha + theta; // 外側に向かって弾け飛ぶ
                pEnemyShot->speed = 3.5;
                pEnemyShot->kind = img_enemyShotScale[5]; // マゼンタの鱗弾
            }
            else if (pEnemyShot->param_i[0] == 1) {
                // 停滞弾は自機狙いに変化
                pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                // 速度にバラつきを持たせて波状攻撃にする
                pEnemyShot->speed = (150 + GetRand(150)) / 100.0;
                pEnemyShot->kind = img_enemyShotSmallBall[0]; // 危険を知らせる赤の小玉
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // ==========================================
    // フェーズ2移行後：各弾の個別移動処理
    // ==========================================
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 反射弾の移動と反射ロジック
        if (pEnemyShot->param_i[0] == 2) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            // 画面境界での反射処理（マージンによる自動消去の前に内側で反射させる）
            if (pEnemyShot->param_i[1] < BOUND_MAX) {
                bool isBounced = false;
                if (pEnemyShot->x < 5.0 || pEnemyShot->x > 475.0) {
                    pEnemyShot->muki = DX_PI - pEnemyShot->muki;
                    pEnemyShot->x = (pEnemyShot->x < 5.0) ? 5.0 : 475.0; // 画面外へ出るのを防ぐ押し出し
                    isBounced = true;
                }
                if (pEnemyShot->y < 5.0 || pEnemyShot->y > 475.0) {
                    pEnemyShot->muki = -pEnemyShot->muki;
                    pEnemyShot->y = (pEnemyShot->y < 5.0) ? 5.0 : 475.0;
                    isBounced = true;
                }

                // 反射した瞬間に音を鳴らす
                if (isBounced) {
                    pEnemyShot->param_i[1]++;
                    if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }
            }
        }
        // 自機狙い弾の移動処理
        else if (pEnemyShot->param_i[0] == 1 && pEnemyShotSet->count > PHASE_TRANSITION_COUNT) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_Shuriken_Gemini()
{
    static int muki;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    // 敵本体の移動（ゆっくり左右に揺れる）
    else if (count > 60) {
        enemy.x += 0.5 * (double)muki;
        if (enemy.x > 340.0) muki = -1;
        if (enemy.x < 140.0) muki = 1;
    }

    // 300フレーム周期で巨大手裏剣を投擲
    if (count % 200 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShurikenCross;

        // 発射位置は敵本体の座標
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // プレイヤーの方向へ向かってゆっくり投げる
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}