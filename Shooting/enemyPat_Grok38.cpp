// enemyPat_Tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ======================
// 螺旋レーザー・プリズン（修正版）
// ======================
static void ShotSpiralLaserPrison(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int BASE_LASER_COUNT = 8;

    if (pEnemyShotSet->count == 0)
    {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 毎フレームレーザーを時間差で発射（連続感を出す）
    if (pEnemyShotSet->count % 6 == 0 && pEnemyShotSet->count < 370)
    {
        int wave = pEnemyShotSet->count / 6;
        double baseAngle = wave * 0.12; // 徐々に回転

        for (int i = 0; i < BASE_LASER_COUNT; i++)
        {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + (double)i / BASE_LASER_COUNT * DX_PI * 2.0;
            pEnemyShot->speed = 7.5;                    // レーザーらしい速さ
            pEnemyShot->kind = img_enemyShotLaser[4];   // 青系レーザー（64x8判定）
            pEnemyShot->margin = 40.0;

            // param_i[0] = 生存フレーム数（短めに）
            pEnemyShot->param_i[0] = 28 * 2;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 各レーザーの移動と角度微調整
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 徐々に角度を変化させて「螺旋」を強調
        if (pEnemyShotSet->count > 120)
        {
            pEnemyShot->muki += 0.008;   // 軽くカーブ
        }

        pEnemyShot->param_i[0]--;        // 寿命

        // 寿命切れで削除（画面外判定はメイン側が行うが、早期削除）
        if (pEnemyShot->param_i[0] <= 0)
        {
            sEnemyShot* next = pEnemyShot->next;
            pEnemyShot->prev->next = pEnemyShot->next;
            pEnemyShot->next->prev = pEnemyShot->prev;
            delete pEnemyShot;
            pEnemyShot = next;
            continue;
        }

        pEnemyShot = pEnemyShot->next;
    }

    // 終盤：収束＋小玉追加
    if (pEnemyShotSet->count > 220 && pEnemyShotSet->count % 8 == 0 && pEnemyShotSet->count <= 360)
    {
        for (int i = 0; i < 5; i++)
        {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (double)i / 5 * DX_PI * 2 + (pEnemyShotSet->count * 0.05);
            pEnemyShot->speed = 3.2;
            pEnemyShot->kind = img_enemyShotSmallBall[GetRand(7)];
            pEnemyShot->param_i[0] = 9999;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // パターン終了
    if (pEnemyShotSet->count > 360)
    {
        if (pEnemyShotSet->count == 361)
        {
            for (int i = 0; i < 24; i++)
            {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = (double)i / 24 * DX_PI * 2.0;
                pEnemyShot->speed = 4.0 + GetRand(100) / 50.0;
                pEnemyShot->kind = img_enemyShotMediumBall[GetRand(7)];
                pEnemyShot->param_i[0] = 9999;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
}

// ======================
// 敵本体パターン
// ======================
void EnemyPat_Laser_Grok()
{
    static int muki = 1;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else
    {
        enemy.x += 1.15 * (double)muki;
        if (count % 130 == 65) muki *= -1;

        if (enemy.x < 70.0) muki = 1;
        if (enemy.x > 410.0) muki = -1;
    }

    // 約4秒ごとにパターン発動
    if (count % 400 == 30)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiralLaserPrison;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}