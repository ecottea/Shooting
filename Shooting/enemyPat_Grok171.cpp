#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：バーンズリーのシダ風（成長する剛体シルエット）
static void ShotFern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 共通の移動ベクトル（全体が同じ速度で動くことで相対形状を維持）
    const double common_muki = DX_PI / 2.0;          // 真下
    const double common_speed = 1.35;
    const double vx = common_speed * cos(common_muki);
    const double vy = common_speed * sin(common_muki);

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 茎（主軸）の生成
    if (pEnemyShotSet->count == 1) {
        for (int i = 0; i < 11*2; i++) {
            pEnemyShot = new sEnemyShot;
            double rdx = (GetRand(6) - 3) * 0.7;
            double rdy = 12.0 + i * 15.5*1.1;
            pEnemyShot->x = pEnemyShotSet->x + rdx + vx * pEnemyShotSet->count;
            pEnemyShot->y = pEnemyShotSet->y + rdy + vy * pEnemyShotSet->count;
            pEnemyShot->muki = common_muki + (GetRand(8) - 4) / 180.0 * DX_PI;
            pEnemyShot->speed = common_speed;
            pEnemyShot->kind = img_enemyShotMediumBall[2];     // 緑・中玉
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 左主枝
    if (pEnemyShotSet->count == 18) {
        for (int i = 0; i < 8*2; i++) {
            pEnemyShot = new sEnemyShot;
            double rdx = -8.0 - i * 13.0*1.1 + (GetRand(5) - 2);
            double rdy = 55.0 + i * 9.5*1.1 + (GetRand(5) - 2);
            pEnemyShot->x = pEnemyShotSet->x + rdx + vx * pEnemyShotSet->count;
            pEnemyShot->y = pEnemyShotSet->y + rdy + vy * pEnemyShotSet->count;
            pEnemyShot->muki = common_muki + (GetRand(8) - 4) / 180.0 * DX_PI;
            pEnemyShot->speed = common_speed;
            pEnemyShot->kind = img_enemyShotMediumOval[2];     // 緑・中楕円
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 右主枝
    if (pEnemyShotSet->count == 22) {
        for (int i = 0; i < 8*2; i++) {
            pEnemyShot = new sEnemyShot;
            double rdx = 8.0 + i * 13.0*1.1 + (GetRand(5) - 2);
            double rdy = 55.0 + i * 9.5*1.1 + (GetRand(5) - 2);
            pEnemyShot->x = pEnemyShotSet->x + rdx + vx * pEnemyShotSet->count;
            pEnemyShot->y = pEnemyShotSet->y + rdy + vy * pEnemyShotSet->count;
            pEnemyShot->muki = common_muki + (GetRand(8) - 4) / 180.0 * DX_PI;
            pEnemyShot->speed = common_speed;
            pEnemyShot->kind = img_enemyShotMediumOval[2];     // 緑・中楕円
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 左小葉群
    if (pEnemyShotSet->count == 38) {
        // 主枝に沿った3箇所の小葉クラスター
        for (int cluster = 0; cluster < 3*2; cluster++) {
            double base_rdx = -20.0 - cluster * 28.0;
            double base_rdy = 70.0 + cluster * 22.0;
            for (int i = 0; i < 6; i++) {
                pEnemyShot = new sEnemyShot;
                double ang = -0.9 + i * 0.28 + (GetRand(10) - 5) / 180.0 * DX_PI;
                double dist = 6.0 + GetRand(8);
                double rdx = base_rdx + dist * cos(ang);
                double rdy = base_rdy + dist * sin(ang) * 0.7;
                pEnemyShot->x = pEnemyShotSet->x + rdx + vx * pEnemyShotSet->count;
                pEnemyShot->y = pEnemyShotSet->y + rdy + vy * pEnemyShotSet->count;
                pEnemyShot->muki = common_muki + (GetRand(10) - 5) / 180.0 * DX_PI;
                pEnemyShot->speed = common_speed;
                // 小玉と鱗弾を混ぜて葉の質感を出す
                if (i % 2 == 0) {
                    pEnemyShot->kind = img_enemyShotSmallBall[2];   // 緑・小玉
                }
                else {
                    pEnemyShot->kind = img_enemyShotScale[2];       // 緑・鱗弾
                }
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 右小葉群
    if (pEnemyShotSet->count == 42) {
        for (int cluster = 0; cluster < 3*2; cluster++) {
            double base_rdx = 20.0 + cluster * 28.0;
            double base_rdy = 70.0 + cluster * 22.0;
            for (int i = 0; i < 6; i++) {
                pEnemyShot = new sEnemyShot;
                double ang = 0.9 - i * 0.28 + (GetRand(10) - 5) / 180.0 * DX_PI;
                double dist = 6.0 + GetRand(8);
                double rdx = base_rdx + dist * cos(ang);
                double rdy = base_rdy + dist * sin(ang) * 0.7;
                pEnemyShot->x = pEnemyShotSet->x + rdx + vx * pEnemyShotSet->count;
                pEnemyShot->y = pEnemyShotSet->y + rdy + vy * pEnemyShotSet->count;
                pEnemyShot->muki = common_muki + (GetRand(10) - 5) / 180.0 * DX_PI;
                pEnemyShot->speed = common_speed;
                if (i % 2 == 0) {
                    pEnemyShot->kind = img_enemyShotSmallBall[2];   // 緑・小玉
                }
                else {
                    pEnemyShot->kind = img_enemyShotScale[2];       // 緑・鱗弾
                }
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 先端の細かい葉
    if (pEnemyShotSet->count == 55) {
        for (int i = 0; i < 9; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = -0.55 + i * 0.14;
            double dist = 8.0 + GetRand(6);
            double rdx = dist*2 * cos(ang);
            double rdy = 175.0 + dist*2 * sin(ang) * 0.8;
            pEnemyShot->x = pEnemyShotSet->x + rdx + vx * pEnemyShotSet->count;
            pEnemyShot->y = pEnemyShotSet->y + rdy + vy * pEnemyShotSet->count;
            pEnemyShot->muki = common_muki + (GetRand(12) - 6) / 180.0 * DX_PI;
            pEnemyShot->speed = common_speed;
            pEnemyShot->kind = img_enemyShotSmallBall[2];       // 緑・小玉
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_BarnsleyFern_Grok()
{
    static int muki;
    static int fern_timer;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        fern_timer = 0;
    }
    else {
        // 緩やかな左右移動
        enemy.x += 0.55 * (double)muki;
        if (enemy.x < 140.0) muki = 1;
        if (enemy.x > 340.0) muki = -1;
    }

    // 一定間隔でシダ弾幕を生成
    if (count >= 40 && (count - 40) % 210 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
