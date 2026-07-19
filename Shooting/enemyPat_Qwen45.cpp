// enemyPat_DNA.cpp
// DNA二重螺旋弾幕 "Double Helix Gate"

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------
// DNA弾幕パターン関数
// ---------------------------------------------------------
static void ShotDNA(sEnemyShotSet* pEnemyShotSet)
{
    // --- 定数定義 ---
    const double OMEGA = 0.12;      // 回転速度 (ねじれの強さ)
    const double RADIUS = 32.0;     // 螺旋の半径 (レーザーの長さと一致させる)
    const double SPEED = 3.5;       // 前進速度
    const int INTERVAL = 5;         // 何フレームごとに1節(骨格ペア+レーザー)を生成するか

    // --- 1. 弾の生成 ---
    // pEnemyShotSet->count はメインルーチンでインクリメントされる
    if (pEnemyShotSet->count % INTERVAL == 0 && pEnemyShotSet->count < 120) {
        // 発射音 (セット生成時のみ)
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        double base_angle = pEnemyShotSet->muki; // 基準進行方向
        double start_x = pEnemyShotSet->x;
        double start_y = pEnemyShotSet->y;

        // 補助関数: 弾を生成してリストに追加する
        auto AddShot = [&](int kind, double phase) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = kind;
            p->speed = SPEED;
            p->muki = base_angle;

            // パラメータ設定 (座標計算用)
            p->param_d[0] = base_angle; // 基準角度
            p->param_d[1] = SPEED;      // 速度
            p->param_d[2] = OMEGA;      // 角速度
            // 半径: 骨格弾はRADIUS、レーザーは0(中心軸上)
            p->param_d[3] = (kind == img_enemyShotLaser[6]) ? 0.0 : RADIUS;
            p->param_d[4] = phase;      // 位相
            p->param_d[5] = start_x;    // 初期X
            p->param_d[6] = start_y;    // 初期Y

            // リスト登録
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        };

        // 骨格弾A (シアン) : 位相 0
        AddShot(img_enemyShotMediumBall[3], 0.0);
        // 骨格弾B (マゼンタ) : 位相 PI (180度反対)
        AddShot(img_enemyShotMediumBall[5], DX_PI);
        // 塩基対 (レーザー) : 中心軸上に配置
        AddShot(img_enemyShotLaser[6], 0.0);
    }

    // --- 2. 弾の座標更新 ---
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)p->count; // 経過時間 (メインで+1される)
        double base_angle = p->param_d[0];
        double speed = p->param_d[1];
        double omega = p->param_d[2];
        double r = p->param_d[3];
        double phase = p->param_d[4];
        double x0 = p->param_d[5];
        double y0 = p->param_d[6];

        // 中心軸の現在位置 (等速直線運動)
        double cx = x0 + speed * cos(base_angle) * t;
        double cy = y0 + speed * sin(base_angle) * t;

        // 現在の回転角度
        double angle = omega * t + phase;

        if (r == 0.0) {
            // --- レーザー (塩基対) ---
            // 中心軸上に位置する
            p->x = cx;
            p->y = cy;
            // 角度は骨格弾を結ぶ方向 (omega * t) に設定し、回転させる
            p->muki = omega * t;
        }
        else {
            // --- 骨格弾 ---
            // 中心軸の周りを円運動する
            p->x = cx + r * cos(angle);
            p->y = cy + r * sin(angle);
        }

        p = p->next;
    }
}

// ---------------------------------------------------------
// 敵本体のパターン
// ---------------------------------------------------------
void EnemyPat_DNA_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 移動 (左右往復)
        enemy.x += 1.48 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // DNA鎖の生成
    // 60フレームごとに新しい鎖(セット)を生成する
    if (count % 60 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDNA;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // 発射角度: 画面下向き(PI/2)を基準に、鎖が交差するように角度をずらす
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->kind = shot_count++;

        // ヘッダ初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リスト登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}