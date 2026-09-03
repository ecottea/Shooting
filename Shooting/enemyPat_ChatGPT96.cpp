// enemyPat_tomatoFestival.cpp
// 弾幕：真夏のトマト投げ祭り

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// トマトが弾けた瞬間に周囲へ赤い小玉を飛び散らせる
// ------------------------------------------------------------
static void ShotTomatoBurst(sEnemyShotSet* pEnemyShotSet, sEnemyShot* tomato)
{
    const double cx = tomato->x;
    const double cy = tomato->y;

    for (int i = 0; i < 16; ++i) {
        sEnemyShot* shot = new sEnemyShot;
        const double a = DX_PI * 2.0 * i / 16.0;

        shot->x = cx;
        shot->y = cy;
        shot->muki = a;
        shot->speed = 2.2 + (i % 3) * 0.35;
        shot->kind = img_enemyShotSmallBall[0]; // 赤
        shot->margin = 40.0;
        shot->param_i[0] = 3;

        shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        shot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = shot;
        pEnemyShotSet->pEnemyShotHead->prev = shot;
    }
}

// ------------------------------------------------------------
// 1個のトマトを放物線で投げ、時間が来たら破裂させる
// param_d[0] = x方向速度
// param_d[1] = y方向速度
// param_d[2] = 重力
// param_d[3] = 軌道開始時のy位置
// param_i[0] = トマトの種類
// ------------------------------------------------------------
static void ShotTomato(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 同じ塊から「トマト本体＋周囲の果肉」を見せる
        for (int i = -1; i <= 1; ++i) {
            sEnemyShot* shot = new sEnemyShot;
            const double side = (double)i * 7.0;
            const double a = pEnemyShotSet->muki + i * 0.16;

            shot->x = pEnemyShotSet->x + side;
            shot->y = pEnemyShotSet->y;

            // 放物線用の速度を保存
            shot->param_d[0] = 1.15 * cos(a);
            shot->param_d[1] = 1.15 * sin(a) - 1.45;
            shot->param_d[2] = 0.055;
            shot->param_d[3] = shot->y;
            shot->param_i[0] = (i == 0 ? 1 : 0);

            shot->muki = atan2(shot->param_d[1], shot->param_d[0]);
            shot->speed = sqrt(shot->param_d[0] * shot->param_d[0]
                + shot->param_d[1] * shot->param_d[1]);
            shot->kind = (i == 0) ? img_enemyShotLargeBall[0]
                : img_enemyShotSmallBall[0]; // 赤
            shot->margin = 60.0;

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        if (pShot->param_i[0] == 3) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = next;
            continue;
        }

        // 全てのトマト片を同じ重力で投げる
        pShot->param_d[1] += pShot->param_d[2];
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];
        pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
        pShot->speed = sqrt(pShot->param_d[0] * pShot->param_d[0]
            + pShot->param_d[1] * pShot->param_d[1]);

        // 本体だけが一定時間後に破裂させる。
        // count はメインルーチンで自動加算される。
        if (pShot->param_i[0] == 1 && pShot->count == 72) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            ShotTomatoBurst(pEnemyShotSet, pShot);

            // 本体を画面上部へ抜けさせるため、以降は直下へ加速させる
            pShot->param_d[1] = 4.5;
            pShot->param_d[2] = 0.0;
            pShot->param_i[0] = 2;
            pShot->kind = img_enemyShotMediumBall[0];
            pShot->margin = 40.0;
        }

        // 破裂後の本体はほぼ停止して見えるため、すぐ上へ抜ける
        if (pShot->param_i[0] == 2) {
            pShot->y -= 0.8;
        }

        pShot = next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ・前半：左右へ移動しながらトマトを投げる
// ・後半：投げる頻度と発射角の幅を増やす
// ・トマトは画面内を放物線で飛び、次々に赤い小玉を撒く
// ------------------------------------------------------------
void EnemyPat_Tomatina_ChatGPT()
{
    static int moveDir;
    static int shotCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
        shotCount = 0;
    }
    else {
        enemy.x += 1.05 * moveDir;
        if (enemy.x < 75.0 || enemy.x > 405.0) moveDir *= -1;
    }

    // トマト投げの間隔。後半ほど祭りが激しくなる。
    int interval = (count < 600) ? 24 : 15;
    if (count % interval == 1) {
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotTomato;
        set->x = enemy.x;
        set->y = enemy.y + 12.0;

        // 左右に投げ分けつつ、基本はプレイヤー方向を狙う
        const double aim = atan2(player.y - set->y, player.x - set->x);
        const double spread = (count < 900) ? 0.42 : 0.70;
        set->muki = aim + ((shotCount++ & 1) ? spread : -spread);

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }
}