// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 各フェーズの弾幕パターン関数
// ============================================================

// フェーズ1：【ストライク】摩擦と火花
static void ShotPhase1_Strike(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 毎フレーム、橙(8)の小さな火花を上下にばら撒く
    if (pSet->count == 0) {
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x + GetRand(20) - 10;
            pShot->y = pSet->y + GetRand(20) - 10;

            // ボスの位置に応じて、火花が飛ぶ基準角度を変える（上なら下へ、下なら上へ）
            double base_muki = (pSet->y < 240) ? DX_PI / 2 : -DX_PI / 2;
            pShot->muki = base_muki + (GetRand(60) - 30) / 180.0 * DX_PI;
            pShot->speed = 3.0 + GetRand(20) / 10.0;

            // 橙の小玉
            pShot->kind = img_enemyShotSmallBall[8];

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pEnemyShot = pSet->pEnemyShotHead->next;
    while (pEnemyShot != pSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// フェーズ2：【イグニッション】着火と遅延加速
static void ShotPhase2_Ignition(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // マッチの軸をイメージした遅延加速弾を円形に配置
        for (int i = 0; i < 32; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = i * 2 * DX_PI / 32.0;
            pShot->speed = 0.1; // 初期速度は極めて遅い
            pShot->kind = img_enemyShotBullet[8]; // 橙の銃弾（軸の代用）
            pShot->param_i[0] = 1; // 加速フラグ

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    else {
        // 既存の軸の弾を徐々に加速させる
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                pShot->speed += 0.05;
                if (pShot->speed > 5.0) {
                    pShot->speed = 5.0;
                    pShot->param_i[0] = 0; // 最大速度に到達したら加速停止
                }
            }
            pShot = pShot->next;
        }

        // 中心から着火の炎（赤い中玉）をプレイヤー狙いで撃つ
        if (pSet->count == 15) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = pSet->muki; // プレイヤー狙い
            pShot->speed = 4.0;
            pShot->kind = img_enemyShotMediumBall[0]; // 赤

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pEnemyShot = pSet->pEnemyShotHead->next;
    while (pEnemyShot != pSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// フェーズ3：【バーニング】燃焼と隙間の縮小
static void ShotPhase3_Burning(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 燃える軸となる弾を放射状に配置
        for (int i = 0; i < 16; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = i * 2 * DX_PI / 16.0;
            pShot->speed = 1.5; // ゆっくり外向きに進む
            pShot->kind = img_enemyShotScale[1]; // 黄の鱗弾（軸）
            pShot->param_i[0] = 60; // 寿命（フレーム）

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 軸の弾の寿命管理と消去
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // 軸の弾（黄の鱗弾）かどうかを判定
        if (pShot->kind == img_enemyShotScale[1]) {
            pShot->param_i[0]--;
            if (pShot->param_i[0] <= 0) {
                // 軸が燃え尽きるときに、火花（橙の小玉）を散らす
                for (int i = 0; i < 8; i++) {
                    sEnemyShot* pSpark = new sEnemyShot;
                    pSpark->x = pShot->x;
                    pSpark->y = pShot->y;
                    pSpark->muki = i * 2 * DX_PI / 8.0;
                    pSpark->speed = 2.0;
                    pSpark->kind = img_enemyShotSmallBall[8]; // 橙

                    pSpark->prev = pSet->pEnemyShotHead->prev;
                    pSpark->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pSpark;
                    pSet->pEnemyShotHead->prev = pSpark;
                }
                // リストから削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        pShot = pNext;
    }

    // 炎の弾（赤い小玉）を放射状に撃つ
    if (pSet->count == 0) {
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            // 時間が経つほど角度がずれ、隙間が縮小・変形していく
            pShot->muki = i * 2 * DX_PI / 8.0 + pSet->count * 0.1;
            pShot->speed = 3.0;
            pShot->kind = img_enemyShotSmallBall[0]; // 赤

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pEnemyShot = pSet->pEnemyShotHead->next;
    while (pEnemyShot != pSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// フェーズ4：【アッシュ】消火と灰の雨
static void ShotPhase4_Ash(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 画面上部からランダムなX座標で灰を生成
        pSet->x = GetRand(480);
        pSet->y = -20;

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = DX_PI / 2; // 真下へ
        pShot->speed = 1.5 + GetRand(10) / 10.0;
        pShot->kind = img_enemyShotSmallBall[7]; // 黒

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pEnemyShot = pSet->pEnemyShotHead->next;
    while (pEnemyShot != pSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Match_Qwen()
{
    static int phase = 0;
    static int local_count = 0;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        local_count = 0;
        phase = 1;
    }
    else {
        local_count++;
        if (local_count >= 480) {
            local_count = 0; // 480フレームでループ
        }

        // フェーズ遷移
        if (local_count == 0)   phase = 1;
        else if (local_count == 120) phase = 2;
        else if (local_count == 240) phase = 3;
        else if (local_count == 360) phase = 4;

        // フェーズに応じたボスの移動
        if (phase == 1) {
            // 左右に往復してマッチを擦る
            enemy.x += 2.0 * cos(local_count * 0.05);
        }
        else if (phase == 2) {
            // 画面中央へ寄って静止
            enemy.x += (240.0 - enemy.x) * 0.1;
        }
        else if (phase == 3) {
            // ゆっくり回転しながら移動
            enemy.x = 240.0 + 100.0 * cos(local_count * 0.02);
        }
        else if (phase == 4) {
            // 画面中央で静止
            enemy.x += (240.0 - enemy.x) * 0.1;
        }
    }

    // 弾幕の生成頻度
    int shot_interval = 10;
    if (phase == 1) shot_interval = 5;  // 火花は高頻度
    if (phase == 4) shot_interval = 2;  // 灰の雨は超高頻度

    if (local_count % shot_interval == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

        // フェーズに応じたパターン関数を設定
        if (phase == 1)      pSet->patternFunc = ShotPhase1_Strike;
        else if (phase == 2) pSet->patternFunc = ShotPhase2_Ignition;
        else if (phase == 3) pSet->patternFunc = ShotPhase3_Burning;
        else if (phase == 4) pSet->patternFunc = ShotPhase4_Ash;

        // 弾リストの初期化
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // ShotSetリストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}