#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
//  使用素材一覧（本パターンで使用するもの）
// ------------------------------------------------------------
// 効果音:
//   sound_enemyCharge          … 突進予告・仕切り
//   sound_enemyShot_medium    … 土俵形成・砂埃弾
//   sound_enemyShot_heavy     … 寄り切り弾幕
//   sound_enemyShot_extreme   … 最終寄り切り
//
// 弾種:
//   img_enemyShotSmallBall[6]     … 土俵線（白・小玉）
//   img_enemyShotMediumBall[8]    … 砂埃弾（橙・中玉）
//   img_enemyShotMediumBall[1]    … 砂埃弾（黄・中玉）
//   img_enemyShotLargeBall[8]     … 熱気弾（橙・大玉）
//   img_enemyShotScale[6]         … 呼び出し弾（白・鱗弾）
//   img_enemyShotDiamond[0]       … 張り手弾（赤・菱形）
//   img_enemyShotMediumOval[8]    … 寄り切り収縮弾（橙・中楕円）
//
// 色番号: 0赤 1黄 2緑 3シアン 4青 5マゼンタ 6白 7黒 8橙
// ============================================================

// ------------------------------------------------------------
// 土俵形成用ショットセット
// 画面中央 (240,240) を中心に半径約 220 の円を白い小玉で形成
// 形成後はほぼ静止（わずかに回転）
// ------------------------------------------------------------
static void ShotDohyoRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double centerX = 240.0;
    const double centerY = 240.0;
    const double radius = 220.0;
    const int    bulletNum = 72;          // 円周上の弾数

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < bulletNum; i++) {
            double ang = (DX_PI * 2.0) * i / bulletNum;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = centerX + radius * cos(ang);
            pEnemyShot->y = centerY + radius * sin(ang);
            pEnemyShot->muki = ang + DX_PI / 2.0;   // 接線方向
            pEnemyShot->speed = 0.15;                // ごくゆっくり回転
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白・小玉
            pEnemyShot->param_d[0] = radius;         // 半径を保持
            pEnemyShot->param_d[1] = ang;            // 初期角度を保持
            pEnemyShot->param_d[2] = centerX;
            pEnemyShot->param_d[3] = centerY;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：円軌道上をゆっくり回転させる
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double r = pShot->param_d[0];
        double ang = pShot->param_d[1] + pShot->speed * 0.02; // 緩やかに回転
        pShot->param_d[1] = ang;
        pShot->x = pShot->param_d[2] + r * cos(ang);
        pShot->y = pShot->param_d[3] + r * sin(ang);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 砂埃弾（突進中に撒く）
// ------------------------------------------------------------
static void ShotSandDust(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12/4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(60) - 30);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(40) - 20);
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(100) - 50) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.8 + GetRand(120) / 100.0;
            // 橙 or 黄 の中玉をランダム
            if (GetRand(1) == 0)
                pEnemyShot->kind = img_enemyShotMediumBall[8];
            else
                pEnemyShot->kind = img_enemyShotMediumBall[1];

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

// ------------------------------------------------------------
// 熱気弾（近接時に放射）
// ------------------------------------------------------------
static void ShotHeatWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16/2; i++) {
            double ang = (DX_PI * 2.0) * i / 16.0*2 + pEnemyShotSet->muki;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 2.5 + GetRand(80) / 100.0;
            pEnemyShot->kind = img_enemyShotLargeBall[8]; // 橙・大玉

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

// ------------------------------------------------------------
// 呼び出し弾（低密度の白鱗弾）
// ------------------------------------------------------------
static void ShotYobidashi(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(200) - 100);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(80) - 40);
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x)
                + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.2 + GetRand(60) / 100.0;
            pEnemyShot->kind = img_enemyShotScale[6]; // 白・鱗弾

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

