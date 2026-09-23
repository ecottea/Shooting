#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// Phase 1: メントスの投下
// ============================================================
static void ShotMentosDrop(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        pEnemyShot = new sEnemyShot;
        // 敵の位置から少し横にずらして落下させる
        pEnemyShot->x = pEnemyShotSet->x + (GetRand(100) - 50);
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = DX_PI / 2.0; // 真下
        pEnemyShot->speed = 2.0 + GetRand(100) / 100.0;

        // 白の小玉（メントス）
        pEnemyShot->kind = img_enemyShotSmallBall[6];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// Phase 2: シュワシュワ反応（泡とコーラの粒が上昇）
// ============================================================
static void ShotMentosSplash(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numShots = 5 + GetRand(3); // 1セットあたり5〜8発
        for (int i = 0; i < numShots; i++) {
            pEnemyShot = new sEnemyShot;

            // 着水地点（画面下部）付近から発生
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(200) - 100);
            pEnemyShot->y = pEnemyShotSet->y + GetRand(40);

            // 上方向を基準にばらつかせる
            double baseMuki = -DX_PI / 2.0;
            pEnemyShot->muki = baseMuki + (GetRand(60) - 30) / 180.0 * DX_PI;

            // 60%の確率でコーラの粒、40%で炭酸の泡
            if (GetRand(100) < 60) {
                // コーラの粒（黒の中玉）
                pEnemyShot->kind = img_enemyShotMediumBall[7];
                pEnemyShot->speed = 3.0 + GetRand(200) / 100.0; // 速め
                pEnemyShot->param_d[0] = 0.0; // 揺れなし
                pEnemyShot->param_d[1] = 0.0;
            }
            else {
                // 炭酸の泡（白の大玉）
                pEnemyShot->kind = img_enemyShotLargeBall[6];
                pEnemyShot->speed = 1.5 + GetRand(100) / 100.0; // 遅め
                // 横揺れのパラメータを設定
                pEnemyShot->param_d[0] = (double)GetRand(360) / 180.0 * DX_PI; // 位相
                pEnemyShot->param_d[1] = 1.0 + GetRand(200) / 100.0;          // 振幅
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本の移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 泡の場合のみ横揺れを追加
        if (pShot->kind == img_enemyShotLargeBall[6]) {
            pShot->x += sin(pShot->count * 0.15 + pShot->param_d[0]) * pShot->param_d[1];
        }
        pShot = pShot->next;
    }
}

// ============================================================
// Phase 3 予告: 泡のチャージ
// ============================================================
static void ShotMentosWarning(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = 480.0; // 画面下端に配置

        // 白の大玉（泡）で予告
        pEnemyShot->kind = img_enemyShotLargeBall[6];
        pEnemyShot->speed = 0.0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 15フレーム経過したら画面上へ高速移動させ、メインルーチンの画面外消去処理に任せる
        if (pEnemyShotSet->count > 15) {
            pShot->y -= 15.0;
        }
        pShot = pShot->next;
    }
}

// ============================================================
// Phase 3: 大噴出（レーザー噴射）
// ============================================================
static void ShotMentosJet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = 480.0; // 画面下から発射

        // 20%の確率で自機狙い、80%で上方向ランダム
        if (GetRand(100) < 20) {
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        }
        else {
            pEnemyShot->muki = -DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
        }

        pEnemyShot->speed = 6.0 + GetRand(200) / 100.0;
        // 橙の短レーザー（コーラの噴水柱）
        pEnemyShot->kind = img_enemyShotLaser[8];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン: メントス・スプラッシュ・ジェット
// ============================================================
void EnemyPat_MentosCola_Qwen()
{
    static int phase = 1;
    static int shot_count = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        phase = 1;
        shot_count = 0;
    }
    
    {
        // 敵の動き：左右にゆっくり往復
        enemy.x += 0.8 * cos(count * 0.02);

        const int T = 660;
        int countT = count % T;

        // -----------------------------------------------------
        // Phase 1: メントスの投下 (count: 1 〜 60)
        // -----------------------------------------------------
        if (phase == 1) {
            if (countT % 15 == 1 && countT <= 60) {
                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotMentosDrop;
                pEnemyShotSet->x = enemy.x;
                pEnemyShotSet->y = enemy.y + 10.0;
                pEnemyShotSet->muki = DX_PI / 2.0;
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
        // -----------------------------------------------------
        // Phase 2: シュワシュワ反応 (count: 61 〜 150)
        // -----------------------------------------------------
        else if (phase == 2) {
            if (countT % 8 == 1) {
                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotMentosSplash;
                pEnemyShotSet->x = enemy.x;
                pEnemyShotSet->y = 400.0; // 画面下部を着水地点とする
                pEnemyShotSet->muki = -DX_PI / 2.0;
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
        // -----------------------------------------------------
        // Phase 3: 大噴出 (count: 151 〜)
        // -----------------------------------------------------
        else if (phase == 3) {
            // レーザー予告（発射の15フレーム前に泡を出す）
            if (countT % 30 == 16) {
                sEnemyShotSet* pWarningSet = new sEnemyShotSet;
                pWarningSet->count = 0;
                pWarningSet->patternFunc = ShotMentosWarning;
                pWarningSet->x = enemy.x + (GetRand(300) - 150); // 発射予定位置X
                pWarningSet->y = 0.0; // ダミー
                pWarningSet->muki = 0.0;
                pWarningSet->kind = shot_count++;

                pWarningSet->pEnemyShotHead = new sEnemyShot;
                pWarningSet->pEnemyShotHead->prev = pWarningSet->pEnemyShotHead;
                pWarningSet->pEnemyShotHead->next = pWarningSet->pEnemyShotHead;

                pWarningSet->prev = enemyShotSetHead.prev;
                pWarningSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pWarningSet;
                enemyShotSetHead.prev = pWarningSet;
            }

            // レーザー発射
            if (countT % 30 == 1) {
                //if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                //PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotMentosJet;
                pEnemyShotSet->x = enemy.x + (GetRand(300) - 150); // 発射位置X
                pEnemyShotSet->y = 480.0;
                pEnemyShotSet->muki = -DX_PI / 2.0;
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

        // Phase遷移制御
        if (countT == 60) phase = 2;
        if (countT == 150) phase = 3;
        if (countT == T - 120) {
            phase = 1;
            shot_count = 0;
        }
    }
}