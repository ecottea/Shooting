// enemyPat_DNA.cpp
// DNAモチーフ弾幕パターン「二重螺旋の遺伝子鎖」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ======================
// DNA二重螺旋弾幕用関数
// ======================
static void ShotDNAHelix(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double PI = DX_PI;
    const int NUM_STRANDS = 2;          // 二重螺旋
    const double HELIX_RADIUS = 45.0;   // 螺旋の半径
    const double HELIX_SPEED = 2.8;     // 前進速度
    const double ROT_SPEED = 0.085;     // 回転速度（ラジアン/フレーム）

    if (pEnemyShotSet->count == 0) {
        // 出現時効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 二重螺旋の2本の弾源を初期化
        for (int strand = 0; strand < NUM_STRANDS; strand++) {
            for (int i = 0; i < 24; i++) {  // 各螺旋に24個程度の弾
                pEnemyShot = new sEnemyShot;
                double angle_offset = strand * PI;  // 2本を反対側から開始

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = HELIX_SPEED;

                // パラメータで螺旋情報を保持
                pEnemyShot->param_i[0] = strand;                    // 0 or 1: 螺旋の種類
                pEnemyShot->param_i[1] = i;                         // 弾のインデックス（位相）
                pEnemyShot->param_d[0] = angle_offset;              // 初期角度オフセット
                pEnemyShot->param_d[1] = HELIX_RADIUS;              // 半径

                // 弾の種類と色（DNAっぽく青と紫系）
                if (strand == 0) {
                    pEnemyShot->kind = img_enemyShotSmallBall[4];   // 青系
                }
                else {
                    pEnemyShot->kind = img_enemyShotSmallBall[5];   // マゼンタ/紫系
                }

                // 連結リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        pEnemyShotSet->param_i[0] = 0;  // 横弾生成カウンタ
    }

    // ======================
    // 毎フレーム更新（螺旋運動）
    // ======================
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->kind != img_enemyShotMediumOval[2]) {
            int strand = pEnemyShot->param_i[0];
            int idx = pEnemyShot->param_i[1];
            double offset = pEnemyShot->param_d[0];
            double radius = pEnemyShot->param_d[1];

            double t = pEnemyShotSet->count * ROT_SPEED + (idx * 0.3) + offset;
            double angle = t;

            double center_x = pEnemyShotSet->x + sin(t * 0.6) * 8.0;  // 軽い蛇行
            double center_y = pEnemyShotSet->y + pEnemyShotSet->count * HELIX_SPEED;

            pEnemyShot->x = center_x + cos(angle) * radius * (strand == 0 ? 1.0 : 0.95);
            pEnemyShot->y = center_y + sin(angle) * (radius * 0.6);

            pEnemyShot->muki = angle + PI / 2;
        }
        else {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }

    // ======================
    // 塩基対（横方向連結弾）
    // ======================
    if (pEnemyShotSet->count % 6 == 0 && pEnemyShotSet->count < 120) {
        pEnemyShotSet->param_i[0]++;
        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;
            double t = pEnemyShotSet->count * ROT_SPEED;
            double cx = pEnemyShotSet->x + sin(t * 0.6) * 8.0;
            double cy = pEnemyShotSet->y + pEnemyShotSet->count * HELIX_SPEED;

            pEnemyShot->x = cx + i * 12.0;
            pEnemyShot->y = cy;
            pEnemyShot->muki = PI / 2 + (GetRand(200) - 100) / 180.0 * PI;
            pEnemyShot->speed = 3.0 + GetRand(80) / 100.0;

            pEnemyShot->kind = img_enemyShotMediumOval[2];  // 緑系

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// ======================
// 敵本体パターン（指定関数名）
void EnemyPat_DNA_Grok()
{
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 1.1 * (double)muki;
        if (count % 140 == 70) muki *= -1;
        enemy.y = 60.0 + sin(count / 30.0) * 12.0;
    }

    // 定期的にDNA螺旋弾幕を発射
    if (count % 85 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDNAHelix;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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