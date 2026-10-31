#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 転世「一条戻り橋」 弾幕パターン
// ============================================================

// 定数定義
static const double PI = 3.14159265358979323846;
static const double UP_ANGLE = -PI / 2.0; // 上方向（ラジアン）

// 青弾の1周期あたりの角度ずれ
static const double BLUE_ANGLE_SHIFT = -2.0 * PI / 15.0;
// 赤弾の1周期あたりの角度ずれ
static const double RED_ANGLE_SHIFT = 4.0 * PI / 35.0;
// 青弾と赤弾の初期相対角度（開幕ランダムの代わりに固定値を使用）
static const double INITIAL_OFFSET = PI / 8.0;
// 同期間隔（フレーム）
static const int SYNC_INTERVAL = 16*2;
// 弾速
static const double BULLET_SPEED = 2.2*0.75;
// 自機狙い大玉の追加開始フレーム（約10秒）
static const int AIMED_SHOT_START = 600;
// 自機狙い大玉の発射間隔
static const int AIMED_SHOT_INTERVAL = 30*2;

// 弾幕：一条戻り橋（画面下から上へ流れる弾幕）
static void ShotReturningBridge(sEnemyShotSet* pEnemyShotSet)
{
    // 初回のみ効果音を再生し、初期弾を生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 初期角度はランダム（再現性のためGetRandを使用）
        double base_angle = UP_ANGLE + (GetRand(360) - 180) / 180.0 * PI;

        // 8連の粒弾を生成
        for (int i = 0; i < 8; i++) {
            // 青弾
            sEnemyShot* pBlue = new sEnemyShot;
            pBlue->x = pEnemyShotSet->x;
            pBlue->y = pEnemyShotSet->y;
            pBlue->muki = base_angle + i * BLUE_ANGLE_SHIFT;
            pBlue->speed = BULLET_SPEED;
            pBlue->kind = img_enemyShotSmallBall[4]; // 青
            pBlue->margin = 480;
            pBlue->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBlue->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBlue;
            pEnemyShotSet->pEnemyShotHead->prev = pBlue;

            // 赤弾
            sEnemyShot* pRed = new sEnemyShot;
            pRed->x = pEnemyShotSet->x;
            pRed->y = pEnemyShotSet->y;
            pRed->muki = base_angle + i * RED_ANGLE_SHIFT + INITIAL_OFFSET;
            pRed->speed = BULLET_SPEED;
            pRed->margin = 480;
            pRed->kind = img_enemyShotSmallBall[0]; // 赤
            pRed->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRed->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRed;
            pEnemyShotSet->pEnemyShotHead->prev = pRed;
        }
    }

    // 2周期目以降：16フレームごとに新たな弾を生成（角度を蓄積）
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % SYNC_INTERVAL == 0) {
        // 発射回数（何周期目か）
        int shot_index = pEnemyShotSet->count / SYNC_INTERVAL;
        double base_angle = UP_ANGLE + (GetRand(360) - 180) / 180.0 * PI; // 開幕と同様にランダム基準

        for (int i = 0; i < 8; i++) {
            // 青弾
            sEnemyShot* pBlue = new sEnemyShot;
            pBlue->x = pEnemyShotSet->x;
            pBlue->y = pEnemyShotSet->y;
            pBlue->muki = base_angle + (shot_index * BLUE_ANGLE_SHIFT) + i * BLUE_ANGLE_SHIFT;
            pBlue->speed = BULLET_SPEED;
            pBlue->kind = img_enemyShotSmallBall[4]; // 青
            pBlue->margin = 480;
            pBlue->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBlue->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBlue;
            pEnemyShotSet->pEnemyShotHead->prev = pBlue;

            // 赤弾
            sEnemyShot* pRed = new sEnemyShot;
            pRed->x = pEnemyShotSet->x;
            pRed->y = pEnemyShotSet->y;
            pRed->muki = base_angle + (shot_index * RED_ANGLE_SHIFT) + i * RED_ANGLE_SHIFT + INITIAL_OFFSET;
            pRed->speed = BULLET_SPEED;
            pRed->kind = img_enemyShotSmallBall[0]; // 赤
            pRed->margin = 480;
            pRed->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRed->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRed;
            pEnemyShotSet->pEnemyShotHead->prev = pRed;
        }
    }

    // 全弾の移動処理（メインルーチンで行う仕様だが、パターン関数内でも移動を記述）
    // ※コメントアウト：移動はメインルーチンで一括処理する場合、ここは不要
     sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
     while (pShot != pEnemyShotSet->pEnemyShotHead) {
         pShot->x += pShot->speed * cos(pShot->muki);
         pShot->y += pShot->speed * sin(pShot->muki);
         pShot = pShot->next;
     }
}

// 大玉専用のパターン関数（発射と移動処理）
static void ShotAimedLargeBall(sEnemyShotSet* pEnemyShotSet)
{
    // count==0 で大玉を発射
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pBig = new sEnemyShot;
        pBig->x = pEnemyShotSet->x;
        pBig->y = pEnemyShotSet->y;
        pBig->muki = pEnemyShotSet->muki; // 生成時に設定済みの自機狙い角度
        pBig->speed = 1.5;
        pBig->kind = img_enemyShotLargeBall[0]; // 赤い大玉
        pBig->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pBig->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pBig;
        pEnemyShotSet->pEnemyShotHead->prev = pBig;
    }

    // 全弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_Modoribashi_DeepSeek()
{
    static int shot_count;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }

    // 弾幕セットの生成（一定間隔で新しい弾幕セットを追加）
    if (count % 120 == 1 && count <= 120*4) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotReturningBridge;
        // 画面下端付近から弾を発射（一条戻り橋の特徴）
        pEnemyShotSet->x = enemy.x + (GetRand(200+100) - 100-50);
        pEnemyShotSet->y = 480.0 - 10.0; // 画面下端
        pEnemyShotSet->muki = UP_ANGLE;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 約10秒（600フレーム）経過後、自機狙いの大玉を追加
    if (count >= AIMED_SHOT_START && count % AIMED_SHOT_INTERVAL == 0) {
        // 自機狙いの角度を計算
        double aimed_angle = atan2(player.y - enemy.y, player.x - enemy.x);

        // 大玉を管理する弾幕セットを生成
        sEnemyShotSet* pBigShotSet = new sEnemyShotSet;
        pBigShotSet->count = 0;
        pBigShotSet->patternFunc = ShotAimedLargeBall; // 大玉専用のパターン関数
        pBigShotSet->x = enemy.x;
        pBigShotSet->y = enemy.y + 10.0;
        pBigShotSet->muki = aimed_angle;
        pBigShotSet->kind = shot_count++;

        pBigShotSet->pEnemyShotHead = new sEnemyShot;
        pBigShotSet->pEnemyShotHead->prev = pBigShotSet->pEnemyShotHead;
        pBigShotSet->pEnemyShotHead->next = pBigShotSet->pEnemyShotHead;

        pBigShotSet->prev = enemyShotSetHead.prev;
        pBigShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBigShotSet;
        enemyShotSetHead.prev = pBigShotSet;
    }
}