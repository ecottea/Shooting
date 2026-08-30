// enemyPat_tmp.cpp
// 信号機モチーフ弾幕「トリコロール・シグナル」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 信号の色
#define SIGNAL_RED    0
#define SIGNAL_YELLOW 1
#define SIGNAL_GREEN  2

// 1色の持続フレーム数（3秒）
#define SIGNAL_PHASE_DURATION 180

// プロトタイプ宣言
static void SignalBodies(sEnemyShotSet* pSet);
static void SignalController(sEnemyShotSet* pSet);
static void RedSingleAimed(sEnemyShotSet* pSet);
static void Red3WayAimed(sEnemyShotSet* pSet);
static void YellowRadial(sEnemyShotSet* pSet);
static void YellowRandom(sEnemyShotSet* pSet);
static void GreenHoming(sEnemyShotSet* pSet);
static void GreenSpiral(sEnemyShotSet* pSet);

// 弾を生成してリスト末尾に追加するヘルパー
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    for (int i = 0; i < 16; ++i) {
        pShot->param_i[i] = 0;
        pShot->param_d[i] = 0.0;
    }
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 新しいショットセットを作成してグローバルリストに追加するヘルパー
static sEnemyShotSet* CreateShotSet(double x, double y, double muki, int kind, void(*patternFunc)(sEnemyShotSet*))
{
    sEnemyShotSet* pNew = new sEnemyShotSet;
    pNew->count = 0;
    pNew->patternFunc = patternFunc;
    pNew->x = x;
    pNew->y = y;
    pNew->muki = muki;
    pNew->kind = kind;
    for (int i = 0; i < 16; ++i) {
        pNew->param_i[i] = 0;
        pNew->param_d[i] = 0.0;
    }
    pNew->pEnemyShotHead = new sEnemyShot;
    pNew->pEnemyShotHead->prev = pNew->pEnemyShotHead;
    pNew->pEnemyShotHead->next = pNew->pEnemyShotHead;

    pNew->prev = enemyShotSetHead.prev;
    pNew->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pNew;
    enemyShotSetHead.prev = pNew;
    return pNew;
}

// ============================================================
// 信号機本体（赤・黄・緑の大型弾を縦に配置）
// ============================================================
static void SignalBodies(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 赤（上）
        AddShot(pSet, pSet->x, pSet->y, 0.0, 0.0, img_enemyShotLargeBall[0]);
        // 黄（中）
        AddShot(pSet, pSet->x, pSet->y + 80.0, 0.0, 0.0, img_enemyShotLargeBall[1]);
        // 緑（下）
        AddShot(pSet, pSet->x, pSet->y + 160.0, 0.0, 0.0, img_enemyShotLargeBall[2]);
    }
    // 本体は動かない（speed=0）ため、以降何もしない
}

// ============================================================
// 信号制御部（アクティブな色を切り替え、対応する弾幕を発生させる）
// ============================================================
static void SignalController(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        pSet->param_i[0] = SIGNAL_RED;   // 現在のアクティブ色
        pSet->param_i[1] = 0;            // フェーズ内タイマー
        pSet->alive = 99999;
        return;
    }

    // タイマー更新
    pSet->param_i[1]++;

    // 一定時間で色切り替え
    if (pSet->param_i[1] >= SIGNAL_PHASE_DURATION) {
        pSet->param_i[0] = (pSet->param_i[0] + 1) % 3;
        pSet->param_i[1] = 0;
        // 切り替え時に効果音（軽め）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    int active = pSet->param_i[0];

    // アクティブ色ごとの弾幕発生
    switch (active) {
    case SIGNAL_RED:
        // 赤：高速自機狙い針弾（0.1秒ごと）
        if (pSet->count % 6 == 0) {
            double muki = atan2(player.y - pSet->y, player.x - pSet->x);
            CreateShotSet(pSet->x, pSet->y, muki, 0, RedSingleAimed);
        }
        // 赤：3-way高速弾（0.5秒ごと）
        if (pSet->count % 30 == 0) {
            double muki = atan2(player.y - pSet->y, player.x - pSet->x);
            CreateShotSet(pSet->x, pSet->y, muki, 0, Red3WayAimed);
        }
        break;

    case SIGNAL_YELLOW:
        // 黄：全方位12方向拡散弾（0.3秒ごと）
        if (pSet->count % 18 == 0) {
            CreateShotSet(pSet->x, pSet->y + 80.0, 0.0, 1, YellowRadial);
        }
        // 黄：ランダム角度6発（1秒ごと）
        if (pSet->count % 60 == 0) {
            double muki = atan2(player.y - (pSet->y + 80.0), player.x - pSet->x);
            CreateShotSet(pSet->x, pSet->y + 80.0, muki, 1, YellowRandom);
        }
        break;

    case SIGNAL_GREEN:
        // 緑：低速追尾弾（0.5秒ごと）
        if (pSet->count % 30 == 0) {
            CreateShotSet(pSet->x, pSet->y + 160.0, 0.0, 2, GreenHoming);
        }
        // 緑：螺旋弾（2秒ごと）
        if (pSet->count % 120 == 0) {
            CreateShotSet(pSet->x, pSet->y + 160.0, 0.0, 2, GreenSpiral);
        }
        break;
    }
}

