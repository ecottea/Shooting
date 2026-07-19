// enemyPat_MatchChain.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// マッチ連鎖弾幕パターン
// =============================================
static void ShotMatchChain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int MATCH_COUNT = 11;           // マッチの本数（奇数推奨）
    const double MATCH_LENGTH = 28.0;     // マッチの長さ（見た目調整）
    const double IGNITION_DELAY = 4.0;    // 着火間隔（フレーム）

    if (pEnemyShotSet->count == 0) {
        // === 初期化：マッチの配置 ===
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseAngle = (pEnemyShotSet->muki + DX_PI / 2.0) / 2; // 基本的に下向き寄り

        for (int i = 0; i < MATCH_COUNT; i++) {
            pEnemyShot = new sEnemyShot;
            double offset = (i - MATCH_COUNT / 2) * 28.0;

            pEnemyShot->x = pEnemyShotSet->x + offset * cos(baseAngle + 0.2);
            pEnemyShot->y = pEnemyShotSet->y + offset * sin(baseAngle + 0.2) - 30.0;

            pEnemyShot->muki = baseAngle;                    // 少し下向き
            pEnemyShot->speed = 1.8 + GetRand(40) / 100.0;         // 緩やかに降下
            pEnemyShot->kind = img_enemyShotBullet[8];              // 橙色の細長い弾（マッチ棒）
            pEnemyShot->param_i[0] = i;                             // マッチの番号（着火順に使用）
            pEnemyShot->param_i[1] = 0;                             // 0:未着火 1:着火中 2:燃え尽き

            // リンク
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // =========================================
    // 毎フレーム処理
    // =========================================
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    int ignitedCount = 0;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        int matchNo = pEnemyShot->param_i[0];
        int state = pEnemyShot->param_i[1];

        // 未着火マッチの移動
        if (state == 0) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            // 着火タイミング
            if (pEnemyShotSet->count > matchNo * IGNITION_DELAY + 20) {
                pEnemyShot->param_i[1] = 1;           // 着火
                pEnemyShot->speed *= 0.6;             // 少し減速
                if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK); // 着火音
            }
        }
        // 着火中
        else if (state == 1) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            // 火花を撒く
            if (pEnemyShotSet->count % 3 == 0) {
                for (int s = 0; s < 5; s++) {
                    sEnemyShot* spark = new sEnemyShot;
                    spark->x = pEnemyShot->x;
                    spark->y = pEnemyShot->y - 8.0;
                    spark->muki = pEnemyShot->muki + (GetRand(120) - 60) / 180.0 * DX_PI * 0.7;
                    spark->speed = 4.5 + GetRand(80) / 100.0;
                    spark->kind = img_enemyShotSmallBall[8];   // 橙色の小玉（火花）
                    spark->param_i[1] = 99;                    // 火花フラグ

                    spark->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    spark->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = spark;
                    pEnemyShotSet->pEnemyShotHead->prev = spark;
                }
            }

            // 一定時間後に燃え尽き
            if (pEnemyShotSet->count > matchNo * IGNITION_DELAY + 45) {
                pEnemyShot->param_i[1] = 2;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒っぽくして燃えカス風
            }
        }
        // 燃え尽きたマッチ（ゆっくり消滅）
        else if (state == 2) {
            pEnemyShot->y += 0.8;
        }
        else {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        ignitedCount += (state >= 1 ? 1 : 0);
        pEnemyShot = pEnemyShot->next;
    }

    // 全マッチ着火完了後のフィニッシュ
    if (ignitedCount == MATCH_COUNT && pEnemyShotSet->count % 12 == 0 && pEnemyShotSet->count < 180) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 大きな火の螺旋
        for (int i = 0; i < 18; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y - 20.0;
            pEnemyShot->muki = pEnemyShotSet->count / 3.0 + i * DX_PI * 2 / 18;
            pEnemyShot->speed = 3.8;
            pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙中玉
            pEnemyShot->param_d[0] = 0.12;                 // 回転加速用
            //pEnemyShot->param_i[1] = -9999;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// =============================================
// 敵本体パターン
// =============================================
void EnemyPat_Match_Grok()
{
    static int muki = 1;
    static int shotTimer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotTimer = 0;
    }
    else {
        // 左右往復移動
        enemy.x += 1.35 * (double)muki;
        if (enemy.x < 80.0 || enemy.x > 400.0) muki *= -1;

        // 時々上下微調整
        if (count % 85 == 0) {
            enemy.y = 50.0 + GetRand(40);
        }
    }

    // パターン発動（約3.5秒間隔）
    if (count % 210 == 30 || count % 210 == 110) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMatchChain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}