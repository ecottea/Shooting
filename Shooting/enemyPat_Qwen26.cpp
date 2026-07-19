// enemyPat_Tmp.cpp

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
//  text     : 文字列
//  fontName : フォント名
//  fontSize : フォントサイズ
//  interval : 弾の間隔
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
//  文字の弾オフセットリスト（初回アクセス時に自動生成）
//  ここでは「地獄」の2文字を設定
// ============================================================
static std::vector<std::pair<double, double>>& GetOffsets(int id)
{
    static std::vector<std::pair<double, double>> offsets[2];
    static bool initialized = false;
    if (!initialized) {
        // サンプルにとらわれず、ボス戦らしい「地獄」を採用
        const wchar_t* chars[] = { L"地", L"獄" };

        // HG創英角ポップ体を使用（サンプルのフォント名を踏襲）
        constexpr REAL  fontSize = 360.0f;
        constexpr double interval = 18.0; // 少し詰めて密度を上げる
        const wchar_t* fontName = L"HGSoeiKakupoptai";

        for (int i = 0; i < 2; ++i) {
            offsets[i] = SampleTextOutline(chars[i], fontName, fontSize, interval);
        }
        initialized = true;
    }
    return offsets[id % 2];
}

// ============================================================
//  弾幕パターン：回転しながら突進する文字弾
// ============================================================
static void ShotRotatingString(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 効果音：重たい音を使用
        if (CheckSoundMem(sound_enemyShot_extreme) == 1) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int charId = pEnemyShotSet->kind % 2; // 0:地, 1:獄
        const auto& offsets = GetOffsets(charId);

        for (const auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + off.first;
            shot->y = pEnemyShotSet->y + off.second;
            shot->muki = 0.0;
            shot->speed = 0.0;
            // 9色のカラーバリエーションを利用
            shot->kind = img_enemyShotSmallBall[(pEnemyShotSet->kind + charId) % 9];
            shot->margin = 240.0;

            // 初期相対座標を保存
            shot->param_d[0] = off.first;
            shot->param_d[1] = off.second;

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }

        // セット全体の回転設定
        pEnemyShotSet->param_d[0] = 0.0; // 現在の回転角
        // 「地」は右回転、「獄」は左回転にして変化をつける
        pEnemyShotSet->param_d[1] = (charId == 0) ? 0.02 : -0.02;
        pEnemyShotSet->param_d[2] = 1.8;  // 突進速度
    }

    // セット全体の移動（自機に向けて突進）
    pEnemyShotSet->x += pEnemyShotSet->param_d[2] * cos(pEnemyShotSet->muki);
    pEnemyShotSet->y += pEnemyShotSet->param_d[2] * sin(pEnemyShotSet->muki);

    // 回転角の更新
    pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[1];
    double angle = pEnemyShotSet->param_d[0];
    double cos_a = cos(angle);
    double sin_a = sin(angle);

    // 弾の座標を更新
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        double rx = p->param_d[0];
        double ry = p->param_d[1];

        // 回転行列を適用
        p->x = pEnemyShotSet->x + (rx * cos_a - ry * sin_a);
        p->y = pEnemyShotSet->y + (rx * sin_a + ry * cos_a);

        // 弾の色を時間経過で変化させる（虹色に点滅）
        p->kind = img_enemyShotSmallBall[(p->count / 4 + pEnemyShotSet->kind) % 9];

        p = p->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Moji_Qwen()
{
    static int muki; // 上下移動の方向
    static int nextWaveFrame;
    static int waveId;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0; // 少し下からスタート
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        nextWaveFrame = 60;
        waveId = 0;
    }
    else {
        // 上下にゆっくりと浮遊する（ボスっぽい動き）
        enemy.y += 0.8 * muki;
        if (enemy.y < 40.0 || enemy.y > 120.0) muki *= -1;
    }

    // 順番に文字を撃つ（地 -> 獄 -> 地 ...）
    if (count == nextWaveFrame) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotRotatingString;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        // 自機を狙う
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = waveId;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリストへ追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        waveId++;
        nextWaveFrame += 120; // 90フレーム間隔で次の文字を撃つ
    }
}