// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>


// ============================================================
// 新規実装：ドップラー効果をモチーフにした弾幕
// ============================================================

// 弾幕：ドップラーシフト
static void ShotDoppler(sEnemyShotSet* pEnemyShotSet)
{
    // 発射処理 (count == 0 の時)
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int numShots = 16 * 3; // 扇状にばら撒く弾数
        for (int i = 0; i < numShots; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 自機狙いを基準に、扇状に角度をばらつかせる
            double angle = pEnemyShotSet->muki + (i - numShots / 2) * (DX_PI / 8.0 / 3);

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;

            // 生成時の自機との距離で初期パラメータを決定
            double dist = hypot(player.x - pEnemyShot->x, player.y - pEnemyShot->y);
            double ratio = dist / 600.0; // 画面対角線(約678)を基準に正規化
            if (ratio > 1.0) ratio = 1.0;
            if (ratio < 0.0) ratio = 0.0;

            // 色: 0:赤, 1:黄, 2:緑, 3:シアン, 4:青
            // 距離が遠い(ratio=1.0)ほど青(4)、近い(ratio=0.0)ほど赤(0)になる
            int color = (int)(4.0 * ratio);
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            // 速度: 近いほど速い (2.0 〜 7.0)
            pEnemyShot->speed = 2.0 + 5.0 * (1.0 - ratio);

            // 連結リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動とドップラー効果の動的更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本的な移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // ドップラー効果の動的更新（毎フレーム、自機との距離を再計算）
        double dist = hypot(player.x - pShot->x, player.y - pShot->y);
        double ratio = dist / 600.0;
        if (ratio > 1.0) ratio = 1.0;
        if (ratio < 0.0) ratio = 0.0;

        // 色の変化: 自機に近づくにつれて赤へ、遠ざかると青へシフト
        int color = (int)(4.0 * ratio);
        pShot->kind = img_enemyShotMediumBall[color];

        // 速度の変化: 自機に近づくにつれて加速し、遠ざかると減速する（滑らかに補間）
        double targetSpeed = 2.0 + 5.0 * (1.0 - ratio);
        pShot->speed += (targetSpeed - pShot->speed) * 0.15;

        pShot = pShot->next;
    }
}

// 敵本体のパターン：ドップラーシフト用
void EnemyPat_Doppler_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0; // 少し下げて発射位置を調整
        enemy.maxHp = enemy.hp = 200; // パターンを少し長くするためのHP
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり移動（ドップラー効果の波源移動を演出）
        enemy.x += 1.5 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    // 一定間隔で発射
    if (count % 20 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDoppler;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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