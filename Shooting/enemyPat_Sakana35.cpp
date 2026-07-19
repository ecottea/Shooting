// enemyPat_Tmp.cpp
// ワインダー型弾幕「蛇行レーザー・スラッシュ」の実装

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 素材一覧（enemyPat_sampleForAI.cpp より）
// 弾の種類: 
//   img_enemyShotSmallBall[i]   (i=0..8)
//   img_enemyShotMediumBall[i]  (i=0..8)
//   img_enemyShotLargeBall[i]   (i=0..8)
//   img_enemyShotBullet[i]      (i=0..8)
//   img_enemyShotScale[i]       (i=0..8)
//   img_enemyShotDiamond[i]     (i=0..8)
// 弾の色: 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
// 効果音:
//   sound_enemyShot_light
//   sound_enemyShot_medium
//   sound_enemyShot_heavy
//   sound_enemyShot_extreme

// ワインダー型弾幕：蛇行レーザー・スラッシュ
static void ShotSnakeLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回フレームのみ音と弾生成
    if (pEnemyShotSet->count == 0) {
        // 効果音: 中くらいの弾発射音を使用
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 弾の総数（レーザーの長さ）
        const int BULLET_COUNT = 24;

        // 基準角度（自機方向）
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 波の周期・振幅（調整用）
        const double WAVE_SPEED = 0.08;  // うねりの速さ
        const double WAVE_AMPLITUDE = 0.6; // うねりの大きさ（ラジアン）

        for (int i = 0; i < BULLET_COUNT; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置を少しずつ前にずらす（レーザー状に並べる）
            pEnemyShot->x = pEnemyShotSet->x + i * 8.0 * cos(baseAngle);
            pEnemyShot->y = pEnemyShotSet->y + i * 8.0 * sin(baseAngle);

            // うねりを加える角度
            double waveOffset = sin(i * WAVE_SPEED) * WAVE_AMPLITUDE;
            pEnemyShot->muki = baseAngle + waveOffset;

            // 速度は一定（レーザー状に見えるように）
            pEnemyShot->speed = 3.0;

            // 弾の種類: 銃弾（細長い見た目でレーザーっぽく）
            // 色: シアン（青緑）で統一
            pEnemyShot->kind = img_enemyShotBullet[3]; // 3=シアン

            // 弱いホーミング成分を入れるパラメータ（後で使う）
            pEnemyShot->param_d[0] = 0.02; // ホーミング強度
            pEnemyShot->param_d[1] = baseAngle; // 基準角度を保存

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの弾移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 自機方向への弱いホーミング
        double targetAngle = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        double homingStrength = pEnemyShot->param_d[0]; // 0.02
        pEnemyShot->muki += homingStrength * (targetAngle - pEnemyShot->muki);

        // 通常移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（関数名は指定通り）
void EnemyPat_Winder_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の左右移動（サンプルと同じ）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で弾幕セットを生成
    if (count % 40 == 0) { // 40フレームごとに発射
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSnakeLaser; // 上で作ったワインダー弾幕
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++; // 種類を変えるためのカウンタ

        // 弾リストのダミーヘッドを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}