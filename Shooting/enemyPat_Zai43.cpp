// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ファイルスコープの共有変数
// 敵本体関数と弾幕更新関数の間で状態を共有するために使用
// ============================================================
static int    g_phase = 0;       // 0:散布, 1:予兆, 2:突進吸引, 3:復帰
static int    g_prevPhase = 0;   // フェーズ遷移の瞬間を検知するための変数
static double g_ramAngle = 0.0;  // 体当たりの突進方向（ラジアン）
static double g_ramStartX = 0.0; // 突進開始時のボスX座標
static double g_ramStartY = 0.0; // 突進開始時のボスY座標


// ============================================================
// 弾幕更新関数：重力彗星用
// ============================================================
static void ShotGravityComet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 散布フェーズから突進(吸引)フェーズに切り替わった瞬間の初速設定
        // ※以降は速度ベクトル(param_d)を使って加速度運動を行う
        if (g_prevPhase == 0 && g_phase == 2) {
            pEnemyShot->param_d[0] = pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->param_d[1] = pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        if (g_phase == 2) {
            // 【吸引処理】
            // ボスの突進ルート（無限に伸びる直線）上の、弾に最も近い点を目標点とする
            double dx = cos(g_ramAngle);
            double dy = sin(g_ramAngle);
            double rx = pEnemyShot->x - g_ramStartX;
            double ry = pEnemyShot->y - g_ramStartY;

            // 内積を利用して目標点を算出
            double dot = rx * dx + ry * dy;
            double tx = g_ramStartX + dx * dot;
            double ty = g_ramStartY + dy * dot;

            // 目標点に向かうベクトルと距離
            double toTargetX = tx - pEnemyShot->x;
            double toTargetY = ty - pEnemyShot->y;
            double toTargetDist = sqrt(toTargetX * toTargetX + toTargetY * toTargetY);

            if (toTargetDist > 1.0) {
                // 重力（加速度）として目標点に向かうベクトルを加算
                double accel = 1.0;
                pEnemyShot->param_d[0] += (toTargetX / toTargetDist) * accel;
                pEnemyShot->param_d[1] += (toTargetY / toTargetDist) * accel;
            }
            else {
                // ルート上に乗ったら速度を急激に減衰させて「残滓」として留める
                pEnemyShot->param_d[0] *= 0.85;
                pEnemyShot->param_d[1] *= 0.85;
            }
            
            // 加速度方式での移動
            pEnemyShot->x += pEnemyShot->param_d[0];
            pEnemyShot->y += pEnemyShot->param_d[1];
            pEnemyShot->margin = 400;
        }
        else {
            // 【通常時（散布時）の移動】
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            pEnemyShot->margin = 20;
        }

        pEnemyShot = pEnemyShot->next;
    }

    // フェーズ遷移検知用の更新
    g_prevPhase = g_phase;
}


// ============================================================
// 敵本体のパターン関数
// ============================================================
void EnemyPat_Tackle_Zai()
{
    static int phase = 0;
    static int shotTimer = 0;
    static int waitCount = 0;
    static double targetX = 240.0;
    static double targetY = 40.0;

    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        phase = 0;
        shotTimer = 0;
        waitCount = 0;
        g_phase = 0;
        g_prevPhase = 0;
    }

    switch (phase) {
    case 0: // 散布フェーズ
        shotTimer++;
        // ゆるやかに左右に揺れながらゆっくりした弾をばら撒く
        enemy.x += sin(shotTimer * 0.05) * 1.5;

        if (shotTimer % 3 == 0) {
            // 1回の発射で2つの弾幕セットを生成
            for (int i = 0; i < 2; i++) {
                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotGravityComet;
                pEnemyShotSet->x = enemy.x;
                pEnemyShotSet->y = enemy.y + 10.0;

                // GetRand(628) は 0～628 の整数を返すため、100.0で割ると 0.00～6.28 になる
                pEnemyShotSet->muki = GetRand(628) / 100.0;
                pEnemyShotSet->kind = 0;

                pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

                // 弾を1つ生成
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = pEnemyShotSet->muki;
                pEnemyShot->speed = 1.5 + GetRand(10) / 10.0; // 1.5 ～ 2.5 (非常にゆっくり)

                // 素材の選定: 鱗弾(4.0x3.0)のシアン(3)
                pEnemyShot->kind = img_enemyShotScale[3];
                
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

                // グローバルリストへの登録
                pEnemyShotSet->prev = enemyShotSetHead.prev;
                pEnemyShotSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pEnemyShotSet;
                enemyShotSetHead.prev = pEnemyShotSet;
            }
        }

        // 120フレーム経過で予兆フェーズへ
        if (shotTimer >= 120) {
            phase = 1;
            waitCount = 0;
            // プレイヤーの現在位置に向かって突進方向を固定
            g_ramAngle = atan2(player.y - enemy.y, player.x - enemy.x);
            g_ramStartX = enemy.x;
            g_ramStartY = enemy.y;
        }
        break;

    case 1: // 予兆フェーズ
        waitCount++;

        // ※注意: 描画処理はメインループの描画タイミングで行うのが理想的ですが、
        // もしここで予兆線を描画する場合は以下のようにします。
        // DrawLine((int)g_ramStartX, (int)g_ramStartY, 
        //          (int)(g_ramStartX + cos(g_ramAngle) * 600), 
        //          (int)(g_ramStartY + sin(g_ramAngle) * 600), 
        //          GetColor(255, 50, 50), 2);
        if (waitCount == 1) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        if (waitCount >= 60) { // 1秒間の予兆時間
            phase = 2;
            g_phase = 2; // 弾幕関数に吸引を開始させる

            // 重い効果音を鳴らす
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        break;

    case 2: // 突進＆吸引フェーズ
        // ボスを高速移動させる
        enemy.x += 18.0 * cos(g_ramAngle);
        enemy.y += 18.0 * sin(g_ramAngle);

        // 画面外へ出たら復帰フェーズへ
        if (enemy.x < -50.0 || enemy.x > 530.0 || enemy.y < -50.0 || enemy.y > 530.0) {
            phase = 3;
            // 復帰先を画面上部のランダムな位置に決定
            // GetRand(240) は 0～240 を返すので、120～360の範囲になる
            targetX = 120.0 + (double)GetRand(240);
            targetY = 40.0;
        }
        break;

    case 3: // 復帰フェーズ
    {
        double dx = targetX - enemy.x;
        double dy = targetY - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < 5.0) {
            enemy.x = targetX;
            enemy.y = targetY;
            phase = 0;       // 散布フェーズに戻る
            shotTimer = 0;   // タイマーリセット
            g_phase = 0;     // 弾幕関数に吸引の終了を通知
        }
        else {
            // 一定速度で復帰位置に向かう
            enemy.x += (dx / dist) * 8.0;
            enemy.y += (dy / dist) * 8.0;
        }
    }
    break;
    }
}