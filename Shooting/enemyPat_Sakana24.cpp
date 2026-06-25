// enemyPat_Mobius.cpp
// メビウスの帯をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// メビウスの帯をモチーフにした弾幕パターン
static void ShotMobiusBand(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音: 中玉系のサウンドを1回だけ再生
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 帯を構成する弾の本数（周方向の分割数）
        const int NUM_BULLETS = 64;

        for (int i = 0; i < NUM_BULLETS; i++) {
            pEnemyShot = new sEnemyShot;

            // 帯の周方向パラメータ (0～2π)
            double theta = (2.0 * DX_PI * i) / NUM_BULLETS;

            // メビウスの帯のねじれ: 角度に応じて半径を変化させる
            // 0～π で半径が小さくなり、π～2π で大きくなるような形で「ねじれ」を表現
            double twistFactor = 1.0 + 0.3 * sin(theta);

            // 初期半径（時間とともに増加させるため、ここでは比較的小さめ）
            double baseRadius = 30.0;

            // 弾の初期位置（敵の位置を中心とした円周上）
            pEnemyShot->x = pEnemyShotSet->x + baseRadius * twistFactor * cos(theta);
            pEnemyShot->y = pEnemyShotSet->y + baseRadius * twistFactor * sin(theta);

            // 向き: 帯の接線方向（+90度回転）＋ねじれ成分
            // メビウスの帯の「表裏が連続する」性質を、角度の符号反転で表現
            double twistAngle = theta * 0.5; // ねじれの強さ
            pEnemyShot->muki = theta + DX_PI / 2.0 + twistAngle;

            // 速度: ゆっくりとした移動で帯の形状を保つ
            pEnemyShot->speed = 1.2;

            // 弾の種類: 鱗弾（帯のイメージに合う）
            // 弾の色: シアン中心に、表裏で少し色を変える
            int colorIndex = (i % 9 == 3) ? 3 : 4; // シアン or 青
            pEnemyShot->kind = img_enemyShotScale[colorIndex];

            // パラメータに「ねじれの基準角度」を保存しておく
            pEnemyShot->param_d[0] = theta; // 初期角度
            pEnemyShot->param_d[1] = twistAngle; // ねじれ角度

            pEnemyShot->margin = 480;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 時間とともに半径を増加させて「帯が広がっていく」ように見せる
        double t = pEnemyShot->count * 0.02 * 2; // 時間係数
        double radiusGrowth = 30.0 * t;      // 半径の増加量

        // 初期角度とねじれ角度を復元
        double theta0 = pEnemyShot->param_d[0];
        double twistAngle = pEnemyShot->param_d[1];

        // ねじれ係数（時間とともに少し変化させる）
        double twistFactor = 1.0 + 0.3 * sin(theta0 + t * 0.5) * 2;

        // 新しい位置を計算
        double baseRadius = 30.0 + radiusGrowth;
        pEnemyShot->x = pEnemyShotSet->x + baseRadius * twistFactor * cos(theta0);
        pEnemyShot->y = pEnemyShotSet->y + baseRadius * twistFactor * sin(theta0);

        // 向きも時間とともに変化させて「帯がねじれていく」ように
        pEnemyShot->muki = theta0 + DX_PI / 2.0 + twistAngle + t * 0.3;

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（ご指定の関数名）
void EnemyPat_Mobius_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 130 == 65) muki *= -1;
    }

    // 一定間隔でメビウスの帯弾幕を発射
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMobiusBand;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // メビウスの帯は自機狙いではなく形状重視のため0固定
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}