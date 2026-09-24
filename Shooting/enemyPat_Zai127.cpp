// enemyPat_Tmp.cpp
// 弾幕:「終末の喇叭(ラッパ)〜The Last Trumpet〜」
//
// 構成:
//   [フェーズ1: ラッパの顕現](set->count 0〜239)
//     - 円錐(ラッパの鐘)を2本の螺旋で描く。手前層=白の小玉、奥層=青の中玉(半径を軸側へ0.8倍して影に見せる)
//     - 螺旋全体を錐軸周りに毎秒60度で回転
//     - 吹き口には大玉(橙)を1発固定
//   [フェーズ2: 審判の號音](set->count 240〜479、以降フェーズ1へ再構築してループ)
//     - 号音: 吹き口から小玉(黄)を毎0.1秒、錐軸に沿って直線射出
//     - 音波: 鐘口からリング弾(中玉24発の円環)を0.6秒ごと。発射直後は低速、30フレーム後に加速
//     - 余韻: 各リングに合わせ、鐘口から下向き60度扇状に中玉(橙)16発
//     - 崩壊と再構築: 螺旋は鐘口へ吸い込まれ(pull-in)、最後の60フレームで元の円錐へ展開してループ
//
// ※ count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//    画面外の弾の消去はメインルーチン側で行う仕様。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const double MOUTH_X = 240.0;   // 吹き口(錐体の頂点)のX
static const double MOUTH_Y = 70.0;    // 吹き口のY
static const double BELL_Y = 230.0;   // 鐘口のY
static const double R_MOUTH = 5.0;     // 吹き口側の半径
static const double R_BELL = 70.0;    // 鐘口側の半径
static const int    SPIRAL_POINTS = 40;        // 螺旋1本あたりの点数
static const int    SPIRAL_TURNS = 5;         // 螺旋の巻き数(×2π)
static const double ROT_SPEED = DX_PI / 180.0; // 回転速度(毎秒60度)
static const int    PHASE1_END = 240;  // フェーズ1終了フレーム
static const int    PHASE2_END = 480;  // フェーズ2終了(ループ)フレーム
static const int    SUCK_END = 180;  // 吸い込み終了(フェーズ2開始からの相対フレーム)

// 弾の役割(param_i[0]に格納)
enum {
    ROLE_SPIRAL_BACK = 0,  // 螺旋・奥層
    ROLE_SPIRAL_FRONT,     // 螺旋・手前層
    ROLE_MOUTH,            // 吹き口の大玉(固定)
    ROLE_CORE,             // 号音(直進する音の芯)
    ROLE_RING,             // 音波(リング)
    ROLE_GLOW              // 鐘の余韻(扇状散開)
};

