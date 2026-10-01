#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  罔両「禅寺に棲む妖蝶」
//  Evil Spirits "Bewitching Butterfly Living in the Zen Temple"
//  八雲紫 Phantasm スペルカード No.136 を参考に実装
//
//  構成:
//   1. 青色の小さな卍を反時計回りにゆっくり回転 + 青い蝶弾
//      （蝶弾は停止後、敵本体へゆっくり戻る）
//   2. 弾消し後、赤色の巨大な卍を時計回りに回転 + 自機狙い赤蝶弾
//      （赤蝶弾は距離に応じて水平方向に広がる）
// ============================================================

// 使える弾画像:
//   img_enemyShotSmallBall[0..8]  小玉
//   img_enemyShotMediumBall[0..8] 中玉
//   img_enemyShotLargeBall[0..8]  大玉
//   img_enemyShotBullet[0..8]     銃弾
//   img_enemyShotScale[0..8]      鱗弾 ← 蝶弾として使用
//   img_enemyShotDiamond[0..8]    菱形弾
//   img_enemyShotMediumOval[0..8] 中楕円弾
//   img_enemyShotLaser[0..8]      短レーザー ← 卍の腕として使用
// 色: 0赤 1黄 2緑 3シアン 4青 5マゼンタ 6白 7黒 8橙

