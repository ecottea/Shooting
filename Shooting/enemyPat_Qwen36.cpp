// enemyPat_popcorn.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 前方宣言
static void ShotKernel(sEnemyShotSet* pEnemyShotSet);
static void ShotPopcorn(sEnemyShotSet* pEnemyShotSet);
static void ShotButter(sEnemyShotSet* pEnemyShotSet);

// ============================================================
//  弾幕パターン関数
// ============================================================

// 1. コーンの粒（跳ね回り、時間経過で破裂する）
static void ShotKernel(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 壁反射ロジック (画面サイズ 480x480)
        if (pEnemyShot->x < 0.0 || pEnemyShot->x > 480.0) {
            pEnemyShot->param_d[0] *= -1.0; // vx反転
        }
        if (pEnemyShot->y < 0.0 || pEnemyShot->y > 480.0) {
            pEnemyShot->param_d[1] *= -1.0; // vy反転
        }

        // 移動処理
        pEnemyShot->x += pEnemyShot->param_d[0];
        pEnemyShot->y += pEnemyShot->param_d[1];

        // 破裂チェック (発射から60フレーム経過)
        if (pEnemyShot->count > 60) {
            // --- 破裂エフェクト＆ポップコーン生成 ---

            // 新しい弾幕セット（ポップコーン用）を生成
            sEnemyShotSet* pNewSet = new sEnemyShotSet;
            pNewSet->count = 0;
            pNewSet->patternFunc = ShotPopcorn;
            pNewSet->x = pEnemyShot->x;
            pNewSet->y = pEnemyShot->y;
            pNewSet->muki = 0;
            pNewSet->kind = 0;
            pNewSet->pEnemyShotHead = new sEnemyShot;
            pNewSet->pEnemyShotHead->prev = pNewSet->pEnemyShotHead;
            pNewSet->pEnemyShotHead->next = pNewSet->pEnemyShotHead;

            // グローバルリストへ登録
            pNewSet->prev = enemyShotSetHead.prev;
            pNewSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pNewSet;
            enemyShotSetHead.prev = pNewSet;

            // ポップコーン弾を8方向に生成
            if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK); // 破裂音
            for (int i = 0; i < 8; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShot->x;
                pShot->y = pEnemyShot->y;
                pShot->muki = (DX_PI * 2.0 / 8.0) * i;
                pShot->speed = 1.2;
                // 弾の色一覧: 6:白
                pShot->kind = img_enemyShotMediumBall[6];
                pShot->count = 0;
                pShot->param_i[0] = i; // 横揺れの位相用

                // セット内リストへ登録
                pShot->prev = pNewSet->pEnemyShotHead->prev;
                pShot->next = pNewSet->pEnemyShotHead;
                pNewSet->pEnemyShotHead->prev->next = pShot;
                pNewSet->pEnemyShotHead->prev = pShot;
            }

            // 自分自身（粒）は画面外へ飛ばして消去
            pEnemyShot->x = -1000.0;
            pEnemyShot->y = -1000.0;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 2. ポップコーン（ヒラヒラ落下しながら加速する）
static void ShotPopcorn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // ヒラヒラ落下 (サインカーブで横揺れ)
        pEnemyShot->x += sin(pEnemyShot->count * 0.1 + pEnemyShot->param_i[0]) * 1.5;
        pEnemyShot->y += pEnemyShot->speed;

        // 徐々に加速
        if (pEnemyShot->speed < 3.0) {
            pEnemyShot->speed += 0.03;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 3. バター（落下し、地面に到達すると池になる）
static void ShotButter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->y < 460.0) {
            // 落下中: サインカーブでヒラヒラ落ちる
            pEnemyShot->x += sin(pEnemyShot->count * 0.05 + pEnemyShot->param_i[0]) * 2.0;
            pEnemyShot->y += 2.5;
        }
        else {
            // 着地後: 画面下部に固定され「池」を形成
            pEnemyShot->y = 470.0;
            pEnemyShot->speed = 0.0;
            // 横にゆっくり広がる（粘性の表現）
            pEnemyShot->x += (pEnemyShot->param_i[0] % 2 == 0) ? 0.2 : -0.2;
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Popcorn_Qwen()
{
    static int muki;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵の移動（左右往復）
        enemy.x += 1.2 * (double)muki;
        if (enemy.x < 40.0 || enemy.x > 440.0) muki *= -1;
    }

    // --- フェーズ1: コーンの粒を撒く (0-180フレーム) ---
    if (count % 15 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotKernel;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = 0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 粒を3個生成
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = 0;
            pShot->speed = 0;
            // 弾の色一覧: 1:黄
            pShot->kind = img_enemyShotSmallBall[1];
            pShot->count = 0;

            // 初期速度ベクトル (param_d[0]=vx, param_d[1]=vy)
            pShot->param_d[0] = (GetRand(200) - 100) / 40.0;
            pShot->param_d[1] = (GetRand(100) + 100) / 40.0;  // 下向き

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --- フェーズ2: バターの雨 (240フレーム以降) ---
    if (count > 240 && count % 20 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotButter;
        pSet->x = GetRand(480);
        pSet->y = -20.0;
        pSet->muki = 0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        if (!CheckSoundMem(sound_enemyShot_heavy)) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // バター弾を1個生成
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = 0;
        pShot->speed = 2.5;
        // 弾の色一覧: 8:橙
        pShot->kind = img_enemyShotScale[8];
        pShot->count = 0;
        pShot->param_i[0] = GetRand(100); // 位相ずらし用

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}