#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <vector>
#include <cmath>
#include <algorithm>

// ============================================================
// へのへのもへじ弾幕
// 既存の小玉・中玉だけで顔を描き、しばらく見せた後に放射状に飛ばす。
// count / sEnemyShotSet::count / sEnemyShot::count の加算と、
// 画面外の弾の削除はメインルーチン側に任せる。
// ============================================================

namespace {
    struct HenohenomohejiPoint {
        double x;
        double y;
        int imageKind;
        int order;
    };

    static void HenohenomohejiAddLine(
        std::vector<HenohenomohejiPoint>& points,
        double x1, double y1, double x2, double y2,
        int imageKind)
    {
        const double dx = x2 - x1;
        const double dy = y2 - y1;
        const double length = std::sqrt(dx * dx + dy * dy);
        const int steps = (std::max)(1, (int)std::ceil(length / 3.6));

        for (int i = 0; i <= steps; ++i) {
            const double t = (double)i / steps;
            HenohenomohejiPoint point{};
            point.x = x1 + dx * t;
            point.y = y1 + dy * t;
            point.imageKind = imageKind;
            point.order = (int)points.size();
            points.push_back(point);
        }
    }

    static void HenohenomohejiAddEllipse(
        std::vector<HenohenomohejiPoint>& points,
        double cx, double cy, double rx, double ry,
        int imageKind)
    {
        const int steps = 64;
        for (int i = 0; i < steps; ++i) {
            const double a = 2.0 * DX_PI * i / steps;
            HenohenomohejiPoint point{};
            point.x = cx + rx * std::cos(a);
            point.y = cy + ry * std::sin(a);
            point.imageKind = imageKind;
            point.order = (int)points.size();
            points.push_back(point);
        }
    }

    static std::vector<HenohenomohejiPoint> HenohenomohejiMakeFace(int variation)
    {
        std::vector<HenohenomohejiPoint> points;
        points.reserve(180);

        // 顔の輪郭。交互にシアンと青を使い、顔の外形を目立たせる。
        const int outlineKind = variation ? img_enemyShotSmallBall[4] : img_enemyShotSmallBall[3];
        HenohenomohejiAddEllipse(points, 0.0, 0.0, 76.0, 91.0, outlineKind);

        // 眉：への字を左右に並べる。
        const int browKind = img_enemyShotSmallBall[5];
        HenohenomohejiAddLine(points, -53.0, -39.0, -35.0, -49.0, browKind);
        HenohenomohejiAddLine(points, -35.0, -49.0, -16.0, -39.0, browKind);
        HenohenomohejiAddLine(points, 16.0, -39.0, 35.0, -49.0, browKind);
        HenohenomohejiAddLine(points, 35.0, -49.0, 53.0, -39.0, browKind);

        // 目：小玉で目の位置を示し、別色の小玉で瞳を作る。
        HenohenomohejiAddEllipse(points, -34.0, -21.0, 10.0, 6.0, img_enemyShotSmallBall[6]);
        HenohenomohejiAddEllipse(points, 34.0, -21.0, 10.0, 6.0, img_enemyShotSmallBall[6]);
        HenohenomohejiAddEllipse(points, -34.0, -21.0, 3.0, 4.0, img_enemyShotSmallBall[1]);
        HenohenomohejiAddEllipse(points, 34.0, -21.0, 3.0, 4.0, img_enemyShotSmallBall[1]);

        // 鼻：中玉で縦線と先端を描く。
        HenohenomohejiAddLine(points, 0.0, -13.0, 0.0, 13.0, img_enemyShotMediumBall[1]);
        HenohenomohejiAddLine(points, 0.0, 13.0, 10.0, 17.0, img_enemyShotMediumBall[1]);

        // 口：への字の形を中玉で大きく描く。
        HenohenomohejiAddLine(points, -38.0, 37.0, 0.0, 49.0, img_enemyShotMediumBall[0]);
        HenohenomohejiAddLine(points, 0.0, 49.0, 38.0, 37.0, img_enemyShotMediumBall[0]);

        // 顔の下に「じ」を思わせる短い顎のアクセント。
        HenohenomohejiAddLine(points, -18.0, 67.0, 18.0, 67.0, img_enemyShotSmallBall[5]);
        HenohenomohejiAddLine(points, 18.0, 67.0, 27.0, 61.0, img_enemyShotSmallBall[5]);

        // 左右反転パターンも用意する。
        if (variation) {
            for (auto& point : points) point.x = -point.x;
        }
        return points;
    }

