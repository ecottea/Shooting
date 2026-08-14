// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// エターナルフォースブリザード
// ============================================================

// ------------------------------------------------------------
// 吹雪：螺旋状に広がる雪片
// ------------------------------------------------------------
static void ShotBlizzard(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 20; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            double a = DX_PI * 2.0 * i / 20.0;
            double r = 8.0 + (double)(i % 5) * 3.0;

            pShot->x = pEnemyShotSet->x + cos(a) * r;
            pShot->y = pEnemyShotSet->y + sin(a) * r;

            pShot->muki = a;
            pShot->speed = 1.0 + (i % 4) * 0.18;

            // 小玉：青・シアン・白を使用
            if (i % 3 == 0)
                pShot->kind = img_enemyShotSmallBall[3];
            else if (i % 3 == 1)
                pShot->kind = img_enemyShotSmallBall[4];
            else
                pShot->kind = img_enemyShotSmallBall[6];

            // 生成位置からの角度
            pShot->param_d[0] = a;
            pShot->param_d[1] = r;
            pShot->param_d[2] = 0.018 + (i % 4) * 0.004 - 0.010;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count;

        // 徐々に外側へ広がる
        double r = pShot->param_d[1] + t * pShot->speed * 0.75;

        // 時間とともに角度が変化して螺旋になる
        double a = pShot->param_d[0] + t * pShot->param_d[2];

        // わずかに波打たせ、吹雪らしい動きを作る
        double wave = sin(t * 0.055 + pShot->param_d[0] * 3.0) * 7.0;

        pShot->x = pEnemyShotSet->x
            + cos(a) * (r + wave);

        pShot->y = pEnemyShotSet->y
            + sin(a) * (r + wave);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 凍結領域：敵の周囲を回転する氷晶リング
// ------------------------------------------------------------
static void ShotFreezeRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 24; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            double a = DX_PI * 2.0 * i / 24.0;

            pShot->x = pEnemyShotSet->x + cos(a) * 130.0;
            pShot->y = pEnemyShotSet->y + sin(a) * 130.0;
            pShot->margin = 240;

            pShot->muki = a;
            pShot->speed = 0.0;

            if (i % 2 == 0)
                pShot->kind = img_enemyShotMediumBall[3];
            else
                pShot->kind = img_enemyShotDiamond[6];

            pShot->param_d[0] = a;
            pShot->param_d[1] = 130.0;
            pShot->param_d[2] = (i % 2 == 0) ? 0.010 : -0.010;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count;

        // 収縮と膨張を繰り返す
        double phase = fmod(t, 150.0) / 150.0;
        double r;

        if (phase < 0.5)
            r = 130.0 - phase * 2.0 * 100.0;
        else
            r = 30.0 + (phase - 0.5) * 2.0 * 100.0;

        double a = pShot->param_d[0]
            + t * pShot->param_d[2];

        pShot->x = pEnemyShotSet->x + cos(a) * r;
        pShot->y = pEnemyShotSet->y + sin(a) * r;

        if (pShot->count == 75 * 5) pShot->margin = -9999;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 永遠凍結：収束したリングから全方向へ炸裂
// ------------------------------------------------------------
static void ShotEternalFreeze(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 32; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            double a = DX_PI * 2.0 * i / 32.0;

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            pShot->muki = a;
            pShot->speed = 2.0 + (i % 4) * 0.25;

            if (i % 3 == 0)
                pShot->kind = img_enemyShotLargeBall[4];
            else if (i % 3 == 1)
                pShot->kind = img_enemyShotLargeBall[3];
            else
                pShot->kind = img_enemyShotLargeBall[6];

            pShot->param_d[0] = a;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count;

        // 最初は中心で停止し、その後一気に炸裂
        double delay = 18.0;

        if (t < delay) {
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
        }
        else {
            double d = (t - delay) * pShot->speed;

            pShot->x = pEnemyShotSet->x
                + cos(pShot->param_d[0]) * d;

            pShot->y = pEnemyShotSet->y
                + sin(pShot->param_d[0]) * d;
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================

void EnemyPat_EternalForceBlizzard_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 155.0;

        enemy.maxHp = enemy.hp = 200;

        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.65 * (double)muki;

        if (enemy.x < 100.0 || enemy.x > 380.0)
            muki *= -1;
    }

    // --------------------------------------------------------
    // 吹雪
    // --------------------------------------------------------
    if (count > 30 && count % 30 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotBlizzard;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 15.0;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --------------------------------------------------------
    // 凍結領域
    // 120フレームごとに発生
    // --------------------------------------------------------
    if (count > 120 && count % 120 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotFreezeRing;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --------------------------------------------------------
    // 永遠凍結
    // 240フレームごとに中心から炸裂
    // --------------------------------------------------------
    if (count > 240 && count % 240 == 121) {
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotEternalFreeze;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}