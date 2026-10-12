#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ルーローの三角形弾幕
// 使用素材:
//   赤い小玉   img_enemyShotSmallBall[0]
//   青い中玉   img_enemyShotMediumBall[4]
//   黄色い小玉 img_enemyShotSmallBall[1]
// 効果音:
//   sound_enemyCharge, sound_enemyShot_heavy

namespace {
    constexpr int ARC_POINT_COUNT = 14*2;
    constexpr int ARC_COUNT = 3;
    constexpr int PATTERN_INTERVAL = 240-20;
    constexpr int BLUE_START_FRAME = 42;
    constexpr int RADIAL_INTERVAL = 30;
    constexpr int RADIAL_LAST_FRAME = 120;
    constexpr double TRIANGLE_RADIUS = 42.0*2;
    constexpr double ROTATION_SPEED = 0.025;
    constexpr double CONTOUR_SHOT_SPEED = 3.2;
    constexpr double RADIAL_SHOT_SPEED = 2.35;

    // param_i[0]: 0=赤い輪郭弾、1=青い輪郭弾、2=黄色い放射弾
    // 輪郭弾の param_i[1]=円弧番号、param_i[2]=円弧上の点番号、
    // param_i[3]=発射済みフラグ、param_i[4]=輪郭から飛び出す時刻
    // param_d[0]=初期回転角、param_d[1]=輪郭中心X、param_d[2]=輪郭中心Y

    void PlayPatternSound(int soundHandle)
    {
        if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
        PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet,
        double x, double y, double muki, double speed, int imageKind)
    {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = x;
        pEnemyShot->y = y;
        pEnemyShot->muki = muki;
        pEnemyShot->speed = speed;
        pEnemyShot->kind = imageKind;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        return pEnemyShot;
    }

    // 円弧番号ごとに、その円弧の中心頂点と開始角を求めて点を配置する。
    // 3本の60度円弧をつなぎ、各頂点の重複を避けてルーローの三角形を作る。
    void GetReuleauxPoint(double centerX, double centerY, double theta,
        int arc, int pointIndex, double* outX, double* outY)
    {
        double centerAngle;
        double startAngle;

        if (arc == 0) {
            // 上の頂点を中心とする、右下から左下へ向かう円弧
            centerAngle = theta - DX_PI / 2.0;
            startAngle = theta + DX_PI / 3.0;
        }
        else if (arc == 1) {
            // 右下の頂点を中心とする、左下から上へ向かう円弧
            centerAngle = theta + DX_PI / 6.0;
            startAngle = theta + DX_PI;
        }
        else {
            // 左下の頂点を中心とする、上から右下へ向かう円弧
            centerAngle = theta + 5.0 * DX_PI / 6.0;
            startAngle = theta + 5.0 * DX_PI / 3.0;
        }

        const double sideLength = TRIANGLE_RADIUS * sqrt(3.0);
        const double arcAngle = startAngle
            + (DX_PI / 3.0) * ((double)pointIndex / (double)ARC_POINT_COUNT);
        const double arcCenterX = centerX + TRIANGLE_RADIUS * cos(centerAngle);
        const double arcCenterY = centerY + TRIANGLE_RADIUS * sin(centerAngle);

        *outX = arcCenterX + sideLength * cos(arcAngle);
        *outY = arcCenterY + sideLength * sin(arcAngle);
    }

