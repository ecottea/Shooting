// enemyPat_hyperbola.cpp
// 双曲線をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 双曲線パラメータ
static constexpr double HYPERBOLA_A = 40.0; // 双曲線の実軸の半分
static constexpr double HYPERBOLA_B = 30.0; // 双曲線の虚軸の半分
static constexpr int HYPERBOLA_SHOT_NUM = 32; // 1 回の発射で出す弾の数
static constexpr double HYPERBOLA_SPEED = 2.0; // 弾の基本速度

// 双曲線モチーフの弾幕
static void ShotHyperbola(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音: 軽めの弾幕用音
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 双曲線 x^2/a^2 - y^2/b^2 = 1 上の点をパラメータ t で生成
        for (int i = -HYPERBOLA_SHOT_NUM; i <= HYPERBOLA_SHOT_NUM; i++) {
            double t = (double)i / HYPERBOLA_SHOT_NUM * 2.0 * DX_PI; // 0～2π

            // 双曲線パラメータ表示: x = a * cosh(t), y = b * sinh(t)
            // ここでは cosh, sinh の代わりに、近似として cosh ≈ cos, sinh ≈ sin の雰囲気で
            // 実際には双曲線の「広がる」感じを出すために、cosh/sinh 的な広がりを模した式にしてもよい
            double x_offset = HYPERBOLA_A * cosh(t);
            double y_offset = HYPERBOLA_B * sinh(t);

            // 敵の向き（プレイヤー方向）を基準に、双曲線を回転させる
            double baseAngle = pEnemyShotSet->muki; // プレイヤー方向
            double angle = baseAngle + atan2(y_offset, x_offset);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + x_offset;
            pEnemyShot->y = pEnemyShotSet->y + y_offset;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = HYPERBOLA_SPEED;

            // 弾の種類と色を絞る
            // 種類: 0=小玉, 5=菱形弾 のどちらか
            // 色  : 0=赤, 4=青 のどちらか
            pEnemyShot->kind = img_enemyShotDiamond[4];

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    if (pEnemyShotSet->count == 0) {
        // 効果音: 軽めの弾幕用音
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 双曲線 x^2/a^2 - y^2/b^2 = 1 上の点をパラメータ t で生成
        for (int i = -HYPERBOLA_SHOT_NUM; i <= HYPERBOLA_SHOT_NUM; i++) {
            double t = (double)i / HYPERBOLA_SHOT_NUM * 2.0 * DX_PI; // 0～2π

            // 双曲線パラメータ表示: x = a * cosh(t), y = b * sinh(t)
            // ここでは cosh, sinh の代わりに、近似として cosh ≈ cos, sinh ≈ sin の雰囲気で
            // 実際には双曲線の「広がる」感じを出すために、cosh/sinh 的な広がりを模した式にしてもよい
            double x_offset = -HYPERBOLA_A * cosh(t);
            double y_offset = -HYPERBOLA_B * sinh(t);

            // 敵の向き（プレイヤー方向）を基準に、双曲線を回転させる
            double baseAngle = pEnemyShotSet->muki; // プレイヤー方向
            double angle = baseAngle + atan2(y_offset, x_offset);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + x_offset;
            pEnemyShot->y = pEnemyShotSet->y + y_offset;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = HYPERBOLA_SPEED;

            // 弾の種類と色を絞る
            // 種類: 0=小玉, 5=菱形弾 のどちらか
            // 色  : 0=赤, 4=青 のどちらか
            pEnemyShot->kind = img_enemyShotSmallBall[0];

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存の弾を移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Hyperbola_Sakana()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で双曲線弾幕を発射
    if (count % 20 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHyperbola;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}