// ------------------------------------------------------------
//  青卍レーザー（反時計回り回転）
//  param_d[0] = 初期角度オフセット
//  param_d[1] = 回転角速度（負で反時計）
//  param_d[2] = 腕の長さスケール
// ------------------------------------------------------------
static void ShotBlueManjiLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 卍の4本の腕をレーザーで構成
        // 卍は直交する2本の線が端で直角に曲がった形
        // 簡易表現: 中心から8方向に短いレーザーを放射し、回転させる
        const int ARM_COUNT = 8;
        for (int i = 0; i < ARM_COUNT; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 初期角度: 45度刻み + オフセット
            pEnemyShot->muki = pEnemyShotSet->param_d[0] + i * (DX_PI / 4.0);
            pEnemyShot->speed = 0.0; // 位置は親に追従、向きだけ回転
            pEnemyShot->kind = img_enemyShotLaser[4]; // 青レーザー
            pEnemyShot->param_d[0] = pEnemyShotSet->param_d[1]; // 角速度
            pEnemyShot->param_d[1] = 28.0 + pEnemyShotSet->param_d[2]; // 腕の長さ（中心からの距離）
            pEnemyShot->param_i[0] = 1; // タイプ: 回転レーザー

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 更新: 敵本体位置を中心に回転
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pShot->muki += pShot->param_d[0];
            // レーザーの中心を敵位置に合わせ、向き方向にオフセット
            double len = pShot->param_d[1];
            pShot->x = enemy.x + len * cos(pShot->muki);
            pShot->y = enemy.y + len * sin(pShot->muki);
            // レーザー画像は向きに合わせて描画される想定（mukiを保持）
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  赤卍レーザー（時計回り・大型）
// ------------------------------------------------------------
static void ShotRedManjiLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int ARM_COUNT = 8;
        for (int i = 0; i < ARM_COUNT; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->param_d[0] + i * (DX_PI / 4.0);
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[0]; // 赤レーザー
            pEnemyShot->param_d[0] = pEnemyShotSet->param_d[1]; // 角速度（正で時計回り）
            pEnemyShot->param_d[1] = 48.0 + pEnemyShotSet->param_d[2]; // 長い腕
            pEnemyShot->param_i[0] = 1;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pShot->muki += pShot->param_d[0];
            double len = pShot->param_d[1];
            pShot->x = enemy.x + len * cos(pShot->muki);
            pShot->y = enemy.y + len * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  青い蝶弾（展開 → 停止 → 敵へゆっくり帰還）
//  param_d[0] = 初期展開速度
//  param_d[1] = 停止までのフレーム数目安
// ------------------------------------------------------------
static void ShotBlueButterfly(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 円状に複数の蝶を展開
        const int BUTTERFLY_NUM = 16;
        for (int i = 0; i < BUTTERFLY_NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * (2.0 * DX_PI / BUTTERFLY_NUM);
            pEnemyShot->speed = pEnemyShotSet->param_d[0]+3; // 展開速度
            pEnemyShot->kind = img_enemyShotScale[4]; // 青の鱗弾＝蝶
            pEnemyShot->margin = 240;
            pEnemyShot->param_i[0] = 0; // フェーズ: 0=展開中, 1=帰還中
            pEnemyShot->param_d[0] = 0.0; // 展開経過用
            pEnemyShot->param_d[1] = pEnemyShotSet->param_d[1]; // 展開継続フレーム

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 展開フェーズ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->param_d[0] += 1.0;
            // 減速
            pShot->speed *= 0.997;
            if (pShot->param_d[0] >= pShot->param_d[1] || pShot->speed < 0.3) {
                pShot->param_i[0] = 1; // 帰還へ
                pShot->speed = 0.0;
            }
        }
        else {
            // 帰還フェーズ: 敵本体へゆっくり向かう
            double dx = enemy.x - pShot->x;
            double dy = enemy.y - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 4.0) {
                double ang = atan2(dy, dx);
                pShot->speed = 1.2; // ゆっくり戻る
                pShot->muki = ang;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                // 敵に近づいたらほぼ停止（見た目上消える扱いはメイン側）
                pShot->speed = 0.0;
            }
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  赤い蝶弾（自機狙いライン、距離に応じて水平拡散）
//  param_d[0] = 基準速度
//  param_d[1] = 水平拡散係数
// ------------------------------------------------------------
static void ShotRedButterfly(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 自機方向を基準に、複数本のラインを水平に少しずらして発射
        double baseAng = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        const int LINE_NUM = 5;   // ライン数
        const int PER_LINE = 6;   // 1ラインあたりの弾数

        for (int line = 0; line < LINE_NUM; line++) {
            double offsetAng = (line - (LINE_NUM - 1) / 2.0) * 0.12; // 水平寄りの角度オフセット
            for (int j = 0; j < PER_LINE; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAng + offsetAng;
                // 後ろの弾ほど少し遅めに出るイメージで初速を調整
                pEnemyShot->speed = pEnemyShotSet->param_d[0] + j * 0.15;
                pEnemyShot->kind = img_enemyShotScale[0]; // 赤の鱗弾＝蝶
                pEnemyShot->param_i[0] = 0;
                pEnemyShot->param_d[0] = pEnemyShotSet->param_d[1]; // 拡散係数
                pEnemyShot->param_d[1] = (double)(line - (LINE_NUM - 1) / 2.0); // ライン番号（符号付き）

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 進行距離に応じて水平方向へ徐々に拡散
        double travel = pShot->speed; // 毎フレームの移動量目安
        // 水平成分を追加（ライン番号の符号で左右に開く）
        double spread = pShot->param_d[0] * pShot->param_d[1] * 0.02;
        pShot->x += pShot->speed * cos(pShot->muki) + spread;
        pShot->y += pShot->speed * sin(pShot->muki);
        // わずかに加速
        pShot->speed += 0.01;
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  追加の装飾用小玉（卍の中心付近から控えめに）
// ------------------------------------------------------------
static void ShotManjiDecor(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        const int NUM = 12*2;
        for (int i = 0; i < NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * (2.0 * DX_PI / NUM) + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0;
            // kind: 青相は青小玉、赤相は赤小玉
            if (pEnemyShotSet->kind == 0)
                pEnemyShot->kind = img_enemyShotSmallBall[4];
            else
                pEnemyShot->kind = img_enemyShotSmallBall[0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Zendera_Grok()
{
    // phase: 0=青相準備, 1=青相実行, 2=赤相準備, 3=赤相実行 をループ
    static int phase;
    static int phase_timer;
    static double base_angle;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 150.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        phase = 0;
        phase_timer = 0;
        base_angle = 0.0;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x = 240.0 + 60.0 * sin(count * 0.015);
        // わずかに上下
        enemy.y = 150.0 + 12.0 * sin(count * 0.01);
    }

    phase_timer++;

    // ---- フェーズ制御 ----
    // 青相: 約 180 フレーム（卍回転 + 蝶展開・帰還）
    // 赤相: 約 150 フレーム（大型卍 + 自機狙い拡散蝶）
    // 間に短いインターバル

    if (phase == 0) {
        // 青相開始
        if (phase_timer == 1) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        if (phase_timer >= 40) {
            phase = 1;
            phase_timer = 0;
            base_angle = 0.0;
        }
    }
    else if (phase == 1) {
        // 青卍レーザー（持続）
        if (phase_timer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotBlueManjiLaser;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->param_d[0] = base_angle;          // 初期角度
            pSet->param_d[1] = -0.025;              // 反時計回り角速度
            pSet->param_d[2] = 0.0;                 // 長さ補正
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 青い蝶を複数波で展開
        if (phase_timer == 10 || phase_timer == 50 || phase_timer == 90) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotBlueButterfly;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = base_angle + (phase_timer * 0.1);
            pSet->kind = 0;
            pSet->param_d[0] = 3.5;   // 展開初速
            pSet->param_d[1] = 45.0;  // 展開フレーム目安
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 装飾小玉
        if (phase_timer % 30 == 15) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotManjiDecor;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = base_angle;
            pSet->kind = 0; // 青
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        if (phase_timer >= 180) {
            phase = 2;
            phase_timer = 0;
        }
    }
    else if (phase == 2) {
        // 赤相準備（短い溜め）
        if (phase_timer == 1) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        if (phase_timer >= 30) {
            phase = 3;
            phase_timer = 0;
            base_angle = DX_PI / 8.0;
        }
    }
    else if (phase == 3) {
        // 赤卍レーザー（持続・大型）
        if (phase_timer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotRedManjiLaser;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 1;
            pSet->param_d[0] = base_angle;
            pSet->param_d[1] = 0.035;   // 時計回り
            pSet->param_d[2] = 8.0;     // 腕を長く
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 赤い自機狙い拡散蝶を複数波
        if (phase_timer == 15 || phase_timer == 45 || phase_timer == 75 || phase_timer == 105) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotRedButterfly;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 1;
            pSet->param_d[0] = 2.8;   // 基準速度
            pSet->param_d[1] = 1.8;   // 水平拡散係数
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 装飾小玉（赤）
        if (phase_timer % 25 == 10) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotManjiDecor;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = base_angle;
            pSet->kind = 1; // 赤
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        if (phase_timer >= 150) {
            phase = 0;
            phase_timer = 0;
        }
    }
}