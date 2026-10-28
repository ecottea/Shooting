#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <string>

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
//  1文字を描画して白ピクセルの相対座標を返す
// ============================================================
static std::vector<std::pair<double, double>> SampleCharPixels(const wchar_t* ch, int fontSize)
{
    InitGdiplusOnce();

    const int W = fontSize * 2;
    const int H = fontSize * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic UI Light");
    Font font(&fontFamily, (REAL)fontSize, FontStyleRegular, UnitPixel);

    g.DrawString(ch, -1, &font, PointF(0, 0), &whiteBrush);

    BitmapData bmpData;
    Rect rect(0, 0, W, H);
    bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);

    BYTE* argb = (BYTE*)bmpData.Scan0;
    std::vector<std::pair<int, int>> whitePixels;
    int minX = W, maxX = -1, minY = H, maxY = -1;

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int idx = y * bmpData.Stride + x * 4;
            BYTE a = argb[idx + 3];
            if (a == 255) {
                whitePixels.emplace_back(x, y);
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }
    bitmap.UnlockBits(&bmpData);

    std::vector<std::pair<double, double>> result;
    if (!whitePixels.empty()) {
        double cx = (minX + maxX) / 2.0;
        double cy = (minY + maxY) / 2.0;
        result.reserve(whitePixels.size());
        for (const auto& p : whitePixels) {
            result.emplace_back(p.first - cx, p.second - cy);
        }
    }
    return result;
}

// ============================================================
//  各文字パーツの定義
// ============================================================
struct FacePart {
    const wchar_t* ch;
    double offsetX;   // 顔中心からの相対X
    double offsetY;   // 顔中心からの相対Y
    double scale;     // 拡大率
    int    colorIndex;// 弾の色
};

// へのへのもへじ を顔の部品として配置
// へ(左眉) の(左目) へ(右眉) の(右目) も(鼻) へ(口) じ(輪郭)
static const FacePart FACE_PARTS[] = {
    { L"へ", -48.0, -55.0, 1.6, 6 },  // 左眉（白）
    { L"の", -38.0, -28.0, 1.4, 3 },  // 左目（シアン）
    { L"へ",  48.0, -55.0, 1.6, 6 },  // 右眉（白）
    { L"の",  38.0, -28.0, 1.4, 3 },  // 右目（シアン）
    { L"も",   0.0,   5.0, 1.5, 1 },  // 鼻（黄）
    { L"へ",   0.0,  45.0, 1.8, 0 },  // 口（赤）
    { L"じ",   0.0,  25.0, 3.2, 4 },  // 輪郭（青・大きめ）
};
static const int FACE_PART_COUNT = sizeof(FACE_PARTS) / sizeof(FACE_PARTS[0]);

// ============================================================
//  全パーツのオフセットを事前計算して保持
// ============================================================
struct PreparedPart {
    std::vector<std::pair<double, double>> pixels;
    double offsetX;
    double offsetY;
    double scale;
    int    colorIndex;
};

static std::vector<PreparedPart>& GetPreparedFaceParts()
{
    static std::vector<PreparedPart> parts;
    static bool initialized = false;
    if (!initialized) {
        parts.reserve(FACE_PART_COUNT);
        for (int i = 0; i < FACE_PART_COUNT; ++i) {
            PreparedPart pp;
            // フォントサイズは固定でサンプリングし、後でscaleで拡大
            pp.pixels = SampleCharPixels(FACE_PARTS[i].ch, 24);
            pp.offsetX = FACE_PARTS[i].offsetX*2;
            pp.offsetY = FACE_PARTS[i].offsetY*2;
            pp.scale = FACE_PARTS[i].scale*2;
            pp.colorIndex = FACE_PARTS[i].colorIndex;
            parts.push_back(std::move(pp));
        }
        initialized = true;
    }
    return parts;
}

// ============================================================
//  弾幕パターン：へのへのもへじ 顔配置版
//
//  各文字を個別にサンプリングし、眉・目・鼻・口・輪郭の
//  適切な位置と大きさで配置して人の顔に見せる。
//  保持後に放射状＋自機狙い成分で崩壊。
// ============================================================
static void ShotHenohenomohejiFace(sEnemyShotSet* pEnemyShotSet)
{
    constexpr int HOLD_FRAMES = 75;

    // --- 生成フェーズ ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const auto& parts = GetPreparedFaceParts();

        // 顔の中心位置（敵の少し下）
        const double centerX = pEnemyShotSet->x;
        const double centerY = pEnemyShotSet->y + 30.0;

        int shotIndex = 0;
        for (const auto& part : parts) {
            for (const auto& px : part.pixels) {
                sEnemyShot* shot = new sEnemyShot;

                // 各文字のローカル座標をスケールして配置
                shot->x = centerX + part.offsetX + px.first * part.scale;
                shot->y = centerY + part.offsetY + px.second * part.scale;

                shot->muki = 0.0;
                shot->speed = 0.0;

                // 色分けしてパーツを識別しやすく
                shot->kind = img_enemyShotSmallBall[part.colorIndex];

                // 形成中心を保存（崩壊時の放射計算用）
                shot->param_d[0] = centerX;
                shot->param_d[1] = centerY;

                shot->param_i[0] = shotIndex++;  // 識別用

                shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                shot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = shot;
                pEnemyShotSet->pEnemyShotHead->prev = shot;
            }
        }
    }

    // --- 毎フレームの弾更新 ---
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count == HOLD_FRAMES) {
            // 形成中心からの放射方向
            double dx = p->x - p->param_d[0];
            double dy = p->y - p->param_d[1];
            double len = sqrt(dx * dx + dy * dy);
            if (len < 1.0) len = 1.0;

            double radialMuki = atan2(dy, dx);
            double toPlayer = atan2(player.y - p->y, player.x - p->x);

            // 放射70% + 自機狙い30%
            p->muki = radialMuki * 0.70 + toPlayer * 0.30;

            // 速度に幅を持たせる
            p->speed = 1.7 + GetRand(140) / 100.0;  // 約1.7〜3.1

            // 崩壊音は最初の1発だけ
            if (p->param_i[0] == 0) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }
        }

        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);

        p = p->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Henohenomoheji_Grok()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 150.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.85 * (double)muki;
        if (count % 150 == 75) {
            muki *= -1;
        }
    }

    // 一定間隔で顔弾幕を発射
    if (count % 100 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHenohenomohejiFace;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}