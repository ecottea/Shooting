// enemyPat_charge.cpp
// 「猪突猛進」：ボス自身が体当たりしてくる突進弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ==================== 調整用定数 ====================
// ボスの初期位置・体力
static const double BOSS_HOME_X = 240.0;
static const double BOSS_HOME_Y = 100.0;
static const int    BOSS_MAX_HP = 200;

// ①狙いフェーズ（ロックオン＆予告弾）
static const int    AIM_DURATION = 45;   // 狙いフェーズの長さ(フレーム)
static const int    AIM_TRACER_INTERVAL = 6;    // 予告弾を撒く間隔
static const double AIM_TRACER_SPEED = 1.2;  // 予告弾の速度

// ②突進フェーズ
static const int    CHARGE_DURATION = 18;   // 突進フェーズの長さ(フレーム)
static const double CHARGE_DISTANCE = 520.0;// 突進の移動距離
static const int    CHARGE_TRAIL_INTERVAL = 4;    // 置き弾を撒く間隔
static const double CHARGE_TRAIL_SPEED = 0.6;  // 置き弾の速度

// ③衝突フェーズ
static const int    IMPACT_HOLD = 10;   // 衝撃波発生後に静止している時間
static const int    IMPACT_BULLET_COUNT = 20;   // 衝撃波弾の本数
static const double IMPACT_BULLET_SPEED = 2.0;  // 衝撃波弾の速度

// ④帰還フェーズ
static const int    RETURN_DURATION = 50;   // 定位置へ戻るのにかかるフレーム数

// ==================== フェーズ定義 ====================
enum EChargePhase {
    PHASE_AIM = 0,
    PHASE_CHARGE,
    PHASE_IMPACT,
    PHASE_RETURN,
};

// ==================== 弾幕パターン ====================

// 予告弾／突進中の置き弾：直進する一発弾
// 細い軌道を示す目的のため、判定の大きいimg_enemyShotLaser(64x4)ではなく
// img_enemyShotBullet(5x2)を採用。
static void ShotStraightBullet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = pEnemyShotSet->param_d[0]; // 速度は生成元で指定
        pEnemyShot->kind = img_enemyShotBullet[pEnemyShotSet->kind % 9]; // kindを色として利用(0-8)

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 衝突時の全方位衝撃波
static void ShotImpactBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // param_d[0]には衝撃波全体の初期回転角(ランダム)が入っている
        for (int i = 0; i < IMPACT_BULLET_COUNT; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->param_d[0] + 2.0 * DX_PI * i / IMPACT_BULLET_COUNT;
            pEnemyShot->speed = IMPACT_BULLET_SPEED;
            pEnemyShot->kind = img_enemyShotMediumBall[i % 9];
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 予告弾／置き弾を1発だけ生成するヘルパー
static void SpawnStraightBullet(double x, double y, double muki, double speed, int colorIndex)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotStraightBullet;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = colorIndex;
    pEnemyShotSet->param_d[0] = speed;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ==================== 敵本体のパターン ====================
// 「猪突猛進」：ロックオン→突進→衝突→帰還 を繰り返す体当たり突進弾幕
void EnemyPat_Tackle_Claude()
{
    static EChargePhase phase;
    static int    phaseTimer;      // 現在フェーズ内の経過フレーム
    static double lockedMuki;      // 突進方向（狙いフェーズ開始時に確定）
    static double chargeStartX, chargeStartY;   // 突進開始位置
    static double chargeTargetX, chargeTargetY; // 突進到達位置
    static double impactX, impactY;             // 衝突地点（衝撃波の発生源）

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = BOSS_HOME_X;
        enemy.y = BOSS_HOME_Y;
        enemy.maxHp = enemy.hp = BOSS_MAX_HP;

        phase = PHASE_AIM;
        phaseTimer = 0;
        lockedMuki = 0.0;
    }

    switch (phase) {
    case PHASE_AIM:
        if (phaseTimer == 0) {
            // 自機の現在地を1回だけロックオンして突進方向を確定
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            lockedMuki = atan2(player.y - enemy.y, player.x - enemy.x);
        }

        // ロックオン方向へ、狙いを示す予告弾(赤)を一定間隔で発射
        if (phaseTimer % AIM_TRACER_INTERVAL == 0) {
            SpawnStraightBullet(enemy.x, enemy.y, lockedMuki, AIM_TRACER_SPEED, 0); // 0:赤
        }

        phaseTimer++;
        if (phaseTimer >= AIM_DURATION) {
            chargeStartX = enemy.x;
            chargeStartY = enemy.y;
            chargeTargetX = enemy.x + cos(lockedMuki) * CHARGE_DISTANCE;
            chargeTargetY = enemy.y + sin(lockedMuki) * CHARGE_DISTANCE;

            phase = PHASE_CHARGE;
            phaseTimer = 0;
        }
        break;

    case PHASE_CHARGE:
    {
        double t = (double)phaseTimer / (double)CHARGE_DURATION;
        if (t > 1.0) t = 1.0;
        enemy.x = chargeStartX + (chargeTargetX - chargeStartX) * t;
        enemy.y = chargeStartY + (chargeTargetY - chargeStartY) * t;

        // 突進の軌跡上に置き弾(橙)を残す
        if (phaseTimer % CHARGE_TRAIL_INTERVAL == 0) {
            SpawnStraightBullet(enemy.x, enemy.y, lockedMuki, CHARGE_TRAIL_SPEED, 8); // 8:橙
        }

        phaseTimer++;
        if (phaseTimer >= CHARGE_DURATION) {
            impactX = enemy.x;
            impactY = enemy.y;

            phase = PHASE_IMPACT;
            phaseTimer = 0;
        }
        break;
    }

    case PHASE_IMPACT:
        if (phaseTimer == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotImpactBurst;
            pEnemyShotSet->x = impactX;
            pEnemyShotSet->y = impactY;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = 0;
            // GetRand(x)は0からxまでのx+1種類の整数を返すため、0〜359度をランダムに取得
            pEnemyShotSet->param_d[0] = GetRand(359) / 180.0 * DX_PI;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        phaseTimer++;
        if (phaseTimer >= IMPACT_HOLD) {
            phase = PHASE_RETURN;
            phaseTimer = 0;
        }
        break;

    case PHASE_RETURN:
    {
        double t = (double)phaseTimer / (double)RETURN_DURATION;
        if (t > 1.0) t = 1.0;
        enemy.x = impactX + (BOSS_HOME_X - impactX) * t;
        enemy.y = impactY + (BOSS_HOME_Y - impactY) * t;

        phaseTimer++;
        if (phaseTimer >= RETURN_DURATION) {
            phase = PHASE_AIM;
            phaseTimer = 0;
        }
        break;
    }

    default:
        break;
    }
}