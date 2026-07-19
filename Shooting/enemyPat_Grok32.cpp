// enemyPat_Bomberman.cpp
// ボンバーマンモチーフ弾幕パターン：「爆裂十字迷宮」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ======================
// 爆弾十字爆発用ショット関数
// ======================
static void ShotBomberCross(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int BOMB_COUNT = 9;          // 爆弾の数
    const int EXPLODE_DELAY = 45;      // 爆発までのフレーム数

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 爆弾設置（param_dで位置管理）
        for (int i = 0; i < BOMB_COUNT; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i % 3 - 1) * 110.0 + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y + (i / 3 - 1) * 90.0 + GetRand(30) - 15;
            pEnemyShot->kind = img_enemyShotLargeBall[2]; // 大玉（爆弾表示用）
            pEnemyShot->param_i[0] = 0;                   // 0:未爆発
            pEnemyShot->param_i[1] = EXPLODE_DELAY + GetRand(20); // 個別遅延

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        return;
    }

    // 爆発処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 0) {
            if (pEnemyShotSet->count >= pEnemyShot->param_i[1]) {
                pEnemyShot->param_i[0] = 1; // 爆発済み
                if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                // 十字爆風生成（4方向）
                double cx = pEnemyShot->x;
                double cy = pEnemyShot->y;
                for (int dir = 0; dir < 4; dir++) {
                    double baseAngle = dir * (DX_PI / 2.0);
                    for (int t = -2; t <= 2; t++) {
                        double angle = baseAngle + t * 0.07;
                        sEnemyShot* blast = new sEnemyShot;
                        blast->x = cx;
                        blast->y = cy;
                        blast->muki = angle;
                        blast->speed = 4.2 + GetRand(12) / 10.0;
                        blast->kind = img_enemyShotMediumBall[0]; // 赤中玉
                        blast->param_i[0] = 999; // 爆風フラグ

                        blast->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        blast->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = blast;
                        pEnemyShotSet->pEnemyShotHead->prev = blast;
                    }
                }

                pEnemyShot->x = 9999;
            }
        }
        pEnemyShot = pEnemyShot->next;
    }

    // 爆風移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 999) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// ======================
// 敵本体パターン（指定関数名）
// ======================
void EnemyPat_Bomberman_Grok()
{
    static int muki = 1;
    static int phase = 0;
    static int phaseTimer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
        phaseTimer = 0;
    }
    else {
        enemy.x += 1.15 * (double)muki;
        if (enemy.x < 90 || enemy.x > 390) muki *= -1;

        phaseTimer++;
        if (phaseTimer > 220) {
            phase = (phase + 1) % 3;
            phaseTimer = phase == 1 ? 110 : 0;
        }
    }

    // メイン十字爆発パターン
    if (count % 105 == 25) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBomberCross;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 25.0;
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

    // フェーズ1：ランダム追加爆弾
    if (phase == 1 && count % 38 == 0) {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = ShotBomberCross;
        p->x = 60 + GetRand(360);
        p->y = 30 + GetRand(90);
        p->muki = 0;
        p->kind = 1;
        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }
}