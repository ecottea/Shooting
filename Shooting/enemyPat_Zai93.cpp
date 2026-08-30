// enemyPat_signal.cpp
// 信号機モチーフの弾幕「交通管制(トラフィック・コントロール)」
//
// 1サイクル 540フレーム(60fpsで9秒)のフェーズループ:
//   青(0〜179)  : 自機狙いの扇状弾(青小玉)。「進んでよし」
//   黄(180〜299): 敵の周りからゆっくり膨らむリング弾(黄小玉)。「注意喚起」
//   赤(300〜479): 画面上部から降下して静止する壁弾(赤中玉)+ 青小玉の狙い撃ち。「停止」
//   点滅(480〜539): 静止弾が赤⇔白で明滅した後、一斉に画面下へ高速で飛び去る
// ループごとに弾数・弾速が少し上がる(4周目で上限)。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const int    CYCLE = 540 - 100;  // 1サイクルのフレーム数(9秒)
static const int    PHASE_YELLOW = 180 - 50; // 黄信号開始
static const int    PHASE_RED = 300 - 100; // 赤信号開始
static const int    PHASE_BLINK = 480 - 100; // 点滅開始
static const int    RELEASE_T = 535 - 100; // 赤弾解放タイミング(サイクル内フレーム)

static const double FIELD_TOP_LIMIT = 70.0; // 赤信号中に自機が入れる最も上のy

// ============================================================
//  ヘルパー:弾・弾セットの生成
//  ※ count 系のインクリメント、画面外の弾の消去はメインルーチン任せ
// ============================================================