// ============================================================
//  弾の生成ヘルパ
// ============================================================
static sEnemyShot* TrumpetAddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, int role)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = role;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// ============================================================
//  ラッパ弾幕本体
// ============================================================
static void ShotTrumpet(sEnemyShotSet* pSet)
{
    const int c = pSet->count;
    const int pc = c % 600 - PHASE1_END; // フェーズ2開始からの相対フレーム(フェーズ1中は負)

    // ---- 効果音 ----
    if (pc == -PHASE1_END) {
        // 顕現の音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    if (pc == 0) {
        // 号音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ---- 吹き口の大玉と螺旋の生成(フェーズ1の最初のみ) ----
    if (c == 0) {
        TrumpetAddShot(pSet, MOUTH_X, MOUTH_Y, 0.0, 0.0,
            img_enemyShotLargeBall[8], ROLE_MOUTH);
    }
    if (c < SPIRAL_POINTS) {
        // 1フレームに1点ずつ、4本の鎖(奥層2本+手前層2本)を上から順に生やす
        double t = (double)c / (double)(SPIRAL_POINTS - 1);
        for (int chain = 0; chain < 4; chain++) {
            int    role = (chain < 2) ? ROLE_SPIRAL_BACK : ROLE_SPIRAL_FRONT;
            double phase = (chain % 2) * DX_PI; // 2本の螺旋は位相180度ずらし
            sEnemyShot* p = TrumpetAddShot(pSet, MOUTH_X, MOUTH_Y, 0.0, 0.0,
                (role == ROLE_SPIRAL_BACK)
                ? img_enemyShotMediumBall[4] // 奥:青の中玉
                : img_enemyShotSmallBall[6], // 手前:白の小玉
                role);
            p->param_d[0] = t;           // 現在の錐体パラメータ(0=吹き口, 1=鐘口)
            p->param_d[1] = phase;       // 螺旋の位相
            p->param_d[2] = t;           // 元のt(再構築用)
        }
    }

    // ---- フェーズ2の射出 ----
    if (pc >= 0 && pc < PHASE2_END - PHASE1_END) {
        auto angle_to_player = atan2(player.y - MOUTH_Y, player.x - MOUTH_X);
        // 号音: 吹き口から錐軸に沿って直進する小玉(毎0.1秒=6フレーム)
        if (pc % 6 == 0) {
            TrumpetAddShot(pSet, MOUTH_X, MOUTH_Y, angle_to_player, 4.5,
                img_enemyShotSmallBall[1], ROLE_CORE);
        }
        // 音波+余韻: 0.6秒(36フレーム)ごと
        if (pc % 36 == 0) {
            // リングの開始角をランダムに(リプレイ対応のためGetRandを使用)
            // ※GetRand(x)は 0〜x の x+1 種類を返すので、360度分なら GetRand(359)
            double baseAngle = GetRand(359) / 180.0 * DX_PI;

            // 音波: 鐘口から全周24発
            for (int i = 0; i < 24; i++) {
                double a = baseAngle + 2.0 * DX_PI * i / 24.0;
                TrumpetAddShot(pSet, MOUTH_X, BELL_Y, a, 1.3,
                    img_enemyShotMediumBall[3], ROLE_RING);
            }
            // 余韻: 鐘口から下向き60度扇状に16発(リングを追いかける)
            for (int i = 0; i < 16; i++) {
                double a = angle_to_player + (DX_PI / 3.0) * ((i / 15.0) - 0.5);
                TrumpetAddShot(pSet, MOUTH_X, BELL_Y, a, 2.0,
                    img_enemyShotMediumBall[8], ROLE_GLOW);
            }
        }
    }

    // ---- 全弾の位置更新 ----
    // 螺旋の現在回転角を進める(フェーズ中は常時回転)
    pSet->param_d[0] += ROT_SPEED;
    double theta = pSet->param_d[0];

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case ROLE_MOUTH:
            // 吹き口の大玉は固定
            break;

        case ROLE_SPIRAL_BACK:
        case ROLE_SPIRAL_FRONT:
        {
            // フェーズ2では鐘口へ吸い込まれ、最後の60フレームで元の円錐へ再構築
            if (pc >= 0 && pc < SUCK_END) {
                pShot->param_d[0] -= 0.008;
                if (pShot->param_d[0] < 0.0) pShot->param_d[0] = 0.0;
            }
            else if (pc >= SUCK_END && pc < PHASE2_END - PHASE1_END) {
                pShot->param_d[0] += 0.024;
                if (pShot->param_d[0] > pShot->param_d[2]) pShot->param_d[0] = pShot->param_d[2];
            }

            // 円錐面上の位置を計算(疑似3D)
            double t = pShot->param_d[0];
            double ang = theta + pShot->param_d[1]
                + 2.0 * DX_PI * SPIRAL_TURNS * t;
            // 奥層は半径を0.8倍して軸側に寄せ、影のように見せる
            double scale = (pShot->param_i[0] == ROLE_SPIRAL_BACK) ? 0.8 : 1.0;
            double r = (R_MOUTH + (R_BELL - R_MOUTH) * t) * scale;

            pShot->x = MOUTH_X + r * cos(ang);
            pShot->y = MOUTH_Y + (BELL_Y - MOUTH_Y) * t;
            break;
        }

        case ROLE_RING:
            // 音波: 30フレーム(予備動作)後に一気に加速
            if (pShot->count > 30) {
                double s = 1.3 + (pShot->count - 30) * 0.14;
                if (s > 5.3) s = 5.3;
                pShot->speed = s;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case ROLE_CORE:
        case ROLE_GLOW:
        default:
            // 直進するだけの弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_GabrielsHorn_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480。ラッパは画面中央上段に固定で顕現する
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 予告(1.5秒)を置いてから弾幕セットを開始
    if (count == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTrumpet;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = DX_PI / 2.0; // 鐘口は下向き
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_d[0] = 0.0;   // 螺旋の回転角初期値

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}