// ------------------------------------------------------------
// 寄り切り収縮弾（土俵線が内側に収縮する演出）
// ------------------------------------------------------------
static void ShotYorikiri(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double centerX = 240.0;
    const double centerY = 240.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const int bulletNum = 48/2;
        for (int i = 0; i < bulletNum; i++) {
            double ang = (DX_PI * 2.0) * i / bulletNum;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = centerX + 210.0 * cos(ang);
            pEnemyShot->y = centerY + 210.0 * sin(ang);
            pEnemyShot->muki = ang + DX_PI;          // 中心方向
            pEnemyShot->speed = 1.4;
            pEnemyShot->kind = img_enemyShotMediumOval[8]; // 橙・中楕円
            pEnemyShot->param_d[0] = 210.0;          // 初期半径

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 半径を徐々に縮める
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double r = pShot->param_d[0] - 0.8;
        if (r < 30.0) r = 30.0;
        pShot->param_d[0] = r;
        double ang = atan2(pShot->y - centerY, pShot->x - centerX);
        pShot->x = centerX + r * cos(ang);
        pShot->y = centerY + r * sin(ang);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_Sumou_Grok()
{
    // フェーズ管理
    // 0: 土俵形成
    // 1: 仕切り・呼び出し
    // 2: 突進ループ
    // 3: 寄り切り（最終）
    static int phase;
    static int phaseTimer;
    static int chargeCount;          // 突進回数
    static double chargeTargetX, chargeTargetY;
    static int heatCooldown;

    // --------------------------------------------------------
    // 初期化
    // --------------------------------------------------------
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseTimer = 0;
        chargeCount = 0;
        heatCooldown = 0;
    }

    // --------------------------------------------------------
    // フェーズ進行
    // --------------------------------------------------------
    phaseTimer++;

    // ===== フェーズ0：土俵形成（約 90 フレーム） =====
    if (phase == 0) {
        if (phaseTimer == 1) {
            // 土俵リングを1セットだけ生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotDohyoRing;
            pSet->x = 240.0;
            pSet->y = 240.0;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        // ボスは中央上で待機
        enemy.x = 240.0;
        enemy.y = 80.0;

        if (phaseTimer >= 90) {
            phase = 1;
            phaseTimer = 0;
        }
    }
    // ===== フェーズ1：仕切り・呼び出し（約 120 フレーム） =====
    else if (phase == 1) {
        // 低密度の呼び出し弾をたまに出す
        if (phaseTimer % 40 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotYobidashi;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 20.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        // ボスはわずかに左右に揺れる
        enemy.x = 240.0 + sin(phaseTimer * 0.08) * 30.0;
        enemy.y = 80.0;

        if (phaseTimer >= 120) {
            phase = 2;
            phaseTimer = 0;
            chargeCount = 0;
        }
    }
    // ===== フェーズ2：突進ループ（最大4回） =====
    else if (phase == 2) {
        // 突進開始タイミング
        if (phaseTimer == 1 || (phaseTimer > 1 && phaseTimer % 90 == 1)) {
            if (chargeCount >= 4) {
                phase = 3;
                phaseTimer = 0;
            }
            else {
                // 突進目標を現在の自機位置に設定
                chargeTargetX = player.x;
                chargeTargetY = player.y;
                chargeCount++;

                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            }
        }

        if (phase < 3) {
            // 突進移動（高速追尾）
            double dx = chargeTargetX - enemy.x;
            double dy = chargeTargetY - enemy.y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 4.0) {
                double spd = 6.5;
                enemy.x += spd * dx / dist;
                enemy.y += spd * dy / dist;
            }

            // 突進中に砂埃弾を撒く
            if (phaseTimer % 8 == 1) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotSandDust;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = atan2(dy, dx);
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
    // ===== フェーズ3：寄り切り（最終弾幕） =====
    else if (phase == 3) {
        if (phaseTimer == 1) {
            // 寄り切り収縮弾を生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotYorikiri;
            pSet->x = 240.0;
            pSet->y = 240.0;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            // 最後の大突進目標
            chargeTargetX = player.x;
            chargeTargetY = player.y;
        }

        // 中央へ戻りつつ最後の突進
        if (phaseTimer < 60) {
            enemy.x += (240.0 - enemy.x) * 0.08;
            enemy.y += (180.0 - enemy.y) * 0.08;
        }
        else {
            double dx = chargeTargetX - enemy.x;
            double dy = chargeTargetY - enemy.y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 3.0) {
                double spd = 7.0;
                enemy.x += spd * dx / dist;
                enemy.y += spd * dy / dist;
            }
        }

        // 一定時間後にループ（仕切りフェーズへ戻る）
        if (phaseTimer >= 180) {
            phase = 1;
            phaseTimer = 0;
            chargeCount = 0;
            // ボスを初期位置付近へ戻す
            enemy.x = 240.0;
            enemy.y = 80.0;
        }
    }

    // --------------------------------------------------------
    // 近接時の斥力処理（全フェーズ共通）
    // player.x / player.y を直接変更して斥力を発生させる
    // --------------------------------------------------------
    {
        double dx = player.x - enemy.x;
        double dy = player.y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);
        const double repulseThreshold = 110.0;   // この距離以下で斥力発生

        if (dist < repulseThreshold && dist > 1.0) {
            // 斥力の強さ（近いほど強い）
            double pullSpeed = (repulseThreshold - dist) / repulseThreshold * 4.5;
            // 斥力の向き：敵→自機 の方向（この方向へ押し出す＝斥力）
            double angle = atan2(dy, dx);

            double ratio = 3.0; // 比例定数
            spawnForceParticles(player.x, player.y,
                pullSpeed * cos(angle) * ratio,
                pullSpeed * sin(angle) * ratio);

            // 自機位置を直接変更して斥力を発生
            player.x += pullSpeed * cos(angle);
            player.y += pullSpeed * sin(angle);

            // 近接中は熱気弾を放射（クールダウン付き）
            if (heatCooldown <= 0) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotHeatWave;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = angle;
                pSet->kind = 0;
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;

                heatCooldown = 25;
            }
        }
        if (heatCooldown > 0) heatCooldown--;
    }
}