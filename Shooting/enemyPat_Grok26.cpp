// enemyPat_tmp.cpp
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
// GDI+ 初期化
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
// 文字輪郭サンプリング（高精度）
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
// 文字データ（自由に追加可能）
// ============================================================
static std::vector<std::pair<double, double>>& GetOffsets(int id)
{
    static std::vector<std::pair<double, double>> offsets[5];
    static bool initialized = false;
    if (!initialized) {
        const wchar_t* chars[] = { L"狂", L"華", L"乱", L"死", L"避" };
        constexpr REAL fontSize = 340.0f;
        constexpr double interval = 17.0;
        const wchar_t* fontName = L"HGSoeiKakupoptai";

        for (int i = 0; i < 5; ++i) {
            offsets[i] = SampleTextOutline(chars[i], fontName, fontSize, interval);
        }
        initialized = true;
    }
    return offsets[id % 5];
}

// ============================================================
// 創造的な文字弾幕：回転＋加速＋多方向波
// ============================================================
static void ShotCreativeText(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 派手な効果音
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int charId = pEnemyShotSet->kind % 5;
        const auto& offsets = GetOffsets(charId);

        // 色をランダムに変化させる
        int colorBase = pEnemyShotSet->kind % 6;

        for (const auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + off.first * 0.95;   // 少し縮小
            shot->y = pEnemyShotSet->y + off.second * 0.95;

            // 基本方向＋文字ごとの微妙な角度差で立体感
            double baseAngle = pEnemyShotSet->muki + (off.first * 0.003);
            shot->muki = baseAngle;
            shot->speed = 1.8 + fabs(off.second) * 0.008;  // 上部は速く、下部は遅めで歪み効果

            int colorIdx = (colorBase + (int)(off.second / 12)) % 6;
            if (colorIdx < 0) colorIdx += 6;
            shot->kind = img_enemyShotSmallBall[colorIdx];

            shot->margin = 280.0;

            // パラメータを使って後から加速・回転させる
            shot->param_i[0] = 0;                    // 生存フレームカウンタ
            shot->param_d[0] = baseAngle;            // 元の角度
            shot->param_d[1] = 0.015 * (off.first > 0 ? -1.0 : 1.0); // 回転速度（文字の左右で逆回転）

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    // 毎フレーム処理：直進＋徐々に加速＋角度回転
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->param_i[0]++;

        // 時間経過で加速（後半ほど速くなる）
        double accel = 1.0 + p->param_i[0] * 0.0035;
        double currentSpeed = p->speed * accel;

        // ゆっくり回転
        p->muki += p->param_d[1];

        p->x += currentSpeed * cos(p->muki);
        p->y += currentSpeed * sin(p->muki);

        p = p->next;
    }
}

// ============================================================
// 敵本体パターン：自由な動き＋創造的弾幕
// ============================================================
void EnemyPat_Moji_Grok()
{
    static int muki = 1;
    static int nextWaveFrame = 45;
    static int waveId = 0;
    static double vy = 0.0;  // 上下揺れ用

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        nextWaveFrame = 50;
        waveId = 0;
        vy = 0.0;
    }

    // 自由な動き：左右＋上下の波
    enemy.x += 0.85 * (double)muki;
    vy += 0.035 * sin(count * 0.045);
    enemy.y = 55.0 + 22.0 * sin(count * 0.032);

    if (enemy.x < 120 || enemy.x > 360) muki *= -1;

    // 文字弾幕を比較的頻繁に発射（迫力重視）
    if (count == nextWaveFrame) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotCreativeText;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 18.0;

        // プレイヤー方向＋ランダムなブレで予測しにくく
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x)
            + (GetRand(80) - 40) / 180.0 * DX_PI * 0.4;

        pSet->kind = waveId;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        waveId++;
        nextWaveFrame += 104 + (waveId % 3) * 8;  // 少し不規則な間隔で緊張感
    }
}