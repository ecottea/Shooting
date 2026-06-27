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
//  text     : 文字列（L"弾" など）
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

    // 0.25pxの超高精度で折れ線化
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

    // 前の線分から持ち越した「余り距離」
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

            // 新しい画（ストローク）の開始点には必ず弾を置く
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
//  文字「弾」の弾オフセットリスト（初回アクセス時に自動生成）
// ============================================================
static std::vector<std::pair<double, double>>& GetOffsets()
{
    static std::vector<std::pair<double, double>> offsets;
    static bool initialized = false;
    if (!initialized) {
        const wchar_t* text = L"弾";
        constexpr REAL  fontSize = 360.0f;
        constexpr double interval = 20.0;
        const wchar_t* fontName = L"HGSoeiKakupoptai";

        offsets = SampleTextOutline(text, fontName, fontSize, interval);
        initialized = true;
    }
    return offsets;
}

// ============================================================
//  弾幕パターン：文字を分解して再構築する弾幕
// ============================================================
static void ShotDecomposeAndReform(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 文字「弾」の形を取得
        const auto& offsets = GetOffsets();

        // 小玉（赤）を使用
        const int imgKind = img_enemyShotSmallBall[0]; // 0:赤

        // 各弾に「本来の位置（文字上の位置）」を覚えさせる
        for (const auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            // 初期位置は画面の四隅などに散らす（ここでは簡易的にランダム）
            while (true) {
                shot->x = GetRand(480);
                shot->y = GetRand(480);
                double dx = shot->x - player.x;
                double dy = shot->y - player.y;
                double dist = sqrt(dx * dx + dy * dy);
                if (dist > 50) break;
            }
            // 本来の相対位置（文字上の位置）を param_d に保存
            shot->param_d[0] = off.first;   // targetX オフセット
            shot->param_d[1] = off.second;  // targetY オフセット
            // 向きはとりあえずランダム
            shot->muki = GetRand(360) / 180.0 * DX_PI;
            shot->speed = 1.5;
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
        // 最初の 120 フレームはランダムに動き回る
        if (pEnemyShotSet->count < 120) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            // 画面端で跳ね返る簡易処理
            if (p->x < 0 || p->x > 480) p->muki = DX_PI - p->muki;
            if (p->y < 0 || p->y > 480) p->muki = -p->muki;
        }
        // 120 フレーム経過後は、文字の形に集まる（ホーミング）
        else if (pEnemyShotSet->count < 360) {
            if (pEnemyShotSet->count == 120) {
                if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                p->speed = 2.0;
            }
            double targetX = pEnemyShotSet->x + p->param_d[0];
            double targetY = pEnemyShotSet->y + p->param_d[1];
            double dx = targetX - p->x;
            double dy = targetY - p->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 1.1) {
                p->muki = atan2(dy, dx);
                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);
            }
            else {
                // ほぼ目標位置に着いたら速度を落とす
                p->speed *= 0.9;
            }
        }
        // 240 フレーム経過後は、プレイヤーに向かって直進
        else if (pEnemyShotSet->count == 360) {
            p->speed = 3.0;
            p->muki = atan2(player.y - p->y, player.x - p->x);
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        else {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// ============================================================
//  敵本体パターン（関数名を EnemyPat_Moji_Sakana に変更）
// ============================================================
void EnemyPat_Moji_Sakana()
{
    static int muki = 1;
    static int nextWaveFrame = 60;   // 最初の波を撃つフレーム
    static int waveId = 0;           // 0:弾

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        nextWaveFrame = 60;          // 最初の波は count==60 で発射
        waveId = 0;
    }

    // 左右移動
    enemy.x += 0.99 * muki;
    if (count % 120 == 60) muki *= -1;

    // 順番に文字を撃つ（60 → 120 → 180）
    if (count == nextWaveFrame) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDecomposeAndReform;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 140.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = waveId;          // 0:弾

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
        nextWaveFrame += 360;         // 60 フレーム間隔
    }
}