    static void HenohenomohejiPlaySound(int sound)
    {
        if (CheckSoundMem(sound)) StopSoundMem(sound);
        PlaySoundMem(sound, DX_PLAYTYPE_BACK);
    }

    static void HenohenomohejiShot(sEnemyShotSet* pEnemyShotSet)
    {
        // 顔の弾を一度だけ生成する。
        if (pEnemyShotSet->count == 0) {
            HenohenomohejiPlaySound(sound_enemyShot_medium);
            const int variation = pEnemyShotSet->kind & 1;
            const auto points = HenohenomohejiMakeFace(variation);
            const double centerX = pEnemyShotSet->x;
            const double centerY = pEnemyShotSet->y + 178.0;

            for (const auto& point : points) {
                sEnemyShot* shot = new sEnemyShot;
                shot->x = centerX + point.x;
                shot->y = centerY + point.y;
                shot->muki = 0.0;
                shot->speed = 0.0;
                shot->kind = point.imageKind;
                shot->param_i[0] = 0;                 // まだ顔として静止中
                shot->param_i[1] = point.order;       // 発射角度に小さなばらつきを付ける
                shot->param_d[0] = centerX;           // 顔の中心X
                shot->param_d[1] = centerY;           // 顔の中心Y
                shot->param_d[2] = (point.imageKind == img_enemyShotMediumBall[0] ||
                    point.imageKind == img_enemyShotMediumBall[1]) ? 2.8 : 2.2;

                shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                shot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = shot;
                pEnemyShotSet->pEnemyShotHead->prev = shot;
            }
        }

        // 顔をしばらく見せた後、各パーツが外向きに飛び散る。
        if (pEnemyShotSet->count == 90) {
            HenohenomohejiPlaySound(sound_enemyShot_extreme);
        }

        sEnemyShot* shot = pEnemyShotSet->pEnemyShotHead->next;
        while (shot != pEnemyShotSet->pEnemyShotHead) {
            if (shot->count >= 90 && shot->param_i[0] == 0) {
                const double dx = shot->x - shot->param_d[0];
                const double dy = shot->y - shot->param_d[1];
                const double baseAngle = std::atan2(dy, dx);
                const int order = shot->param_i[1];
                const double angleOffset = ((order % 9) - 4) * 0.025;

                shot->muki = baseAngle + angleOffset;
                shot->speed = shot->param_d[2] + (order % 5) * 0.12;
                shot->param_i[0] = 1;
            }

            if (shot->param_i[0] != 0) {
                shot->x += shot->speed * std::cos(shot->muki);
                shot->y += shot->speed * std::sin(shot->muki);
            }
            shot = shot->next;
        }
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Henohenomoheji_ChatGPT()
{
    static int direction = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 42.0;
        enemy.maxHp = enemy.hp = 200;
        direction = 1;
    }
    else {
        enemy.x += 1.15 * direction;
        if (enemy.x >= 360.0) {
            enemy.x = 360.0;
            direction = -1;
        }
        else if (enemy.x <= 120.0) {
            enemy.x = 120.0;
            direction = 1;
        }
    }

    // 顔を描いて飛び散らせる。左右反転を交互に見せる。
    if (count == 1 || (count - 1) % 100 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = HenohenomohejiShot;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = std::atan2(player.y - enemy.y, player.x - enemy.x);
        pEnemyShotSet->kind = ((count - 1) / 100) % 2;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
