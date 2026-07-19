// enemyPat_Tmp.cpp
// 「四方手裏剣・旋風」弾幕の実装例

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕関数：四方手裏剣・旋風
// ============================================================
static void ShotShurikenSpin(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回フレームのみ発射処理
    if (pEnemyShotSet->count == 0) {
        // 効果音: 中くらいの重さの敵弾音を使う
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 敵の向き（pEnemyShotSet->muki）を基準に、8方向に手裏剣を放つ
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;

            // 弾の初期位置は敵の位置から少し下にずらす
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 10.0;

            // 8方向（45度刻み）に発射
            double angle = pEnemyShotSet->muki + (i * DX_PI / 4.0);
            pEnemyShot->muki = angle;

            // 速度は一定値 + ランダムで少し揺らす
            pEnemyShot->speed = 2.0 + (GetRand(10) / 10.0);

            // 弾の種類: 菱形弾（手裏剣っぽい見た目）
            // 弾の色: 橙（8）で和風テイスト
            pEnemyShot->kind = img_enemyShotDiamond[8];

            // 旋回弾かどうかを param_i[0] にフラグとして持たせる
            // 0: 通常弾, 1: 旋回弾
            pEnemyShot->param_i[0] = (i % 2 == 1) ? 1 : 0; // 3発に1発を旋回弾に

            // 旋回弾用のパラメータ初期化
            if (pEnemyShot->param_i[0] == 1) {
                pEnemyShot->param_d[0] = 0.0;  // 旋回角度の蓄積
                pEnemyShot->param_d[1] = 0.02; // 旋回の強さ（ラジアン/フレーム）
            }

            pEnemyShot->margin = 480;

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの弾移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 旋回弾かどうかで挙動を分岐
        if (pEnemyShot->param_i[0] == 1) {
            // 旋回弾: 一定時間経過後に円軌道を描く
            if (pEnemyShot->count > 30) {
                pEnemyShot->param_d[0] += pEnemyShot->param_d[1]; // 旋回角度を進める
                pEnemyShot->muki += pEnemyShot->param_d[1];       // 向きを少しずつ回す
            }
        }

        // 共通の移動処理
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体のパターン関数：EnemyPat_Shuriken_Sakana
// ============================================================
void EnemyPat_Shuriken_Sakana()
{
    static int muki;        // 敵の移動方向（1:右, -1:左）
    static int shot_count;  // 弾幕の種類を切り替えるカウンタ
    static int phase;       // 弾幕のフェーズ（0,1,2）

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;    // 画面中央
        enemy.y = 140.0;     // 画面上部寄り
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        phase = 0;
    }
    else {
        // 敵の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60)
            muki *= -1; // 60フレームごとに移動方向反転
    }

    // 弾幕発射間隔
    if (count % 20 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShurikenSpin;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // プレイヤー方向を向く
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // フェーズに応じて弾幕の種類（kind）を変える
        // kind は弾の見た目・色に影響するパラメータとして使う
        pEnemyShotSet->kind = shot_count++;

        // フェーズ管理用パラメータ
        pEnemyShotSet->param_i[0] = phase;

        // フェーズ移行
        if (count > 600) phase = 1;
        if (count > 1200) phase = 2;

        // 弾リストのダミーヘッド作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾セットリストに挿入
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}