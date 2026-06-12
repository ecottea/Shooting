#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 渦巻き弾幕（螺旋）
static void ShotWafuSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numBullets = 8; // 1周あたりの弾数
        double baseAngle = pEnemyShotSet->muki; // プレイヤー方向を基準
        double spiralOffset = pEnemyShotSet->count * 0.1; // フレームで少しずつ回転

        for (int i = 0; i < numBullets; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 螺旋状に角度をずらす
            pEnemyShot->muki = baseAngle + spiralOffset + (360.0f / numBullets) * i;
            pEnemyShot->speed = 3.0f;

            // 和風テイストの弾：小玉 or 菱形弾＋赤 or 紫
            int type = GetRand(1); // 0:小玉, 1:菱形弾
            int color = (i % 2 == 0) ? 0 : 4; // 赤 or 青（紫に差し替え）
            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotDiamond[color];
                break;
            }

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存弾の移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 扇状弾幕（扇形）
static void ShotWafuFan(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numBullets = 7; // 扇の枚数
        double baseAngle = pEnemyShotSet->muki; // プレイヤー方向
        double spread = 60.0; // 扇の開き角度

        for (int i = 0; i < numBullets; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 扇状に角度を広げる
            pEnemyShot->muki = baseAngle - spread/2 + (spread / (numBullets-1)) * i;
            pEnemyShot->speed = 4.0f;

            // 和風テイスト：鱗弾 or 銃弾＋赤 or 黄
            int type = GetRand(1); // 0:鱗弾, 1:銃弾
            int color = (i % 2 == 0) ? 0 : 1; // 赤 or 黄
            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotBullet[color];
                break;
            }

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

// 円形弾幕（環状）
static void ShotWafuCircle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numBullets = 16; // 円周上の弾数
        double radius = 30.0; // 発射時の円半径
        double speed = 3.0;

        for (int i = 0; i < numBullets; i++) {
            double angle = (360.0 / numBullets) * i;
            double rad = angle * 3.14159265 / 180.0;

            pEnemyShot = new sEnemyShot;
            // 円周上の位置から外側へ発射
            pEnemyShot->x = pEnemyShotSet->x + radius * cos(rad);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(rad);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = speed;

            // 和風テイスト：中玉 or 大玉＋緑 or シアン
            int type = GetRand(1); // 0:中玉, 1:大玉
            int color = (i % 2 == 0) ? 2 : 3; // 緑 or シアン
            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotLargeBall[color];
                break;
            }

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

// 和風弾幕パターン（例：渦巻き＋扇状＋円形を順番に）
void EnemyPat_Japanese_Sakana()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = 150;
        enemy.hp = enemy.maxHp;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 10フレームごとに弾幕セットを生成
    if (count % 5 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // フレームに応じてパターンを切り替え
        if ((count / 5) % 3 == 0) {
            pEnemyShotSet->patternFunc = ShotWafuSpiral;   // 渦巻き
        }
        else if ((count / 5) % 3 == 1) {
            pEnemyShotSet->patternFunc = ShotWafuFan;      // 扇状
        }
        else {
            pEnemyShotSet->patternFunc = ShotWafuCircle;   // 円形
        }

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
