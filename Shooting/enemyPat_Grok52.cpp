// enemyPat_Tmp.cpp
// 蜂モチーフ弾幕パターン: 【蜂の巣攻勢 - Honeycomb Swarm】
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// 蜂の巣攻勢用 弾幕更新関数
// =============================================
static void ShotHoneycombSwarm(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int phase = pEnemyShotSet->count;

    // 効果音（初回のみ）
    if (phase == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // Phase 0〜2: 六角形から蜂弾を波状放出（3回）
    if (phase >= 0 && phase <= 5) {
        if (true) {
            // 六角形の6方向から同時発射
            for (int i = 0; i < 6; i++) {
                pEnemyShot = new sEnemyShot;

                double baseAngle = pEnemyShotSet->muki + i * (DX_PI / 3.0); // 60度間隔

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAngle + (GetRand(40) - 20) / 180.0 * DX_PI * 0.6; // 軽いランダム性

                // 蜂らしい速度（やや遅めで波打つ）
                pEnemyShot->speed = 1.8 + (GetRand(60) / 100.0);

                // 蜂弾として黄色小玉 or 鱗弾を使用
                //if (GetRand(1) == 0) {
                //    pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄色小玉
                //}
                //else {
                    pEnemyShot->kind = img_enemyShotScale[1];     // 黄色鱗弾（蜂らしい）
                //}

                // パラメータで個別挙動を制御
                pEnemyShot->param_i[0] = i;                    // 六角形の位置番号
                pEnemyShot->param_d[0] = (GetRand(100) / 100.0) * 0.08; // 波の振幅係数

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // Phase 3〜: 毒針（太い直線弾）
    if (phase == 45) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 15.0;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x) + i * 0.12;
            pEnemyShot->speed = 3.2 + 5;
            pEnemyShot->kind = img_enemyShotMediumOval[5]; // マゼンタ中楕円（毒針風）
            pEnemyShot->param_i[0] = -1; // 分裂フラグ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾更新（波状移動 + 花粉遅延弾）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;

        // 蜂弾の波状移動
        if (pShot->param_i[0] >= 0) {  // 通常の蜂弾
            double wave = sin(pShot->count * 0.18) * pShot->param_d[0] * 1.8;
            double vx = pShot->speed * cos(pShot->muki);
            double vy = pShot->speed * sin(pShot->muki) + wave;

            pShot->x += vx;
            pShot->y += vy;

            // === 修正：軽いホーミング（不連続性を上方向に移動）===
            if (pShot->count % 8 == 0 && pShot->count < 80) {
                double targetAngle = atan2(player.y - pShot->y, player.x - pShot->x);

                // 角度差を -π〜π の範囲に正規化（最短角度差）
                double diff = targetAngle - pShot->muki;
                while (diff > DX_PI) diff -= DX_PI * 2.0;
                while (diff < -DX_PI) diff += DX_PI * 2.0;

                // 上方向を中心に不連続が発生しにくくなるよう基準を調整
                // 必要に応じてさらに + DX_PI/2 を加減算して調整可能
                pShot->muki += diff * 0.085;   // 0.08 → 0.085 に微調整
            }
        }
        // 毒針の分裂処理
        else if (pShot->param_i[0] == -1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 一定距離進んだら分裂
            if (pShot->count > 28) {
                for (int k = 0; k < 5; k++) {
                    sEnemyShot* split = new sEnemyShot;
                    split->x = pShot->x;
                    split->y = pShot->y;
                    split->muki = pShot->muki + (k - 2) * 0.45;
                    split->speed = 1.6;
                    split->kind = img_enemyShotSmallBall[1]; // 黄色小玉に分裂
                    split->param_i[0] = -2; // 分裂済み

                    split->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    split->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = split;
                    pEnemyShotSet->pEnemyShotHead->prev = split;
                }
                // 元の弾は削除（次ループで消去される）
                pShot->x = -9999;
            }
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = nextShot;
    }
}

// =============================================
// 敵本体パターン関数（指定名）
// =============================================
void EnemyPat_Bee_Grok()
{
    static int muki = 1;
    static int shotTimer = 0;

    if (count == 1) {
        // 初期配置（ゲーム画面 480x480）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotTimer = 30;
    }
    else {
        // 左右往復移動（少しゆったり）
        enemy.x += 1.15 * (double)muki;
        if (count % 110 == 55) muki *= -1;

        // 軽い上下揺れ（蜂らしい）
        enemy.y = 60.0 + sin(count * 0.06) * 12.0;
    }

    // 定期的に新しいShotSetを生成
    shotTimer++;
    if (shotTimer >= 55) {  // 約1.1秒ごとに新しい波
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHoneycombSwarm;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;

        // プレイヤー方向をベースに六角形を展開
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shotTimer = 0;
    }
}