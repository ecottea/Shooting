// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：屈折する光の庭（プリズム・ガーデン） ※固定長レーザー版
static void PrismGarden(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回呼び出し時にプリズムを生成
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_heavy)) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 3つのプリズム（中楕円弾）を生成
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pPrism = new sEnemyShot;
            pPrism->kind = img_enemyShotMediumBall[3]; // シアン色の中楕円弾をプリズムに見立てる
            pPrism->param_i[0] = 0; // 0:プリズムフラグ
            pPrism->param_i[1] = i; // プリズム番号(0～2)

            // 円運動のパラメータ設定
            pPrism->param_d[0] = (DX_PI * 2.0 / 3.0) * i; // 初期角度
            pPrism->param_d[1] = 100.0;                   // 半径
            pPrism->param_d[2] = 0.02;                    // 角速度

            // 初期位置
            pPrism->x = enemy.x + pPrism->param_d[1] * cos(pPrism->param_d[0]);
            pPrism->y = enemy.y + 60.0 + pPrism->param_d[1] * sin(pPrism->param_d[0]);
            pPrism->speed = 0.0; // 自前で座標計算するため移動速度は0

            // リストに追加
            pPrism->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pPrism->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pPrism;
            pEnemyShotSet->pEnemyShotHead->prev = pPrism;
        }
    }

    // リスト内のオブジェクトを更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 0) { // プリズムの場合
            // 円運動の更新
            pEnemyShot->param_d[0] += pEnemyShot->param_d[2];
            pEnemyShot->x = enemy.x + pEnemyShot->param_d[1] * cos(pEnemyShot->param_d[0]);
            pEnemyShot->y = enemy.y + 60.0 + pEnemyShot->param_d[1] * sin(pEnemyShot->param_d[0]);

            // プリズムから「固定長レーザー」を接線方向（進行方向）へ発射
            if (pEnemyShotSet->count % 8 == 0) {
                sEnemyShot* pLaser = new sEnemyShot;
                pLaser->kind = img_enemyShotLaser[3]; // シアン色のレーザー(64.0 x 4.0)

                // 発射方向はプリズムの「接線方向」(円の進行方向)
                double tangent_muki = pEnemyShot->param_d[0] + DX_PI / 2.0;

                pLaser->x = pEnemyShot->x;
                pLaser->y = pEnemyShot->y;
                pLaser->muki = tangent_muki;
                pLaser->speed = 4.0; // 速度を持って画面外へ飛んでいく
                pLaser->param_i[0] = 1; // 1:レーザーフラグ
                pLaser->margin = 100;

                // リストに追加
                pLaser->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pLaser->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pLaser;
                pEnemyShotSet->pEnemyShotHead->prev = pLaser;

                // 発射音（連続するので軽量な音）
                if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            // プリズムから二次弾（光の粒）を放射状に発射
            if (pEnemyShotSet->count % 30 == 0) {
                for (int i = 0; i < 3; i++) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    pNewShot->kind = img_enemyShotSmallBall[6]; // 白色の小玉
                    pNewShot->x = pEnemyShot->x;
                    pNewShot->y = pEnemyShot->y;
                    pNewShot->muki = (DX_PI * 2.0 / 6.0) * i + pEnemyShotSet->count * 0.05;
                    pNewShot->speed = 1.8;
                    pNewShot->param_i[0] = 2; // 2:二次弾フラグ

                    // リストに追加
                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }
            }
        }
        pEnemyShot = pEnemyShot->next;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Laser_Qwen()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右にユラユラ移動
        enemy.x += 1.2 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 弾幕の発射 (4秒に1回程度)
    if (count % 280 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = PrismGarden;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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