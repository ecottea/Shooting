// enemyPat_Tmp.cpp
// 自由な発想で設計した弾幕："キ"文字を使った「ダイナミックテキストストーム」
// - 敵は正弦波で左右に移動
// - 60フレームごとに「キ」文字の弾幕を発射
// - 弾はランダムな方向・速度・色を持つ
// - 2種類の効果音を交互に使用

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// ============================================================
//  GDI+ 初期化
// ============================================================
static void InitGdiplusOnce()
{
    static bool initialized = false;
    if (!initialized) {
        GdiplusStartupInput gdiInput;
        ULONG_PTR token;
        GdiplusStartup(&token, &gdiInput, nullptr);
        initialized = true;
    }
}

// ============================================================
//  文字「キ」の輪郭を等間隔でサンプリング
// ============================================================
static std::vector<std::pair<double, double>> SampleTextOutlineKI()
{
    InitGdiplusOnce();

    FontFamily fontFamily(L"HGSoeiKakupoptai");
    GraphicsPath path;
    StringFormat fmt;
    path.AddString(L"キ", -1, &fontFamily, FontStyleRegular, 360.0f, PointF(0, 0), &fmt);
    path.Flatten(nullptr, 0.25f);

    int pointCount = path.GetPointCount();
    if (pointCount < 2) return {};

    std::vector<PointF> pts(pointCount);
    std::vector<BYTE> types(pointCount);
    path.GetPathPoints(pts.data(), pointCount);
    path.GetPathTypes(types.data(), pointCount);

    RectF bounds;
    path.GetBounds(&bounds);
    double cx = bounds.X + bounds.Width / 2.0;
    double cy = bounds.Y + bounds.Height / 2.0;

    std::vector<std::pair<double, double>> result;
    PointF subpathStart = pts[0];
    PointF prevPoint = pts[0];
    double leftover = 0.0;
    constexpr double interval = 20.0;

    auto sampleLineTo = [&](const PointF& to) {
        double dx = to.X - prevPoint.X;
        double dy = to.Y - prevPoint.Y;
        double segLen = std::sqrt(dx * dx + dy * dy);

        if (segLen > 0.0) {
            double dirX = dx / segLen;
            double dirY = dy / segLen;
            double currentPos = interval - leftover;

            while (currentPos <= segLen) {
                double x = prevPoint.X + dirX * currentPos;
                double y = prevPoint.Y + dirY * currentPos;
                result.emplace_back(x - cx, y - cy);
                currentPos += interval;
            }
            leftover = segLen - (currentPos - interval);
        }
        prevPoint = to;
    };

    for (int i = 0; i < pointCount; ++i) {
        BYTE type = types[i];
        PointF pt = pts[i];
        BYTE pathType = type & PathPointTypePathTypeMask;

        if (pathType == PathPointTypeStart) {
            subpathStart = pt;
            prevPoint = pt;
            result.emplace_back(pt.X - cx, pt.Y - cy);
            leftover = 0.0;
        }
        else {
            sampleLineTo(pt);
        }

        if ((type & PathPointTypeCloseSubpath) == PathPointTypeCloseSubpath) {
            sampleLineTo(subpathStart);
        }
    }

    return result;
}

// ============================================================
//  «キ»文字のオフセットリスト（初回アクセス時に自動生成）
// ============================================================
static std::vector<std::pair<double, double>>& GetOffsetsKI()
{
    static std::vector<std::pair<double, double>> offsets;
    static bool initialized = false;
    if (!initialized) {
        offsets = SampleTextOutlineKI();
        initialized = true;
    }
    return offsets;
}

// ============================================================
//  弾幕パターン：ランダムな方向・速度・色を持つ「キ」文字の弾幕
// ============================================================
static void ShotFormationDynamicKI(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 効果音を交互に使用
        int sound = (pEnemyShotSet->kind % 2 == 0) ? sound_enemyShot_medium : sound_enemyShot_light;
        if (CheckSoundMem(sound) == 1) StopSoundMem(sound);
        PlaySoundMem(sound, DX_PLAYTYPE_BACK);

        const auto& offsets = GetOffsetsKI();

        for (size_t i = 0; i < offsets.size(); ++i) {
            sEnemyShot* shot = new sEnemyShot;
            // ランダムな水平オフセットを追加（広がりを作る）
            shot->x = pEnemyShotSet->x + offsets[i].first + GetRand(4) - 2;
            shot->y = pEnemyShotSet->y + offsets[i].second;

            // 基本方向：プレイヤーへ向かう
            double baseAngle = pEnemyShotSet->muki;
            // ランダムな方向変化（-15°から+15°）
            double angleVariation = (GetRand(4) - 2) * DX_PI / 180.0;
            shot->muki = baseAngle + angleVariation;

            // 速度：2.0から3.5
            shot->speed = 2.0 + GetRand(4) / 10.0;

            // 色：6種類の小玉をサイクル
            shot->kind = img_enemyShotSmallBall[i % 6];
            shot->margin = 240.0;

            // リストに追加
            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    // 毎フレーム移動
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Moji_Vibe()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 正弦波で左右に移動（振幅80、周期120フレーム）
        enemy.x = 240.0 + 80.0 * sin(count / 60.0);
    }

    // 60フレームごとに発射（kindを使って効果音を交互に切り替え）
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFormationDynamicKI;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = (count / 60) % 2; // 効果音切り替え用

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}