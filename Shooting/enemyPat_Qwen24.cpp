// enemyPat_mobius.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// メビウスの帯をモチーフにした弾幕
static void ShotMobius(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 帯の形状パラメータ
    const double R0 = 15.0;       // 初期半径
    const double Vr = 1.5;        // 半径の拡大速度
    const double Vtheta = 0.12;   // 回転速度（ねじれの進み具合）
    const double W = 25.0;        // 帯の幅
    const double z_scale = 0.6;   // 奥行き(z)のY座標への投影スケール

    // 一定間隔で新しい弾を生成し、帯を延伸させる
    if (pEnemyShotSet->count % 3 == 0 && pEnemyShotSet->count <= 120) {
        // 効果音: 軽やかで神秘的なイメージの light を選択
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 帯の幅方向に7発の弾を生成して厚みを出す
        for (int i = 0; i < 7; ++i) {
            double v = (i - 3) / 3.0; // -1.0 から 1.0 までの幅方向の位置

            pEnemyShot = new sEnemyShot;
            pEnemyShot->count = 0;

            // 弾の種類: 鱗弾(img_enemyShotScale)は帯状に並べると連続した曲面のように見えるため選択
            // 色: 0〜8 のグラデーションを使い、帯に虹色の彩りを与える
            int color_idx = (int)((v + 1.0) * 4.0);
            if (color_idx < 0) color_idx = 0;
            if (color_idx > 8) color_idx = 8;
            pEnemyShot->kind = img_enemyShotScale[color_idx];

            // 自由パラメータに帯の幅方向位置(v)と初期位相を保存
            pEnemyShot->param_d[0] = v;
            pEnemyShot->param_d[1] = pEnemyShotSet->kind * 1.5; // 帯ごとにねじれの開始角度をずらす

            // 生成時の初期座標を計算
            double theta = pEnemyShot->param_d[1];
            double r = R0;
            double offset = v * W * cos(theta / 2.0);
            double z = v * W * sin(theta / 2.0);
            double cx = r * cos(theta);
            double cy = r * sin(theta);
            double x = cx + offset * cos(theta);
            double y_proj = cy + offset * sin(theta) + z * z_scale;

            pEnemyShot->x = pEnemyShotSet->x + x * cos(pEnemyShotSet->muki) - y_proj * sin(pEnemyShotSet->muki);
            pEnemyShot->y = pEnemyShotSet->y + x * sin(pEnemyShotSet->muki) + y_proj * cos(pEnemyShotSet->muki);

            pEnemyShot->margin = 240;

            // 弾リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存の弾の座標を更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double v = pEnemyShot->param_d[0];
        double theta0 = pEnemyShot->param_d[1];

        // 時間経過による角度と半径の変化
        double theta = theta0 + pEnemyShot->count * Vtheta;
        double r = R0 + pEnemyShot->count * Vr;

        // メビウスの帯のパラメトリック方程式に基づく座標計算
        // cos(theta/2) により、1周(2π)で帯の表裏が入れ替わる
        double offset = v * W * cos(theta / 2.0);
        double z = v * W * sin(theta / 2.0);

        double cx = r * cos(theta);
        double cy = r * sin(theta);
        double x = cx + offset * cos(theta);
        double y_proj = cy + offset * sin(theta) + z * z_scale;

        // 発射方向(muki)に合わせて回転・平行移動
        pEnemyShot->x = pEnemyShotSet->x + x * cos(pEnemyShotSet->muki) - y_proj * sin(pEnemyShotSet->muki);
        pEnemyShot->y = pEnemyShotSet->y + x * sin(pEnemyShotSet->muki) + y_proj * cos(pEnemyShotSet->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Mobius_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 1.2 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // メビウスの帯弾幕を生成
    if (count % 90 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMobius;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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