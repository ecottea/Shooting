// enemyPat_fountain.cpp
// 天恵の噴泉（ブレス・オブ・セタシア）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 噴泉パターン（重力落下＋時間差展開）
// ------------------------------------------------------------
static void ShotFountain(sEnemyShotSet* pEnemyShotSet)
{
    // ----- 発射イベント -------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 発射音（中程度）
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // ① 主噴出飛沫（画面上部から放射）
        int num = 250;
        for (int i = 0; i < num; i++) {
            sEnemyShot* p = new sEnemyShot;

            p->x = pEnemyShotSet->x + GetRand(80) - 40;   // 横ばらつき ±40
            p->y = pEnemyShotSet->y - 10.0;                                   // 噴出先端（画面上部）
            // 上方向（-PI/2）を中心に ±30度
            double angle = -DX_PI / 2 + (GetRand(60) - 30) * DX_PI / 180.0;
            p->muki = angle;
            p->speed = 2.0 + GetRand(1000) / 100.0;         // 2.0 ～ 3.5

            p->kind = img_enemyShotSmallBall[4];           // 小玉・青
            p->margin = 50;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
    else if (pEnemyShotSet->count == 40) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        
        // ② 二次分裂飛沫（画面中段に小滴が出現）
        int num2 = 100;
        for (int i = 0; i < num2; i++) {
            sEnemyShot* p = new sEnemyShot;

            p->x = 40.0 + GetRand(400);                    // 画面横方向に散らす
            p->y = 220.0 + GetRand(40) + 10;                    // y=220 ～ 260
            // 下方向中心（PI/2） ±60度
            double angle = DX_PI / 2 + (GetRand(120) - 60) * DX_PI / 180.0;
            p->muki = angle;
            p->speed = 1.0 + GetRand(100) / 100.0;         // 1.0 ～ 2.0

            p->kind = img_enemyShotSmallBall[3];           // 小玉・シアン

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
    else if (pEnemyShotSet->count == 70) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        
        // ③ 余韻の飛沫（敵本体からポタポタ）
        int num3 = 10;
        for (int i = 0; i < num3; i++) {
            sEnemyShot* p = new sEnemyShot;

            p->x = pEnemyShotSet->x + GetRand(30) - 15;    // 噴気孔付近
            p->y = pEnemyShotSet->y + 10.0;
            // 真下近辺 ±15度
            double angle = DX_PI / 2 + (GetRand(30) - 15) * DX_PI / 180.0;
            p->muki = angle;
            p->speed = 0.5 + GetRand(50) / 100.0;          // 0.5 ～ 1.0

            p->kind = img_enemyShotSmallBall[6];           // 小玉・白

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // ----- 毎フレームの移動（重力を適用） --------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double vx = pShot->speed * cos(pShot->muki);
        double vy = pShot->speed * sin(pShot->muki);
        vy += 0.08;                 // 重力加速度

        pShot->x += vx;
        pShot->y += vy;

        pShot->speed = hypot(vx, vy);
        pShot->muki = atan2(vy, vx);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Spout_DeepSeek()
{
    static int muki;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.7 * muki;
        if (enemy.x < 60)  muki = 1;
        if (enemy.x > 420) muki = -1;
    }

    // 一定間隔で噴泉弾幕セットを生成
    if (count % 120 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFountain;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}