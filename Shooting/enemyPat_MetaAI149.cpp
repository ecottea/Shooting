#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// マシンガン弾幕 - Bouncing Gatling
static void ShotGatling(sEnemyShotSet* pEnemyShotSet)
{
    // 初回：チャージ音と基準角度保存
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        pEnemyShotSet->param_d[0] = pEnemyShotSet->muki; // base muki
        pEnemyShotSet->param_d[1] = pEnemyShotSet->muki; // current muki (揺れる)
        pEnemyShotSet->param_d[2] = 0.0;
    }

    const int CHARGE_END = 30;
    const int SHOOT_END = 150; // 30 + 120f = 2秒連射
    const int OVERHEAT_END = 210;

    // ---- 生成パート ----
    if (pEnemyShotSet->count < CHARGE_END) {
        // 予告チカチカ
        if (pEnemyShotSet->count % 6 == 0) {
            sEnemyShot* pFlash = new sEnemyShot;
            pFlash->x = pEnemyShotSet->x;
            pFlash->y = pEnemyShotSet->y;
            pFlash->muki = pEnemyShotSet->param_d[1];
            pFlash->speed = 0.0;
            pFlash->kind = img_enemyShotMediumBall[8]; // 橙 中玉 = フラッシュ
            pFlash->count = 0;
            pFlash->param_i[0] = 2; // 2=フラッシュ
            pFlash->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pFlash->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pFlash;
            pEnemyShotSet->pEnemyShotHead->prev = pFlash;
        }
    }
    else if (pEnemyShotSet->count < SHOOT_END) {
        // 0.06秒間隔 = 3fに1発の高速連射
        if (pEnemyShotSet->count % 3 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // 1. 実弾
            sEnemyShot* pBullet = new sEnemyShot;
            // GetRand(x)は0～xのx+1通りなので ±8度は GetRand(16)-8
            double spread = (GetRand(16) - 8) / 180.0 * DX_PI;
            pBullet->x = pEnemyShotSet->x;
            pBullet->y = pEnemyShotSet->y;
            pBullet->muki = pEnemyShotSet->param_d[1] + spread;
            pBullet->speed = 7.0 + GetRand(100) / 100.0; // 7.0～8.0
            pBullet->kind = img_enemyShotBullet[6]; // 白 銃弾
            pBullet->count = 0;
            pBullet->param_i[0] = 0;
            pBullet->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBullet->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBullet;
            pEnemyShotSet->pEnemyShotHead->prev = pBullet;

            // 2. 薬莢演出
            sEnemyShot* pCase = new sEnemyShot;
            pCase->x = pEnemyShotSet->x;
            pCase->y = pEnemyShotSet->y;
            // 真横～斜め後ろに排出
            double ejectDir = pEnemyShotSet->muki + DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
            pCase->muki = ejectDir;
            pCase->speed = 1.5 + GetRand(100) / 100.0;
            pCase->kind = img_enemyShotSmallBall[1]; // 黄 小玉 = 薬莢
            pCase->count = 0;
            pCase->param_i[0] = 1; // 1=薬莢
            pCase->param_d[0] = 0.0; // 落下加速度
            pCase->margin = 100.0; // 画面外に少し出るまで残す
            pCase->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pCase->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pCase;
            pEnemyShotSet->pEnemyShotHead->prev = pCase;

            // 3. マズルフラッシュ
            sEnemyShot* pFlash = new sEnemyShot;
            pFlash->x = pEnemyShotSet->x;
            pFlash->y = pEnemyShotSet->y;
            pFlash->muki = pBullet->muki;
            pFlash->speed = 0.0;
            pFlash->kind = img_enemyShotSmallBall[8]; // 橙 小玉をフラッシュに
            pFlash->count = 0;
            pFlash->param_i[0] = 2;
            pFlash->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pFlash->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pFlash;
            pEnemyShotSet->pEnemyShotHead->prev = pFlash;

            // 反動で照準が徐々に暴れる
            pEnemyShotSet->param_d[1] += (GetRand(6) - 3) / 180.0 * DX_PI;
            double diff = pEnemyShotSet->param_d[1] - pEnemyShotSet->param_d[0];
            const double LIM = 15.0 / 180.0 * DX_PI;
            if (diff > LIM) pEnemyShotSet->param_d[1] = pEnemyShotSet->param_d[0] + LIM;
            if (diff < -LIM) pEnemyShotSet->param_d[1] = pEnemyShotSet->param_d[0] - LIM;
        }
    }
    else if (pEnemyShotSet->count == SHOOT_END) {
        // オーバーヒート演出
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pOver = new sEnemyShot;
            pOver->x = pEnemyShotSet->x;
            pOver->y = pEnemyShotSet->y;
            pOver->muki = (GetRand(360) / 180.0 * DX_PI);
            pOver->speed = 0.5 + GetRand(100) / 100.0;
            pOver->kind = (i == 0)? img_enemyShotLargeBall[8] : img_enemyShotMediumBall[8];
            pOver->count = 0;
            pOver->param_i[0] = 2;
            pOver->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pOver->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pOver;
            pEnemyShotSet->pEnemyShotHead->prev = pOver;
        }
    }

    // ---- 移動パート ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot!= pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 1) {
            // 薬莢：放物線落下
            pShot->param_d[0] += 0.18; // 重力
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki) + pShot->param_d[0];
        }
        else if (pShot->param_i[0] == 2) {
            // フラッシュ：敵の銃口に追従して短命
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            if (pShot->count > 5) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        else {
            // 実弾：直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pNext;
    }
}

void EnemyPat_MachineGun_MetaAI()
{
    static int muki;
    static int dir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        dir = 0;
    }
    else {
        int mod = count % 240;
        bool isShooting = (mod >= 30 && mod < 150);

        if (isShooting) {
            // 反動でブルブル後退
            enemy.y -= 0.08;
            if (enemy.y < 20.0) enemy.y = 20.0;
            enemy.x += (GetRand(4) - 2) * 0.25;
        }
        else {
            // 通常時は左右にゆっくり移動しつつY=40に戻る
            enemy.x += 0.9 * (double)muki;
            if (count % 120 == 60) muki *= -1;
            if (enemy.x < 80) muki = 1;
            if (enemy.x > 400) muki = -1;
            if (enemy.y < 40.0) enemy.y += 0.5;
            if (enemy.y > 40.0) enemy.y -= 0.5;
        }
    }

    // 240f周期で1セット生成
    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGatling;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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

    // 生成済みセットの発射原点を敵に追従させる
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet!= &enemyShotSetHead) {
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        // 撃ってない待機中は常にプレイヤーを狙い直す
        if (pSet->count < 30) {
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->param_d[0] = pSet->muki;
        }
        pSet = pSet->next;
    }
}