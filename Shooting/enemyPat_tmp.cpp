#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <map>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

//============================================================
// GDI+ 初期化
//============================================================
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

//============================================================
// 任意の文字列をラスタライズして白ピクセルの相対座標を返す
//============================================================
static std::vector<std::pair<double, double>> SampleTextPixels(const wchar_t* txt)
{
    InitGdiplusOnce();

    const int FONT_SIZE = 28/2;
    int len = (int)wcslen(txt);
    if (len == 0) len = 1;
    const int W = FONT_SIZE * len * 2 + 40; // 長いコメント用に幅を確保
    const int H = FONT_SIZE * 3;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic UI");
    Font font(&fontFamily, (REAL)FONT_SIZE, FontStyleBold, UnitPixel);
    g.DrawString(txt, -1, &font, PointF(2, 2), &whiteBrush);

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
        for (auto& p : whitePixels) {
            result.emplace_back(p.first - cx, p.second - cy);
        }
    }
    return result;
}

// コメント種別
static const wchar_t* COMMENT_TEXTS[] = {
    L"ｗｗｗｗｗｗ",
    L"草",
    L"88888888",
    L"うぽつ",
    L"かわいい",
    L"大草原不可避"
};
static const int COMMENT_NUM = 6;

static std::vector<std::pair<double, double>>& GetOffsetsByKind(int kind)
{
    static std::map<int, std::vector<std::pair<double, double>>> cache;
    static bool first = true;
    kind = kind % COMMENT_NUM;
    if (kind < 0) kind = 0;
    auto it = cache.find(kind);
    if (it != cache.end()) return it->second;

    auto vec = SampleTextPixels(COMMENT_TEXTS[kind]);
    cache[kind] = vec;
    return cache[kind];
}

//============================================================
// ニコニコ風コメント流し弾幕
// pEnemyShotSet->param_d[0]=speed
// param_d[1]=amp, [2]=freq, [3]=phase, [4]=scale
// param_i[0]=colorIdx
//============================================================
static void ShotNicoCommentFlow(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 使える効果音: sound_enemyShot_medium を採用
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int commentKind = pEnemyShotSet->kind % COMMENT_NUM;
        const auto& offsets = GetOffsetsByKind(commentKind);

        double speed = pEnemyShotSet->param_d[0];
        double amp = pEnemyShotSet->param_d[1];
        double freq = pEnemyShotSet->param_d[2];
        double phase = pEnemyShotSet->param_d[3];
        double scale = pEnemyShotSet->param_d[4]*2;
        int colorIdx = pEnemyShotSet->param_i[0];

        // 色は smallBall の色違いで表現 0:赤 6:白 8:橙 など
        int bulletImg = img_enemyShotSmallBall[colorIdx % 9];

        for (size_t i = 0; i < offsets.size(); ++i) {
            sEnemyShot* shot = new sEnemyShot;

            // コメントの形を保ったまま右から出現
            shot->x = pEnemyShotSet->x + offsets[i].first * scale;
            shot->y = pEnemyShotSet->y + offsets[i].second * scale;
            shot->muki = DX_PI; // 左方向
            shot->speed = speed;
            shot->kind = bulletImg;
            shot->margin = 480.0; // 画面外判定の余白

            // 波打つ動き用にベースYとパラメータを保存
            shot->param_d[0] = shot->y; // baseY
            shot->param_d[1] = amp;
            shot->param_d[2] = freq;
            shot->param_d[3] = phase + offsets[i].first * 0.02; // 文字内で位相を少しずらすと波が綺麗

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    // 移動: 基本は左へ、ampがあればsin波で上下
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x -= p->speed;
        double amp = p->param_d[1];
        if (amp > 0.1) {
            double freq = p->param_d[2];
            double phase = p->param_d[3];
            double baseY = p->param_d[0];
            p->y = baseY + amp * sin(p->x * freq + phase);
        }
        if (p->x < -20) p->margin = -999;
        p = p->next;
    }
}

//============================================================
// 敵本体
//============================================================
void EnemyPat_Tmp()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 12フレームごとに右端からコメントを流す
    if (count % 24 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNicoCommentFlow;

        // 右画面外から出現 (ゲーム画面 480x480)
        pEnemyShotSet->x = 560.0;
        // yはランダム 60～380の間
        pEnemyShotSet->y = 60.0 + GetRand(320+80);
        pEnemyShotSet->muki = DX_PI;

        // コメント種類 0～5 (GetRand(5)は0..5の6通り)
        pEnemyShotSet->kind = GetRand(COMMENT_NUM - 1);

        // レイヤー抽選 0:高速赤 1:波打ち白 2:低速壁橙
        int layer = GetRand(4)/2; // 0..2の3通り
        double speed, amp = 0.0, freq = 0.05, phase, scale;
        int colorIdx;

        phase = GetRand(628) / 100.0; // 0..6.28

        if (layer == 0) {
            // 高速 5.0～7.0
            speed = (50 + GetRand(20)) / 10.0;
            scale = 2.2;
            colorIdx = 0; // 赤
        }
        else if (layer == 1) {
            // 中速 波打ち 2.0～3.5
            speed = (20 + GetRand(15)) / 10.0;
            amp = 10.0 + GetRand(30)/3; // 10～40
            freq = (3 + GetRand(7)) / 100.0; // 0.03～0.10
            scale = 2.0;
            colorIdx = 6; // 白
        }
        else {
            // 低速 壁 1.0～2.0 巨大
            speed = (10 + GetRand(10)) / 10.0;
            scale = 3.0;
            colorIdx = 8; // 橙
        }

        pEnemyShotSet->param_d[0] = speed;
        pEnemyShotSet->param_d[1] = amp;
        pEnemyShotSet->param_d[2] = freq;
        pEnemyShotSet->param_d[3] = phase;
        pEnemyShotSet->param_d[4] = scale;
        pEnemyShotSet->param_i[0] = colorIdx;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}