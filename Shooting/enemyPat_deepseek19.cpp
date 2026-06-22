// enemyPat_hyperbola.cpp
// 双曲線をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  弾幕パターン：回転する双曲線に沿って弾が流れる
// ------------------------------------------------------------
static void ShotHyperbola(sEnemyShotSet* pEnemyShotSet)
{
    const double a = 50.0;          // 双曲線のパラメータ a
    const double b = 40.0;          // 双曲線のパラメータ b
    const double dt = 0.002;         // 1フレームあたりのパラメータ t の増分
    const double rotateSpeed = 0.015; // 双曲線全体の回転速度

    if (pEnemyShotSet->count == 0) {
        // 効果音（中程度）を再生
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 両方の枝に計16個の弾を生成（正負の枝各8個）
        for (int i = 0; i < 16; i++) {
            sEnemyShot* p = new sEnemyShot;

            // 弾ごとの初期パラメータ t (正の枝は正、負の枝は負)
            double t0 = 0.2 + (i % 8) * 0.25;          // 0.2 ～ 1.95 まで
            if (GetRand(1)) t0 += 0.1 * (GetRand(100) / 100.0); // ランダムな揺らぎ
            if (i >= 8) t0 = -t0;                      // 後半8個は負の枝

            p->muki = t0;          // ここにパラメータ t を格納（角度としては使わない）
            p->speed = 0.0;        // 既定の直線移動を無効化

            // ランダムな見た目（小玉～菱形弾、色もランダム）
            int type = GetRand(5);   // [0..5]
            int color = GetRand(7);  // [0..7]
            switch (1) {
            case 0: p->kind = img_enemyShotSmallBall[color]; break;
            case 1: p->kind = img_enemyShotMediumBall[color]; break;
            case 2: p->kind = img_enemyShotLargeBall[color]; break;
            case 3: p->kind = img_enemyShotBullet[color]; break;
            case 4: p->kind = img_enemyShotScale[color]; break;
            case 5: p->kind = img_enemyShotDiamond[color]; break;
            }

            p->margin = 999;

            // 生成時の双曲線座標 (t の絶対値でcosh, 符号付きでsinh)
            double t_abs = fabs(t0);
            double x_rel = a * cosh(t_abs);
            double y_rel = b * sinh(t0);

            // 弾幕セットの初期回転角 pEnemyShotSet->muki で回転させる
            double cosR = cos(pEnemyShotSet->muki);
            double sinR = sin(pEnemyShotSet->muki);
            p->x = pEnemyShotSet->x + x_rel * cosR - y_rel * sinR;
            p->y = pEnemyShotSet->y + x_rel * sinR + y_rel * cosR;

            // 双方向リストに追加
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
    else {
        // 2フレーム目以降：双曲線全体をゆっくり回転させ、各弾の位置を更新
        pEnemyShotSet->muki += rotateSpeed;
        double cosR = cos(pEnemyShotSet->muki);
        double sinR = sin(pEnemyShotSet->muki);

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            double t = p->muki;
            double sign = (t >= 0) ? 1.0 : -1.0;
            t += sign * dt;               // パラメータ t を進行方向に進める
            p->muki = t;

            double x_rel = a * cosh(fabs(t));
            double y_rel = b * sinh(t);

            p->x = pEnemyShotSet->x + x_rel * cosR - y_rel * sinR;
            p->y = pEnemyShotSet->y + x_rel * sinR + y_rel * cosR;

            p = p->next;
        }
    }
}

// ------------------------------------------------------------
//  敵本体パターン（関数名：EnemyPat_Hyperbola_DeepSeek）
// ------------------------------------------------------------
void EnemyPat_Hyperbola_DeepSeek()
{
    static int muki = 1;

    if (count == 1) {
        // 初期配置・HP設定
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆっくり左右に動く
        enemy.x += 0.8 * muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 90フレームごとに双曲線弾幕セットを一つ生成
    if (count % 70 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHyperbola;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        // 双曲線の初期角度はランダム
        pEnemyShotSet->muki = (GetRand(360) / 180.0) * DX_PI;

        // ダミーヘッドを用意
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットをメインリストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}