// enemyPat_Tmp.cpp
// 「絶対に回避できない弾幕」実装例

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 自機の速度（直近フレームの移動量）を推定するためのバッファ
static double playerPrevX = 0.0, playerPrevY = 0.0;
static double playerVx = 0.0, playerVy = 0.0;

// 自機速度推定の更新（毎フレーム呼ぶ想定）
static void UpdatePlayerVelocity()
{
    double dx = player.x - playerPrevX;
    double dy = player.y - playerPrevY;
    // 低域フィルタでノイズを減らす
    playerVx = 0.7 * playerVx + 0.3 * dx;
    playerVy = 0.7 * playerVy + 0.3 * dy;
    playerPrevX = player.x;
    playerPrevY = player.y;
}

// 弾幕：回避不能を狙った追尾＋封じ弾
static void ShotUnavoidable(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音：中弾発射音を使用
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 自機の予測位置を計算（弾が届くまでの時間を考慮）
        double dist = hypot(player.x - pEnemyShotSet->x, player.y - pEnemyShotSet->y);
        double t = dist / 3.0; // 弾速3.0と仮定した到達時間（調整可能）
        double predictX = player.x + playerVx * t;
        double predictY = player.y + playerVy * t;

        // 予測位置への角度
        double baseAngle = atan2(predictY - pEnemyShotSet->y, predictX - pEnemyShotSet->x);

        // 弾の種類と色の決定
        int shotKind = pEnemyShotSet->kind % 6;
        int color = pEnemyShotSet->kind % 9; // 0〜8の色

        // 追尾弾：予測位置を狙う
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 角度にわずかなばらつきを加えて回避の隙を作らない
            double angle = baseAngle + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 3.0; // 速めの弾速

            // 弾の種類と色を設定
            switch (shotKind) {
            case 0: pEnemyShot->kind = img_enemyShotSmallBall[color]; break;
            case 1: pEnemyShot->kind = img_enemyShotMediumBall[color]; break;
            case 2: pEnemyShot->kind = img_enemyShotLargeBall[color]; break;
            case 3: pEnemyShot->kind = img_enemyShotBullet[color]; break;
            case 4: pEnemyShot->kind = img_enemyShotScale[color]; break;
            case 5: pEnemyShot->kind = img_enemyShotDiamond[color]; break;
            }

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 封じ弾：自機の左右の回避経路を塞ぐ
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 自機の左右に広がる角度で発射
            double sideOffset = (i < 2 ? -1.0 : 1.0) * (0.3 + 0.1 * (i % 2));
            double angle = baseAngle + sideOffset;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.5;

            // 弾の種類と色（追尾弾とは別の種類・色にする例）
            int sideKind = (shotKind + 1) % 6;
            int sideColor = (color + 3) % 9;
            switch (sideKind) {
            case 0: pEnemyShot->kind = img_enemyShotSmallBall[sideColor]; break;
            case 1: pEnemyShot->kind = img_enemyShotMediumBall[sideColor]; break;
            case 2: pEnemyShot->kind = img_enemyShotLargeBall[sideColor]; break;
            case 3: pEnemyShot->kind = img_enemyShotBullet[sideColor]; break;
            case 4: pEnemyShot->kind = img_enemyShotScale[sideColor]; break;
            case 5: pEnemyShot->kind = img_enemyShotDiamond[sideColor]; break;
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（関数名を EnemyPat_Unavoidable_Sakana に変更）
void EnemyPat_Unavoidable_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 100;
        muki = 1;
        shot_count = 0;
        playerPrevX = player.x;
        playerPrevY = player.y;
        playerVx = playerVy = 0.0;
    }
    else {
        // 敵の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 自機速度推定の更新
    UpdatePlayerVelocity();

    // 弾幕発射（例：5フレームごとに発射）
    if (count % 5 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotUnavoidable;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 角度は弾幕関数内で計算
        pEnemyShotSet->kind = shot_count++; // 弾の種類・色の決定に使用

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}