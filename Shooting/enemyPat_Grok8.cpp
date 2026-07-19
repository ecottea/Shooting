// enemyPat_Thunder.cpp
// 雷をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
//  雷モチーフ弾幕：稲妻連鎖ショット
// =============================================
static void ShotThunder(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初期化（初回のみ）
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);  // 重い雷音

        int branchCount = 10;  // メイン + 枝
        double baseAngle = pEnemyShotSet->muki;

        for (int i = 0; i < branchCount; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置（敵本体から少し下）
            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y + 15.0;

            // 角度：メイン方向を中心に少しばらつき（稲妻の枝分かれ感）
            double angleOffset = (i - branchCount / 2) * 0.22;
            pEnemyShot->muki = baseAngle + angleOffset + (GetRand(40) - 20) / 180.0 * DX_PI / 8.0;

            // 速度：かなり速め（雷らしい瞬発性）
            pEnemyShot->speed = 4.8 + GetRand(120) / 100.0;

            // 雷らしい見た目（青・シアン・白中心）
            int color = GetRand(3) + 3; // 3:シアン,4:青,5:マゼンタ寄り,6:白
            int type = GetRand(3);      // 0:小玉,1:中玉,3:銃弾寄り

            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                break;
            default:
                pEnemyShot->kind = img_enemyShotBullet[color];
                break;
            }

            // リンク挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 更新処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本移動（直線）
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 雷らしい微細ジグザグ（時間経過で少し揺れる）
        if (pEnemyShotSet->count % 3 == 0) {
            pEnemyShot->muki += (GetRand(20) - 10) / 300.0;
        }

        // 少し加速（落ちていく雷の勢い）
        if (pEnemyShot->speed < 7.5) {
            pEnemyShot->speed += 0.035;
        }

        pEnemyShot = pEnemyShot->next;
    }

    pEnemyShotSet->count++;
}

static void ShotThunder2(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初期化（初回のみ）
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);  // 重い雷音

        int branchCount = 3;  // メイン + 枝
        double baseAngle = pEnemyShotSet->muki;

        for (int i = 0; i < branchCount; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置（敵本体から少し下）
            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y + 15.0;

            // 角度：メイン方向を中心に少しばらつき（稲妻の枝分かれ感）
            double angleOffset = (i - branchCount / 2) * 0.22;
            pEnemyShot->muki = baseAngle + angleOffset + (GetRand(40) - 20) / 180.0 * DX_PI / 8.0;

            // 速度：かなり速め（雷らしい瞬発性）
            pEnemyShot->speed = 4.8 + GetRand(120) / 100.0;

            // 雷らしい見た目（青・シアン・白中心）
            int color = GetRand(3) + 3; // 3:シアン,4:青,5:マゼンタ寄り,6:白
            pEnemyShot->kind = img_enemyShotLargeBall[color];

            // リンク挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 更新処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本移動（直線）
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 雷らしい微細ジグザグ（時間経過で少し揺れる）
        if (pEnemyShotSet->count % 3 == 0) {
            pEnemyShot->muki += (GetRand(20) - 10) / 300.0;
        }

        // 少し加速（落ちていく雷の勢い）
        if (pEnemyShot->speed < 7.5) {
            pEnemyShot->speed += 0.035;
        }

        pEnemyShot = pEnemyShot->next;
    }

    pEnemyShotSet->count++;
}


// =============================================
//  敵本体パターン（雷モチーフ）
// =============================================
void EnemyPat_Thunder_Grok()
{
    static int muki = 1;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆったり左右往復移動
        enemy.x += 1.15 * (double)muki;
        if (enemy.x < 80.0)  muki = 1;
        if (enemy.x > 400.0) muki = -1;

        // 軽い上下揺れ（雷雲っぽく）
        enemy.y = 60.0 + sin(count / 45.0) * 12.0;
    }

    // 弾幕発射（比較的頻度高めで雷を落とす）
    if (count % 18 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotThunder;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // プレイヤー方向を基本に（雷を落とすイメージ）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 双方向リスト準備
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾セットリストに連結
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // たまに太い中央雷（強攻撃）
    if (count % 85 == 40) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotThunder2;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}