// enemyPat_match.cpp
// マッチをモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// マッチの炎が広がる火花弾幕（マッチモチーフ）
static void ShotMatchFlame(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // フェーズ管理用パラメータ
    int& phase = pEnemyShotSet->param_i[0]; // 0:点火前, 1:炎成長, 2:火花散開, 3:燃え尽き
    int& flameCount = pEnemyShotSet->param_i[1]; // 炎の成長時間カウンタ
    int& sparkCount = pEnemyShotSet->param_i[2]; // 火花散開時間カウンタ

    if (pEnemyShotSet->count == 0) {
        phase = 0;
        flameCount = 0;
        sparkCount = 0;
    }

    // フェーズ進行
    if (phase == 0 && pEnemyShotSet->count >= 0) {
        phase = 1;
        flameCount = 0;
        // 点火音（軽めの効果音）
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    else if (phase == 1 && flameCount >= 90) {
        phase = 2;
        sparkCount = 0;
    }
    else if (phase == 2 && sparkCount >= 120) {
        phase = 3;
    }

    // フェーズごとの弾生成
    switch (phase) {
    case 1: // 炎の成長フェーズ
        if (flameCount % 15 == 0) {
            // 炎のライン弾（上下に伸びる）
            for (int i = 0; i < 2; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // 上方向（少しプレイヤー寄り）と下方向
                if (i == 0) {
                    pEnemyShot->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) + (GetRand(30) - 15) / 180.0 * DX_PI;
                }
                else {
                    pEnemyShot->muki = DX_PI / 2.0; // 真下
                }
                pEnemyShot->speed = 1.5 + (GetRand(50)) / 100.0;

                // 弾の種類：小玉 or 中玉（炎っぽい）
                // 色：0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
                int color = (pEnemyShotSet->kind % 2 == 0) ? 0 : 8; // 赤 or 橙
                pEnemyShot->kind = img_enemyShotSmallBall[color];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        flameCount++;
        break;

    case 2: // 火花の散開フェーズ
        if (sparkCount % 5 == 0) {
            // 火花弾（四方八方に飛び散る）
            for (int i = 0; i < 4; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = (GetRand(360)) / 180.0 * DX_PI;
                pEnemyShot->speed = 1.0 + (GetRand(100)) / 100.0;

                // 弾の種類：小玉 or 鱗弾（火花っぽい）
                // 色：黄色 or 白（火花のイメージ）
                int color = (pEnemyShotSet->kind % 2 == 0) ? 1 : 6; // 黄 or 白
                if (pEnemyShotSet->kind % 2 == 0) {
                    pEnemyShot->kind = img_enemyShotSmallBall[color];
                }
                else {
                    pEnemyShot->kind = img_enemyShotScale[color];
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        sparkCount++;
        break;

    case 3: // 燃え尽きフェーズ（最後の小さな爆発）
        if (sparkCount == 0) {
            // 最後のリング弾
            for (int i = 0; i < 8; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = (i * 45) / 180.0 * DX_PI;
                pEnemyShot->speed = 2.0;

                // 弾の種類：小玉
                // 色：赤（最後の光）
                pEnemyShot->kind = img_enemyShotSmallBall[0];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        sparkCount++;
        break;
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（マッチ弾幕）
void EnemyPat_Match_Sakana()
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
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMatchFlame;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // ここでは使用しない
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