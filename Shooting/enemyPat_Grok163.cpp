#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：雪の結晶（六華結晶弾）
// 既存素材のみ使用
// 主軸：中玉（白）  一次分岐：小玉（シアン）  二次分岐：小玉（白）
static void ShotSnowflake(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 最初のフレームで主軸6本を生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // プレイヤー方向を基準に少しランダム回転を加えて対称性を保ちつつ変化をつける
        double base_muki = pEnemyShotSet->muki + (GetRand(40) - 20) / 180.0 * DX_PI;

        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_muki + i * (DX_PI / 3.0);   // 60度間隔
            pEnemyShot->speed = 1.55;
            pEnemyShot->kind = img_enemyShotMediumBall[6];       // 白の中玉
            pEnemyShot->param_i[0] = 1;  // 種類：1=主軸
            pEnemyShot->param_i[1] = 0;  // 分岐済みフラグ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の移動＋分岐処理
    // リスト末尾に追加するため、現在のループでは新規弾は処理されない
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 主軸から一次分岐（左右±30度）
        if (pShot->param_i[0] == 1 && pShot->param_i[1] == 0 && pShot->count == 20) {
            for (int j = -1; j <= 1; j += 2) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = pShot->x;
                pNew->y = pShot->y;
                pNew->muki = pShot->muki + j * (DX_PI / 6.0);  // ±30度
                pNew->speed = 2.05;
                pNew->kind = img_enemyShotSmallBall[3];         // シアンの小玉
                pNew->param_i[0] = 2;  // 種類：2=一次分岐
                pNew->param_i[1] = 0;

                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
            pShot->param_i[1] = 1;  // 分岐済み
        }
        // 一次分岐から二次分岐（左右±20度）
        else if (pShot->param_i[0] == 2 && pShot->param_i[1] == 0 && pShot->count == 14) {
            for (int j = -1; j <= 1; j += 2) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = pShot->x;
                pNew->y = pShot->y;
                pNew->muki = pShot->muki + j * (DX_PI / 9.0);  // ±20度
                pNew->speed = 2.35;
                pNew->kind = img_enemyShotSmallBall[6];         // 白の小玉
                pNew->param_i[0] = 3;  // 種類：3=二次分岐（これ以上分岐しない）
                pNew->param_i[1] = 0;

                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
            pShot->param_i[1] = 1;  // 分岐済み
        }
        else if (pShot->param_i[0] == 3 && pShot->param_i[1] == 0 && pShot->count == 8) {
            for (int j = -1; j <= 1; j += 2) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = pShot->x;
                pNew->y = pShot->y;
                pNew->muki = pShot->muki + j * (DX_PI / 18.0);
                pNew->speed = 2.65;
                pNew->kind = img_enemyShotSmallBall[4];
                pNew->param_i[0] = 4;
                pNew->param_i[1] = 1;

                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
            pShot->param_i[1] = 1;
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Snowflake_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.75 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 一定間隔で結晶弾幕を生成
    if (count % 35 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSnowflake;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        // プレイヤー方向を基準にする（結晶の向きがプレイヤーを意識する）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
