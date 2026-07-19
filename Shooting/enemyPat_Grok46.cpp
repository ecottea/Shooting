// enemyPat_Tmp_SandStorm.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// 大砂嵐をモチーフにした弾幕パターン
// =============================================
static void ShotSandStorm(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int phase = pEnemyShotSet->count / 60;  // 約60フレームごとにフェーズ進行

    if (pEnemyShotSet->count == 0) {
        // 発動時の効果音
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // Phase 0: 前兆・砂粒の降下 (0〜59f)
    if (phase == 0) {
        if (pEnemyShotSet->count % 3 == 0) {
            for (int i = 0; i < 12; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + GetRand(400) - 200;
                pEnemyShot->y = pEnemyShotSet->y - 30 + GetRand(40);
                pEnemyShot->muki = DX_PI / 2 + (GetRand(80) - 40) / 180.0 * DX_PI; // 下方向に少し揺らぎ
                pEnemyShot->speed = (80 + GetRand(60)) / 100.0;
                pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄色小玉（砂粒）
                pEnemyShot->param_d[0] = (GetRand(100) - 50) / 100.0; // 蛇行用のランダムオフセット

                // リンク
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    // Phase 1: 狂風渦 + 横殴り砂嵐 (60〜179f)
    else if (phase == 1) {
        if (pEnemyShotSet->count % 8 == 0) {
            // 中心からの二重螺旋（砂の奔流）
            double baseAngle = pEnemyShotSet->count / 4.0 * 0.12;
            for (int s = 0; s < 2; s++) {
                for (int i = 0; i < 5; i++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pEnemyShotSet->x;
                    pEnemyShot->y = pEnemyShotSet->y;
                    double angle = baseAngle + i * DX_PI * 2 / 5 + s * DX_PI;
                    pEnemyShot->muki = angle;
                    pEnemyShot->speed = 2.8 + s * 0.4;
                    pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙色小玉
                    pEnemyShot->param_i[0] = s; // 螺旋方向識別

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }

        // 横殴りの砂嵐（左右から）
        if (pEnemyShotSet->count % 12 == 0) {
            for (int side = 0; side < 2; side++) {
                double startX = (side == 0) ? -40 : 520;
                for (int i = 0; i < 7; i++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = startX;
                    pEnemyShot->y = 80 + i * 55 + GetRand(30) - 15;
                    pEnemyShot->muki = (side == 0) ? 0.15 : DX_PI - 0.15; // やや下向き
                    pEnemyShot->speed = 4.2 + GetRand(80) / 100.0;
                    pEnemyShot->kind = img_enemyShotMediumOval[1]; // 黄色中楕円（砂の流れ）
                    pEnemyShot->param_d[0] = (GetRand(60) - 30) / 80.0; // 軽い波打ち

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }
    // Phase 2: 収束・砂の埋没 (180f〜)
    else if (phase == 2) {
        if (pEnemyShotSet->count % 5 == 0) {
            // 高速奔流（太め）
            for (int i = 0; i < 4; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = DX_PI * (0.3 + i * 0.35) + sin(pEnemyShotSet->count / 10.0) * 0.2;
                pEnemyShot->speed = 5.5;
                pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙色中玉
                pEnemyShot->param_i[0] = 1; // 高速奔流フラグ

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 画面下半分を埋める加速砂粒
        if (pEnemyShotSet->count % 8 == 0) {
            for (int i = 0; i < 9; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = GetRand(480);
                pEnemyShot->y = 100 + GetRand(150) - 50;
                pEnemyShot->muki = DX_PI / 2 + (GetRand(120) - 60) / 180.0 * DX_PI;
                pEnemyShot->speed = 3.5 + GetRand(120) / 100.0 - 0.5;
                pEnemyShot->kind = img_enemyShotSmallBall[1];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 全弾の更新ループ
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pEnemyShot->next;

        // Phase 0: 蛇行降下
        if (phase == 0) {
            pEnemyShot->muki += sin(pEnemyShotSet->count / 8.0 + pEnemyShot->param_d[0]) * 0.018;
            pEnemyShot->speed += 0.012; // 徐々に加速
        }
        // Phase 1: 螺旋弾の回転補正 + 横殴り波打ち
        else if (phase == 1) {
            if (pEnemyShot->param_i[0] == 0 || pEnemyShot->param_i[0] == 1) { // 螺旋弾
                pEnemyShot->muki += 0.045 * (pEnemyShot->param_i[0] ? 1.0 : -1.0);
            }
            else {
                // 横殴り弾の波打ち
                pEnemyShot->y += sin(pEnemyShotSet->count / 6.0 + pEnemyShot->param_d[0]) * 1.2;
            }
        }
        // Phase 2: 奔流の加速
        else if (phase == 2) {
            if (pEnemyShot->param_i[0] == 1) {
                pEnemyShot->speed += 0.08; // 徐々に加速
            }
        }

        // 標準移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = nextShot;
    }
}

// =============================================
// 敵本体のパターン（大砂嵐）
// =============================================
void EnemyPat_SandStorm_Grok()
{
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆったり移動
        enemy.x += 1.1 * (double)muki;
        if (count % 140 == 70) muki *= -1;

        // 軽く上下移動
        enemy.y = 60.0 + sin(count / 45.0) * 18.0;
    }

    // 定期的に砂嵐弾幕を発動
    if (count % 110 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSandStorm;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}