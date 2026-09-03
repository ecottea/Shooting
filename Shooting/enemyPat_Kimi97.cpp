// enemyPat_PerfectFreeze.cpp
// 東方紅魔郷 凍符「パーフェクトフリーズ」(Lunatic) の弾幕再現
// チルノのスペルカード No.06

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  凍符「パーフェクトフリーズ」虹色弾パターン
//  ・発射時：虹色（赤/黄/緑/シアン/青/マゼンタ）の小玉を
//    ランダムな方向に発射
//  ・凍結時（count==50）：画面内の弾を停止し、白色に変更
//  ・解凍時（count==110）：各弾がランダムな方向に飛び散る
// ============================================================
static void ShotPerfectFreezeRainbow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 虹色弾発射時の効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // Lunatic: 虹色の小玉をランダム方向に28発発射
        for (int i = 0; i < 28 * 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // ランダム方向（0～360度）
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            // 速度: 1.2～2.7
            pEnemyShot->speed = (120 + GetRand(150)) / 100.0 * 2;

            // 虹色（赤=0、黄=1、緑=2、シアン=3、青=4、マゼンタ=5）
            int color = GetRand(5);
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            // param_i[0]: 0=通常移動, 1=凍結中
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    else if (pEnemyShotSet->count == 50+20) {
        // 凍結：弾を停止し、色を白に変更
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->speed = 0.0;
            pShot->param_i[0] = 1;              // 凍結中フラグ
            pShot->kind = img_enemyShotSmallBall[6]; // 白色(6)
            pShot = pShot->next;
        }
    }
    else if (pEnemyShotSet->count == 110+20) {
        // 解凍：各弾がランダムな方向に飛び散る
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->muki = GetRand(360) / 180.0 * DX_PI;
            pShot->speed = (100 + GetRand(200)) / 100.0; // 1.0～3.0
            pShot->param_i[0] = 0;                         // 通常移動に戻す
            pShot = pShot->next;
        }
    }

    // 弾の位置更新（凍結中は移動しない）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  凍符「パーフェクトフリーズ」青弾パターン
//  ・自機狙い6-way（Lunatic）
//  ・青色の小玉を自機方向を中心に左右に広げて発射
// ============================================================
static void ShotPerfectFreezeBlue(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 青弾発射時の効果音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 自機への角度を計算
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // Lunatic: 自機狙い6-way
        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 中心を自機方向とし、左右に15度ずつ広げた6-way
            double spread = (i - 2.5) * (DX_PI / 12.0);
            pEnemyShot->muki = baseAngle + spread;
            pEnemyShot->speed = 2.5;

            // 青弾（色=4:青）
            pEnemyShot->kind = img_enemyShotSmallBall[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の位置更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン：凍符「パーフェクトフリーズ」(Lunatic)
//  ・チルノが画面上部中央で左右に移動
//  ・180フレームサイクルで以下を繰り返し：
//      0f   : 虹色弾セット生成（ランダム方向発射）
//      50f  : 虹色弾凍結（ShotPerfectFreezeRainbow内で自動処理）
//      55f  : 青弾セット生成（自機狙い6-way）
//      110f : 凍結弾解凍・ランダム方向飛散（ShotPerfectFreezeRainbow内）
//      180f : サイクル終了、次のサイクルへ
// ============================================================
void EnemyPat_PerfectFreeze_Kimi()
{
    static int muki;
    static int phaseTimer;

    if (count == 1) {
        // 初期化：ゲーム画面は480x480
        enemy.x = 240.0;  // 画面中央
        enemy.y = 160.0;   // 上部
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        phaseTimer = 0;
    }
    else {
        // 左右移動（チルノらしい動き）
        enemy.x += 1.2 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // フェーズ管理（180フレームサイクル）
    if (phaseTimer == 0) {
        // 虹色弾セット生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPerfectFreezeRainbow;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else if (phaseTimer == 55) {
        // 青弾セット生成（自機狙い6-way）
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPerfectFreezeBlue;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 1;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    phaseTimer++;
    if (phaseTimer >= 180) {
        phaseTimer = 0;
    }
}