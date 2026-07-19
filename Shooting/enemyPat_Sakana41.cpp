// enemyPat_Tmp.cpp
// ブロック崩し風弾幕パターン（敵本体：EnemyPat_BlockBreak_Sakana）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ブロック崩し風弾幕パターン
static void ShotBlockBreaker(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // パターン開始時（pEnemyShotSet->count == 0）にブロック群を生成
    if (pEnemyShotSet->count == 0) {
        // 効果音：ブロックが崩れるので中〜重めの弾音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // ブロック配置：3行×6列のブロック群
        const int ROWS = 4;
        const int COLS = 10;
        const double BLOCK_WIDTH = 60.0;
        const double BLOCK_HEIGHT = 20.0;
        const double START_X = pEnemyShotSet->x - (COLS - 1) * BLOCK_WIDTH * 0.5;
        const double START_Y = pEnemyShotSet->y;

        for (int row = 0; row < ROWS; ++row) {
            for (int col = 0; col < COLS; ++col) {
                pEnemyShot = new sEnemyShot;

                // ブロックの中心位置
                pEnemyShot->x = START_X + col * BLOCK_WIDTH;
                pEnemyShot->y = START_Y + row * BLOCK_HEIGHT;

                // 弾の種類と色はブロックの種類に対応させる
                int blockType = (row * COLS + col) % 4; // 0:赤, 1:青, 2:黄, 3:緑
                int color = blockType; // 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, ... なので注意

                // 弾の種類一覧:
                // img_enemyShotSmallBall[i]   : 小玉(2.5x2.5)
                // img_enemyShotMediumBall[i]  : 中玉(7.0x7.0)
                // img_enemyShotLargeBall[i]   : 大玉(20.0x20.0)
                // img_enemyShotBullet[i]      : 銃弾(5.0x2.0)
                // img_enemyShotScale[i]       : 鱗弾(4.0x3.0)
                // img_enemyShotDiamond[i]     : 菱形弾(4.5x2.5)
                // img_enemyShotMediumOval[i]  : 中楕円弾(10.5x7.0)
                // img_enemyShotLaser[i]       : 短レーザー(64.0x4.0)

                // ブロックは大きめの弾で表現
                switch (blockType) {
                case 0: // 赤ブロック：中玉
                    pEnemyShot->kind = img_enemyShotMediumBall[color];
                    break;
                case 1: // 青ブロック：中楕円弾
                    pEnemyShot->kind = img_enemyShotMediumOval[color];
                    break;
                case 2: // 黄ブロック：大玉
                    pEnemyShot->kind = img_enemyShotLargeBall[color];
                    break;
                case 3: // 緑ブロック：菱形弾
                    pEnemyShot->kind = img_enemyShotDiamond[color];
                    break;
                }

                // ブロック崩しなので、弾は最初は静止（speed=0）
                pEnemyShot->speed = 0.0;
                pEnemyShot->muki = 0.0;

                // ブロックの種類を param_i[0] に保存しておく
                pEnemyShot->param_i[0] = blockType;

                // リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾幕の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // ブロックが壊れるタイミング（例：pEnemyShotSet->count が一定値以上）で弾を発射
        if (pEnemyShotSet->count >= 60 && pShot->speed == 0.0) {
            // ブロックの種類に応じて弾の挙動を変える
            switch (pShot->param_i[0]) {
            case 0: // 赤ブロック：直進弾
                pShot->speed = 2.0;
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                break;
            case 1: // 青ブロック：ゆっくり曲がるホーミング弾
                pShot->speed = 1.5;
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->param_d[0] = 0.02; // 曲がり係数
                break;
            case 2: // 黄ブロック：遅延弾（少し止まってから動く）
                pShot->speed = 0.0;
                pShot->param_i[1] = pEnemyShotSet->count + 30; // 30フレーム後に発射
                break;
            case 3: // 緑ブロック：分裂弾
                pShot->speed = 1.8;
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->param_i[1] = 0; // 分裂フラグ
                break;
            }
        }

        // 黄ブロックの遅延発射
        if (pShot->param_i[0] == 2 && pEnemyShotSet->count == pShot->param_i[1]) {
            pShot->speed = 2.0;
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
        }

        // 青ブロックのホーミング
        if (pShot->param_i[0] == 1 && pShot->speed > 0.0) {
            double targetAngle = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = targetAngle - pShot->muki;
            // 角度差を -π〜π に正規化
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            pShot->muki += pShot->param_d[0] * diff;
        }

        // 緑ブロックの分裂（画面端で分裂）
        if (pShot->param_i[0] == 3 && pShot->speed > 0.0 && pShot->param_i[1] == 0) {
            if (pShot->x <= 0.0 || pShot->x >= 480.0 || pShot->y <= 0.0 || pShot->y >= 480.0) {
                pShot->param_i[1] = 1; // 分裂済みフラグ

                // 左右2方向に分裂弾を追加
                for (int i = -1; i <= 1; i += 2) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = pShot->muki + i * DX_PI / 4.0;
                    pNew->speed = pShot->speed;
                    pNew->kind = pShot->kind;
                    pNew->param_i[0] = 3; // 緑ブロック
                    pNew->param_i[1] = 1; // すでに分裂済み

                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }
            }
        }

        // 弾の移動
        if (pShot->speed > 0.0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_BlockBreak_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔でブロック崩し弾幕を生成
    if (count % 120 == 1) { // 2秒ごとにブロック群を生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBlockBreaker;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // ブロック崩しでは向きは使わない
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