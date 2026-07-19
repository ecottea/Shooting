// enemyPat_tmp.cpp
//
// 「絶対に回避できない弾幕」
//   予告の後、画面全域(-24〜504, SPACING=24px間隔)を隙間なく大玉でタイル状に埋め尽くす。
//   プレイヤーがどの座標にいようと、埋め尽くしの瞬間に必ず弾と重なるため回避不可能。
//   ランダム性は一切使わない（乱数で隙間ができるリスクを避けるため、確定配置にしている）。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：問答無用の幕（画面全域を大玉で埋め尽くす）
static void ShotCurtainOfNoEscape(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    const int WARNING_TIME = 60; // 幕が降りるまでの予告時間（約1秒）
    const int HOLD_TIME = 30;    // 幕が降りた後、静止して視界と足場を完全に封じる時間
    const int SPACING = 24;      // 大玉の配置間隔（隙間が生まれない値）

    if (pEnemyShotSet->count == 0) {
        // 予告音。これから来る幕を知らせて緊張感を出す
        if (CheckSoundMem(sound_enemyShot_extreme) == 1) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count == WARNING_TIME) {
        // 予告終了と同時に、画面(480x480)全域を隙間なくタイル状に埋める
        // 端をわずかにはみ出させて配置することで画面端の隙間も潰す
        for (int gy = -24; gy <= 504; gy += SPACING) {
            for (int gx = -24; gx <= 504; gx += SPACING) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = (double)gx;
                pEnemyShot->y = (double)gy;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0; // 幕は瞬間的にその場へ確定配置される（移動させると隙間ができるため）
                pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤の大玉に統一し、幕として視認させる

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 幕が降りたあとは HOLD_TIME だけ静止し、その後はゆっくり下方向へ流して排出する
    // （画面外に出た弾の消去はメインルーチン側の仕様に従う）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->count > HOLD_TIME) {
            pEnemyShot->y += 1.2;
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン：絶対に回避できない弾幕
void EnemyPat_Unavoidable_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 100; // 200で固定
    }

    if (count == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCurtainOfNoEscape;
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