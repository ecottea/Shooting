#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：審判の螺旋ラッパ（横から見た下向きラッパ構図）
// ============================================================
static void ShotGabrielTrumpet(sEnemyShotSet* pEnemyShotSet)
{
    // 時間経過とともに左右の開き角を大きくし、逆V字（漏斗型）の軌跡を描かせる
    if (pEnemyShotSet->count % 4 == 0) {
        double progress = (double)pEnemyShotSet->count / 120.0; // 120フレームで形状を完成させる

        if (progress <= 1.0) {
            double max_spread = DX_PI / 2.5; // 最大開き角（約72度）
            double current_spread = progress * max_spread;

            // 左側の壁（管の側面）
            sEnemyShot* pShotL = new sEnemyShot;
            pShotL->x = pEnemyShotSet->x;
            pShotL->y = pEnemyShotSet->y;
            pShotL->muki = (DX_PI / 2.0) - current_spread; // 下向きを基準に左へ開く
            pShotL->speed = 3.0 + progress * 3.0; // 下に行くほど速く（遠近感）
            pShotL->kind = img_enemyShotScale[1]; // 黄色の鱗弾（進行方向を向くため管の質感が出る）

            pShotL->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShotL->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShotL;
            pEnemyShotSet->pEnemyShotHead->prev = pShotL;

            // 右側の壁（管の側面）
            sEnemyShot* pShotR = new sEnemyShot;
            pShotR->x = pEnemyShotSet->x;
            pShotR->y = pEnemyShotSet->y;
            pShotR->muki = (DX_PI / 2.0) + current_spread; // 下向きを基準に右へ開く
            pShotR->speed = 3.0 + progress * 3.0;
            pShotR->kind = img_enemyShotScale[1];

            pShotR->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShotR->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShotR;
            pEnemyShotSet->pEnemyShotHead->prev = pShotR;

            // 中央の芯（管の内部）
            if (pEnemyShotSet->count % 12 == 0) {
                sEnemyShot* pShotC = new sEnemyShot;
                pShotC->x = pEnemyShotSet->x + GetRand(10) - 5; // わずかな揺らぎ
                pShotC->y = pEnemyShotSet->y;
                pShotC->muki = (DX_PI / 2.0) + (GetRand(20) - 10) / 100.0; // わずかな揺らぎ
                pShotC->speed = 4.0 + progress * 2.0;
                pShotC->kind = img_enemyShotBullet[6]; // 白色の銃弾（細い管を表現）

                pShotC->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShotC->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShotC;
                pEnemyShotSet->pEnemyShotHead->prev = pShotC;
            }
        }
        else if (progress > 1.0 && progress < 1.05) {
            // ベル（開口部）の放音演出（1回のみ実行）
            if (pEnemyShotSet->param_i[0] == 0) {
                pEnemyShotSet->param_i[0] = 1;
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                // 画面下部（ラッパの先端）から横方向へ衝撃波を放出
                for (int i = 0; i < 16; i++) {
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = pEnemyShotSet->x;
                    pShot->y = pEnemyShotSet->y + 350.0; // 画面下部付近
                    pShot->muki = DX_PI + (double)i / 16.0 * DX_PI; // 左向きから右向きへ
                    pShot->speed = 5.0 + GetRand(30) / 10.0;
                    pShot->kind = img_enemyShotMediumOval[6]; // 白色の中楕円弾

                    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pShot;
                }
            }
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン（エントリポイント）
// ============================================================
void EnemyPat_GabrielsHorn_Qwen()
{
    static int trumpet_timer = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        trumpet_timer = 0;
    }
    else {
        // 敵の浮遊動作
        enemy.x = 240.0 + sin(count / 60.0 * DX_PI) * 30.0;
        trumpet_timer++;

        // パターン開始時の予告音
        if (trumpet_timer == 1) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 180フレーム周期でラッパの発動を繰り返す
        if (trumpet_timer % 90 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotGabrielTrumpet;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = 0;
            pEnemyShotSet->param_i[0] = 0; // 放音演出のフラグ用

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}