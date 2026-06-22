// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：クモの巣
static void ShotSpiderWeb(sEnemyShotSet* pEnemyShotSet)
{
    // 放射状の糸（count == 0 のときのみ生成）
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numRadius = 120; // 放射状の糸の数
        for (int i = 0; i < numRadius; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 基準角度から等間隔に配置
            pEnemyShot->muki = pEnemyShotSet->muki + (2.0 * DX_PI / numRadius) * i;
            pEnemyShot->speed = 4.0; // 速い速度で直進
            pEnemyShot->kind = img_enemyShotBullet[6]; // 白の銃弾（細長い形状を糸に見立てる）

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 横糸（同心円状に広がる弾）
    // count が 20 から 120 まで、20フレームごとに生成
    if (pEnemyShotSet->count >= 20 && pEnemyShotSet->count <= 120 && pEnemyShotSet->count % 20 == 0) {
        int numSpiral = 32; // 横糸の弾の数
        for (int i = 0; i < numSpiral; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 放射状の糸と交互になるように角度をずらす (7.5度)
            pEnemyShot->muki = pEnemyShotSet->muki + (2.0 * DX_PI / numSpiral) * i + (DX_PI / numSpiral / 2.0);

            // GetRand(20) で 0～20 の整数を取得し、100.0で割って 0.00～0.20 の揺らぎを速度に加える
            pEnemyShot->speed = 1.5 + GetRand(20) / 100.0;

            pEnemyShot->kind = img_enemyShotScale[3]; // シアンの鱗弾（円周上に並べるとリング状になる）

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SpiderWeb_Qwen()
{
    if (count == 1) {
        // 画面中央上部に配置 (クモが天井からぶら下がるイメージ)
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵はゆっくり左右に動く
        enemy.x += sin(count * 0.02) * 1.0;
    }

    // 一定間隔 (120フレームごと) でクモの巣ShotSetを生成
    if (count % 80 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiderWeb;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // 生成ごとに基準角度をずらすことで、網目全体が回転する演出
        pEnemyShotSet->muki = count * 0.05;

        // ShotSetの初期化とリストへの追加
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}