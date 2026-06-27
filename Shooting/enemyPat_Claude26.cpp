// enemyPat_Tmp.cpp
// 文字弾幕「神速乱舞」
// 4文字それぞれを螺旋軌道（muki を毎フレーム加算）で飛ばす。
// 文字ごとに色と旋回方向が交互に変わる。

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
//  GDI+ 初期化（初回のみ）
// ============================================================
static void InitGdiplusOnce_Tmp()
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
//  文字の輪郭を等間隔でサンプリング
//  重心が (0,0) になる相対座標リストを返す
// ============================================================
static std::vector<std::pair<double, double>> SampleTextOutline_Tmp(
    const wchar_t* text,
    const wchar_t* fontName,
    REAL           fontSize,
    double         interval)
{
    InitGdiplusOnce_Tmp();

    FontFamily   fontFamily(fontName);
    GraphicsPath path;
    StringFormat fmt;
    path.AddString(text, -1, &fontFamily, FontStyleRegular, fontSize, PointF(0, 0), &fmt);
    path.Flatten(nullptr, 0.25f);

    int pointCount = path.GetPointCount();
    if (pointCount < 2) return {};

    std::vector<PointF> pts(pointCount);
    std::vector<BYTE>   types(pointCount);
    path.GetPathPoints(pts.data(), pointCount);
    path.GetPathTypes(types.data(), pointCount);

    RectF  bounds;
    path.GetBounds(&bounds);
    double cx = bounds.X + bounds.Width / 2.0;
    double cy = bounds.Y + bounds.Height / 2.0;

    std::vector<std::pair<double, double>> result;
    PointF subpathStart = pts[0];
    PointF prevPoint = pts[0];
    double leftover = 0.0;

    auto sampleLineTo = [&](const PointF& to) {
        double dx = to.X - prevPoint.X;
        double dy = to.Y - prevPoint.Y;
        double segLen = std::sqrt(dx * dx + dy * dy);
        if (segLen > 0.0) {
            double dirX = dx / segLen;
            double dirY = dy / segLen;
            double currentPos = interval - leftover;
            while (currentPos <= segLen) {
                result.emplace_back(prevPoint.X + dirX * currentPos - cx,
                    prevPoint.Y + dirY * currentPos - cy);
                currentPos += interval;
            }
            leftover = segLen - (currentPos - interval);
        }
        prevPoint = to;
    };

    for (int i = 0; i < pointCount; ++i) {
        BYTE   type = types[i];
        PointF pt = pts[i];
        BYTE   pathType = type & PathPointTypePathTypeMask;

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
//  「神速乱舞」4文字のオフセットリスト（初回アクセス時に生成）
// ============================================================
static std::vector<std::pair<double, double>>& GetOffsets_Tmp(int id)
{
    static std::vector<std::pair<double, double>> offsets[4];
    static bool initialized = false;
    if (!initialized) {
        const wchar_t* chars[] = { L"神", L"速", L"乱", L"舞" };
        const wchar_t* fontName = L"HGSoeiKakupoptai";
        constexpr REAL   fontSize = 380.0f;
        constexpr double interval = 18.0;

        for (int i = 0; i < 4; ++i) {
            offsets[i] = SampleTextOutline_Tmp(chars[i], fontName, fontSize, interval);
        }
        initialized = true;
    }
    return offsets[id % 4];
}

// ============================================================
//  弾幕パターン：文字隊形 + 螺旋軌道
//
//  param_d[0] : 各弾の muki 加算量（角速度）
//               正 → 右回転(時計回り), 負 → 左回転(反時計回り)
//
//  文字ごとの設定
//    kind%4 == 0（神）: 赤   ,  omega = +0.030 rad/frame
//    kind%4 == 1（速）: 橙   ,  omega = -0.030 rad/frame
//    kind%4 == 2（乱）: 黄   ,  omega = +0.045 rad/frame
//    kind%4 == 3（舞）: 青   ,  omega = -0.045 rad/frame
// ============================================================
static void ShotSpiralString(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int charId = pEnemyShotSet->kind % 4;
        const auto& offsets = GetOffsets_Tmp(charId);

        // 色: 0=赤, 1=黄, 4=青, 8=橙
        constexpr int colorTable[] = { 0, 8, 1, 4 };
        const int imgKind = img_enemyShotSmallBall[colorTable[charId]];

        // 旋回方向・速さ：偶数=右回転 / 奇数=左回転、後半の文字ほど速く螺旋
        constexpr double omegaTable[] = { +0.030, -0.030, +0.045, -0.045 };
        const double omega = omegaTable[charId] / 2.2;

        for (const auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + off.first;
            shot->y = pEnemyShotSet->y + off.second;
            shot->muki = pEnemyShotSet->muki;
            shot->speed = 1.5;
            shot->kind = imgKind;
            shot->margin = 240.0;   // 画面外余裕を広めに（螺旋で端に行きやすい）
            shot->param_d[0] = omega;  // 角速度を弾ごとに保存

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    // 毎フレーム：muki を角速度で更新してから移動（螺旋軌道）
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->muki += p->param_d[0];
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Moji_Claude()
{
    static int muki;
    static int nextWaveFrame;
    static int waveId;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 200.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        nextWaveFrame = 60;   // 最初の文字を count==60 で発射
        waveId = 0;
    }

    // 左右移動
    enemy.x += 0.98 * (double)muki;
    if (count % 120 == 60) muki *= -1;

    // 60 フレームごとに文字を順番に発射（4文字ループ）
    if (count == nextWaveFrame && count <= 60 * 4) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSpiralString;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = waveId % 4;   // 0:神, 1:速, 2:乱, 3:舞

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        waveId++;
        nextWaveFrame += 60;
    }
}