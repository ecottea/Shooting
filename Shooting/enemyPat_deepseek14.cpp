// enemyPat_toxicSpore.cpp
// 毒をモチーフにした弾幕パターン：毒胞子 (Toxic Spore)
//   - ゆっくり降下しながら左右に漂う敵
//   - 一定間隔で胞子の輪を放出
//   - 胞子は一定時間後に破裂し、小さな胞子をばらまく

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 毒胞子の挙動パターン
static void ToxicSpore(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回フレームで弾を生成
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int   ringNum = 12;
        const double angleStep = 2.0 * DX_PI / ringNum;
        for (int i = 0; i < ringNum; ++i) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 基本角度にランダム性を加える
            pEnemyShot->muki = angleStep * i + GetRand(30) * DX_PI / 180.0;
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0;
            pEnemyShot->count = 0;   // メインルーチンが毎フレーム加算

            // 毒をイメージした色（緑・マゼンタ・黄）をランダムに
            int color;
            switch (GetRand(2)) {   // GetRand(2) → 0..2
            case 0: color = 2; break; // 緑
            case 1: color = 5; break; // マゼンタ
            default:color = 1; break; // 黄（予備）
            }
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存の弾を移動し、必要なら分裂
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 生成から30フレーム経過で破裂（pEnemyShot->count はメインルーチンが増やす）
        if (pEnemyShot->count == 30) {
            for (int j = 0; j < 3; ++j) {
                sEnemyShot* pChild = new sEnemyShot;
                pChild->x = pEnemyShot->x;
                pChild->y = pEnemyShot->y;
                pChild->muki = pEnemyShot->muki + (GetRand(120) - 60) * DX_PI / 180.0;
                pChild->speed = pEnemyShot->speed * 1.2 + GetRand(100) / 100.0;
                pChild->count = 99999; // これ以上分裂させない

                int childColor = (GetRand(1) == 0) ? 2 : 5; // 緑 or マゼンタ
                pChild->kind = img_enemyShotSmallBall[childColor];

                pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pChild->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
                pEnemyShotSet->pEnemyShotHead->prev = pChild;
            }
            // 親弾は再分裂しないよう count を大きな値へ
            pEnemyShot->count = 10000;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（関数名は EnemyPat_Poison_DeepSeek で固定）
void EnemyPat_Poison_DeepSeek()
{
    // 初回フレームの初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 動き：サインカーブで左右にゆらゆら、徐々に降下
    enemy.x = 240.0 + sin(count * 0.02) * 160.0;
    enemy.y = 40.0 + count * 0.5;
    if (enemy.y > 400.0) enemy.y = 400.0; // 画面下部で停滞

    // 90フレームごとに毒胞子の塊を発射
    if (count % 90 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ToxicSpore;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

        // ダミーヘッドの準備
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットをグローバルリストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}