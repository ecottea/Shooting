#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 超越幾何学「無限回廊のペンタクロノス・TAS専用版」
//
// 五回対称の星形、逆回転らせん、多重交差弾列を融合した超高難易度弾幕。
// HP低下に伴って弾幕の密度と軌道の複雑さが増大する。
// 弾数カウント、弾セットのカウント、弾のカウントの加算と
// 画面外の弾の削除はメインルーチン側で行う。
// ============================================================

static const double PT_PI = DX_PI;
static const int PT_RING_COUNT = 6;
static const int PT_BULLETS_PER_RING = 48;

// 弾の初期位置と経過時間から軌道を計算する。
// 軌道は連続的に変化し、途中で位置が瞬間移動しない。
static void PT_GetPosition(
    sEnemyShot* pShot,
    double age,
    double& x,
    double& y)
{
    const double cx = pShot->param_d[0];
    const double cy = pShot->param_d[1];
    const double theta0 = pShot->param_d[2];
    const double radius0 = pShot->param_d[3];
    const double radialSpeed = pShot->param_d[4];
    const double omega = pShot->param_d[5];
    const double petalAmp = pShot->param_d[6];
    const double phase = pShot->param_d[7];
    const double fineAmp = pShot->param_d[8];
    const double twistAmp = pShot->param_d[9];
    const double twistFreq = pShot->param_d[10];
    const double twistPhase = pShot->param_d[11];

    const double theta =
        theta0 + omega * age
        + twistAmp * sin(twistFreq * age + twistPhase);

    const double petal0 = 5.0 * theta0 + phase;
    const double petal1 = 5.0 * theta + phase;

    const double fine0 = 10.0 * theta0 - phase;
    const double fine1 = 10.0 * theta - phase;

    const double radius =
        radius0
        + radialSpeed * age
        + petalAmp * (sin(petal1) - sin(petal0))
        + fineAmp * (sin(fine1) - sin(fine0));

    x = cx + radius * cos(theta);
    y = cy + radius * sin(theta);
}

// 軌道の接線方向と幾何学的な区画に基づいて弾の外観を設定する。
static void PT_SetBulletImage(sEnemyShot* pShot, int layer, double theta)
{
    double angle = fmod(theta, 2.0 * PT_PI);
    if (angle < 0.0) angle += 2.0 * PT_PI;

    const int sector = (int)(angle / (PT_PI / 5.0));
    const int pulse = (sector + pShot->count / 9 + layer) % 2;

    switch (layer % 6) {
    case 0:
        pShot->kind = pulse
            ? img_enemyShotSmallBall[5]
            : img_enemyShotSmallBall[0];
        break;

    case 1:
        pShot->kind = pulse
            ? img_enemyShotMediumBall[3]
            : img_enemyShotMediumBall[4];
        break;

    case 2:
        pShot->kind = pulse
            ? img_enemyShotDiamond[1]
            : img_enemyShotDiamond[5];
        break;

    case 3:
        pShot->kind = pulse
            ? img_enemyShotScale[4]
            : img_enemyShotScale[3];
        break;

    case 4:
        pShot->kind = pulse
            ? img_enemyShotMediumOval[6]
            : img_enemyShotMediumOval[5];
        break;

    default:
        pShot->kind = pulse
            ? img_enemyShotBullet[0]
            : img_enemyShotBullet[5];
        break;
    }
}

// 弾をリスト末尾に追加する。
// カウント類はメインルーチン側で管理する。
static sEnemyShot* PT_CreateBullet(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = new sEnemyShot;

    pShot->margin = 240;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;

    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;

    return pShot;
}

// ============================================================
// 多重幾何学弾幕
// ============================================================

