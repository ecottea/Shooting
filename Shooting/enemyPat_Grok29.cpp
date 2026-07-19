// enemyPat_Tmp.cpp
// シャボン玉モチーフ弾幕パターン：泡沫の連鎖 ～Bubble Chain～
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// 内部ヘルパー関数
// =============================================

// 虹色っぽい色を選択（0〜8）
static int GetBubbleColor(int phase) {
    return (phase % 9 + 9) % 9; // 0:赤〜8:橙で虹っぽく回す
}

// 大きめのシャボン玉を生成
static void CreateBigBubble(sEnemyShotSet* pSet, double x, double y, double angle, double speed) {
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = angle;
    p->speed = speed;
    p->kind = img_enemyShotLargeBall[GetBubbleColor((int)(angle * 10))]; // 虹色風
    p->margin = 60.0; // 少し大きめに余裕を持たせる
    p->param_i[0] = 0;     // 状態：0=浮遊中
    p->param_i[1] = 45 + GetRand(30); // 爆発までのフレーム数（約0.75〜1.25秒）
    p->param_d[0] = (GetRand(100) - 50) / 100.0 * 0.8; // ゆらゆら用の角速度

    // リンク
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// 小さめのシャボン玉を生成（爆発用）
static void CreateSmallBubble(sEnemyShotSet* pSet, double x, double y, double angle, double speed) {
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = angle;
    p->speed = speed;
    p->kind = img_enemyShotMediumBall[GetBubbleColor((int)(angle * 8))];
    p->margin = 40.0;
    p->param_i[0] = 1;     // 状態：1=小泡（さらに爆発可能）
    p->param_i[1] = 25 + GetRand(20); // 短めの寿命
    p->param_d[0] = (GetRand(80) - 40) / 80.0 * 1.2;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// =============================================
// 弾幕更新関数
// =============================================
static void BubbleChainPattern(sEnemyShotSet* pEnemyShotSet) {
    if (pEnemyShotSet->count == 0) {
        // 初回効果音
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = p->next; // 削除対策

        p->count++;

        // ゆらゆら移動（sin波）
        double wave = sin(p->count * 0.08 + p->param_d[0]) * 1.8;
        p->x += p->speed * cos(p->muki) + wave * 0.6;
        p->y += p->speed * sin(p->muki) * 0.7; // やや上寄り

        // 状態別処理
        if (p->param_i[0] == 0) { // 大泡
            if (p->count >= p->param_i[1]) {
                // 爆発！
                int split = 9 + GetRand(4); // 9〜12方向
                for (int i = 0; i < split; i++) {
                    double ang = p->muki + (i - split / 2.0) * (DX_PI * 2.0 / split) + (GetRand(40) - 20) / 180.0 * DX_PI;
                    double spd = 1.2 + GetRand(120) / 100.0;
                    CreateSmallBubble(pEnemyShotSet, p->x, p->y, ang, spd);
                }
                // 大泡は画面外へ飛ばして消滅させる
                p->x = -9999.0;
            }
        }
        else if (p->param_i[0] == 1) { // 小泡
            if (p->count >= p->param_i[1]) {
                // 小爆発（さらに細かい弾）
                int split = 5 + GetRand(3);
                for (int i = 0; i < split; i++) {
                    double ang = p->muki + (GetRand(360) - 180) / 180.0 * DX_PI;
                    double spd = 0.8 + GetRand(100) / 120.0;
                    sEnemyShot* tiny = new sEnemyShot;
                    tiny->x = p->x;
                    tiny->y = p->y;
                    tiny->muki = ang;
                    tiny->speed = spd;
                    tiny->kind = img_enemyShotSmallBall[GetBubbleColor(p->count)];
                    tiny->margin = 30.0;
                    tiny->param_i[0] = 2; // 最終小弾（これ以上分裂しない）
                    // リンク
                    tiny->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    tiny->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = tiny;
                    pEnemyShotSet->pEnemyShotHead->prev = tiny;
                }
                p->x = -9999.0; // 消滅
            }
        }

        p = next;
    }
}

// =============================================
// 敵本体パターン
// =============================================
void EnemyPat_SoapBubbles_Grok() {
    static int muki = 1;
    static int shotTimer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotTimer = 0;
    }
    else {
        // ゆったり左右移動
        enemy.x += 1.1 * (double)muki;
        if (enemy.x < 120) muki = 1;
        if (enemy.x > 360) muki = -1;

        // 少し上下にも動きを追加
        enemy.y = 60.0 + sin(count * 0.03) * 25.0;
    }

    // 定期的にシャボン玉セットを生成
    shotTimer++;
    if (shotTimer >= 38) {  // 約0.6秒間隔
        shotTimer = -200;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = BubbleChainPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // セットをリンク
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        // このセットの初回で複数の大泡を生成
        int numBubbles = 5 + GetRand(3);
        for (int i = 0; i < numBubbles; i++) {
            double baseAngle = DX_PI / 2.0 + (i - numBubbles / 2.0) * 0.35; // 上方向中心
            double angle = baseAngle + (GetRand(60) - 30) / 180.0 * DX_PI;
            double speed = 0.65 + GetRand(45) / 100.0;
            CreateBigBubble(pSet,
                pSet->x + (GetRand(80) - 40),
                pSet->y + (GetRand(30) - 15),
                angle, speed);
        }
    }
}