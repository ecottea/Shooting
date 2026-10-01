#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 罔両「禅寺に棲む妖蝶」ループ版
// 青卍(左回り)+青蝶帰巣 → 赤卍(右回り)+赤蝶横広がり → ループ
// 鱗弾=蝶、短レーザー=卍

static void ShotManji(sEnemyShotSet* pSet)
{
    const int LIFE = 190;
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 0) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int arm = 0; arm < 4; arm++) {
            for (int j = 0; j < 3; j++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = enemy.x; p->y = enemy.y;
                p->param_d[0] = arm * DX_PI_F / 2.0;
                p->param_d[1] = 22.0 + j * 26.0;
                p->kind = img_enemyShotLaser[(pSet->kind == 0) ? 4 : 0];
                p->margin = 120;
                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
    }

    bool isEnd = pSet->count > LIFE;
    double rotSpeed = (pSet->kind == 0) ? -0.018 : 0.038;
    double expand = (pSet->kind == 0) ? 0.08 : 0.18;

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (!isEnd) {
            pShot->param_d[0] += rotSpeed;
            double ang = pShot->param_d[0];
            double rad = pShot->param_d[1] + pSet->count * expand;
            pShot->x = enemy.x + rad * cos(ang);
            pShot->y = enemy.y + rad * sin(ang);
            pShot->muki = ang + DX_PI_F / 2.0;
        }
        else {
            // 寿命後は外へ飛ばして画面外削除に任せる
            pShot->x += 3.0 * cos(pShot->muki);
            pShot->y += 3.0 * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

static void ShotBlueButterfly(sEnemyShotSet* pSet)
{
    const int SPAWN_END = 180;
    if (pSet->count < SPAWN_END && pSet->count % 6 == 0) {
        for (int k = 0; k < 8; k++) {
            sEnemyShot* p = new sEnemyShot;
            double baseAng = DX_PI_F * 2.0 * k / 8.0 + (double)GetRand(20) / 180.0 * DX_PI_F;
            p->x = enemy.x; p->y = enemy.y;
            p->muki = baseAng;
            p->speed = 3.2 + (double)GetRand(80) / 100.0+12;
            p->kind = img_enemyShotScale[4];
            p->param_i[0] = 0;
            p->param_i[1] = GetRand(100);
            p->margin = 240;
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            pShot->speed *= 0.955;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->speed < 0.45 || pShot->count > 55) {
                pShot->param_i[0] = 1;
                pShot->speed = 0.6;
            }
        }
        else {
            double dx = enemy.x - pShot->x;
            double dy = enemy.y - pShot->y;
            double target = atan2(dy, dx);
            double diff = target - pShot->muki;
            while (diff > DX_PI_F) diff -= DX_PI_F * 2.0;
            while (diff < -DX_PI_F) diff += DX_PI_F * 2.0;
            pShot->muki += diff * 0.07;
            pShot->muki += sin(pShot->count * 0.18 + pShot->param_i[1]) * 0.09;
            pShot->speed = 0.9 + (double)GetRand(30) / 100.0;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            if (pSet->count > SPAWN_END + 40) {
                double d = dx * dx + dy * dy;
                if (d < 100.0) { // 敵に戻ったら消去
                    pShot->y = -50.0;
                }
            }
        }
        pShot = pShot->next;
    }
}

static void ShotRedButterfly(sEnemyShotSet* pSet)
{
    const int SPAWN_END = 190;
    if (pSet->count < SPAWN_END && pSet->count % 10 == 0) {
        double mukiCenter = atan2(player.y - enemy.y, player.x - enemy.x);
        for (int s = -2; s <= 2; s++) {
            double streamAng = mukiCenter + s * 0.19;
            for (int k = 0; k < 6; k++) {
                sEnemyShot* p = new sEnemyShot;
                double off = k * 14.0;
                p->x = enemy.x + cos(streamAng) * off;
                p->y = enemy.y + sin(streamAng) * off;
                p->muki = streamAng + (GetRand(20) - 10) / 180.0 * DX_PI_F * 0.3;
                p->speed = 2.1 + k * 0.18 + (double)GetRand(20) / 100.0;
                p->kind = img_enemyShotScale[0];
                p->param_i[0] = s;
                p->param_i[1] = GetRand(100);
                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
        if (pSet->count % 20 == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy) == 0) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double side = (double)pShot->param_i[0] * pShot->count * 0.015;
        double mvMuki = pShot->muki + sin(pShot->count * 0.22 + pShot->param_i[1]) * 0.06;
        pShot->x += pShot->speed * cos(mvMuki) - side * 0.25 * sin(mvMuki);
        pShot->y += pShot->speed * sin(mvMuki) + side * 0.25 * cos(mvMuki);
        pShot = pShot->next;
    }
}

void EnemyPat_Zendera_MetaAI()
{
    const int BLUE_TIME = 200;
    const int RED_TIME = 200;
    const int LOOP = BLUE_TIME + RED_TIME; // 400Fループ

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 170.0;
        enemy.maxHp = enemy.hp = 200;
    }
    enemy.x += cos((count - 1) * 0.012) * 0.25;

    int loopPos = (count - 1) % LOOP;

    // ループ毎の青開始
    if (loopPos == 0) {
        sEnemyShotSet* pManji = new sEnemyShotSet;
        pManji->count = 0; pManji->patternFunc = ShotManji;
        pManji->kind = 0; pManji->x = enemy.x; pManji->y = enemy.y;
        pManji->pEnemyShotHead = new sEnemyShot;
        pManji->pEnemyShotHead->prev = pManji->pEnemyShotHead;
        pManji->pEnemyShotHead->next = pManji->pEnemyShotHead;
        pManji->prev = enemyShotSetHead.prev; pManji->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pManji; enemyShotSetHead.prev = pManji;

        sEnemyShotSet* pBlue = new sEnemyShotSet;
        pBlue->count = 0; pBlue->patternFunc = ShotBlueButterfly;
        pBlue->kind = 0; pBlue->x = enemy.x; pBlue->y = enemy.y;
        pBlue->pEnemyShotHead = new sEnemyShot;
        pBlue->pEnemyShotHead->prev = pBlue->pEnemyShotHead;
        pBlue->pEnemyShotHead->next = pBlue->pEnemyShotHead;
        pBlue->prev = enemyShotSetHead.prev; pBlue->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBlue; enemyShotSetHead.prev = pBlue;
    }
    // ループ内の赤開始
    else if (loopPos == BLUE_TIME) {
        if (CheckSoundMem(sound_enemyCharge) == 0) PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pManji = new sEnemyShotSet;
        pManji->count = 0; pManji->patternFunc = ShotManji;
        pManji->kind = 1; pManji->x = enemy.x; pManji->y = enemy.y;
        pManji->pEnemyShotHead = new sEnemyShot;
        pManji->pEnemyShotHead->prev = pManji->pEnemyShotHead;
        pManji->pEnemyShotHead->next = pManji->pEnemyShotHead;
        pManji->prev = enemyShotSetHead.prev; pManji->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pManji; enemyShotSetHead.prev = pManji;

        sEnemyShotSet* pRed = new sEnemyShotSet;
        pRed->count = 0; pRed->patternFunc = ShotRedButterfly;
        pRed->kind = 1; pRed->x = enemy.x; pRed->y = enemy.y;
        pRed->pEnemyShotHead = new sEnemyShot;
        pRed->pEnemyShotHead->prev = pRed->pEnemyShotHead;
        pRed->pEnemyShotHead->next = pRed->pEnemyShotHead;
        pRed->prev = enemyShotSetHead.prev; pRed->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRed; enemyShotSetHead.prev = pRed;
    }
}