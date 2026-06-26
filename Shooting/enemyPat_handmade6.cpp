// enemyPat_Gian.cpp
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
//  文字の輪郭を等間隔でサンプリング
//  text     : 文字列（L"俺" など）
//  fontName : フォント名（L"Yu Gothic" 推奨）
//  fontSize : フォントサイズ（ピクセル相当）
//  interval : 弾の間隔（ピクセル）
//  戻り値   : 重心が (0,0) になるようにシフトした相対座標のリスト
// ============================================================
static std::vector<std::pair<double, double>> SampleTextOutline(
    const wchar_t* text,
    const wchar_t* fontName,
    REAL fontSize,
    double interval)
{
    InitGdiplusOnce();

    FontFamily fontFamily(fontName);
    GraphicsPath path;
    StringFormat fmt;
    path.AddString(text, -1, &fontFamily, FontStyleRegular, fontSize, PointF(0, 0), &fmt);

    // 0.25pxの超高精度で折れ線化（見た目は完全に曲線になります）
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

    // ★ 前の線分から持ち越した「余り距離」
    double leftover = 0.0;

    // 線分を辿りながら等間隔で点を打つ（端数を持ち越す）
    auto sampleLineTo = [&](const PointF& to) {
        double dx = to.X - prevPoint.X;
        double dy = to.Y - prevPoint.Y;
        double segLen = std::sqrt(dx * dx + dy * dy);

        if (segLen > 0.0) {
            double dirX = dx / segLen;
            double dirY = dy / segLen;

            // 次に弾を置くべき位置（現在の線分上での距離）
            double currentPos = interval - leftover;

            // 線分の長さの中で置けるだけ弾を置く
            while (currentPos <= segLen) {
                double x = prevPoint.X + dirX * currentPos;
                double y = prevPoint.Y + dirY * currentPos;
                result.emplace_back(x - cx, y - cy);
                currentPos += interval;
            }

            // 次の線分へ持ち越す「余り距離」を計算
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

            // ★ 新しい画（ストローク）の開始点には必ず弾を置く
            // （これにより、文字の「角」や「書き出し」がくっきりと表現されます）
            result.emplace_back(pt.X - cx, pt.Y - cy);
            leftover = 0.0; // 距離をリセット
        }
        else {
            sampleLineTo(pt);
        }

        // サブパスを閉じる
        if ((type & PathPointTypeCloseSubpath) == PathPointTypeCloseSubpath) {
            sampleLineTo(subpathStart);
        }
    }

    return result;
}

// ============================================================
//  11 個の文字の弾オフセットリスト（初回アクセス時に自動生成）
// ============================================================
static std::vector<std::pair<double, double>>& GetOffsets(int id)
{
    static std::vector<std::pair<double, double>> offsets[11];
    static bool initialized = false;
    if (!initialized) {
        const wchar_t* chars[] = {
            L"俺", L"は", L"ジ", L"ャ", L"イ", L"ア", L"ン",
            L"ガ", L"キ", L"大", L"将"
        };
        // 設定値（あとで調整しやすいように）
        constexpr REAL  fontSize = 360.0f;
        constexpr double interval = 20.0;
        const wchar_t* fontName = L"HGSoeiKakupoptai";

        for (int i = 0; i < 11; ++i) {
            offsets[i] = SampleTextOutline(chars[i], fontName, fontSize, interval);
        }
        initialized = true;
    }
    return offsets[id];
}

// ============================================================
//  弾幕パターン：指定された文字の隊形で飛ばす
// ============================================================
static void ShotFormationString(sEnemyShotSet* pEnemyShotSet)
{
    const int sound = sound_enemyShot_medium;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound) == 1) StopSoundMem(sound);
        PlaySoundMem(sound, DX_PLAYTYPE_BACK);

        // kind でどの文字を使うか決める (0:俺, 1:は, ...)
        int charId = pEnemyShotSet->kind % 11;
        const auto& offsets = GetOffsets(charId);

        // 小玉（赤）を使用
        const int imgKind = img_enemyShotSmallBall[pEnemyShotSet->kind % 6];

        for (const auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + off.first;
            shot->y = pEnemyShotSet->y + off.second;
            shot->muki = pEnemyShotSet->muki;
            shot->speed = 2.0;
            shot->kind = imgKind;
            shot->margin = 240.0;

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
//  敵本体パターン
// ============================================================
void EnemyPat_Gian()
{
    static int muki = 1;
    static int nextWaveFrame = 60;   // 最初の波を撃つフレーム
    static int waveId = 0;           // 0:俺, 1:は, ...

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        nextWaveFrame = 60;          // 最初の波は count==60 で発射
        waveId = 0;
    }

    // 左右移動
    enemy.x += 0.98 * muki;
    if (count % 120 == 60) muki *= -1;

    // 順番に文字を撃つ（60 → 120 → 180）
    if (count == nextWaveFrame) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFormationString;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = waveId;          // 0,1,... で文字指定

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリストへ追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        // 次の波の準備
        waveId++;
        nextWaveFrame += 60;         // 60 フレーム間隔
    }
}