static void ShotPentachronosTAS(sEnemyShotSet* pSet)
{
    const int phase = pSet->param_i[0];

    if (pSet->count == 0) {
        if (phase >= 3) {
            if (CheckSoundMem(sound_enemyShot_extreme))
                StopSoundMem(sound_enemyShot_extreme);

            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        else if (phase >= 2) {
            if (CheckSoundMem(sound_enemyShot_heavy))
                StopSoundMem(sound_enemyShot_heavy);

            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        else {
            if (CheckSoundMem(sound_enemyShot_medium))
                StopSoundMem(sound_enemyShot_medium);

            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 六重の星形らせんを同時に生成する。
        // 隣接する層は回転方向と位相が異なり、隙間が時間とともに移動する。
        for (int layer = 0; layer < PT_RING_COUNT; ++layer) {
            for (int i = 0; i < PT_BULLETS_PER_RING; ++i) {
                sEnemyShot* pShot = PT_CreateBullet(pSet);

                const double baseAngle =
                    pSet->muki
                    + 2.0 * PT_PI * i / PT_BULLETS_PER_RING
                    + layer * PT_PI / 30.0;

                const double direction =
                    (layer % 2 == 0) ? 1.0 : -1.0;

                const double omega =
                    direction * (0.009 + layer * 0.0021
                        + phase * 0.0018);

                pShot->param_d[0] = pSet->x;
                pShot->param_d[1] = pSet->y;
                pShot->param_d[2] = baseAngle;

                // 内側から外側へ、異なる速度で広がる。
                pShot->param_d[3] =
                    13.0 + layer * 8.0;

                pShot->param_d[4] =
                    1.65 + layer * 0.19 + phase * 0.17;

                pShot->param_d[5] = omega;

                // 五回対称と十回対称の変形を重ねる。
                pShot->param_d[6] =
                    7.0 + layer * 1.5 + phase * 1.4;

                pShot->param_d[7] =
                    layer * PT_PI / 5.0;

                pShot->param_d[8] =
                    2.5 + layer * 0.45 + phase * 0.5;

                // 角速度を時間変化させ、静的な安全地帯を作らない。
                pShot->param_d[9] =
                    0.12 + layer * 0.025 + phase * 0.02;

                pShot->param_d[10] =
                    0.012 + layer * 0.0017 + phase * 0.0015;

                pShot->param_d[11] =
                    layer * PT_PI / 7.0 + phase * PT_PI / 9.0;

                pShot->param_i[0] = layer;

                double x, y;
                PT_GetPosition(pShot, 0.0, x, y);

                pShot->x = x;
                pShot->y = y;
                pShot->muki = baseAngle + PT_PI / 2.0;
                pShot->speed = pShot->param_d[4];

                PT_SetBulletImage(pShot, layer, baseAngle);
            }
        }

        // 高難易度段階では、星形の頂点を結ぶ交差弾列を追加する。
        // これらはプレイヤーの発射時点の位置を狙い、幾何学模様の
        // 内側に細い通路を作ることでフレーム単位の回避を要求する。
        if (phase >= 1) {
            const double aimAngle =
                atan2(player.y - pSet->y, player.x - pSet->x);

            const int needleCount = (phase >= 3) ? 36 : 24;

            for (int i = 0; i < needleCount; ++i) {
                sEnemyShot* pShot = PT_CreateBullet(pSet);

                const double angle =
                    aimAngle
                    + 2.0 * PT_PI * i / needleCount
                    + (i % 2 ? 1.0 : -1.0)
                    * (0.035 + phase * 0.012);

                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = angle;
                pShot->speed = 2.5 + phase * 0.65;

                pShot->param_d[0] = pSet->x;
                pShot->param_d[1] = pSet->y;
                pShot->param_d[2] = angle;
                pShot->param_d[3] = 0.0;
                pShot->param_d[4] = pShot->speed;
                pShot->param_d[5] = 0.0;
                pShot->param_d[6] = 0.0;
                pShot->param_d[7] = 0.0;
                pShot->param_d[8] = 0.0;
                pShot->param_d[9] = 0.0;
                pShot->param_d[10] = 0.0;
                pShot->param_d[11] = 0.0;

                pShot->param_i[0] = 6;

                pShot->kind =
                    (i % 2 == 0)
                    ? img_enemyShotSmallBall[0]
                    : img_enemyShotSmallBall[5];
            }
        }

        // 最終段階では、さらに細かい交差線を形成する。
        if (phase >= 3) {
            const double aimAngle =
                atan2(player.y - pSet->y, player.x - pSet->x);

            for (int i = 0; i < 20; ++i) {
                sEnemyShot* pShot = PT_CreateBullet(pSet);

                const double angle =
                    aimAngle + 2.0 * PT_PI * i / 20.0
                    + PT_PI / 20.0;

                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = angle;
                pShot->speed = 3.1;

                pShot->param_d[0] = pSet->x;
                pShot->param_d[1] = pSet->y;
                pShot->param_d[2] = angle;
                pShot->param_d[3] = 0.0;
                pShot->param_d[4] = pShot->speed;
                pShot->param_d[5] = 0.0;
                pShot->param_d[6] = 0.0;
                pShot->param_d[7] = 0.0;
                pShot->param_d[8] = 0.0;
                pShot->param_d[9] = 0.0;
                pShot->param_d[10] = 0.0;
                pShot->param_d[11] = 0.0;

                pShot->param_i[0] = 7;
                pShot->kind = img_enemyShotDiamond[5];
            }
        }
    }

    // 生成済みの弾を更新する。
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;

    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] >= 6) {
            // 直線状の交差弾列。
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 幾何学的ならせん弾。
            const double age = (double)pShot->count;

            double x, y;
            double nextX, nextY;

            PT_GetPosition(pShot, age, x, y);
            PT_GetPosition(pShot, age + 1.0, nextX, nextY);

            pShot->x = x;
            pShot->y = y;

            pShot->muki =
                atan2(nextY - y, nextX - x);

            pShot->speed =
                sqrt((nextX - x) * (nextX - x)
                    + (nextY - y) * (nextY - y));

            PT_SetBulletImage(
                pShot,
                pShot->param_i[0],
                atan2(
                    y - pShot->param_d[1],
                    x - pShot->param_d[0]));
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================

void EnemyPat_GeometryTAS_ChatGPT()
{
    static int shotSerial;
    static int lastPhase;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;

        enemy.maxHp = enemy.hp = 200;

        shotSerial = 0;
        lastPhase = 0;
    }
    else {
        // ボスの移動も複数の周期を重ね、単純な往復運動を避ける。
        enemy.x =
            240.0
            + 76.0 * sin(count * 0.0080)
            + 21.0 * sin(count * 0.0210);

        enemy.y =
            57.0
            + 13.0 * sin(count * 0.0130)
            + 7.0 * sin(count * 0.0310);
    }

    int phase;

    if (enemy.hp > 150) {
        phase = 0;
    }
    else if (enemy.hp > 100) {
        phase = 1;
    }
    else if (enemy.hp > 50) {
        phase = 2;
    }
    else {
        phase = 3;
    }

    if (phase != lastPhase) {
        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);

        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        lastPhase = phase;
    }

    // HPが減るほど発射間隔を短縮する。
    int interval;

    switch (phase) {
    case 0:
        interval = 38;
        break;

    case 1:
        interval = 32;
        break;

    case 2:
        interval = 27;
        break;

    default:
        interval = 23;
        break;
    }

    if (count % interval == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotPentachronosTAS;

        pSet->x = enemy.x;
        pSet->y = enemy.y + 8.0;

        // 再生可能な乱数を利用し、波ごとに回転位相を変える。
        pSet->muki =
            GetRand(3599) / 3600.0 * 2.0 * PT_PI;

        pSet->kind = shotSerial++;
        pSet->param_i[0] = phase;

        pSet->pEnemyShotHead = new sEnemyShot;

        pSet->pEnemyShotHead->prev =
            pSet->pEnemyShotHead;

        pSet->pEnemyShotHead->next =
            pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}