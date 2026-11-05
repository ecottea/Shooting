#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ペンデュラムウェーブ風弾幕
//    ・画面上部に仮想支点を9個横一列に並べる
//    ・各振り子の位相を少しずつずらして「波」を作る
//    ・12秒後に再び全振り子が揃う（＝収束）
//    ・糸はシアンの小弾を等間隔に並べて表現
//    ・おもりはマゼンタの大弾
//    ・おもりが下を左→右に通過するたびに3-way自機狙い
//    ・再収束時はリング弾を追加
//  使用素材：
//    糸    … img_enemyShotSmallBall[3]  (シアン小弾)
//    おもり… img_enemyShotLargeBall[5]  (マゼンタ大弾)
//    発射弾… img_enemyShotMediumBall[0] (赤中弾)
//    リング… img_enemyShotMediumBall[6] (白中弾)
// ============================================================

static const int    PENDULUM_NUM = 9;
static const int    PENDULUM_F0 = 3;    // 12秒間に(F0+i)周期
static const double PENDULUM_PERIOD = 12.0; // 再収束までの秒数
static const double PENDULUM_AMP = 24.0 * DX_PI / 180.0; // 振幅
static const int    PENDULUM_BEADS = 8;    // 糸を表す小弾の数

// 振り子1本分の更新関数
static void ShotPendulum(sEnemyShotSet* pEnemyShotSet)
{
    const int    i = pEnemyShotSet->param_i[0]; // 振り子番号 0〜8
    const double L = pEnemyShotSet->param_d[0]; // 振り子の長さ
    const double pivot_x = pEnemyShotSet->x;          // 支点X
    const double pivot_y = pEnemyShotSet->y;          // 支点Y

    // ---- 初回：糸のビーズ＋おもりを生成 ----
    if (pEnemyShotSet->count == 0) {
        for (int b = 0; b <= PENDULUM_BEADS; b++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pivot_x;
            pEnemyShot->y = pivot_y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            // b < PENDULUM_BEADS → 糸用の小弾、最後の1個 → おもり用の大弾
            pEnemyShot->kind = (b < PENDULUM_BEADS)
                ? img_enemyShotSmallBall[3]
                : img_enemyShotLargeBall[5];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        pEnemyShotSet->param_d[1] = 0.0; // 前フレームの角度
    }

    // ---- 現在の角度 ----
    // θ(t) = A * sin(2π(f0+i)t / T)
    //  t=0 と t=T で全振り子が下（θ=0）で揃い、間で位相がずれて波になる
    const double t = (double)pEnemyShotSet->count / 60.0;
    const double omega = 2.0 * DX_PI * (PENDULUM_F0 + i) / PENDULUM_PERIOD;
    const double theta = PENDULUM_AMP * sin(omega * t);

    const double weight_x = pivot_x + L * sin(theta);
    const double weight_y = pivot_y + L * cos(theta);

    // ---- ビーズ・おもりの位置更新／発射済みの弾は等速移動 ----
    int idx = 0;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (idx <= PENDULUM_BEADS) {
            // idx 0〜7: 糸のビーズ、idx 8: おもり
            const double r = (idx < PENDULUM_BEADS)
                ? L * (double)(idx + 1) / (double)(PENDULUM_BEADS + 1)
                : L;
            pShot->x = pivot_x + r * sin(theta);
            pShot->y = pivot_y + r * cos(theta);
            pShot->speed = 0.0;
        }
        else {
            // 発射済みの弾は通常移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        idx++;
        pShot = pShot->next;
    }

    // ---- おもりが下（θ=0）を左→右に通過したら3-way自機狙い ----
    const double prev_theta = pEnemyShotSet->param_d[1];
    if (prev_theta < 0.0 && theta >= 0.0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const double base = atan2(player.y - weight_y, player.x - weight_x);
        for (int k = -1; k <= 1; k++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = weight_x;
            pEnemyShot->y = weight_y;
            pEnemyShot->muki = base + k * (15.0 * DX_PI / 180.0);
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤中弾
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    pEnemyShotSet->param_d[1] = theta;

    // ---- 12秒ごとの再収束時にリング弾を放出 ----
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 720 == 0) {
        if (i == 0) { // 効果音は1本だけ鳴らす（多重再生を避ける）
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        for (int k = 0; k < 6*3; k++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = weight_x;
            pEnemyShot->y = weight_y;
            pEnemyShot->muki = (2.0 * DX_PI / 6.0/3) * k;
            pEnemyShot->speed = 1.8;
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白中弾
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 720 == 360) {
        if (i == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        for (int k = 0; k < 6*2; k++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = weight_x;
            pEnemyShot->y = weight_y;
            pEnemyShot->muki = (2.0 * DX_PI / 6.0/2) * k;
            pEnemyShot->speed = 1.8;
            pEnemyShot->kind = img_enemyShotMediumBall[4];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_PendulumWave_DeepSeek()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 20.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しない

        // 開始予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 振り子9本分のショットセットを生成
        for (int i = 0; i < PENDULUM_NUM; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotPendulum;
            // 支点を横一列に等間隔配置（画面中央揃え）
            pEnemyShotSet->x = 240.0 + (i - (PENDULUM_NUM - 1) / 2.0) * 36.0;
            pEnemyShotSet->y = 40.0;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = 0;
            pEnemyShotSet->param_i[0] = i;
            // 長さ：遅い振り子ほど長く（物理的な振り子波に近づける）
            //   i=0 … L=200（周期4秒）, i=8 … L=80（周期1.09秒）
            pEnemyShotSet->param_d[0] = 200.0 - 15.0 * i;
            pEnemyShotSet->param_d[1] = 0.0;
            // 弾リストの番兵
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            // グローバルリストへ登録
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    else {
        // 敵本体はゆっくり横揺れ（見た目のみ）
        enemy.x = 240.0 + 30.0 * sin(count * 0.015);
        enemy.y = 20.0;
    }
}