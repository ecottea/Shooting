#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 角度を 0～2π に正規化
static double NormalizeAngle(double angle)
{
    angle = fmod(angle, 2.0 * DX_PI);
    if (angle < 0.0) angle += 2.0 * DX_PI;
    return angle;
}

// angle が [start, start + width] の範囲に入っているか
// 2π をまたぐ場合も考慮
static bool InAngleRange(double angle, double start, double width)
{
    if (width <= 0.0) return false;
    if (width >= 2.0 * DX_PI) return true;

    double a = NormalizeAngle(angle);
    double s = NormalizeAngle(start);
    double e = s + width;

    if (e <= 2.0 * DX_PI) {
        return (a >= s && a <= e);
    }
    else {
        e -= 2.0 * DX_PI;
        return (a >= s || a <= e);
    }
}

// 弾幕：サブタレイニアンローズ
static void ShotSubterraneanRose(sEnemyShotSet* pEnemyShotSet)
{
    const int N = 60; // 全方位弾の方向数

    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int type = pEnemyShotSet->param_i[0]; // 0:青, 1:赤
        double V = pEnemyShotSet->param_d[2];
        double angleStep = 2.0 * DX_PI / N;

        for (int i = 0; i < N; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * angleStep;
            pEnemyShot->speed = V;

            // 最初は中玉。この後の更新で毎フレーム大玉/中玉を切り替える
            pEnemyShot->kind = (type == 0)
                ? img_enemyShotMediumBall[4]  // 青中玉
                : img_enemyShotMediumBall[0]; // 赤中玉

            pEnemyShot->count = 0;

            // 弾リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    int type = pEnemyShotSet->param_i[0];
    double A = pEnemyShotSet->param_d[0];
    double B = pEnemyShotSet->param_d[1];

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 基準角から見た偏角
        double offset = NormalizeAngle(pShot->muki - pEnemyShotSet->muki);

        // 青： count*A ～ count*A+B
        // 赤： -count*A ～ -count*A+B
        double start = (type == 0)
            ? (pEnemyShotSet->count * A)
            : (-pEnemyShotSet->count * A);

        if (InAngleRange(offset, start, B)) {
            pShot->kind = (type == 0)
                ? img_enemyShotLargeBall[4]  // 青大玉
                : img_enemyShotLargeBall[0]; // 赤大玉
        }
        else {
            pShot->kind = (type == 0)
                ? img_enemyShotMediumBall[4]  // 青中玉
                : img_enemyShotMediumBall[0]; // 赤中玉
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SubterraneanRose_DeepSeek()
{
    static int muki;
    static int shotType;   // 0:青, 1:赤
    static int shotTimer;
    static int pauseTimer;
    static int nextHp;
    static int stage;
    static int T;
    static double V, A, B;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 400から変更しないこと

        muki = 1;
        shotType = 0;
        shotTimer = 0;
        pauseTimer = 0;
        nextHp = 150;
        stage = 0;

        // 初期パラメータ
        T = 45;
        V = 2.2;
        A = 2.0 * DX_PI / 180.0;
        B = 30.0 * DX_PI / 180.0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // HPが1/4削れる度に、少し停止して T,V,A,B を変化
    if (pauseTimer == 0 && nextHp > 0 && enemy.hp <= nextHp) {
        pauseTimer = 60;
        nextHp -= 50;
        if (stage < 3) stage++;

        switch (stage) {
        case 1:
            T = 40;
            V = 2.5;
            A = 2.5 * DX_PI / 180.0;
            B = 40.0 * DX_PI / 180.0;
            break;
        case 2:
            T = 35;
            V = 2.8;
            A = 3.0 * DX_PI / 180.0;
            B = 50.0 * DX_PI / 180.0;
            break;
        case 3:
            T = 30;
            V = 3.1;
            A = 3.5 * DX_PI / 180.0;
            B = 60.0 * DX_PI / 180.0;
            break;
        default:
            break;
        }

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        shotTimer = 0;
    }

    // 発射管理
    if (pauseTimer > 0) {
        pauseTimer--;
        if (pauseTimer == 0) {
            shotTimer = 0; // 再開
        }
    }
    else {
        if (shotTimer > 0) shotTimer--;

        if (shotTimer == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSubterraneanRose;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;

            // 全方位弾の基準角は毎回ランダム
            pEnemyShotSet->muki = GetRand(359) * DX_PI / 180.0;

            pEnemyShotSet->kind = shotType;
            pEnemyShotSet->param_i[0] = shotType; // 0:青, 1:赤
            pEnemyShotSet->param_d[0] = A;
            pEnemyShotSet->param_d[1] = B;
            pEnemyShotSet->param_d[2] = V;

            // 弾リストのダミーヘッド
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            // 敵弾セットリストへ追加
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            // 青と赤を交互に
            shotType = 1 - shotType;
            shotTimer = T;
        }
    }
}