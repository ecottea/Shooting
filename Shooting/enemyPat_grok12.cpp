// enemyPat_Tmp_Solar.cpp
// 太陽をモチーフにした弾幕パターン
// 敵本体関数：void EnemyPat_Sun_Grok()

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ================================================================
// 太陽モチーフ弾幕：「Solar Corona Burst」
// 中心からコロナのように放射状に光線（弾の列）を展開し、
// 徐々に回転しながら外側へ広がる。時折強いフレア（集中放射）を発生。
// ================================================================
static void ShotSolarCorona(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初期化（生成）
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int numRays = 16;           // 光線の本数（太陽のコロナ風）
        const int bulletsPerRay = 6;      // 1光線あたりの弾の数（尾を表現）

        double baseAngle = pEnemyShotSet->muki;

        for (int i = 0; i < numRays; i++) {
            double rayAngle = baseAngle + i * (DX_PI * 2.0 / numRays);

            for (int j = 0; j < bulletsPerRay; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                // 角度にわずかなばらつき＋距離による遅延で自然な広がり
                pEnemyShot->muki = rayAngle + (GetRand(20) - 10) * 0.008;
                pEnemyShot->speed = 1.8 + j * 0.45;   // 外側の弾ほど速く（広がる）

                // 太陽らしい暖色系（赤・黄・白中心）
                int color = (j < 2) ? 0 : ((j < 4) ? 1 : 6); // 0:赤, 1:黄, 6:白

                if (j == 0) {
                    // 先端は大玉（フレアの頭）
                    pEnemyShot->kind = img_enemyShotLargeBall[color];
                }
                else if (j <= 2) {
                    pEnemyShot->kind = img_enemyShotMediumBall[color];
                }
                else {
                    pEnemyShot->kind = img_enemyShotSmallBall[color];
                }

                // 双方向リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 定期的な追加フレア（太陽活動風）
    else if (pEnemyShotSet->count % 80 == 10) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int flareRays = 16;
        double flareAngle = pEnemyShotSet->muki + (pEnemyShotSet->count / 8) * 0.15; // 回転

        for (int i = 0; i < flareRays; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = flareAngle + i * (DX_PI * 2.0 / flareRays) + (GetRand(30) - 15) * 0.01;
            pEnemyShot->speed = 2.8 + GetRand(80) / 100.0;

            int type = GetRand(2); // 小玉 or 中玉
            int color = GetRand(3); // 赤・黄・白寄り
            if (type == 0) {
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[color];
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾移動 + 軽い回転効果（外側に行くほど角度変化）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double accel = 1.0 + (pEnemyShotSet->count * 0.0008); // 徐々に加速（爆発感）

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki) * accel;
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki) * accel;

        // 太陽フレアらしいわずかなカーブ
        pEnemyShot->muki += 0.003;

        pEnemyShot = pEnemyShot->next;
    }
}

// ================================================================
// 敵本体パターン
// ================================================================
void EnemyPat_Sun_Grok()
{
    static int muki = 1;

    if (count == 1) {
        // 初期位置（上部中央）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆったり左右移動＋軽い上下揺れ（太陽が浮かんでいるような動き）
        enemy.x += 1.15 * (double)muki;
        enemy.y += sin(count * 0.018) * 0.6;

        if (count % 110 == 55) muki *= -1;

        // 画面端で跳ね返り
        if (enemy.x < 80.0) muki = 1;
        if (enemy.x > 400.0) muki = -1;
    }

    // 弾幕発射タイミング
    if (count % 96 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSolarCorona;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;

        // プレイヤー方向をベースにしつつ、少しランダムで太陽の不規則さを表現
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x)
            + (GetRand(40) - 20) * 0.018;

        // 双方向リスト初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ShotSetリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}