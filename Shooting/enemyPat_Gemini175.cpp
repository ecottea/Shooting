#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ペンデュラム・ウェーブ・カスケード
static void ShotPendulumWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音の再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int numBullets = 20; // 弾の総数
        double startX = pEnemyShotSet->x - 180.0;
        double spacing = 360.0 / (numBullets - 1);

        for (int i = 0; i < numBullets; i++) {
            pEnemyShot = new sEnemyShot;

            // 基準となる初期X座標をparam_d[0]に保存
            double base_x = startX + i * spacing;
            pEnemyShot->param_d[0] = base_x;

            // ペンデュラムウェーブの肝：インデックス毎にわずかに異なる周期を設定
            double period = 90.0 + (double)i * 3.5;
            pEnemyShot->param_d[1] = period;

            pEnemyShot->x = base_x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 下方向への等速移動設定
            pEnemyShot->speed = 1.5;
            pEnemyShot->muki = DX_PI/2; // 下向き (90度)

            // 弾の種類：視認性の高い小玉を使用し、色をグラデーション状に割り当て
            int color_idx = i % 9;
            pEnemyShot->kind = img_enemyShotSmallBall[color_idx];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの弾の移動およびウェーブ計算
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 下方向への進行
        pShot->y += pShot->speed * sin(pShot->muki);

        // 水平方向へのサイン波オフセット（ペンデュラムウェーブ干渉）
        double base_x = pShot->param_d[0];
        double period = pShot->param_d[1];
        double amplitude = 45.0; // 左右の揺れ幅

        // pShot->count はメインルーチンで毎フレーム自動インクリメントされる
        pShot->x = base_x + amplitude * sin(2.0 * DX_PI * (double)pShot->count / period);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_PendulumWave_Gemini()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        enemy.x += 0.8 * (double)muki;
        if (enemy.x < 100.0 || enemy.x > 380.0) muki *= -1;
    }

    // 一定間隔でペンデュラムウェーブ弾幕セットを生成
    if (count % 60 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPendulumWave;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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