// ============================================================
// 赤：高速自機狙い弾（単発）
// ============================================================
static void RedSingleAimed(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 赤い銃弾（針の代用）を高速で自機狙い
        AddShot(pSet, pSet->x, pSet->y, pSet->muki, 5.0, img_enemyShotBullet[0]);
    }

    // 弾を移動（直線）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 赤：3-way高速弾
// ============================================================
static void Red3WayAimed(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseMuki = pSet->muki;
        for (int i = -1; i <= 1; ++i) {
            double muki = baseMuki + i * (15.0 / 180.0 * DX_PI);
            AddShot(pSet, pSet->x, pSet->y, muki, 5.0, img_enemyShotBullet[0]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 黄：全方位12方向拡散弾
// ============================================================
static void YellowRadial(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12; ++i) {
            double muki = i * (DX_PI * 2.0 / 12.0);
            AddShot(pSet, pSet->x, pSet->y, muki, 2.5, img_enemyShotMediumBall[1]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 黄：ランダム角度6発（自機方向基準±30°）
// ============================================================
static void YellowRandom(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double baseMuki = pSet->muki;
        for (int i = 0; i < 6; ++i) {
            // GetRand(60) は 0～60 の整数を返す。±30°に変換
            double offset = (GetRand(60) - 30) / 180.0 * DX_PI;
            double muki = baseMuki + offset;
            AddShot(pSet, pSet->x, pSet->y, muki, 2.5, img_enemyShotMediumBall[1]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 緑：低速追尾弾
// ============================================================
static void GreenHoming(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 緑色の中玉、低速
        sEnemyShot* pShot = AddShot(pSet, pSet->x, pSet->y, 0.0, 1.2, img_enemyShotMediumBall[2]);
        pShot->param_d[0] = 0.03; // 旋回速度（ラジアン/フレーム）
    }

    // 追尾処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 自機方向を計算
        double desired = atan2(player.y - pShot->y, player.x - pShot->x);
        double diff = desired - pShot->muki;
        // -π ～ π に正規化
        while (diff > DX_PI) diff -= DX_PI * 2.0;
        while (diff < -DX_PI) diff += DX_PI * 2.0;
        double turn = pShot->param_d[0];
        if (diff > turn) diff = turn;
        if (diff < -turn) diff = -turn;
        pShot->muki += diff;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 緑：螺旋弾（8方向×2連射、回転オフセット）
// ============================================================
static void GreenSpiral(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 1連射目
        for (int i = 0; i < 8; ++i) {
            double muki = i * (DX_PI * 2.0 / 8.0) + pSet->muki;
            AddShot(pSet, pSet->x, pSet->y, muki, 1.5, img_enemyShotMediumBall[2]);
        }
    }
    else if (pSet->count == 20) {
        // 2連射目（22.5°回転）
        double offset = 22.5 / 180.0 * DX_PI;
        for (int i = 0; i < 8; ++i) {
            double muki = i * (DX_PI * 2.0 / 8.0) + pSet->muki + offset;
            AddShot(pSet, pSet->x, pSet->y, muki, 1.5, img_enemyShotMediumBall[2]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン（エントリポイント）
// ============================================================
void EnemyPat_TrafficLight_DeepSeek()
{
    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 60.0;   // 信号機の最上部（赤）の位置
        enemy.maxHp = enemy.hp = 200;

        // 信号機本体（静止した大型弾）を生成
        CreateShotSet(enemy.x, enemy.y, 0.0, 0, SignalBodies);

        // 信号制御部を生成
        CreateShotSet(enemy.x, enemy.y, 0.0, 0, SignalController);
    }
    // 以降は各ショットセットのpatternFuncが毎フレーム呼ばれる
}