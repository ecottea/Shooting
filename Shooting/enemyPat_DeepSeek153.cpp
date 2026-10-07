// enemyPat_Tmp.cpp
// 相転移弾幕『三態循環 ― 固・液・気 ―』
// 既存の敵弾素材のみを使用して、固体→液体→気体→凝縮→固体の循環を表現する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 1サイクルの長さ（フレーム数）
#define CYCLE 560

// ------------------------------------------------------------
// 相転移弾幕パターン
// ------------------------------------------------------------
static void ShotPhaseTransition(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count; // メインルーチンで毎フレーム+1される

    // ------------------------------------------------
    // 初回：固体格子を生成
    // ------------------------------------------------
    if (t == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double cx = pEnemyShotSet->x;
        double cy = pEnemyShotSet->y;

        // 六方格子状に小弾（分子）を配置
        for (int row = -3*2; row <= 3*2; row++) {
            for (int col = -3*2; col <= 3*2; col++) {
                double x = cx + col * 18.0 + (row % 2) * 9.0;
                double y = cy + row * 16.0;
                double dx = x - cx;
                double dy = y - cy;
                if (dx * dx + dy * dy < 120.0 * 120.0) {
                    sEnemyShot* p = new sEnemyShot;
                    p->x = x;
                    p->y = y;
                    p->muki = 0.0;
                    p->speed = 0.0;
                    p->kind = img_enemyShotSmallBall[3]; // シアン
                    p->margin = 480;
                    p->param_d[0] = x; // 基準X
                    p->param_d[1] = y; // 基準Y
                    p->param_d[2] = (double)GetRand(100) / 100.0 * 2.0 * DX_PI; // 振動位相
                    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    p->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = p;
                    pEnemyShotSet->pEnemyShotHead->prev = p;
                }
            }
        }
        pEnemyShotSet->param_d[0] = cx;
        pEnemyShotSet->param_d[1] = cy;
    }

    // ------------------------------------------------
    // 相転移イベント
    // ------------------------------------------------
    if (t == 180) {
        // 融解：ランダムな低速運動へ
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            p->muki = (double)GetRand(359) / 180.0 * DX_PI;
            p->speed = 0.5 + (double)GetRand(100) / 100.0 * 0.5;
            p->kind = img_enemyShotSmallBall[4]; // 青
            p = p->next;
        }
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
    }

    if (t == 240) {
        // 液体：周回運動へ
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            double dx = p->x - pEnemyShotSet->param_d[0];
            double dy = p->y - pEnemyShotSet->param_d[1];
            double dist = sqrt(dx * dx + dy * dy);
            if (dist < 1.0) dist = 1.0;
            p->param_d[3] = atan2(dy, dx);
            p->param_d[4] = dist;
            p->kind = img_enemyShotSmallBall[4]; // 青
            p = p->next;
        }
    }

    if (t == 480) {
        // 蒸発：高速で四方へ
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            double dx = p->x - pEnemyShotSet->param_d[0];
            double dy = p->y - pEnemyShotSet->param_d[1];
            p->muki = atan2(dy, dx);
            p->speed = 3.0 + (double)GetRand(200) / 100.0 * 2.0-3;
            p->kind = img_enemyShotSmallBall[6]; // 白
            p = p->next;
        }

        // 追加の気体分子
        for (int i = 0; i < 12; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->param_d[0];
            p->y = pEnemyShotSet->param_d[1];
            p->muki = (double)GetRand(359) / 180.0 * DX_PI;
            p->speed = 4.0 + (double)GetRand(200) / 100.0 * 2.0-4;
            p->kind = img_enemyShotSmallBall[6]; // 白
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    if (t == 720-200) {
        // 凝縮：冷却核出現、吸引開始
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            p->kind = img_enemyShotSmallBall[3]; // シアン
            p = p->next;
        }

        // 冷却核（大玉）
        sEnemyShot* core = new sEnemyShot;
        core->x = pEnemyShotSet->param_d[0];
        core->y = pEnemyShotSet->param_d[1];
        core->muki = 0.0;
        core->speed = 0.0;
        core->kind = img_enemyShotLargeBall[3]; // シアン
        core->param_i[0] = 1; // コアフラグ
        core->prev = pEnemyShotSet->pEnemyShotHead->prev;
        core->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = core;
        pEnemyShotSet->pEnemyShotHead->prev = core;
    }

    // ------------------------------------------------
    // 液体相の中心移動（ループ前に一度だけ更新）
    // ------------------------------------------------
    if (t >= 240 && t < 480) {
        double cx = pEnemyShotSet->x + sin(t * 0.02) * 60.0;
        double cy = pEnemyShotSet->y + cos(t * 0.03) * 40.0;
        pEnemyShotSet->param_d[0] = cx;
        pEnemyShotSet->param_d[1] = cy;
    }

    // ------------------------------------------------
    // 弾の更新
    // ------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // コアは中心固定
        if (pShot->param_i[0] == 1) {
            pShot->x = pEnemyShotSet->param_d[0];
            pShot->y = pEnemyShotSet->param_d[1];
            pShot = pShot->next;
            continue;
        }

        if (t < 180) {
            // 固体：振動
            double baseX = pShot->param_d[0];
            double baseY = pShot->param_d[1];
            double phase = pShot->param_d[2];
            pShot->x = baseX + sin(t * 0.3 + phase) * 1.5;
            pShot->y = baseY + cos(t * 0.4 + phase) * 1.5;
            pShot->speed = 0.0;
        }
        else if (t < 240) {
            // 融解：直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (t < 480) {
            // 液体：周回
            pShot->param_d[3] += 0.02 + 0.01 * sin(t * 0.1 + pShot->param_d[4]);
            double r = pShot->param_d[4];
            pShot->x = pEnemyShotSet->param_d[0] + cos(pShot->param_d[3]) * r;
            pShot->y = pEnemyShotSet->param_d[1] + sin(pShot->param_d[3]) * r;
            pShot->param_d[4] += sin(t * 0.05 + pShot->param_d[3]) * 0.1;
            if (pShot->param_d[4] < 5.0) pShot->param_d[4] = 5.0;
        }
        else if (t < 540) {
            // 蒸発：加速
            pShot->speed += 0.1;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (t < 720-200) {
            // 気体：直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (t < CYCLE) {
            // 凝縮：中心へ吸引
            double cx = pEnemyShotSet->param_d[0];
            double cy = pEnemyShotSet->param_d[1];
            double dx = cx - pShot->x;
            double dy = cy - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 1.0) {
                pShot->muki = atan2(dy, dx);
                pShot->speed = 2.0 + dist * 0.05;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                pShot->speed = 0.0;
            }
        }
        else {
            // サイクル終了後：そのまま直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_PhaseTransition_DeepSeek()
{
    static int shot_count;
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        shot_count = 0;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定周期で新しい相転移セットを生成
    if (count % CYCLE == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPhaseTransition;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}