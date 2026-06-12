// enemyPat_tmp.cpp（修正版）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 滝をモチーフにした弾幕（自由発想版・修正）
static void ShotWaterfallFree(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // パラメータ群
    const int LANE_COUNT = 7;          // 滝の筋の本数
    const double LANE_SPACING = 15.0;  // 筋の間隔
    const int SPAWN_INTERVAL = 3;      // 弾生成間隔（フレーム）
    const double FALL_SPEED = 4.0;     // 落下速度
    const double SPLASH_SPEED = 2.5;   // 跳ね返り速度
    const int SPLASH_LIFE = 60;        // 跳ね返り弾の寿命（フレーム）

    // === 滝の筋の生成 ===
    if (pEnemyShotSet->count % SPAWN_INTERVAL == 0 && pEnemyShotSet->count < 300) {
        // 軽い発射音
        if (pEnemyShotSet->count % (2 * SPAWN_INTERVAL)) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < LANE_COUNT; i++) {
            pEnemyShot = new sEnemyShot;

            // 滝の源（敵の少し上）を基準に、左右に筋を配置
            double laneOffsetX = (i - (LANE_COUNT - 1) / 2.0) * LANE_SPACING;
            pEnemyShot->x = pEnemyShotSet->x + laneOffsetX;
            pEnemyShot->y = pEnemyShotSet->y;

            // 真下方向に移動
            pEnemyShot->muki = DX_PI / 2.0; // 下向き
            pEnemyShot->speed = FALL_SPEED;

            // 滝の筋は「中玉・青系」で統一して水の流れを表現
            pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // === 既存の弾の移動と跳ね返り処理 ===
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pEnemyShot->next; // 安全に次へ進むため

        // 落下中の弾
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 画面下付近で跳ね返り弾を生成
        if (pEnemyShot->y > min(pEnemyShotSet->count * 3, 480)) {
            // 左右どちらかに跳ね返る方向
            double splashMuki = -DX_PI * GetRand(180) / 180.0; // 斜め上方向

            // 跳ね返り弾を新規生成
            sEnemyShot* pSplash = new sEnemyShot;
            pSplash->x = pEnemyShot->x;
            pSplash->y = pEnemyShot->y;
            pSplash->muki = splashMuki;
            pSplash->speed = SPLASH_SPEED;
            pSplash->kind = img_enemyShotSmallBall[6]; // 白（水しぶき）

            // リストに追加
            pSplash->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pSplash->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pSplash;
            pEnemyShotSet->pEnemyShotHead->prev = pSplash;

            // 元の落下弾は削除
            pEnemyShot->prev->next = pEnemyShot->next;
            pEnemyShot->next->prev = pEnemyShot->prev;
            delete pEnemyShot;
        }

        pEnemyShot = pNext;
    }
}

void EnemyPat_Waterfall_Sakana()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (count % 360 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWaterfallFree; // ← ここで指定
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y - 30.0; // 敵の少し上を滝の源に
        pEnemyShotSet->muki = 0.0; // 方向は ShotWaterfallFree 内で制御

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}