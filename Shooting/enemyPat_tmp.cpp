//--------------------------------------------------------------
// 弾幕：ルーロー三角形モチーフ「境界輪廻(ルーロー・サイクル)」
//
//  [フェーズ1] 頂点の生成 : 3つの固定小玉(頂点砲台)を設置
//  [フェーズ2] 輪郭の描画 : 各頂点から「他の2頂点」へ向かって扇状に小玉を発射。
//                           減速して一辺の長さの距離で停止するので、
//                           3つの円弧が合わさり外側に膨らんだ三角形の輪郭が浮かぶ
//  [フェーズ3] 回転       : 輪郭ごと重心周りに回転。頂点から内側へ中玉をばら撒く
//  [フェーズ4] 放出       : 全弾を重心から放射状に炸裂させ、次サイクルへ
//--------------------------------------------------------------
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --- ジオメトリ定数 ---
static const double REULEAU_RC = 70.0;                     // 中心〜頂点(外接半径)
static const double REULEAU_L = REULEAU_RC * sqrt(3.0);   // 一辺の長さ = ルーロー三角形の幅
static const int    ROT_START = 120;                      // 回転開始フレーム(セット内カウント)
static const int    ROT_END = 300;                      // 放出フレーム
static const int    CYCLE = 400-70;                      // 1サイクルの長さ

// 弾をリスト末尾に追加するヘルパー
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 弾幕パターン本体
static void ShotReuleaux(sEnemyShotSet* pEnemyShotSet)
{
    const double CX = pEnemyShotSet->x;   // 三角形の重心
    const double CY = pEnemyShotSet->y;
    const double BASE = pEnemyShotSet->param_d[0];  // サイクルごとの初期角度
    const double OMEGA = 0.008 * ((pEnemyShotSet->param_i[1] == 0) ? 1.0 : -1.0); // 回転角速度(交互に反転)
    const bool   rotating = (pEnemyShotSet->count >= ROT_START && pEnemyShotSet->count < ROT_END);
    const int    cnt = pEnemyShotSet->count;

    // ---------- フェーズ1: 頂点砲台(固定弾)の設置 ----------
    if (cnt == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        for (int v = 0; v < 3; v++) {
            double ang = BASE + v * 2.0 * DX_PI / 3.0;
            sEnemyShot* pShot = AddShot(pEnemyShotSet,
                CX + REULEAU_RC * cos(ang), CY + REULEAU_RC * sin(ang),
                0.0, 0.0, img_enemyShotSmallBall[6]);   // 白
            pShot->param_i[0] = 1;  // 頂点砲台フラグ(速度0で場に残る)
        }
    }

    // ---------- フェーズ2: 輪郭(3つの円弧)の描画 ----------
    if (cnt == 40) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int    FAN = 16;
        const double V0 = 3.5;
        const double DECEL = V0 * V0 / (2.0 * REULEAU_L);   // 一辺の長さちょうどで停止する減速度

        for (int v = 0; v < 3; v++) {
            double angV = BASE + v * 2.0 * DX_PI / 3.0;
            double vx = CX + REULEAU_RC * cos(angV);
            double vy = CY + REULEAU_RC * sin(angV);
            // 他の2頂点への方向(その間が60度の円弧になる)
            double dir[2];
            for (int k = 0; k < 2; k++) {
                double angO = BASE + ((v + 1 + k) % 3) * 2.0 * DX_PI / 3.0;
                dir[k] = atan2(CY + REULEAU_RC * sin(angO) - vy,
                    CX + REULEAU_RC * cos(angO) - vx);
            }
            // 2方向の中間角(円弧の中心方向)
            double mid = atan2(sin(dir[0]) + sin(dir[1]), cos(dir[0]) + cos(dir[1]));
            // ±30度の扇状に発射 → 半径Lの円弧上で静止する
            for (int j = 0; j < FAN; j++) {
                double t = (double)j / (FAN - 1) - 0.5;     // -0.5〜+0.5
                sEnemyShot* pShot = AddShot(pEnemyShotSet, vx, vy,
                    mid + t * DX_PI / 3.0, V0, img_enemyShotSmallBall[3]);  // シアン
                pShot->param_i[1] = 1;      // 減速フラグ
                pShot->param_d[0] = DECEL;  // 減速度
            }
        }
    }

    // ---------- フェーズ3: 頂点からの内側ばら撒き(回転中、50フレームごと) ----------
    // GetRand(60)-30 は -30〜+30 の61種類を返すことに注意
    if (cnt >= ROT_START && cnt < ROT_END && cnt % 30 == 20) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double rot = BASE + pEnemyShotSet->param_d[1];  // 現在の回転角を加味した頂点位置
        for (int v = 0; v < 3; v++) {
            double ang = rot + v * 2.0 * DX_PI / 3.0;
            double vx = CX + REULEAU_RC * cos(ang);
            double vy = CY + REULEAU_RC * sin(ang);
            // 自機狙い1発
            AddShot(pEnemyShotSet, vx, vy,
                atan2(player.y - vy, player.x - vx), 2.2, img_enemyShotMediumBall[0]);  // 赤
            // 重心方向への拡散4発
            for (int j = 0; j < 4*4; j++) {
                double muki = atan2(CY - vy, CX - vx) + (GetRand(60) - 30) / 180.0 * DX_PI;
                double speed = 1.5 + GetRand(100) / 100.0;
                AddShot(pEnemyShotSet, vx, vy, muki, speed, img_enemyShotMediumBall[0]);
            }
        }
    }

    // ---------- フェーズ4: 全弾放出(重心から放射状に炸裂) ----------
    if (cnt == ROT_END) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->muki = atan2(pShot->y - CY, pShot->x - CX);
            pShot->speed = 3.0;
            pShot->param_i[0] = 0;  // フラグ解除
            pShot->param_i[1] = 0;
            pShot = pShot->next;
        }
    }

    // ---------- 毎フレーム: 移動 & 重心周りの回転 ----------
    double cosW = cos(OMEGA), sinW = sin(OMEGA);
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->speed != 0.0) {
            if (pShot->param_i[1] == 1) {   // 減速しながら直進(円弧になる輪郭弾)
                pShot->speed -= pShot->param_d[0];
                if (pShot->speed <= 0.0) {
                    pShot->speed = 0.0;
                    pShot->param_i[1] = 0;
                }
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        if (rotating) { // 回転フェーズ中は全弾(頂点砲台含む)を重心周りで公転させる
            double dx = pShot->x - CX;
            double dy = pShot->y - CY;
            pShot->x = CX + dx * cosW - dy * sinW;
            pShot->y = CY + dx * sinW + dy * cosW;
            pShot->muki += OMEGA;
        }
        pShot = pShot->next;
    }
    if (rotating) pEnemyShotSet->param_d[1] += OMEGA;   // 現在回転角を累積
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    static int shot_count;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        shot_count = 0;
    }
    // ゆっくり左右に揺れる(三角形の中心は固定なので弾幕には影響しない)
    enemy.x = 240.0 + 30.0 * sin(count * 0.02);

    // 1サイクルごとに新しいショットセットを生成
    if (count % CYCLE == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotReuleaux;
        pEnemyShotSet->x = 240.0;               // ルーロー三角形の重心
        pEnemyShotSet->y = 150.0+50;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count;
        pEnemyShotSet->param_i[0] = shot_count;
        pEnemyShotSet->param_i[1] = shot_count % 2;     // 回転方向(サイクルごとに反転)
        pEnemyShotSet->param_d[0] = shot_count * 0.35;  // 初期角度を少しずつずらす
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
        shot_count++;
    }
}