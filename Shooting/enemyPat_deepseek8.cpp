// 雷をモチーフにした弾幕パターン
// ファイル名: enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  稲妻ストライク : 高速・急角度変化・枝分かれ
// ------------------------------------------------------------
static void LightningStrike(sEnemyShotSet* pSet)
{
    // 初回：メインの雷弾（大玉・白）を1発生成
    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pMain = new sEnemyShot;
        pMain->x = pSet->x;
        pMain->y = pSet->y;
        pMain->muki = pSet->muki;              // プレイヤー方向
        pMain->speed = 6.0;
        pMain->kind = img_enemyShotLargeBall[6]; // 白・大玉

        pMain->prev = pSet->pEnemyShotHead->prev;
        pMain->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pMain;
        pSet->pEnemyShotHead->prev = pMain;
    }

    // 全弾の移動（標準の座標更新）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }

    // --- メイン雷弾の急角度変化（ジグザグを印象づける） ---
    if (pSet->count > 0 && pSet->count % 4 == 0) {   // 4フレームごとに
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            // 大玉（白）＝メイン雷弾だけ処理
            if (pShot->kind == img_enemyShotLargeBall[6]) {
                // -70°～+70° の急な方向転換
                if (pShot->y <= 300) pShot->muki += (GetRand(140) - 70) / 180.0 * DX_PI;
            }
            pShot = pShot->next;
        }
    }

    // --- 枝分かれ（側枝）の発生 ---
    if (pSet->count > 0 && pSet->count % 6 == 0 && pSet->count <= 30) {
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->kind == img_enemyShotLargeBall[6]) {
                // 左右2方向に枝を伸ばす
                for (int side = 0; side < 2; ++side) {
                    double offsetAngle = (side == 0 ? 40.0 : -40.0) / 180.0 * DX_PI;
                    sEnemyShot* pBranch = new sEnemyShot;
                    pBranch->x = pShot->x;
                    pBranch->y = pShot->y;
                    pBranch->muki = pShot->muki + offsetAngle;
                    pBranch->speed = 5.0;
                    pBranch->kind = img_enemyShotMediumBall[1]; // 黄・中玉（枝を強調）

                    pBranch->prev = pSet->pEnemyShotHead->prev;
                    pBranch->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pBranch;
                    pSet->pEnemyShotHead->prev = pBranch;
                }
            }
            pShot = pShot->next;
        }
    }

    // --- 枝弾自身もわずかにジグザグさせる ---
    if (pSet->count > 0 && pSet->count % 8 == 0) {
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            // 黄色の中玉＝枝弾のみ処理
            if (pShot->kind == img_enemyShotMediumBall[1]) {
                if (pShot->y <= 300) pShot->muki += (GetRand(60) - 30) / 180.0 * DX_PI;
            }
            pShot = pShot->next;
        }
    }

    ++pSet->count;
}

// ------------------------------------------------------------
//  敵本体のパターン (雷モチーフ)
// ------------------------------------------------------------
void EnemyPat_Thunder_DeepSeek()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右に少し不規則に動く（嵐のイメージ）
        enemy.x += 1.2 * (double)muki;
        if (count % 100 == 50) {
            muki = (GetRand(1) == 0) ? 1 : -1;  // ランダムに方向転換
        }

        if (enemy.x <= 30) muki = 1;
        if (enemy.x >= 480 - 30) muki = -1;
    }

    // 稲妻弾幕を一定間隔で発射
    if (count % 18 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = LightningStrike;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

        // ダミーヘッド作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバル弾幕リストに接続
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}