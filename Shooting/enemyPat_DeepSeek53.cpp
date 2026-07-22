// enemyPat_Tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <cmath>

// -------------------------------------------------------------
//  補助：A の字形を定義する定数
// -------------------------------------------------------------
static const double APEX_X = 240.0, APEX_Y = 80.0;            // Aの頂点
static const double LEFT_END_X = 50.0, LEFT_END_Y = 480.0;   // 左足先
static const double RIGHT_END_X = 430.0, RIGHT_END_Y = 480.0;// 右足先
static const double CROSSBAR_Y = 300.0;                       // 横棒の y 座標

// -------------------------------------------------------------
//  フェーズ 1：左斜線を描く弾
// -------------------------------------------------------------
static void DrawLegLeft(sEnemyShotSet* pSet)
{
    const int DURATION = 120;           // 弾を撃つフレーム数
    const int INTERVAL = 4;             // 弾の発射間隔

    // ショットセットの位置をボスに追従（左斜線の描き始めがずれる演出）
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    // 一定時間だけ一定間隔で発射
    if (pSet->count < DURATION && pSet->count % INTERVAL == 0) {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        // 左斜線の向き（頂点→左下）
        pShot->muki = std::atan2(LEFT_END_Y - APEX_Y, LEFT_END_X - APEX_X);
        pShot->speed = 2.5;
        pShot->kind = img_enemyShotBullet[0];   // 赤い針弾

        // 双方向リストに追加
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // 既存の弾はメインルーチンが自動で動かすのでここでは何もしない

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// -------------------------------------------------------------
//  フェーズ 2：右斜線を描く弾
// -------------------------------------------------------------
static void DrawLegRight(sEnemyShotSet* pSet)
{
    const int DURATION = 120;
    const int INTERVAL = 4;

    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    if (pSet->count < DURATION && pSet->count % INTERVAL == 0) {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        // 右斜線の向き（頂点→右下）
        pShot->muki = std::atan2(RIGHT_END_Y - APEX_Y, RIGHT_END_X - APEX_X);
        pShot->speed = 2.5;
        pShot->kind = img_enemyShotBullet[5];   // 赤紫（マゼンタ）の針弾

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// -------------------------------------------------------------
//  フェーズ 3：横棒（白レーザー）
// -------------------------------------------------------------
static void Crossbar(sEnemyShotSet* pSet)
{
    // 一度だけ画面上に静止レーザーを敷き詰める
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 画面幅 480 をカバーするように短レーザーを並べる
        for (double x = 40.0; x <= 440.0; x += 32.0) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = x;
            pShot->y = CROSSBAR_Y;
            pShot->muki = 0.0;
            pShot->speed = 0.0;                         // 静止
            pShot->kind = img_enemyShotLaser[6];       // 白レーザー

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    // あとは消えるまで放置（画面外判定はメインが行う）

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// -------------------------------------------------------------
//  フェーズ 4：崩落（ばらまき＋自機狙い）
// -------------------------------------------------------------
static void Collapse(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme))
            StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // --- 左斜線上の点から青・黄の小玉をばらまく ---
        for (int i = 1; i <= 18; ++i) {
            double t = i / 18.0;   // 頂点(0) → 左下(1)
            double sx = APEX_X + t * (LEFT_END_X - APEX_X);
            double sy = APEX_Y + t * (LEFT_END_Y - APEX_Y);

            for (int j = 0; j < 2; ++j) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = sx;
                pShot->y = sy;
                // ランダムな方向へ飛ばす（元の線から離れるようにバイアス）
                double baseAngle = std::atan2(LEFT_END_Y - APEX_Y, LEFT_END_X - APEX_X);
                pShot->muki = baseAngle + (GetRand(120) - 60) / 180.0 * DX_PI;
                pShot->speed = (150 + GetRand(100)) / 100.0;
                pShot->kind = (j == 0) ? img_enemyShotSmallBall[4]   // 青
                    : img_enemyShotSmallBall[1];  // 黄

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }

        // --- 右斜線上の点からも同様に ---
        for (int i = 1; i <= 18; ++i) {
            double t = i / 18.0;
            double sx = APEX_X + t * (RIGHT_END_X - APEX_X);
            double sy = APEX_Y + t * (RIGHT_END_Y - APEX_Y);

            for (int j = 0; j < 2; ++j) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = sx;
                pShot->y = sy;
                double baseAngle = std::atan2(RIGHT_END_Y - APEX_Y, RIGHT_END_X - APEX_X);
                pShot->muki = baseAngle + (GetRand(120) - 60) / 180.0 * DX_PI;
                pShot->speed = (150 + GetRand(100)) / 100.0;
                pShot->kind = (j == 0) ? img_enemyShotSmallBall[4]
                    : img_enemyShotSmallBall[1];

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }

        // --- 三角形内部から緑の鱗弾を自機狙いで連射 ---
        for (int k = 0; k < 16; ++k) {
            sEnemyShot* pShot = new sEnemyShot;
            // 三角形の重心付近
            pShot->x = APEX_X + (GetRand(40) - 20);
            pShot->y = APEX_Y + 120 + (GetRand(60) - 30);
            pShot->muki = std::atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 1.8;
            pShot->kind = img_enemyShotScale[2];   // 緑の鱗弾

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// -------------------------------------------------------------
//  フェーズ 5：再鋭（高速描画＋巨大 A レーザー落下）
// -------------------------------------------------------------
static void RegenerateFastLegs(sEnemyShotSet* pSet)
{
    const int DURATION = 80;
    const int INTERVAL = 2;   // 高速連射

    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    if (pSet->count < DURATION && pSet->count % INTERVAL == 0) {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 左右交互に針弾を撃つ
        if ((pSet->count / INTERVAL) % 2 == 0) {
            // 左
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = std::atan2(LEFT_END_Y - APEX_Y, LEFT_END_X - APEX_X);
            pShot->speed = 3.5;
            pShot->kind = img_enemyShotBullet[0];   // 赤
            pShot->margin = 40;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        else {
            // 右
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = std::atan2(RIGHT_END_Y - APEX_Y, RIGHT_END_X - APEX_X);
            pShot->speed = 3.5;
            pShot->kind = img_enemyShotBullet[5];   // 赤紫
            pShot->margin = 40;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

static void GiantAFall(sEnemyShotSet* pSet)
{
    // 一度だけ巨大な A の形をしたレーザー群を生成し、真下へ落下させる
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);   // 予告音

        const double fallSpeed = 2.0;
        const double startY = APEX_Y;   // 頂点から落下開始

        // 左斜線レーザー
        for (int i = 0; i <= 12; ++i) {
            double t = i / 12.0;
            double sx = APEX_X + t * (LEFT_END_X - APEX_X);
            double sy = startY + t * (LEFT_END_Y - APEX_Y);

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = sx;
            pShot->y = sy;
            pShot->muki = DX_PI / 2.0;   // 真下
            pShot->speed = fallSpeed;
            pShot->kind = img_enemyShotLaser[4];   // 青レーザー（金色がないので青で代用、あるいは白）
            pShot->margin = 40;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // 右斜線レーザー
        for (int i = 0; i <= 12; ++i) {
            double t = i / 12.0;
            double sx = APEX_X + t * (RIGHT_END_X - APEX_X);
            double sy = startY + t * (RIGHT_END_Y - APEX_Y);

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = sx;
            pShot->y = sy;
            pShot->muki = DX_PI / 2.0;
            pShot->speed = fallSpeed;
            pShot->kind = img_enemyShotLaser[4];
            pShot->margin = 40;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // 横棒レーザー（落下開始位置は CROSSBAR_Y）
        for (double x = 40.0; x <= 440.0; x += 16.0) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = x;
            pShot->y = CROSSBAR_Y;   // 横棒はこの高さから落下
            pShot->muki = DX_PI / 2.0;
            pShot->speed = fallSpeed;
            pShot->kind = img_enemyShotLaser[6];   // 白レーザー
            pShot->margin = 40;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// -------------------------------------------------------------
//  敵本体パターン (void EnemyPat_Daimonji_DeepSeek)
// -------------------------------------------------------------
void EnemyPat_Daimonji_DeepSeek()
{
    // フェーズ管理用の静的変数
    static int phase = 0;   // 0-5
    static int phaseTimer = 0;   // 現在のフェーズ開始からのフレーム数

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseTimer = 0;
        return;
    }

    // ---- フェーズ進行タイマー ----
    ++phaseTimer;
    switch (phase) {
        // 左足描画（ボスはゆっくり左へ）
    case 0:
        if (phaseTimer == 1) {
            // このフェーズ用のショットセットを作成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = DrawLegLeft;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        enemy.x -= 0.7;   // 左移動
        if (phaseTimer >= 120) { phase = 1; phaseTimer = 0; }
        break;

        // 右足描画（ボスはゆっくり右へ）
    case 1:
        if (phaseTimer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = DrawLegRight;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        enemy.x += 0.7;   // 右移動
        if (phaseTimer >= 120) { phase = 2; phaseTimer = 0; }
        break;

        // 横棒レーザー
    case 2:
        if (phaseTimer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = Crossbar;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        // ボスは素早く中央へ戻る
        enemy.x += (240.0 - enemy.x) * 0.1;
        if (phaseTimer >= 60) { phase = 3; phaseTimer = 0; }
        break;

        // 崩落
    case 3:
        if (phaseTimer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = Collapse;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        // 崩落中はボス静止
        if (phaseTimer >= 90) { phase = 4; phaseTimer = 0; }
        break;

        // 再鋭（高速再描画）
    case 4:
        if (phaseTimer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = RegenerateFastLegs;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        // ボスは小刻みに震える演出
        enemy.x += (GetRand(2) - 1) * 1.5;
        if (phaseTimer >= 80) { phase = 5; phaseTimer = 0; }
        break;

        // 巨大 A 落下
    case 5:
        if (phaseTimer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = GiantAFall;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        // ボスは硬直
        if (phaseTimer >= 120) {
            // 1サイクル終了、最初に戻る
            phase = 0;
            phaseTimer = 0;
            enemy.x = 240.0;
            enemy.y = 80.0;
        }
        break;
    }
}