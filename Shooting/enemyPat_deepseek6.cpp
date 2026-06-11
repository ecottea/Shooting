// ------------------------------------------------------------
//  東方文花帖 LEVEL EX 「境符「波と粒の境界」」再現パターン
// ------------------------------------------------------------
#include "DxLib.h"
#include "gv.h"
#include <math.h>


// 波動弾幕（正弦波スネーク）
static void ShotWaveSnake(sEnemyShotSet* pEnemyShotSet)
{
    const double SPEED_Y = 1.8;   // 降下速度
    const double AMPLITUDE = 70.0;  // 振幅
    const double FREQ = 0.045; // 周波数
    const int    BULLET_COUNT = 32;     // 波を構成する弾数

    if (pEnemyShotSet->count == 0) {
        // サウンド（紫らしく重めのSE）
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < BULLET_COUNT; ++i) {
            sEnemyShot* p = new sEnemyShot;

            // 初期位置：上部に縦一列、位相を少しずつずらす
            double initY = pEnemyShotSet->y - 200.0 + i * 14.0;
            p->x = pEnemyShotSet->x + AMPLITUDE * sin(FREQ * initY + i * 0.6);
            p->y = initY;
            p->muki = i * 0.6;          // 位相オフセットを保持
            p->speed = SPEED_Y;
            p->kind = img_enemyShotMediumBall[4];  // 青色（中玉）

            // 双方向リストに追加
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 毎フレーム移動：y座標に応じて正弦波でx座標を更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->y += pShot->speed;
        pShot->x = pEnemyShotSet->x + AMPLITUDE * sin(FREQ * pShot->y + pShot->muki);
        pShot = pShot->next;
    }
}

// 粒子弾幕（全方向リング）
static void ShotParticleRing(sEnemyShotSet* pEnemyShotSet)
{
    const int    RING_COUNT = 32;      // 粒の数
    const double SPEED = 2.2;     // 拡散速度（一定）

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < RING_COUNT; ++i) {
            sEnemyShot* p = new sEnemyShot;
            double angle = (2.0 * DX_PI * i) / RING_COUNT;

            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = angle;
            p->speed = SPEED;
            p->kind = img_enemyShotSmallBall[0];  // 赤色（小玉）

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 等速直線運動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体の制御（紫の配置と発射タイミング）
void EnemyPat_Namistubu_DeepSeek()
{
    // 紫は画面上部で固定（文花帖のボスはあまり動かない）
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;          // 少し下げて弾幕の起点に余裕を持たせる
        enemy.maxHp = enemy.hp = 100;  // 撮影ゲーム想定（無敵）
    }

    // 波スネーク：100フレームごとに新規生成（切れ目なく波を維持）
    if (count % 100 == 0) {
        sEnemyShotSet* pWaveSet = new sEnemyShotSet;
        pWaveSet->count = 0;
        pWaveSet->patternFunc = ShotWaveSnake;
        pWaveSet->x = enemy.x;
        pWaveSet->y = enemy.y;
        pWaveSet->muki = 0.0;
        pWaveSet->kind = 0;

        pWaveSet->pEnemyShotHead = new sEnemyShot;
        pWaveSet->pEnemyShotHead->prev = pWaveSet->pEnemyShotHead;
        pWaveSet->pEnemyShotHead->next = pWaveSet->pEnemyShotHead;

        pWaveSet->prev = enemyShotSetHead.prev;
        pWaveSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWaveSet;
        enemyShotSetHead.prev = pWaveSet;
    }

    // 粒リング：60フレームごとに放射
    if (count % 60 == 0) {
        sEnemyShotSet* pRingSet = new sEnemyShotSet;
        pRingSet->count = 0;
        pRingSet->patternFunc = ShotParticleRing;
        pRingSet->x = enemy.x;
        pRingSet->y = enemy.y;
        pRingSet->muki = 0.0;
        pRingSet->kind = 1;

        pRingSet->pEnemyShotHead = new sEnemyShot;
        pRingSet->pEnemyShotHead->prev = pRingSet->pEnemyShotHead;
        pRingSet->pEnemyShotHead->next = pRingSet->pEnemyShotHead;

        pRingSet->prev = enemyShotSetHead.prev;
        pRingSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRingSet;
        enemyShotSetHead.prev = pRingSet;
    }
}