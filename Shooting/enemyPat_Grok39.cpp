// enemyPat_Tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
//  インベーダー風弾幕用 サブパターン関数群
// =============================================

// 基本直線レーザー → 途中で粒弾分裂
static void ShotInvaderBasic(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(60) - 30;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.2 + GetRand(80) / 100.0;

            pEnemyShot->kind = img_enemyShotLaser[i % 8]; // レーザー
            if (i % 3 == 0) pEnemyShot->kind = img_enemyShotLargeBall[i % 8]; // 時々大玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動 + 分裂処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 一定距離進んだら分裂
        if (pEnemyShot->count > 35 && pEnemyShot->count % 12 == 0) {
            for (int i = 0; i < 5; i++) {
                sEnemyShot* split = new sEnemyShot;
                split->x = pEnemyShot->x;
                split->y = pEnemyShot->y;
                split->muki = pEnemyShot->muki + (i - 2) * 0.35;
                split->speed = 2.8;
                split->kind = img_enemyShotSmallBall[i % 8];
                split->count = -9999;
                // リンク（簡易）
                split->prev = pEnemyShotSet->pEnemyShotHead->prev;
                split->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = split;
                pEnemyShotSet->pEnemyShotHead->prev = split;
            }
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// 扇状5-way弾（最前列風）
static void ShotFan(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = -2; i <= 2; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = pEnemyShotSet->muki + i * 0.28;
            p->speed = 4.5;
            p->kind = img_enemyShotMediumBall[(i + 2) % 8];
            // 双方向リスト登録
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
    // 移動処理
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// リング弾幕（青列風）
static void ShotRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 12; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = i * DX_PI * 2 / 12;
            p->speed = 2.8;
            p->kind = img_enemyShotDiamond[i % 8];
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// 複合親子弾（最奥列風）
static void ShotComplex(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_heavy)) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 大型親弾
        for (int i = 0; i < 3; i++) {
            sEnemyShot* parent = new sEnemyShot;
            parent->x = pEnemyShotSet->x + GetRand(40) - 20;
            parent->y = pEnemyShotSet->y;
            parent->muki = pEnemyShotSet->muki + (GetRand(60) - 30) / 180.0 * DX_PI;
            parent->speed = 2.4;
            parent->kind = img_enemyShotLargeBall[i % 8];
            parent->param_i[0] = 28; // 分裂までのカウント
            // リスト登録...
            parent->prev = pEnemyShotSet->pEnemyShotHead->prev;
            parent->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = parent;
            pEnemyShotSet->pEnemyShotHead->prev = parent;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);

        if (p->count > p->param_i[0]) {
            for (int i = 0; i < 6; i++) {
                sEnemyShot* child = new sEnemyShot;
                child->x = p->x;
                child->y = p->y;
                child->muki = p->muki + i * DX_PI * 2 / 6;
                child->speed = 4.2;
                child->kind = img_enemyShotSmallBall[i % 8];
                child->param_i[0] = 999;
                child->prev = pEnemyShotSet->pEnemyShotHead->prev;
                child->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = child;
                pEnemyShotSet->pEnemyShotHead->prev = child;
            }
            // 親弾削除（簡易）
            p->x = -9999;
        }
        p = p->next;
    }
}

// =============================================
//  本体パターン：EnemyPat_Invader_Grok
// =============================================
void EnemyPat_Invader_Grok()
{
    static int muki = 1;
    static int phase = 0;
    static int shotTimer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
        shotTimer = 0;
    }

    // インベーダーらしい左右移動＋徐々降下
    enemy.x += 1.35 * muki;
    if (enemy.x < 80 || enemy.x > 400) muki *= -1;

    // 時間経過で降下
    if (count % 180 == 0 && enemy.y < 180) {
        enemy.y += 28.0;
    }

    shotTimer++;

    // 弾幕発射スケジュール
    if (shotTimer % 45 == 0) {
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->x = enemy.x;
        set->y = enemy.y + 20;
        set->muki = atan2(player.y - set->y, player.x - set->x);
        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        // フェーズに応じてパターン変化（インベーダー列を模倣）
        int pat = (phase % 5);
        if (pat == 0 || pat == 4) {
            set->patternFunc = ShotFan;           // 赤列風
        }
        else if (pat == 1) {
            set->patternFunc = ShotInvaderBasic;  // 基本レーザー
        }
        else if (pat == 2) {
            set->patternFunc = ShotRing;          // 青列風
        }
        else {
            set->patternFunc = ShotComplex;       // 黄列風
        }

        // リスト連結
        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;

        phase++;
    }

    // フォーメーション加速演出（後半）
    if (enemy.hp < 80 && count % 120 == 0) {
        enemy.y += 12.0; // 急降下
    }
}