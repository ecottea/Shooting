// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================================
// 弾幕パターン：「ベクトルの収束と拡散」 (Vector Convergence & Divergence)
// ============================================================================
static void ShotArrowPattern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 矢印の構成設定（ここで弾数を調整可能）
    constexpr int WING_PAIRS = 7; // 羽の対の数。ここを4や5に増やすと長い矢印になります。
    constexpr int TOTAL_BULLETS = 1 + WING_PAIRS * 2; // コア1発 ＋ (羽の数×左右)

    // 【フェーズ1：予兆と配置】
    if (pEnemyShotSet->count == 0) {
        // 0 〜 360 の値をランダムに取得し、ラジアン角（初期の進行方向）に変換
        double base_muki = GetRand(360) / 180.0 * DX_PI;
        pEnemyShotSet->muki = base_muki;

        for (int i = 0; i < TOTAL_BULLETS; i++) {
            pEnemyShot = new sEnemyShot;

            double lx = 0.0;
            double ly = 0.0;
            int role = 0; // 0: コア(先端), 1: ウイング(羽)

            // 先頭(i==0)以外はウイングとして相対座標を計算
            if (i > 0) {
                int pairNum = (i - 1) / 2 + 1; // 何番目の対か (1, 2, 3...)
                int isRight = (i - 1) % 2;     // 0なら左、1なら右

                lx = -16.0 * pairNum;
                ly = (isRight ? 12.0 : -12.0) * pairNum;
                role = 1;
            }

            // 画面端で矢印の形が崩れないよう、消去判定のマージンを大きく取る
            pEnemyShot->margin = 400.0;

            // フェーズ2のアフィン変換（回転）用に、初期のローカル相対座標を保存
            pEnemyShot->param_d[0] = lx;
            pEnemyShot->param_d[1] = ly;
            pEnemyShot->param_i[0] = role;

            // 初期進行方向（base_muki）に合わせてローカル座標を回転
            double rotX = lx * cos(base_muki) - ly * sin(base_muki);
            double rotY = lx * sin(base_muki) + ly * cos(base_muki);

            pEnemyShot->x = pEnemyShotSet->x + rotX;
            pEnemyShot->y = pEnemyShotSet->y + rotY;
            pEnemyShot->muki = base_muki;
            pEnemyShot->speed = 1.5; // 配置フェーズは低速で画面内に滑り込ませる

            // 素材選定
            if (role == 0) {
                pEnemyShot->kind = img_enemyShotLargeBall[5]; // コア：マゼンタの菱形弾
            }
            else {
                pEnemyShot->kind = img_enemyShotScale[3];   // 羽：シアンの鱗弾
            }

            // 双方向リストへの追加処理
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 【フェーズ2：反転とロックオン】
    // 60フレーム目（約1秒後）、配置された矢印が一斉に自機を向いて凝固する
    if (pEnemyShotSet->count == 60) {
        //if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        //PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // まず、基準となる「コア弾」の現在の画面座標を探す
        double coreX = pEnemyShotSet->x;
        double coreY = pEnemyShotSet->y;
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 0) {
                coreX = pEnemyShot->x;
                coreY = pEnemyShot->y;
                break;
            }
            pEnemyShot = pEnemyShot->next;
        }

        // コア弾の位置からプレイヤー（自機）への角度を算出
        double target_muki = atan2(player.y - coreY, player.x - coreX);
        pEnemyShotSet->muki = target_muki;

        // コア弾の座標を基準に、全てのウイング弾の配置を新しい角度へ回転（アフィン変換）させる
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            double lx = pEnemyShot->param_d[0];
            double ly = pEnemyShot->param_d[1];

            // 自機へのロックオン角度（target_muki）で再配置
            double rotX = lx * cos(target_muki) - ly * sin(target_muki);
            double rotY = lx * sin(target_muki) + ly * cos(target_muki);

            pEnemyShot->x = coreX + rotX;
            pEnemyShot->y = coreY + rotY;
            pEnemyShot->muki = target_muki;
            pEnemyShot->speed = 0.0; // チャージ演出のため一時静止

            pEnemyShot = pEnemyShot->next;
        }
    }

    // 【フェーズ3-1：コア弾の高速射出】
    // 静止から30フレーム後、矢印の先端（コア弾）が超高速で自機に突っ込む
    else if (pEnemyShotSet->count == 90) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 0) {
                pEnemyShot->speed = 6.5; // 高速直進
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // 【フェーズ3-2：ウイング弾の時間差追従】
    // さらに15フレーム後、残された羽が一斉にコアの軌跡を追うように中速で射出される
    else if (pEnemyShotSet->count == 105) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 1) {
                pEnemyShot->speed = 3.8; // 中速追従
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // 全ての弾の通常移動処理（毎フレーム実行）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================================
// 敵本体の行動パターン
// ============================================================================
void EnemyPat_Arrow_Gemini()
{
    static int muki;

    if (count == 1) {
        // 初期配置とHPのセット（仕様通り200固定）
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右への緩やかな往復移動ルーチン
        enemy.x += 1.0 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 80フレームごとに新しく「矢印の塊（セット）」を1つ生成して射出
    if (count % 40 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotArrowPattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 子弾用リストヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // エネミーショットセットの全体リストへ連結
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}