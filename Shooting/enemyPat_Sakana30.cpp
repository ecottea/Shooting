// enemyPat_shotgun.cpp
// ショットガンをモチーフにした弾幕パターン実装

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ショットガン弾幕パターン（敵の弾）
// ============================================================

// ショットガン弾幕：扇状に広がる弾を一度に撃つ
static void ShotShotgun(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // pEnemyShotSet->count == 0 のフレームで一度だけ弾を生成
    if (pEnemyShotSet->count == 0) {
        // 効果音：ショットガンらしい重めの音
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // ショットガンの弾数（扇状に広がる弾の本数）
        const int SHOT_COUNT = 7 * 3;

        // ショットガンの拡散角度（ラジアン）
        const double SPREAD_ANGLE = DX_PI / 4.0; // 45度

        // ショットガンの中心方向（プレイヤー方向）
        double baseMuki = pEnemyShotSet->muki;

        // 各弾の方向を計算して生成
        for (int i = 0; i < SHOT_COUNT; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置は敵の位置から少しだけ前に出す
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 扇状に広がる向きを計算
            // i=0 が真ん中、i=1,2... が左右に広がる
            double offset = (i - (SHOT_COUNT - 1) / 2.0) / (SHOT_COUNT - 1) * SPREAD_ANGLE;
            pEnemyShot->muki = baseMuki + offset;

            // 弾速：ショットガンらしく近距離で速いが、遠くでは少し遅くなるような設定
            // ここでは基本速さ + ランダムで少しばらつきを付ける
            pEnemyShot->speed = (300 + GetRand(200)) / 100.0; // 3.0〜5.0 くらい

            // 弾の種類と色：銃弾（img_enemyShotBullet）をベースに、色をランダムで変える
            // kind の値によって弾の色が変わる仕様を利用
            int colorIndex = GetRand(8); // 0〜8（9色）のうちランダム
            pEnemyShot->kind = img_enemyShotBullet[colorIndex];

            // パラメータに「ショットガン弾」であることを記録しておく（必要なら）
            pEnemyShot->param_i[0] = 1; // ショットガン弾フラグ

            // 弾をリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム、弾を移動させる
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体のパターン（指定された関数名）
// ============================================================
void EnemyPat_Shotgun_Sakana()
{
    static int muki;        // 敵の移動方向（1:右, -1:左）
    static int shot_count;  // ショットの種類・色を変えるためのカウンタ

    if (count == 1) {
        // 初期化
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の左右移動（ショットガンらしくあまり速く動かない）
        enemy.x += 0.5 * (double)muki;
        if (count % 120 == 60) muki *= -1; // 一定間隔で方向反転
    }

    // ショットガン発射間隔（例：30フレームに1回）
    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        // 初期化
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShotgun; // ショットガン弾幕パターン
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0; // 敵の少し下から発射

        // プレイヤー方向を向く
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // kind を shot_count で回すことで、弾の種類・色を変える
        pEnemyShotSet->kind = shot_count++;

        // 弾リストのダミーヘッドを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}