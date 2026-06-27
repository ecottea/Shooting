// enemyPat_Tmp.cpp
// 文字を活用したオリジナル弾幕：「敵」の字が回転しながら迫る
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
//  文字の輪郭を等間隔でサンプリング（共通関数）
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

    // 超高精度で折れ線化
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
//  「敵」の字形オフセット（初回生成）
// ============================================================
static const std::vector<std::pair<double, double>>& GetOffsets()
{
    static std::vector<std::pair<double, double>> offsets;
    static bool initialized = false;
    if (!initialized) {
        // フォントは視認性の良いメイリオ（太字）を使用
        const wchar_t* text = L"敵";
        constexpr REAL  fontSize = 500.0f;
        constexpr double interval = 25.0;
        const wchar_t* fontName = L"Meiryo";
        offsets = SampleTextOutline(text, fontName, fontSize, interval);
        initialized = true;
    }
    return offsets;
}

// ============================================================
//  弾幕パターン：文字を回転させながら直進させる
// ============================================================
static void ShotRotatingText(sEnemyShotSet* pEnemyShotSet)
{
    const int sound = sound_enemyShot_medium;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound) == 1) StopSoundMem(sound);
        PlaySoundMem(sound, DX_PLAYTYPE_BACK);

        const auto& offsets = GetOffsets();
        // 回転方向を param_i[0] から取得（1 or -1）
        int rotDir = pEnemyShotSet->param_i[0];
        if (rotDir == 0) rotDir = 1; // 安全策

        // 色を kind から決定（0～5）
        int colorIdx = pEnemyShotSet->kind % 6;
        // 中玉を使用（小玉より存在感があり、大玉より軽い）
        int imgKind = img_enemyShotSmallBall[colorIdx];

        for (const auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + off.first;
            shot->y = pEnemyShotSet->y + off.second;
            shot->muki = pEnemyShotSet->muki; // 一応設定（今回は使用しない）
            shot->speed = 0.0;                // 自前で移動させるため0
            shot->kind = imgKind;
            shot->margin = 240.0;

            // 元の相対オフセットを保存
            shot->param_d[0] = off.first;
            shot->param_d[1] = off.second;

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    // 毎フレーム：回転＋直進
    double setX = pEnemyShotSet->x;
    double setY = pEnemyShotSet->y;
    double time = (double)pEnemyShotSet->count; // 経過フレーム
    double rotSpeed = 0.003;                     // 回転速度
    int rotDir = pEnemyShotSet->param_i[0];     // 回転方向
    if (rotDir == 0) rotDir = 1;
    double angle = rotDir * time * rotSpeed;
    double cosA = cos(angle);
    double sinA = sin(angle);

    double speed = 2.0;                         // 前進速度
    double dx = speed * cos(pEnemyShotSet->muki);
    double dy = speed * sin(pEnemyShotSet->muki);

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        double ox = p->param_d[0];
        double oy = p->param_d[1];
        // 回転
        double rx = ox * cosA - oy * sinA;
        double ry = ox * sinA + oy * cosA;
        // 回転中心＋直進移動
        p->x = setX + rx + dx * time;
        p->y = setY + ry + dy * time;
        p = p->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Moji_DeepSeek()
{
    static int muki = 1;
    static int nextWaveFrame = 50;   // 最初の発射フレーム
    static int waveId = 0;           // 色と回転方向の切り替え用

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        nextWaveFrame = 50;
        waveId = 0;
    }

    // 水平往復移動（やや広めに）
    enemy.x += 0.9 * muki;
    if (enemy.x > 360.0) muki = -1;
    if (enemy.x < 120.0) muki = 1;

    // 50フレームごとに「敵」の字形弾を発射
    if (count == nextWaveFrame) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotRotatingText;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = waveId % 6;           // 0～5 の色
        // 回転方向を交互に（waveId の偶奇で反転）
        pSet->param_i[0] = (waveId % 2 == 0) ? 1 : -1;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        waveId++;
        nextWaveFrame += 200;   // 次の発射まで50フレーム
    }
}