// 弾を1発追加して返す
static sEnemyShot* Signal_AddShot(sEnemyShotSet* pSet, int img,
    double x, double y, double muki, double speed)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = img; // kind には画像ハンドル(=弾種+色)を入れる

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 弾セットを1つ生成して返す
static sEnemyShotSet* Signal_NewShotSet(void(*func)(sEnemyShotSet*),
    double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// ============================================================
//  弾パターン:直進(青信号の扇状弾・赤信号中の狙い撃ち用)
// ============================================================
static void Signal_ShotStraight(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  弾パターン:黄信号リング
//  ゆっくり外側へ膨らむ(少しずつ加速する)
// ============================================================
static void Signal_ShotYellowRing(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->speed < 2.2) pShot->speed += 0.015; // 膨らむ速度
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  弾パターン:赤信号の壁弾
//   降下 → 静止 → 点滅(赤⇔白) → 一斉解放
//  param_d[0] : 弾ごとの静止y座標
//  param_i[1] : 弾セット全体の解放フラグ
// ============================================================
static void Signal_ShotRedWall(sEnemyShotSet* pSet)
{
    int t = (count - 1) % CYCLE; // サイクル内の現在フレーム

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pSet->param_i[1] == 0) {
            // --- 未解放 ---
            if (pShot->speed > 0.0) {
                // 降下中:目的のラインで静止する
                pShot->y += pShot->speed;
                if (pShot->y >= pShot->param_d[0]) {
                    pShot->y = pShot->param_d[0];
                    pShot->speed = 0.0;
                }
            }
            else if (t >= PHASE_BLINK) {
                // 静止済み & 点滅フェーズ:5フレームごとに赤⇔白を切り替える
                // (同種の中玉なので当たり判定半径は変わらない)
                int blink = ((t - PHASE_BLINK) / 5) % 2;
                pShot->kind = img_enemyShotLargeBall[(blink == 0) ? 0 : 6];
            }
        }
        else {
            // --- 解放後:画面下へ高速で飛び去る ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // 点滅終了で一斉解放
    if (pSet->param_i[1] == 0 && t == RELEASE_T) {
        pSet->param_i[1] = 1;
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            p->muki = DX_PI / 2.0; // 真下へ
            p->speed = 5.5;
            p->kind = img_enemyShotLargeBall[0]; // 色を赤に戻す
            p = p->next;
        }
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_TrafficLight_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }

    int t = (count - 1) % CYCLE;   // サイクル内経過フレーム
    int cyc = (count - 1) / CYCLE;   // 周回数(0始まり)
    int lv = (cyc < 4) ? cyc : 4;   // 難易度(0〜4で頭打ち)
    lv *= 10;

    // --- 敵の移動:青〜黄信号の間はゆっくり巡回、赤信号〜点滅の間は停止(信号待ち) ---
    if (count > 1 && t < PHASE_RED) {
        enemy.x += 1.5 * muki;
        if (enemy.x < 70.0) { enemy.x = 70.0;  muki = 1; }
        if (enemy.x > 410.0) { enemy.x = 410.0; muki = -1; }
    }

    // --- 赤信号中は自機が敵の近く(壁の上側)に回り込めないようにする ---
    if (t >= PHASE_RED) {
        if (player.y < FIELD_TOP_LIMIT) player.y = FIELD_TOP_LIMIT;
    }

    // ==========================================================
    //  青信号フェーズ:自機狙いの扇状弾(青小玉)
    // ==========================================================
    if (t < PHASE_YELLOW && t % 40 == 0) {
        sEnemyShotSet* pSet = Signal_NewShotSet(Signal_ShotStraight,
            enemy.x, enemy.y + 10.0, 0.0, 0);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double base = atan2(player.y - pSet->y, player.x - pSet->x);
        int n = 5 + lv; // 周回で弾数増加(最大9)
        for (int i = 0; i < n; i++) {
            double ang = base + (i - (n - 1) / 2.0) * 0.12;
            Signal_AddShot(pSet, img_enemyShotSmallBall[4],   // 小玉:青
                pSet->x, pSet->y, ang, 3.0 + 0.2 * lv);
        }
    }

    // ==========================================================
    //  黄信号フェーズ:予告音 → ゆっくり膨らむリング(黄小玉)
    // ==========================================================
    if (t == PHASE_YELLOW) {
        // 赤信号への予告音
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (t >= PHASE_YELLOW && t < PHASE_RED && t % 30 == 0) {
        sEnemyShotSet* pSet = Signal_NewShotSet(Signal_ShotYellowRing,
            enemy.x, enemy.y, 0.0, 0);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int n = 20 + 2 * lv; // 周回で弾数増加(最大28)
        double off = (t == 260) ? (DX_PI / n) : 0.0; // 2発目は半ステップずらす
        for (int i = 0; i < n; i++) {
            double ang = off + i * (2.0 * DX_PI / n);
            Signal_AddShot(pSet, img_enemyShotMediumBall[1],   // 小玉:黄
                enemy.x, enemy.y, ang, 0.5);       // 初速は遅め(徐々に加速)
        }
    }

    // ==========================================================
    //  赤信号フェーズ:静止する壁弾(赤中玉)+ 狙い撃ち(青小玉)
    // ==========================================================
    if (t == PHASE_RED) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pSet = Signal_NewShotSet(Signal_ShotRedWall,
            enemy.x, enemy.y, 0.0, 0);
        pSet->param_i[0] = lv;

        // ジグザグの壁:40px間隔の11本。静止ラインは交互に160/230
        for (int col = 0; col < 11; col++) {
            double x = 40.0 + col * 40.0;
            double stopY = 160.0 + (col % 2) * 70.0;
            sEnemyShot* pShot = Signal_AddShot(pSet, img_enemyShotLargeBall[0], // 中玉:赤
                x, -12.0, DX_PI / 2.0, 2.6);
            pShot->param_d[0] = stopY; // 弾ごとの静止y
        }
        // 難易度2以上:下段にもう1ライン追加
        if (lv >= 2) {
            for (int k = 0; k < 5; k++) {
                sEnemyShot* pShot = Signal_AddShot(pSet, img_enemyShotLargeBall[0],
                    60.0 + k * 80.0, -12.0, DX_PI / 2.0, 2.0);
                pShot->param_d[0] = 310.0;
            }
        }
    }

    // 赤信号中の狙い撃ち(青小玉3連):静止弾の隙間を移動させるための弾
    if (t >= PHASE_RED && t < PHASE_BLINK && t % 60 == 30) {
        sEnemyShotSet* pSet = Signal_NewShotSet(Signal_ShotStraight,
            enemy.x, enemy.y + 10.0, 0.0, 0);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double base = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int i = -1; i <= 1; i++) {
            Signal_AddShot(pSet, img_enemyShotSmallBall[4],   // 小玉:青
                pSet->x, pSet->y, base + i * 0.15, 2.4 + 0.15 * lv);
        }
    }
}