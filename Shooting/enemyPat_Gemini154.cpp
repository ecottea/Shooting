#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// 弾幕：土俵（勝負俵）の描画
// 画面外周ギリギリに白い小玉を円状に並べて定着させる
static void ShotDohyo(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 180; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = i * DX_PI * 2.0 / 180.0;
            // 画面中心(240, 240)から半径220の円を描く
            pEnemyShot->x = 240.0 + 220.0 * cos(angle);
            pEnemyShot->y = 240.0 + 220.0 * sin(angle);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0; // 動かさずに定着
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白の小玉(塩のイメージ)

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// 弾幕：突進の予兆（見合い）
// ボスの向いている方向（自機狙い）へ赤い短レーザーを放つ
static void ShotYochou(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 15.0; // 高速のレーザー
        pEnemyShot->kind = img_enemyShotLaser[0]; // 赤の短レーザー

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

// 弾幕：突っ張り
// 突進中、進行方向の左右に扇状に赤の中楕円弾を放つ
static void ShotTsuppari(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base_angles[2] = { pEnemyShotSet->muki + DX_PI / 2.0, pEnemyShotSet->muki - DX_PI / 2.0 };
        for (int side = 0; side < 2; side++) {
            for (int i = 0; i < 3; i++) { // 手のひらを模して3WAYずつ
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // 左右それぞれの方向から少し広げる
                pEnemyShot->muki = base_angles[side] + (i - 1) * 0.3;
                pEnemyShot->speed = 4.0;
                pEnemyShot->kind = img_enemyShotMediumOval[0]; // 赤の中楕円弾

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾幕：猫騙し・衝撃波
// 突進の終点で全方位に赤の大玉と鱗弾のリングを放つ
static void ShotShockwave(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 大玉の衝撃波
        for (int i = 0; i < 24; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * DX_PI * 2.0 / 24.0;
            pEnemyShot->speed = 6.0;
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤の大玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // 鱗弾の衝撃波(少し遅れて広がる)
        for (int i = 0; i < 36; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * DX_PI * 2.0 / 36.0 + (DX_PI / 36.0);
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotScale[0]; // 赤の鱗弾

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

// （※別途どこかに extern void spawnForceParticles(double x, double y, double vx, double vy); 等が定義されている前提）

// 敵本体のパターン：霊峰『天地開辟の金剛土俵と見えざる圧力』
void EnemyPat_Sumou_Gemini()
{
    static int state;
    static int timer;
    static double target_x, target_y;
    static double start_x, start_y;
    static double repulse_power;

    if (count == 1) {
        // ボス初期化
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        state = 0;
        timer = 0;
        repulse_power = 1.0;

        // 土俵の設置（最初のみ1度だけ呼び出す）
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDohyo;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ----------------------------------------------------
    // 【近接時のみ発動する斥力ギミック】
    // ボスと自機の距離が一定値（160px）以内のときのみ外側へ押し出す
    // ----------------------------------------------------
    double dx = player.x - enemy.x;
    double dy = player.y - enemy.y;
    double dist = sqrt(dx * dx + dy * dy);

    const double REPULSE_RADIUS = 160.0+100; // 斥力が働き始める距離（閾値）

    if (dist < REPULSE_RADIUS && dist > 0.1) {
        // 近づくほど指数関数的に強力になる押し出し力
        double distance_ratio = (REPULSE_RADIUS - dist) / REPULSE_RADIUS; // 0.0(境界) ～ 1.0(ボス中心)
        double force = repulse_power * distance_ratio * distance_ratio * 5.0; // 距離が詰まるほど急激に反発力アップ

        // 座標を押し出し
        player.x += (dx / dist) * force;
        player.y += (dy / dist) * force;

        // ------------------------------------------------
        // 斥力発生のパーティクルエフェクト演出
        // ------------------------------------------------
        double pullSpeed = force;       // 引き寄せの強さ（ここでは押し出す斥力の強さとして扱う）
        double angle = atan2(dy, dx);   // 引き寄せの向き（ここではボスから自機へ向かう角度）
        double ratio = 3.0;             // 比例定数（パーティクルの飛ぶ速度の係数。必要に応じて調整してください）

        spawnForceParticles(player.x, player.y, pullSpeed * cos(angle) * ratio, pullSpeed * sin(angle) * ratio);
    }

    // ----------------------------------------------------
    // 【状態遷移ベースの行動ロジック】
    // ----------------------------------------------------
    switch (state) {
    case 0: // 待機（見合い）
        // 左右にゆったりと揺れる
        enemy.x = 240.0 + 80.0 * sin(timer * DX_PI / 60.0);
        enemy.y = 80.0;

        if (timer >= 120) {
            state = 1;
            timer = 0;
        }
        break;

    case 1: // 予兆（狙い定め）
        if (timer == 1) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            target_x = player.x;
            target_y = player.y;
            start_x = enemy.x;
            start_y = enemy.y;
        }

        // 予告としてレーザー弾をターゲットに向けて数発撃つ
        if (timer % 10 == 0 && timer < 40) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotYochou;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = atan2(target_y - start_y, target_x - start_x);
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 力を溜めるようにブルブル震える
        enemy.x = start_x + (GetRand(4) - 2);
        enemy.y = start_y + (GetRand(4) - 2);

        if (timer >= 60) {
            state = 2;
            timer = 0;
            // 突進先を自機のさらに奥（画面端方面）へ延長
            double dash_angle = atan2(target_y - start_y, target_x - start_x);
            double dash_dist = 300.0; // ぶちかましの距離
            target_x = start_x + dash_dist * cos(dash_angle);
            target_y = start_y + dash_dist * sin(dash_angle);
        }
        break;

    case 2: // 突進（ぶちかまし）＆突っ張り弾幕
        if (timer == 1) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        {
            // 高速で突進 (20フレームで到達)
            double progress = (double)timer / 20.0;
            if (progress > 1.0) progress = 1.0;
            enemy.x = start_x + (target_x - start_x) * progress;
            enemy.y = start_y + (target_y - start_y) * progress;

            // 突進中、左右に張り手弾（突っ張り）を出す
            if (timer % 4 == 0) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotTsuppari;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = atan2(target_y - start_y, target_x - start_x);
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }

            if (timer >= 20) {
                state = 3;
                timer = 0;
            }
        }
        break;

    case 3: // 土俵際の衝撃（猫騙し）
        if (timer == 1) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            // ドンッ！という衝撃で一瞬だけ自機を強烈に押し出す
            repulse_power = 3.0;

            // 全方位に衝撃波を放つ
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotShockwave;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 10フレーム後に斥力を元に戻す
        if (timer >= 10) {
            repulse_power = 1.0;
        }

        if (timer >= 40) {
            state = 4;
            timer = 0;
            start_x = enemy.x;
            start_y = enemy.y;
        }
        break;

    case 4: // 跳躍して元の位置に戻る
    {
        double progress = (double)timer / 30.0;
        if (progress > 1.0) progress = 1.0;

        // イージングをかけてフワッと戻る
        double ease = 1.0 - pow(1.0 - progress, 3.0);
        enemy.x = start_x + (240.0 - start_x) * ease;
        enemy.y = start_y + (80.0 - start_y) * ease;

        if (timer >= 30) {
            state = 0;
            timer = 0;
        }
    }
    break;
    }

    timer++;
}