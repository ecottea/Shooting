// 滝（Waterfall）パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <cmath>

// 水滴が落ちて、画面下部で飛沫になる弾セットの動作
static void ShotWaterfallStream(sEnemyShotSet* pEnemyShotSet)
{
    const double STREAM_SPEED = 3.8;
    const double SPLASH_Y = 480.0;

    // 初回：水滴を生成
    if (pEnemyShotSet->count == 0) {
        // 音を鳴らす
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numDrops = 6 + GetRand(3);  // 6～9滴
        for (int i = 0; i < numDrops; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x + GetRand(20) - 10;
            p->y = pEnemyShotSet->y + GetRand(10);
            // ほぼ真下、少し左右にばらつく
            double angleOffset = (GetRand(30) - 15) * DX_PI / 180.0;
            p->muki = DX_PI / 2.0 + angleOffset;
            p->speed = STREAM_SPEED + (GetRand(20) - 10) / 10.0;
            p->kind = img_enemyShotSmallBall[3];  // シアン（水色）

            // 連結リストに追加
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 全弾の移動と、画面下部での飛沫処理
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pEnemyShot->next;  // 削除されるかもしれないので先に避難

        // 水滴が画面下部に達したら飛沫を発生させる
        if (pEnemyShot->y > SPLASH_Y) {
            // 連結リストから外す
            pEnemyShot->prev->next = pEnemyShot->next;
            pEnemyShot->next->prev = pEnemyShot->prev;
            double splashX = pEnemyShot->x;
            // ★ 飛沫の発生位置を SPLASH_Y より上に設定することで再発火を防ぐ
            double splashY = SPLASH_Y - 1.0;
            delete pEnemyShot;

            // 飛沫（上向き＆横方向）の弾を5～8個生成
            int splashNum = 1;
            for (int i = 0; i < splashNum; ++i) {
                sEnemyShot* sp = new sEnemyShot;
                sp->x = splashX;
                sp->y = splashY;   // ここで SPLASH_Y 未満にする
                double baseAngle = -DX_PI / 2.0;  // 真上
                double spread = (GetRand(140) - 70) * DX_PI / 180.0;
                sp->muki = baseAngle + spread;
                sp->speed = 1.0 + GetRand(15) / 10.0;
                sp->kind = img_enemyShotSmallBall[6];  // 白（飛沫）

                // 連結リストの末尾に追加
                sp->prev = pEnemyShotSet->pEnemyShotHead->prev;
                sp->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = sp;
                pEnemyShotSet->pEnemyShotHead->prev = sp;
            }
        }
        else {
            // まだ画面下部に達していない：通常移動
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pNext;
    }
}

// 敵本体のパターン（滝）
void EnemyPat_Waterfall_DeepSeek()
{
    static int moveDir = 1;   // 左右移動方向
    static int streamTimer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
        streamTimer = 0;
    }
    else {
        // 左右移動（画面上部を往復）
        enemy.x += 0.8 * moveDir;
        if (enemy.x > 400.0 || enemy.x < 80.0)
            moveDir *= -1;

        // 一定間隔で水滴の流れ（弾セット）を発生
        ++streamTimer;
        if (streamTimer >= 48) {   // 約18フレームおき
            streamTimer = 0;

            // 3本の流れを作る（左・中央・右）
            for (int i = -1; i <= 1; ++i) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotWaterfallStream;
                pSet->x = enemy.x + i * 96.0;        // 横にずらす
                pSet->y = enemy.y + 12.0;
                pSet->muki = DX_PI / 2.0;            // 真下

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                // 連結リストに追加
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
    }
}