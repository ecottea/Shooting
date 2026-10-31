// 転世「一条戻り橋」(東方永夜抄 Extra・上白沢慧音の第2スペルカード)
//
// 【再現している挙動】
//  ・赤と青の米粒(針)弾が、画面の外周から敵本体に向かって収束する
//  ・青・赤それぞれ「4way(π/2刻み) × 8連(扇状に8発)」を16フレーム毎に同期して発射
//  ・毎波ごとの基準角の回転量: 青 -2π/15、赤 +4π/35 (攻略wikiの解析値)
//    -> 2系統の相対角度がずれていくので、壁の隙間が少しずつ流れていく
//  ・開幕の角度だけランダム(GetRandはリプレイ用seedで再現可能)、以降は完全パターン
//  ・開始10秒後(残り50秒相当)から、一定間隔で自機狙いの青大玉を追加
//
// 【設計メモ】
//  ・ShotSet は「1波 = 1セット」。セット生成後の最初のフレーム(count==0)で64発を一括生成する
//    (enemyPat_sampleForAI.cpp と同じ構造。弾が全て消えたセットはメインルーチンが片付ける想定)
//  ・画面外の弾はメインルーチンが消すので、弾は「画面の縁の少し内側」に生成する
//    (MODORI_SPAWN_INSET。メインルーチンの消去判定に余白があるなら負値にして外側から出してよい)
//  ・弾は敵本体の位置に到達したら自分で消える(param_d[0] に残り移動距離を持つ)
//  ・count / pEnemyShotSet->count / pEnemyShot->count の加算はメインルーチン側なのでここでは触らない
//
// 【調整しやすい定数】 速度・扇の間隔・波の間隔などは下の定数をいじること。
//  wikiの数値は自己検算で完全には整合しなかったため、リプレイ動画と見比べて合わせ込むこと。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  定数 (CX/CY のようなWindows SDKと衝突する短い名前は避け、MODORI_ を付けている)
// ------------------------------------------------------------
static const double MODORI_FIELD_W = 480.0;            // ゲーム画面の幅
static const double MODORI_FIELD_H = 480.0;            // ゲーム画面の高さ
static const double MODORI_SPAWN_INSET = 2.0;              // 弾の生成位置を画面の縁から内側へずらす量[px]
static const double MODORI_HOME_X = 240.0;            // 敵本体の定位置
static const double MODORI_HOME_Y = 48.0+100;

static const int    MODORI_START_FRAME = 45;               // 最初の波を撃つフレーム(移動と予告音の後)
static const int    MODORI_WAVE_INTERVAL = 16;              // 波の間隔[フレーム]
static const int    MODORI_WAY_NUM = 4;                // 方向数(4way)
static const int    MODORI_FAN_NUM = 8-2;                // 1方向あたりの弾数(8連=扇状に8発)
static const double MODORI_FAN_STEP = DX_PI / 18.0;     // 扇の弾同士の角度間隔(10度)。隣の扇との隙間が弾2個分になる
static const double MODORI_BLUE_STEP = -2.0 * DX_PI / 15.0;  // 1波ごとの青の基準角の変化量
static const double MODORI_RED_STEP = 4.0 * DX_PI / 35.0;   // 1波ごとの赤の基準角の変化量
static const double MODORI_RED_OFFSET = DX_PI / 4.0;      // 開幕時の青に対する赤の相対角(固定値)
static const double MODORI_SHOT_SPEED = 2.0;              // 米粒弾の速さ
static const double MODORI_KILL_RADIUS = 8.0;              // 敵本体からこの距離まで来たら弾を消す

static const int    MODORI_AIMED_START = 600;            // 自機狙い大玉の開始フレーム(10秒後)
static const int    MODORI_AIMED_INTERVAL = 60;             // 自機狙い大玉の間隔[フレーム]
static const double MODORI_AIMED_SPEED = 2.6;            // 自機狙い大玉の速さ

// 弾の色(img_enemyShotXxx[色] の添字): 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
static const int    MODORI_COLOR_RED = 0;
static const int    MODORI_COLOR_BLUE = 4;

// ------------------------------------------------------------
//  補助関数
// ------------------------------------------------------------

