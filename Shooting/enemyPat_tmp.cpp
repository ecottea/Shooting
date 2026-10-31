#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：転世「一条戻り橋」
// ============================================================
static void ShotIchijoModoribashi(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 予告音の再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 発射音の再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // プレイヤー方向を基準に、左右に扇状に展開する角度を計算
        double target_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        int numShots = 17;                  // 弾の数（奇数で中心軸を揃える）
        double spread = DX_PI * 0.6;        // 展開角度の範囲（約108度）
        double start_angle = target_muki - spread / 2.0;

        for (int i = 0; i < numShots; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 角度を均等に配置して「橋」のような美しい弧を作る
            pEnemyShot->muki = start_angle + spread * (double)i / (double)(numShots - 1);

            // 初期速度と位置
            pEnemyShot->speed = 3.0;
            pEnemyShot->x = pEnemyShotSet->x + cos(pEnemyShot->muki) * 20.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(pEnemyShot->muki) * 20.0;

            // 状態管理パラメータの初期化
            pEnemyShot->param_i[0] = 0;     // 状態: 0=前進, 1=減速停止, 2=後退
            pEnemyShot->param_i[1] = 50*2;    // 反転を開始するカウント（約0.8秒後）
            pEnemyShot->param_d[0] = 4.5;   // 後退時の最大速度

            // 前進時の弾の種類：青い中玉 (4:青)
            pEnemyShot->kind = img_enemyShotSmallBall[4];
            pEnemyShot->margin = 480;

            // 環状リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int state = pShot->param_i[0];

        if (state == 0) {
            // 【状態0: 前進】
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 指定カウントに達したら減速状態へ移行
            if (pShot->count >= pShot->param_i[1]) {
                pShot->param_i[0] = 1;
            }
        }
        else if (state == 1) {
            // 【状態1: 減速して空中停止】
            pShot->speed -= 0.2; // 徐々に減速

            if (pShot->speed <= 0.0) {
                pShot->speed = 0.0;
                pShot->param_i[0] = 2; // 停止したら後退状態へ

                // ▼ 反転演出 ▼
                // 1. 向きを180度反転させる
                pShot->muki += DX_PI;
                // 2. 弾の種類を「赤い大玉」に変更し、戻ってくる危険を視覚的に強調
                pShot->kind = img_enemyShotLargeBall[0];
            }
            else {
                // 減速中の移動
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (state == 2) {
            // 【状態2: 後退（加速して戻ってくる）】
            if (pShot->speed < pShot->param_d[0]) {
                pShot->speed += 0.15; // 徐々に加速して迫ってくる
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン：転世「一条戻り橋」用
// ============================================================
void EnemyPat_Tmp()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0+100;          // 画面上部で左右に動きながら撃つ配置
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // 左右への緩やかな移動
        enemy.x += 1.2 * (double)muki;

        // 画面端（左右60pxの位置）で折り返し
        if (enemy.x <= 60.0 || enemy.x >= 420.0) {
            muki *= -1;
        }
    }

    // 60フレーム（約1秒）ごとに弾幕セットを生成
    if (count % 60 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotIchijoModoribashi;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->kind = 0;

        // 弾リストのヘッダ初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 環状リストにセットを登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}