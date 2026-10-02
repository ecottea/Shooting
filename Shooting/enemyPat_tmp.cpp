// enemyPat_Tmp.cpp
// 弾幕パターン「渦潮」： 全弾1px/frame以下の低速弾のみで構成
//  1440フレーム(24秒)で1ループする3ウェーブ構成
//  ・ウェーブ1(   0～ 600f): 渦の生成 … 双渦スパイラル(2方向から同時に8°ずつ回転発射、
//                             発射60フレーム後から0.5°/frameで進行方向を曲げて渦を描く)
//  ・ウェーブ2( 600～1080f): 潮の満ち引き … 全弾にサイン波の横揺れ(振幅20px)を加え、
//                             画面下部からも低速の狙い弾を出して下への逃げ場を塞ぐ
//  ・ウェーブ3(1080～1440f): 大渦の収束 … 全弾を減速(0.3)し、1°/frameで自機方向へ偏向

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {
    const double SPIRAL_SPEED = 0.9;  // 渦弾の速度
    const double AIMED_SPEED = 0.5;  // 下部からの狙い弾の速度
    const double HOME_SPEED = 0.3;  // 収束時の速度
    const double BEND_RAD = 0.5 * DX_PI / 180.0*0.75; // 渦弾が1フレームごとに曲がる角度
    const double HOME_RAD = 1.0 * DX_PI / 180.0; // 収束時の偏向角(1°/frame)
    const double SWAY_AMP = 20.0; // 横揺れの振幅(px)
    const double SWAY_FREQ = 0.03; // 横揺れの角周波数
    const int    CYCLE = 1440; // 1ループのフレーム数
    const int    W1_END = 600;  // ウェーブ1終了
    const int    W2_END = 1080; // ウェーブ2終了
}

// ----------------------------------------------------------
// 渦弾マスターセットのパターン関数
//  弾の param_d[0]/[1] : 横揺れの基準となる座標
//  弾の param_i[0]     : 渦を巻く方向(+1/-1)
//  弾の param_i[1]     : 横揺れ開始フラグ
//  弾の param_i[2]     : 横揺れ振幅の立ち上がりカウンタ
// ----------------------------------------------------------
static void ShotUzushio(sEnemyShotSet* pEnemyShotSet)
{
    const int c = pEnemyShotSet->count % CYCLE;

    // サイクル頭で発射音、ウェーブ3開始で予告音
    if (c == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    if (c == W2_END) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* pEnemyShot;

    // ── ウェーブ1: 渦の生成(双渦スパイラル) ──
    // 6フレームに1回、pEnemyShotSet->param_i[0]を発射番号として
    // 発射角を8°ずつ回転させ、180°反対の2方向から同時に発射する
    if (c < W1_END && c % 6 == 0) {
        double base = DX_PI / 2.0 + pEnemyShotSet->param_i[0] * 8.0 * DX_PI / 180.0;
        for (int arm = 0; arm < 2; arm++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base + arm * DX_PI;
            pEnemyShot->speed = SPIRAL_SPEED;
            pEnemyShot->margin = 120;
            // シアン/青の中玉で潮のイメージ
            pEnemyShot->kind = img_enemyShotMediumBall[(arm == 0) ? 3 : 4];
            pEnemyShot->param_d[0] = pEnemyShot->x;
            pEnemyShot->param_d[1] = pEnemyShot->y;
            pEnemyShot->param_i[0] = (arm == 0) ? 1 : -1; // 2つの腕は互い違いに渦を巻く
            // リストの末尾に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        pEnemyShotSet->param_i[0]++;
    }

    // ── 全弾の更新 ──
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (c >= W2_END) {
            // ── ウェーブ3: 減速して自機方向へ1°/frameで偏向 ──
            pShot->speed = HOME_SPEED;
            double target = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = target - pShot->muki;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            if (diff > HOME_RAD)        pShot->muki += HOME_RAD;
            else if (diff < -HOME_RAD)  pShot->muki -= HOME_RAD;
            else                        pShot->muki = target;
        }
        else {
            // ── ウェーブ1～2: 発射60フレーム後から渦を巻く ──
            if (pShot->count > 60) pShot->muki += pShot->param_i[0] * BEND_RAD;
        }

        // 基準位置を移動(横揺れの影響を受けない座標)
        pShot->param_d[0] += pShot->speed * cos(pShot->muki);
        pShot->param_d[1] += pShot->speed * sin(pShot->muki);

        // ── ウェーブ2以降: サイン波による横揺れ ──
        if (c >= W1_END && pShot->param_i[1] == 0) pShot->param_i[1] = 1;
        double amp = 0.0;
        if (pShot->param_i[1] == 1) {
            if (pShot->param_i[2] < 60) pShot->param_i[2]++; // 振幅をなめらかに立ち上げる
            amp = SWAY_AMP * pShot->param_i[2] / 60.0;
        }
        pShot->x = pShot->param_d[0] + sin(pShot->count * SWAY_FREQ) * amp;
        pShot->y = pShot->param_d[1];

        pShot = pShot->next;
    }
}

// ----------------------------------------------------------
// ウェーブ2限定: 画面下部から自機を狙う低速直進弾(3発/2秒ごと)
// ----------------------------------------------------------
static void ShotAimedBottom(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 240.0 + (i - 1) * 40.0;
            pEnemyShot->y = 492.0; // 画面のわずかに下(margin 20px以内)から出す
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->speed = AIMED_SPEED;
            pEnemyShot->kind = img_enemyShotSmallBall[5]; // マゼンタの小玉(渦弾と区別)
            // リストの末尾に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 直進のみ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    static sEnemyShotSet* pMaster = nullptr; // 渦弾マスターセット(常時弾を保持し続けるため空にならない)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 300; // 200から変更しないこと

        // 渦弾マスターセットの生成(以後、消滅させず渦の中心として使い続ける)
        pMaster = new sEnemyShotSet;
        pMaster->count = 0;
        pMaster->patternFunc = ShotUzushio;
        pMaster->x = enemy.x;
        pMaster->y = enemy.y + 10.0;
        pMaster->muki = 0.0;
        pMaster->kind = 0;
        pMaster->param_i[0] = 0; // 発射番号(スパイラルの回転角に使用)
        pMaster->pEnemyShotHead = new sEnemyShot;
        pMaster->pEnemyShotHead->prev = pMaster->pEnemyShotHead;
        pMaster->pEnemyShotHead->next = pMaster->pEnemyShotHead;
        pMaster->prev = enemyShotSetHead.prev;
        pMaster->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMaster;
        enemyShotSetHead.prev = pMaster;
    }
    else {
        // 敵はゆっくりと漂うように移動する(=渦の中心が動き、待ち続けられなくなる)
        enemy.x = 240.0 + sin(count * 0.010) * 100.0;
        enemy.y = 240.0 + sin(count * 0.021) * 55.0;

        // 渦の中心を敵に追従させる
        if (pMaster != nullptr) {
            pMaster->x = enemy.x;
            pMaster->y = enemy.y + 10.0;
        }

        // ── ウェーブ2の期間だけ、下から低速の狙い弾を2秒ごとに3発 ──
        int c2 = (count - 1) % CYCLE; // マスターセットのcountと同期した周期位置
        if (c2 >= W1_END && c2 < W2_END && count % 120 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotAimedBottom;
            pSet->x = 240.0;
            pSet->y = 492.0;
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
    }
}