// 弾をセットのリスト末尾に連結する
static void ModoriLinkShot(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->margin = 480;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ShotSetを生成してグローバルのリストに連結する
static sEnemyShotSet* ModoriCreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = 0;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// (ox, oy) から角度(dx, dy)方向へ進んだとき、画面の縁(内側にMODORI_SPAWN_INSETずらした矩形)までの距離
static double ModoriDistToEdge(double ox, double oy, double dx, double dy)
{
    const double minX = MODORI_SPAWN_INSET;
    const double maxX = MODORI_FIELD_W - MODORI_SPAWN_INSET;
    const double minY = MODORI_SPAWN_INSET;
    const double maxY = MODORI_FIELD_H - MODORI_SPAWN_INSET;
    double t = 1.0e9;
    double tmp;
    if (dx > 1.0e-9) {
        tmp = (maxX - ox) / dx;
        if (tmp < t) t = tmp;
    }
    else if (dx < -1.0e-9) {
        tmp = (minX - ox) / dx;
        if (tmp < t) t = tmp;
    }
    if (dy > 1.0e-9) {
        tmp = (maxY - oy) / dy;
        if (tmp < t) t = tmp;
    }
    else if (dy < -1.0e-9) {
        tmp = (minY - oy) / dy;
        if (tmp < t) t = tmp;
    }
    return t;
}

// ------------------------------------------------------------
//  弾幕：1波分の収束弾 (青・赤 各 4way × 8連 = 64発)
//   param_d[0] : 青の基準角
//   param_d[1] : 赤の基準角
//   セットの x, y : 収束先(生成時の敵本体の位置)
// ------------------------------------------------------------
static void ShotModoriWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int c = 0; c < 2; c++) {
            const double base = pEnemyShotSet->param_d[c];
            // 米粒弾は細長い弾の銃弾で表現する(青・赤)
            const int img = (c == 0) ? img_enemyShotDiamond[MODORI_COLOR_BLUE] : img_enemyShotDiamond[MODORI_COLOR_RED];
            for (int way = 0; way < MODORI_WAY_NUM; way++) {
                for (int j = 0; j < MODORI_FAN_NUM; j++) {
                    // 敵本体から見た、この弾が来る方向
                    const double theta = base + (double)way * (2.0 * DX_PI / (double)MODORI_WAY_NUM)
                        + ((double)j - (MODORI_FAN_NUM - 1) / 2.0) * MODORI_FAN_STEP;
                    const double dx = cos(theta);
                    const double dy = sin(theta);
                    const double t = ModoriDistToEdge(pEnemyShotSet->x, pEnemyShotSet->y, dx, dy);

                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pEnemyShotSet->x + dx * t;
                    pEnemyShot->y = pEnemyShotSet->y + dy * t;
                    pEnemyShot->muki = theta + DX_PI;               // 進行方向(敵本体向き)。弾の向きにも使う
                    pEnemyShot->speed = MODORI_SHOT_SPEED;
                    pEnemyShot->param_d[0] = t - MODORI_KILL_RADIUS; // 消えるまでの残り移動距離
                    pEnemyShot->kind = img;
                    ModoriLinkShot(pEnemyShotSet, pEnemyShot);
                }
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除に備えて先に保持
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->param_d[0] -= pShot->speed;
        if (pShot->param_d[0] <= 0.0) {
            // 敵本体に到達したので消す
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        pShot = pNext;
    }
}

// ------------------------------------------------------------
//  弾幕：自機狙いの青大玉(1発)
//   セットの muki : 自機への角度
// ------------------------------------------------------------
static void ShotModoriAimed(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = MODORI_AIMED_SPEED;
        pEnemyShot->kind = img_enemyShotLargeBall[MODORI_COLOR_BLUE];
        ModoriLinkShot(pEnemyShotSet, pEnemyShot);
    }

    // 画面外の消去はメインルーチン側
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Modoribashi_Claude()
{
    static double theta0; // 開幕の基準角(ランダム。以降の角度は全てここからの決定論的な式)

    if (count == 1) {
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        // GetRand(359) は 0〜359 の360通り。リプレイseedで再現可能
        theta0 = (double)GetRand(359) * DX_PI / 180.0;
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 予告音
    }

    // 定位置(画面上部中央)へ寄せる
    enemy.x += (MODORI_HOME_X - enemy.x) * 0.1;
    enemy.y += (MODORI_HOME_Y - enemy.y) * 0.1;

    // 収束弾: 16フレーム毎に1波
    if (count >= MODORI_START_FRAME && (count - MODORI_START_FRAME) % MODORI_WAVE_INTERVAL == 0) {
        const int wave = (count - MODORI_START_FRAME) / MODORI_WAVE_INTERVAL;
        sEnemyShotSet* pEnemyShotSet = ModoriCreateShotSet(ShotModoriWave, enemy.x, enemy.y, 0.0);
        pEnemyShotSet->param_d[0] = theta0 + (double)wave * MODORI_BLUE_STEP;
        pEnemyShotSet->param_d[1] = theta0 + MODORI_RED_OFFSET + (double)wave * MODORI_RED_STEP;
    }

    // 自機狙い大玉: 開始から10秒後以降、一定間隔
    if (count >= MODORI_AIMED_START && (count - MODORI_AIMED_START) % MODORI_AIMED_INTERVAL == 0) {
        const double sx = enemy.x;
        const double sy = enemy.y + 10.0;
        ModoriCreateShotSet(ShotModoriAimed, sx, sy, atan2(player.y - sy, player.x - sx));
    }
}