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
//  任意の文字列を指定サイズで描画し、白ピクセルの相対座標を返す
// ============================================================
static std::vector<std::pair<double, double>> SampleTextPixels(const wchar_t* text, int fontSize = 28)
{
    InitGdiplusOnce();

    // 十分な幅を確保（日本語コメント用）
    const int W = fontSize * (int)wcslen(text) + fontSize * 2;
    const int H = fontSize * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));

    // アンチエイリアスOFF（単色描画）
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic UI");
    Font font(&fontFamily, (REAL)fontSize, FontStyleRegular, UnitPixel);

    g.DrawString(text, -1, &font, PointF((REAL)(fontSize / 2), (REAL)(fontSize / 4)), &whiteBrush);

    // ピクセルデータ取得
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

    // バウンディングボックスの中心を原点に
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
//  ニコニコ風コメント弾幕パターン
//  右端から左へ複数のコメント文字列が流れる
// ============================================================
static void ShotNicoCommentFlood(sEnemyShotSet* pEnemyShotSet)
{
    // 流すコメント一覧（短めに抑えて弾数を制御）
    static const wchar_t* comments[] = {
        L"うｐ主うますぎｗ",
        L"弾幕きたー！！",
        L"やべぇこれ",
        L"神プレイ",
        L"右から左へ",
        L"ニコニコ感",
        L"つえーつえー",
        L"あぶねｗｗ",
    };
    constexpr int COMMENT_NUM = sizeof(comments) / sizeof(comments[0]);
    constexpr double SCALE = 2.2*2;          // 文字の拡大率
    constexpr double START_X = 520.0;      // 画面右外から出現

    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 色バリエーション（白・黄・シアン・橙を中心にニコニコっぽく）
        const int colorTable[] = { 6, 1, 3, 8, 6, 1, 3, 6 }; // 白,黄,シアン,橙...

        for (int c = 0; c < COMMENT_NUM; ++c) {
            const auto& offsets = SampleTextPixels(comments[c], 26/2);
            if (offsets.empty()) continue;

            // 各コメントの基準Y（画面を上下に分散）
            double baseY = 40.0 + c * 58.0 + (GetRand(20) - 10);

            // 速度を少しバラつかせる（右→左）
            double speed = 1.6 + GetRand(12) / 10.0;   // 1.6 ～ 2.8

            // 色
            int colorIdx = colorTable[c % 8];

            for (size_t i = 0; i < offsets.size(); ++i) {
                sEnemyShot* shot = new sEnemyShot;
                shot->x = START_X + offsets[i].first * SCALE;
                shot->y = baseY + offsets[i].second * SCALE;
                shot->muki = DX_PI;                     // 真左方向
                shot->speed = speed;
                shot->kind = img_enemyShotSmallBall[colorIdx];
                // 画面外判定用マージンを大きめに（長いコメント対応）
                shot->param_d[0] = 80.0;                // 余裕を持たせる
                shot->margin = 480;

                // リスト接続
                shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                shot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = shot;
                pEnemyShotSet->pEnemyShotHead->prev = shot;
            }
        }
    }

    // 弾の移動（右→左）
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        if (p->x < -20) p->margin = -999;
        p = p->next;
    }
}

// ============================================================
//  敵本体のパターン
//  関数名: EnemyPat_NicoNico_Grok
// ============================================================
void EnemyPat_NicoNico_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += 0.7 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 一定間隔でコメント洪水を発射
    if (count % 180 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNicoCommentFlood;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;          // このパターンでは使わない
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リスト接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}