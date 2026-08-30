// enemyPat_Tmp.cpp
// 信号機モチーフ弾幕（Traffic Signal Cycle）
// 赤：高密度低速の壁 / 黄：加速する注意弾 / 緑：レーン状の疎弾
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// -------------------------------------------------
// 赤信号：左右から迫る高密度の赤壁
// -------------------------------------------------
static void ShotRedWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 左右どちらかから壁を生成（kind の偶奇で左右を切り替え）
        int side = (pEnemyShotSet->kind % 2 == 0) ? -1 : 1;
        double baseX = (side < 0) ? -20.0 : 500.0;
        double baseY = pEnemyShotSet->y;

        const int N = 14 * 2;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = baseX;
            pEnemyShot->y = baseY + (i - (N - 1) / 2.0) * 18.0 * 14 / N + (GetRand(6) - 3);
            // 中央方向へゆっくり進む
            pEnemyShot->muki = (side < 0) ? 0.0 : DX_PI;
            pEnemyShot->speed = 1.35 + GetRand(20) / 100.0;
            // 赤の中玉
            pEnemyShot->kind = img_enemyShotMediumBall[0];
            pEnemyShot->margin = 30.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動（速度は固定、徐々に中央で少し減速させてもよい）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki) * 0.15; // ほぼ水平
        pShot = pShot->next;
    }
}

// -------------------------------------------------
// 黄信号：赤の隙間を埋める加速黄弾
// -------------------------------------------------
static void ShotYellowCaution(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 自機狙い＋ランダムオフセットで複数発
        double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(20) - 10);
            pEnemyShot->muki = baseMuki + (GetRand(50) - 25) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.0; // 初期速度（後で加速）
            // 黄の中玉
            pEnemyShot->kind = img_enemyShotMediumBall[1];
            // param_d[0] を加速用に使用
            pEnemyShot->param_d[0] = 0.045 + GetRand(15) / 1000.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 徐々に加速
        pShot->speed += pShot->param_d[0];
        if (pShot->speed > 5.5) pShot->speed = 5.5;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// -------------------------------------------------
// 緑信号：中央に縦レーンを作る疎な緑弾
// -------------------------------------------------
static void ShotGreenLane(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 左右の壁寄りに弾を配置し、中央に通行可能なレーンを残す
        // kind の値でレーン幅や配置を少し変化
        int offsetBase = (pEnemyShotSet->kind % 3) * 8;
        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < 5; i++) {
                pEnemyShot = new sEnemyShot;
                // 左右に寄せて中央を空ける
                pEnemyShot->x = 240.0 + side * (95.0 + offsetBase + GetRand(25));
                pEnemyShot->y = pEnemyShotSet->y - 30.0 - i * 22.0;
                // ほぼ真下、わずかに外側へ開く
                pEnemyShot->muki = DX_PI / 2.0 + side * 0.08;
                pEnemyShot->speed = 1.8 + GetRand(40) / 100.0;
                // 緑の中玉（一部小玉で変化）
                if (i % 2 == 0)
                    pEnemyShot->kind = img_enemyShotMediumBall[2];
                else
                    pEnemyShot->kind = img_enemyShotSmallBall[2];

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

// -------------------------------------------------
// 敵本体パターン
// 信号サイクル：赤(180f) → 黄(60f) → 緑(180f) を繰り返す
// -------------------------------------------------
void EnemyPat_TrafficLight_Grok()
{
    static int phase;          // 0:赤 1:黄 2:緑
    static int phaseTimer;
    static int shotIndex;

    // ゲーム画面は 480x480
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;             // 赤から開始
        phaseTimer = 0;
        shotIndex = 0;
    }

    // 敵はほぼ静止（わずかに左右に揺れる程度）
    enemy.x = 240.0 + sin(count / 40.0) * 12.0;
    enemy.y = 50.0;

    // フェーズ管理（赤180 / 黄60 / 緑180）
    phaseTimer++;
    if (phase == 0 && phaseTimer >= 180) {      // 赤終了
        phase = 1;
        phaseTimer = 0;
    }
    else if (phase == 1 && phaseTimer >= 60) {  // 黄終了
        phase = 2;
        phaseTimer = 0;
    }
    else if (phase == 2 && phaseTimer >= 180) { // 緑終了 → 赤へ
        phase = 0;
        phaseTimer = 0;
    }

    // ----- 赤信号フェーズ -----
    if (phase == 0) {
        // 左右交互に壁を発射（約0.4秒ごと）
        if (phaseTimer % 24 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotRedWall;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = 120.0 + phaseTimer * 1.32; // 少し上下にばらけさせる
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = shotIndex++;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        // 赤の間は自機の移動をわずかに制限（「止まれ」感）
        // 必要に応じてコメントアウト可能
        double dx = player.x - 240.0;
        if (dx > 80.0) {
            spawnForceParticles(player.x, player.y, -5, 0);
            spawnForceParticles(player.x, player.y, -5, 0);
            spawnForceParticles(player.x, player.y, -5, 0);
            player.x = 240.0 + 80.0;
            spawnForceParticles(player.x, player.y, -5, 0);
            spawnForceParticles(player.x, player.y, -5, 0);
            spawnForceParticles(player.x, player.y, -5, 0);
        }
        if (dx < -80.0) {
            spawnForceParticles(player.x, player.y, 5, 0);
            spawnForceParticles(player.x, player.y, 5, 0);
            spawnForceParticles(player.x, player.y, 5, 0);
            player.x = 240.0 - 80.0;
            spawnForceParticles(player.x, player.y, 5, 0);
            spawnForceParticles(player.x, player.y, 5, 0);
            spawnForceParticles(player.x, player.y, 5, 0);
        }
    }
    // ----- 黄信号フェーズ -----
    else if (phase == 1) {
        // 短時間に集中して注意弾を飛ばす
        if (phaseTimer % 12 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotYellowCaution;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 15.0;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = shotIndex++;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    // ----- 緑信号フェーズ -----
    else if (phase == 2) {
        // レーン形成弾を定期発射
        if (phaseTimer % 28 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotGreenLane;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->muki = DX_PI / 2.0;
            pEnemyShotSet->kind = shotIndex++;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}