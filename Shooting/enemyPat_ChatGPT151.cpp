#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const int ROSE_WAY = 60;

static const double ROSE_T[4] = {
    16.0,
    13.0,
    10.0,
    8.0
};

static const double ROSE_V[4] = {
    2.2,
    2.4,
    2.7,
    3.0
};

static const double ROSE_A[4] = {
    0.015,
    0.021,
    0.028,
    0.036
};

static const double ROSE_B[4] = {
    0.70,
    0.58,
    0.46,
    0.34
};

static const int ROSE_PAUSE = 30;

static double NormalizeAngleRose(double angle)
{
    const double twoPi = DX_PI * 2.0;

    while (angle < 0.0) {
        angle += twoPi;
    }
    while (angle >= twoPi) {
        angle -= twoPi;
    }

    return angle;
}

static bool IsAngleInRangeRose(double angle, double start, double width)
{
    const double twoPi = DX_PI * 2.0;

    angle = NormalizeAngleRose(angle);
    start = NormalizeAngleRose(start);

    double diff = angle - start;
    if (diff < 0.0) {
        diff += twoPi;
    }

    return diff <= width;
}

// サブタレイニアンローズ本体
static void ShotSubterraneanRose(sEnemyShotSet* pEnemyShotSet)
{
    const int phase = pEnemyShotSet->param_i[0];
    const int color = pEnemyShotSet->kind;
    const double A = ROSE_A[phase];
    const double B = ROSE_B[phase];

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        const double bulletAngle = NormalizeAngleRose(pShot->param_d[0]);
        const double offset = count * A;

        bool isLarge = false;

        if (color == 0) {
            // 青：
            // 偏角が count*A ～ count*A+B の範囲
            const double startAngle = offset;
            isLarge = IsAngleInRangeRose(
                bulletAngle,
                startAngle,
                B
            );

            if (phase <= 1) {
                if (isLarge) {
                    pShot->kind = img_enemyShotLargeBall[4];
                }
                else {
                    pShot->kind = img_enemyShotMediumBall[4];
                }
            }
            else {
                if (isLarge) {
                    pShot->kind = img_enemyShotMediumBall[4];
                }
                else {
                    pShot->kind = img_enemyShotSmallBall[4];
                }
            }
        }
        else {
            // 赤：
            // 偏角が -(count*A+B) ～ -count*A の範囲
            const double startAngle = -offset - B;
            isLarge = IsAngleInRangeRose(
                bulletAngle,
                startAngle,
                B
            );

            if (phase <= 1) {
                if (isLarge) {
                    pShot->kind = img_enemyShotLargeBall[0];
                }
                else {
                    pShot->kind = img_enemyShotMediumBall[0];
                }
            }
            else {
                if (isLarge) {
                    pShot->kind = img_enemyShotMediumBall[0];
                }
                else {
                    pShot->kind = img_enemyShotSmallBall[0];
                }
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SubterraneanRose_ChatGPT()
{
    static int phase;
    static int pauseCount;
    static int shotParity;
    static int shotInterval;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;

        phase = 0;
        pauseCount = 0;
        shotParity = 0;
        shotInterval = (int)ROSE_T[0];
    }

    // HPに応じた現在フェーズを決定
    int newPhase = 0;

    if (enemy.hp <= 150) {
        newPhase = 1;
    }
    if (enemy.hp <= 100) {
        newPhase = 2;
    }
    if (enemy.hp <= 50) {
        newPhase = 3;
    }

    // フェーズ移行時は少しの間、発射を停止
    if (newPhase != phase) {
        phase = newPhase;
        pauseCount = ROSE_PAUSE;
        shotInterval = (int)ROSE_T[phase];

        if (CheckSoundMem(sound_enemyCharge)) {
            StopSoundMem(sound_enemyCharge);
        }
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 既存の弾は通常通り動かす
    // 新しい弾の発射だけ一時停止
    if (pauseCount > 0) {
        pauseCount--;
    }
    else {
        if (count % shotInterval == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSubterraneanRose;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = 0.0;

            // 0:青、1:赤
            pEnemyShotSet->kind = shotParity;

            // 現在のフェーズを保持
            pEnemyShotSet->param_i[0] = phase;

            // 今回の全方位弾の基準角を毎回ランダムに決定
            // GetRand(3599) は 0～3599 の 3600 通り
            const double baseAngle =
                GetRand(3599) / 3600.0 * DX_PI * 2.0;

            if (CheckSoundMem(sound_enemyShot_medium)) {
                StopSoundMem(sound_enemyShot_medium);
            }
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev =
                pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next =
                pEnemyShotSet->pEnemyShotHead;

            for (int i = 0; i < ROSE_WAY; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                const double angle =
                    baseAngle
                    + DX_PI * 2.0 * i / (double)ROSE_WAY;

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = ROSE_V[phase];

                // 毎フレームの大玉／中玉判定に使う初期偏角
                pEnemyShot->param_d[0] =
                    NormalizeAngleRose(angle);

                if (shotParity == 0) {
                    pEnemyShot->kind = img_enemyShotMediumBall[4];
                }
                else {
                    pEnemyShot->kind = img_enemyShotMediumBall[0];
                }

                pEnemyShot->prev =
                    pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next =
                    pEnemyShotSet->pEnemyShotHead;

                pEnemyShotSet->pEnemyShotHead->prev->next =
                    pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev =
                    pEnemyShot;
            }

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            shotParity ^= 1;
        }
    }
}