    void AddContourPoint(sEnemyShotSet* pEnemyShotSet,
        int colorType, int arc, int pointIndex, int launchFrame)
    {
        const double theta = pEnemyShotSet->param_d[0]
            + pEnemyShotSet->count * ROTATION_SPEED;
        double x, y;
        GetReuleauxPoint(pEnemyShotSet->x, pEnemyShotSet->y,
            theta, arc, pointIndex, &x, &y);

        const int imageKind = (colorType == 0)
            ? img_enemyShotSmallBall[0]
            : img_enemyShotMediumBall[4];
        sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet, x, y, 0.0, 0.0, imageKind);
        pEnemyShot->param_i[0] = colorType;
        pEnemyShot->param_i[1] = arc;
        pEnemyShot->param_i[2] = pointIndex;
        pEnemyShot->param_i[3] = 0;
        pEnemyShot->param_i[4] = launchFrame;
        pEnemyShot->param_d[0] = pEnemyShotSet->param_d[0];
        pEnemyShot->param_d[1] = pEnemyShotSet->x;
        pEnemyShot->param_d[2] = pEnemyShotSet->y;
    }

    void AddRadialWave(sEnemyShotSet* pEnemyShotSet)
    {
        const int waveIndex = pEnemyShotSet->count / RADIAL_INTERVAL;
        const int bulletCount = 12*2;
        const double rotation = pEnemyShotSet->param_d[0] + waveIndex * 0.13;

        for (int i = 0; i < bulletCount; ++i) {
            const double angle = rotation + 2.0 * DX_PI * i / bulletCount;
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                angle, RADIAL_SHOT_SPEED, img_enemyShotSmallBall[1]);
            pEnemyShot->param_i[0] = 2;
        }
    }

    static void ShotReuleauxTriangle(sEnemyShotSet* pEnemyShotSet)
    {
        // 赤い小弾が3つの円弧を描きながら輪郭を作る。
        if (pEnemyShotSet->count <= (ARC_POINT_COUNT - 1) * 3
            && pEnemyShotSet->count % 3 == 0) {
            const int pointIndex = pEnemyShotSet->count / 3;
            for (int arc = 0; arc < ARC_COUNT; ++arc) {
                const int launchFrame = 112 + (arc * ARC_POINT_COUNT + pointIndex) * 2;
                AddContourPoint(pEnemyShotSet, 0, arc, pointIndex, launchFrame);
            }
        }

        // 赤い輪郭に続き、青い中弾で同じ円弧をなぞって形を強調する。
        const int blueAge = pEnemyShotSet->count - BLUE_START_FRAME;
        if (blueAge >= 0
            && blueAge <= (ARC_POINT_COUNT - 1) * 3
            && blueAge % 3 == 0) {
            const int pointIndex = blueAge / 3;
            for (int arc = 0; arc < ARC_COUNT; ++arc) {
                const int launchFrame = 136 + (arc * ARC_POINT_COUNT + pointIndex) * 2;
                AddContourPoint(pEnemyShotSet, 1, arc, pointIndex, launchFrame);
            }
        }

        // 三角形の内側から黄色い小弾を放射状に発射する。
        if (pEnemyShotSet->count > 0
            && pEnemyShotSet->count <= RADIAL_LAST_FRAME
            && pEnemyShotSet->count % RADIAL_INTERVAL == 0) {
            AddRadialWave(pEnemyShotSet);
        }

        // 輪郭から弾が飛び出し始めるタイミングで発射音を鳴らす。
        if (pEnemyShotSet->count == 112) {
            PlayPatternSound(sound_enemyShot_heavy);
        }

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                // 発射前の輪郭弾は、回転する3本の円弧上を移動する。
                if (pShot->param_i[3] == 0
                    && pEnemyShotSet->count < pShot->param_i[4]) {
                    const double theta = pShot->param_d[0]
                        + pEnemyShotSet->count * ROTATION_SPEED;
                    double x, y;
                    GetReuleauxPoint(pShot->param_d[1], pShot->param_d[2],
                        theta, pShot->param_i[1], pShot->param_i[2], &x, &y);
                    pShot->x = x;
                    pShot->y = y;
                }
                else {
                    // 輪郭の弾を順番にプレイヤーへ向けて撃ち出す。
                    if (pShot->param_i[3] == 0) {
                        pShot->muki = atan2(player.y - pShot->y,
                            player.x - pShot->x);
                        pShot->speed = CONTOUR_SHOT_SPEED;
                        pShot->param_i[3] = 1;
                    }
                    pShot->x += pShot->speed * cos(pShot->muki);
                    pShot->y += pShot->speed * sin(pShot->muki);
                }
            }
            else {
                // 黄色い放射弾は発射方向へ直進する。
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }
}

// 敵本体の弾幕パターン
void EnemyPat_ReuleauxTriangle_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 168.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 横にゆっくり移動し、生成される三角形の位置にも変化をつける。
        enemy.x = 240.0 + 95.0 * sin(count * 0.012);
        enemy.y = 168.0 + 8.0 * sin(count * 0.019);
    }

    if ((count - 1) % PATTERN_INTERVAL == 0) {
        PlayPatternSound(sound_enemyCharge);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotReuleauxTriangle;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->kind = 0;
        // GetRand(359) は0～359の360通り。リプレイ時も同じ乱数列になる。
        pEnemyShotSet->param_d[0] = GetRand(359) * DX_PI / 180.0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
