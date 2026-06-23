// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：砂時計（8の字軌道）
static void ShotHourglass(sEnemyShotSet* pEnemyShotSet)
{
    const double A = 100.0 * 2.5;
    const double B = 120.0 * 2.5;

    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音を再生
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 20個の弾を8の字軌道上に配置
        for (int i = 0; i < 20; i++) {
            pEnemyShot = new sEnemyShot;
            double t = i * (2.0 * DX_PI / 20.0);
            pEnemyShot->x = pEnemyShotSet->x + A * sin(t);
            pEnemyShot->y = pEnemyShotSet->y + B * sin(2.0 * t);
            pEnemyShot->param_d[0] = t; // 初期位相を保存
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0; // 微速度差を付ける
            pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青の中玉のみ
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動（8の字軌道上を移動）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->param_d[0] += 0.03 * pEnemyShot->speed; // 位相を進める
        double t = pEnemyShot->param_d[0];
        t = t * 0.2 + 1.5;
        pEnemyShot->x = pEnemyShotSet->x + A * sin(t);
        pEnemyShot->y = pEnemyShotSet->y + B * sin(2.0 * t);

        if (pEnemyShot->count >= 600) pEnemyShot->margin = 20;

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Hourglass_Vibe()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 小さな楕円軌道で移動
        enemy.x = 240.0 + 180.0 * sin(count * 0.015);
        enemy.y = 160.0 + 50.0 * sin(count * 0.03);
    }

    if (count % 110 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHourglass;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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