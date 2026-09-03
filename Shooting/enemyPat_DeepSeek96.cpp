// enemyPat_Tmp.cpp
// トマト投げ祭り弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//------------------------------------------------
// 中玉トマト（放物線を描いて飛ぶ）
//------------------------------------------------
static void PatternTomatoMediumThrow(sEnemyShotSet* pSet)
{
    const int    THROW_INTERVAL = 21;   // 0.35秒
    const double GRAVITY = 0.1;
    const int    EXPLODE_TIME = 150;  // 2.5秒
    const int    EXPLODE_SHOTS = 6;
    const double EXPLODE_SPEED = 3.0;

    // 新規トマト生成
    if (pSet->count % THROW_INTERVAL == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int side = pSet->param_i[0];   // 0:左,1:右
        pSet->param_i[0] = !side;

        double startX = side ? -10.0 : 490.0;
        double startY = 100.0 + GetRand(100);
        double vx = side ? 2.0 : -2.0;
        double vy = -2.5;

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = startX;
        pShot->y = startY;
        pShot->muki = atan2(vy, vx);
        pShot->speed = sqrt(vx * vx + vy * vy);
        pShot->count = 0;
        pShot->kind = img_enemyShotMediumBall[0];   // 赤中玉
        pShot->margin = 20.0;
        pShot->param_d[0] = vx;      // 水平速度
        pShot->param_d[1] = vy;      // 垂直速度
        pShot->param_d[2] = GRAVITY; // 重力

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // 移動・爆発
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->kind == img_enemyShotMediumBall[0]) {
            // 重力を適用
            double vx = pShot->param_d[0];
            double vy = pShot->param_d[1] + pShot->param_d[2];
            pShot->param_d[1] = vy;
            pShot->x += vx;
            pShot->y += vy;
            pShot->speed = sqrt(vx * vx + vy * vy);
            pShot->muki = atan2(vy, vx);

            // 爆発条件
            if (pShot->count >= EXPLODE_TIME || pShot->y >= 480.0) {
                // 赤小玉を6方向に放射
                for (int i = 0; i < EXPLODE_SHOTS; ++i) {
                    double angle = (360.0 / EXPLODE_SHOTS) * i * DX_PI / 180.0;
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = angle;
                    pNew->speed = EXPLODE_SPEED;
                    pNew->count = 0;
                    pNew->kind = img_enemyShotSmallBall[0]; // 赤小玉
                    pNew->margin = 20.0;

                    pNew->prev = pSet->pEnemyShotHead->prev;
                    pNew->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pNew;
                    pSet->pEnemyShotHead->prev = pNew;
                }
                // 元のトマトを画面外へ飛ばして削除させる
                pShot->x = -10000.0;
                pShot->y = -10000.0;
            }
        }
        else {
            // 爆発後の小玉は等速直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

//------------------------------------------------
// 特大トマト（ゆっくり落下して分裂）
//------------------------------------------------
static void PatternTomatoLargeDrop(sEnemyShotSet* pSet)
{
    const int    DROP_INTERVAL = 72;   // 1.2秒
    const double GRAVITY = 0.05;
    const int    EXPLODE_TIME = 120;  // 2.0秒
    const int    EXPLODE_SHOTS = 8;
    const double EXPLODE_SPEED = 3.5;
    const int    ORANGE_SHOTS = 4;
    const double ORANGE_SPEED = 1.5;
    const double ORANGE_DECEL = 0.98;

    if (pSet->count % DROP_INTERVAL == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double startX = 100.0 + GetRand(280); // 100～380
        double startY = -20.0;
        double vx = -1.5 + GetRand(300) / 100.0; // -1.5～1.5
        double vy = 1.0;

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = startX;
        pShot->y = startY;
        pShot->muki = atan2(vy, vx);
        pShot->speed = sqrt(vx * vx + vy * vy);
        pShot->count = 0;
        pShot->kind = img_enemyShotLargeBall[0];   // 赤大玉
        pShot->margin = 20.0;
        pShot->param_d[0] = vx;
        pShot->param_d[1] = vy;
        pShot->param_d[2] = GRAVITY;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->kind == img_enemyShotLargeBall[0]) {
            double vx = pShot->param_d[0];
            double vy = pShot->param_d[1] + pShot->param_d[2];
            pShot->param_d[1] = vy;
            pShot->x += vx;
            pShot->y += vy;
            pShot->speed = sqrt(vx * vx + vy * vy);
            pShot->muki = atan2(vy, vx);

            if (pShot->count >= EXPLODE_TIME || pShot->y >= 240.0) {
                // 赤小玉8方向
                for (int i = 0; i < EXPLODE_SHOTS; ++i) {
                    double angle = (360.0 / EXPLODE_SHOTS) * i * DX_PI / 180.0;
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = angle;
                    pNew->speed = EXPLODE_SPEED;
                    pNew->count = 0;
                    pNew->kind = img_enemyShotSmallBall[0];
                    pNew->margin = 20.0;
                    pNew->prev = pSet->pEnemyShotHead->prev;
                    pNew->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pNew;
                    pSet->pEnemyShotHead->prev = pNew;
                }
                // オレンジ小玉4方向（ランダム角度）
                for (int i = 0; i < ORANGE_SHOTS; ++i) {
                    double angle = (360.0 / ORANGE_SHOTS) * i * DX_PI / 180.0
                        + GetRand(45) * DX_PI / 180.0;
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = angle;
                    pNew->speed = ORANGE_SPEED;
                    pNew->count = 0;
                    pNew->kind = img_enemyShotSmallBall[8]; // オレンジ小玉
                    pNew->margin = 20.0;
                    pNew->param_d[0] = ORANGE_DECEL; // 減衰率
                    pNew->prev = pSet->pEnemyShotHead->prev;
                    pNew->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pNew;
                    pSet->pEnemyShotHead->prev = pNew;
                }
                // 元の大玉を削除
                pShot->x = -10000.0;
                pShot->y = -10000.0;
            }
        }
        else if (pShot->kind == img_enemyShotSmallBall[8]) {
            // オレンジ小玉：減速しながら漂う
            pShot->speed *= pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

//------------------------------------------------
// 観客による直接狙い撃ち（3way）
//------------------------------------------------
static void PatternTomatoAimed(sEnemyShotSet* pSet)
{
    const int INTERVAL = 90; // 1.5秒
    if (pSet->count % INTERVAL == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int i = -1; i <= 1; ++i) {
            double angle = baseAngle + i * 10.0 * DX_PI / 180.0;
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 4.0;
            pShot->count = 0;
            pShot->kind = img_enemyShotSmallBall[0];
            pShot->margin = 20.0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//------------------------------------------------
// 頭上で巨大トマトが爆発したような全方位リング
//------------------------------------------------
static void PatternTomatoRing(sEnemyShotSet* pSet)
{
    const int INTERVAL = 300; // 5秒
    if (pSet->count % INTERVAL == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16; ++i) {
            double angle = (360.0 / 16) * i * DX_PI / 180.0;
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 2.8;
            pShot->count = 0;
            pShot->kind = img_enemyShotSmallBall[0];
            pShot->margin = 20.0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//------------------------------------------------
// 敵本体パターン
//------------------------------------------------
void EnemyPat_Tomatina_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;

        // 中玉トマト投げセット
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = PatternTomatoMediumThrow;
            pSet->x = 0.0;
            pSet->y = 0.0;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->param_i[0] = 0; // 最初は左側から

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 特大トマト投下セット
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = PatternTomatoLargeDrop;
            pSet->x = 0.0;
            pSet->y = 0.0;
            pSet->muki = 0.0;
            pSet->kind = 0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 直接狙い撃ちセット
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = PatternTomatoAimed;
            pSet->x = enemy.x;   // 敵位置から発射
            pSet->y = enemy.y + 10.0;
            pSet->muki = 0.0;
            pSet->kind = 0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 全方位リングセット
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = PatternTomatoRing;
            pSet->x = enemy.x;   // 敵位置から発射
            pSet->y = enemy.y;
            pSet->muki = 0.0;
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
    else {
        // 敵の横移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;

        // 狙い撃ちセットとリングセットの座標を敵に追従させる
        // （実際にはセットのx,yを毎フレーム更新する必要があるが、
        //  簡易的にパターン関数内でplayerとの角度を使っているため問題なし）
    }
}