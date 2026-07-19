// enemyPat_Tmp.cpp
// 弾幕パターン「蛇腹追詰レーザー “メアンダサーペント”」の実装
// 敵本体の関数名: void EnemyPat_HenyoriLaser_DeepSeek()

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---- レーザー挙動に関する定数 ----
constexpr double EMITTER_Y = 50.0;            // レーザー発射口のY座標
constexpr double TIP_SPEED = 1.8;             // 先端の伸び速度 (px/frame)
constexpr double SCREEN_H = 480.0;           // 画面下端
constexpr double MAX_LASER_LENGTH = SCREEN_H - EMITTER_Y;  // 画面上で伸びきるまでの長さ
constexpr int    NUM_LASER_NODES = 100;             // レーザーを構成する弾の数
constexpr int    LASER_LIFE_FRAMES = 480;             // レーザー1波が完全に消えるまでのフレーム数（余裕を持たせる）
constexpr int    INTERVAL_FRAMES = 0;             // 再攻撃までの待機フレーム (5秒)

// ---- レーザー用ショットセットの更新関数 ----
static void LaserUpdate(sEnemyShotSet* pSet) {
    int t = pSet->count;   // このセットが生成されてからの経過フレーム

    // パラメータ取り出し (param_d[] に保存)
    double amp = pSet->param_d[0];  // 振幅（初期140 → 7秒後190）
    double omega = pSet->param_d[1];  // 角速度（初期0.08 → 7秒後0.12）
    double phi_base = pSet->param_d[2];  // 基本位相（左/右でずらす）
    double trackX = pSet->param_d[3];  // 自機追従用オフセット

    // 自機X方向へゆっくり追従
    trackX += (player.x - trackX) * 0.025;
    pSet->param_d[3] = trackX;

    // 位相の低周波揺らぎ（4秒周期、振幅0.4 rad）
    double phiFluct = 0.4 * sin(t * 2.0 * DX_PI / (4.0 * 60.0));
    double phi = phi_base + phiFluct;

    // 7秒（420フレーム）経過で攻勢段階に移行（未移行なら）
    if (t >= 420 && pSet->param_i[1] == 0) {
        amp = 190.0;
        omega = 0.12;
        pSet->param_d[0] = amp;
        pSet->param_d[1] = omega;
        pSet->param_i[1] = 1;   // 移行済みフラグ
    }

    // 現在のレーザー光束の始点Yと長さ
    double curLength = (TIP_SPEED * t < MAX_LASER_LENGTH) ? TIP_SPEED * t : MAX_LASER_LENGTH;
    double startY = EMITTER_Y + ((TIP_SPEED * t > MAX_LASER_LENGTH) ? (TIP_SPEED * t - MAX_LASER_LENGTH) : 0.0);

    // 節弾（ふしだま）の発生判定用：現在のエミッター位置での位相と sin 値
    double emitterPhase = omega * t + phi;               // d=0 の時の位相
    double curSin = sin(emitterPhase);

    // 節弾スポーン（前回のsin値がしきい値未満で、今回0.85以上になった瞬間）
    if (fabs(curSin) >= 0.85 && fabs(pSet->param_d[5]) < 0.85 && t > 0) {
        double emX = trackX + amp * curSin;  // エミッター位置のX座標
        // 基本の1発
        sEnemyShot* pNew = new sEnemyShot;
        pNew->x = emX;
        pNew->y = EMITTER_Y;
        pNew->muki = atan2(player.y - EMITTER_Y, player.x - emX);
        pNew->speed = 2.4;
        pNew->kind = img_enemyShotSmallBall[3];   // シアン小玉（青白い光弾）
        pNew->param_i[2] = 1;                     // 節弾である印
        pNew->margin = 480.0;
        // リスト末尾に追加
        pNew->prev = pSet->pEnemyShotHead->prev;
        pNew->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pNew;
        pSet->pEnemyShotHead->prev = pNew;

        // 攻勢段階では50%の確率でもう1発追加
        if (pSet->param_i[1] == 1 && GetRand(1) == 0) {
            sEnemyShot* pNew2 = new sEnemyShot;
            pNew2->x = emX;
            pNew2->y = EMITTER_Y + (GetRand(20) - 10) * 0.1;  // わずかにYをずらす
            pNew2->muki = atan2(player.y - pNew2->y, player.x - pNew2->x);
            pNew2->speed = 2.4;
            pNew2->kind = img_enemyShotSmallBall[4];  // 青小玉
            pNew2->param_i[2] = 1;
            pNew2->margin = 480.0;
            pNew2->prev = pSet->pEnemyShotHead->prev;
            pNew2->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pNew2;
            pSet->pEnemyShotHead->prev = pNew2;
        }
    }
    pSet->param_d[5] = curSin;  // 次回判定用に保存

    // ---- 全弾の更新 ----
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[2] == 0) {
            // レーザー節点ノード：曲線上に再配置
            double alpha = pShot->param_d[0];          // 0～1 の位置パラメータ
            double y = startY + alpha * curLength;     // 現在のY座標
            double d = y - EMITTER_Y;                  // エミッタからの距離
            double delay = d / TIP_SPEED;              // エミッタの動きが伝わる遅延
            double wavePhase = omega * (t - delay) + phi;
            pShot->x = trackX + amp * sin(wavePhase);
            pShot->y = y;
        }
        else {
            // 節弾ノード：等速直線運動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ---- レーザーセット生成のヘルパ ----
static void CreateLaserSet(int id, double basePhase) {
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = LaserUpdate;
    pSet->x = enemy.x;
    pSet->y = EMITTER_Y;
    pSet->kind = 0;

    // パラメータ設定
    pSet->param_i[0] = id;               // 0:左, 1:右
    pSet->param_i[1] = 0;                // 攻勢段階フラグ（0:通常, 1:強化）
    pSet->param_d[0] = 140.0;            // 振幅
    pSet->param_d[1] = 0.08;             // 角速度
    pSet->param_d[2] = basePhase;        // 基本位相（左右でずらす）
    pSet->param_d[3] = enemy.x;          // 自機追従オフセットの初期値
    pSet->param_d[5] = 0.0;              // 前回のsin値（節弾判定用）

    // ダミーヘッド作成
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    // レーザー節点ノードとなる弾を NUM_LASER_NODES 個生成
    for (int j = 0; j < NUM_LASER_NODES; ++j) {
        sEnemyShot* pNode = new sEnemyShot;
        pNode->param_i[2] = 0;                              // レーザーノード
        pNode->param_d[0] = (double)j / (NUM_LASER_NODES - 1); // 曲線上の位置(0～1)
        pNode->kind = img_enemyShotSmallBall[4];             // 青小玉（レーザー表現）
        pNode->x = 0.0;
        pNode->y = 0.0;
        pNode->speed = 0.0;
        pNode->muki = 0.0;
        pNode->margin = 480.0;
        // リストに追加
        pNode->prev = pSet->pEnemyShotHead->prev;
        pNode->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pNode;
        pSet->pEnemyShotHead->prev = pNode;
    }

    // グローバルな shot set リストに登録
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ---- 敵本体パターン関数 ----
void EnemyPat_HenyoriLaser_DeepSeek() {
    static int stateCount = 0;
    if (count == 1) stateCount = 0;

    stateCount++;

    // 初回呼び出しで敵の初期化
    if (stateCount == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 1波目（開始直後）
    if (stateCount % (LASER_LIFE_FRAMES + INTERVAL_FRAMES) == 1) {
        // 効果音再生（一度だけStopSoundMemで重複防止）
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 左レーザー (sin)、右レーザー (cos) に対応するため位相をずらす
        CreateLaserSet(0, 0.0);               // 左：sin
        CreateLaserSet(1, DX_PI / 2.0);       // 右：cos
    }

    //// 2波目（1波目のレーザーが完全に消えた後、5秒のインターバルをおいて開始）
    //// 1波目の全ライフ ≒ 684フレーム + インターバル 300 = 984 → 985フレーム目に発射
    //if (stateCount == 1 + LASER_LIFE_FRAMES + INTERVAL_FRAMES) {
    //    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
    //    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

    //    // 2波目は位相をさらに +π/4 ずらして見た目を変える
    //    CreateLaserSet(0, DX_PI / 4.0);
    //    CreateLaserSet(1, DX_PI / 2.0 + DX_PI / 4.0);
    //}
}