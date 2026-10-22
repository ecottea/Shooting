#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：雪の結晶「ヘキサゴナル・デンドライト」
static void ShotHexagonalDendrite(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 弾幕セット生成時の初期化（1波の最初に6方向の結晶の骨組みを生成）
    if (pEnemyShotSet->count == 0) {
        // 重厚感のあるショット音を再生
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 6回対称（60度間隔）で骨組みを発射
        for (int i = 0; i < 6; i++) {
            double angle = i * (2.0 * DX_PI / 6.0);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 3.5;
            // 骨組みにはシャープで鋭い印象を与える「白の銃弾」を採用
            pEnemyShot->kind = img_enemyShotBullet[6];
            pEnemyShot->param_i[0] = 0; // 0: 骨組み（幹）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 各弾の挙動制御（移動・静止・枝の展開・再加速）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 【骨組み（幹）の挙動】
            // 0〜35F: 前進しながら徐々に減速
            // 35〜75F: ピタッと静止して雪の結晶のシルエットを形成 ＆ 左右に枝を展開
            // 75F〜: 再加速して外側へ拡散
            if (pShot->count < 35) {
                pShot->speed = 3.5 * (1.0 - (double)pShot->count / 45.0);
                if (pShot->speed < 0.2) pShot->speed = 0.2;
            }
            else if (pShot->count >= 35 && pShot->count < 75) {
                pShot->speed = 0.0; // 静止

                // 静止した瞬間に一度だけ、左右垂直方向に結晶の枝（小玉）を発生させる
                if (pShot->count == 35) {
                    for (int j = 1; j <= 2*5; j++) {
                        // 右側の枝パーツ
                        sEnemyShot* pBranchR = new sEnemyShot;
                        pBranchR->x = pShot->x;
                        pBranchR->y = pShot->y;
                        pBranchR->muki = pShot->muki + DX_PI / 2.0;
                        pBranchR->speed = j * 1.2;
                        // 枝のディテールには美しく映える「シアンの小玉」を採用
                        pBranchR->kind = img_enemyShotSmallBall[3];
                        pBranchR->param_i[0] = 1; // 1: 枝パーツ

                        pBranchR->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pBranchR->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pBranchR;
                        pEnemyShotSet->pEnemyShotHead->prev = pBranchR;

                        // 左側の枝パーツ
                        sEnemyShot* pBranchL = new sEnemyShot;
                        pBranchL->x = pShot->x;
                        pBranchL->y = pShot->y;
                        pBranchL->muki = pShot->muki - DX_PI / 2.0;
                        pBranchL->speed = j * 1.2;
                        pBranchL->kind = img_enemyShotSmallBall[3]; // シアンの小玉
                        pBranchL->param_i[0] = 1; // 1: 枝パーツ

                        pBranchL->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pBranchL->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pBranchL;
                        pEnemyShotSet->pEnemyShotHead->prev = pBranchL;
                    }
                }
            }
            else {
                pShot->speed = 2.5; // 再加速
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == 1) {
            // 【枝パーツ（小玉）の挙動】
            // 静止期間は幹に同調して留まり、75F以降は外側へ流れる
            if (pShot->count >= 75) {
                pShot->speed = 2.0;
            }
            else {
                if (pShot->count < 40) {
                    // 展開初期のわずかな移動
                }
                else {
                    pShot->speed = 0.0;
                }
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Snowflake_Gemini()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        enemy.x += 0.8 * (double)muki;
        if (enemy.x < 100.0 || enemy.x > 380.0) {
            muki *= -1;
        }
    }

    // 一定間隔（180フレームごと）で結晶弾幕を生成
    if (count % 60 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHexagonalDendrite;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
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