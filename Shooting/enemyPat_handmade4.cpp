#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void Shot5Way(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (count % 3 == 0) {
        PlaySoundMem(sound_enemyShot_noize, DX_PLAYTYPE_BACK);
    }

    // param_d[0] : 現在の角度, param_d[1] : 角速度 として状態を保持
    double angle = pEnemyShotSet->param_d[0];
    double angularVelocity = pEnemyShotSet->param_d[1];

    // 現在の角度で 5 方向に弾を発生
    for (int i = 0; i < 5; i++) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = angle + DX_PI * 2.0 / 5.0 * (double)i;
        pEnemyShot->speed = 3.0;
        pEnemyShot->kind = img_enemyShotDiamond[5];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 角度を進める
    pEnemyShotSet->param_d[0] = angle + angularVelocity;
    pEnemyShotSet->param_d[1] = angularVelocity + 2 * 2 * DX_PI / (64 * 60 * 5);

    // 既存の弾を移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

void EnemyPat_Namitsubu()
{
    static sEnemyShotSet* pEnemyShotSet = nullptr;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 300;

        // ShotSet は 1 つだけ生成し、以降はこの中で弾を発生させる
        pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = Shot5Way;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // 角度と角速度を ShotSet 内に保持
        pEnemyShotSet->param_d[0] = -DX_PI / 2.0; // 初期角度
        pEnemyShotSet->param_d[1] = 0.0;          // 初期角速度

        // 弾リストの番兵
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾セットリストに接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}