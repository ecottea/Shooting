#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <string>
#include <map>

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
//  任意の文字列を描画し、白ピクセルの相対座標を返す (キャッシュ付き)
// ============================================================
static std::vector<std::pair<double, double>> GetTextPixels(const wchar_t* text)
{
    static std::map<std::wstring, std::vector<std::pair<double, double>>> cache;

    std::wstring wtext(text);
    if (cache.find(wtext) != cache.end()) {
        return cache[wtext];
    }

    InitGdiplusOnce();

    const int SIZE = 32;          // フォントサイズ（ピクセル）
    const int W = SIZE * 4;       // 複数文字に対応するため幅を広げる
    const int H = SIZE * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));    // 透明でクリア

    // アンチエイリアスOFF（単色描画でドット絵的なコメントっぽさを出す）
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    // ニコニコ動画のコメントっぽさを出すため MS PGothic を使用
    FontFamily fontFamily(L"MS PGothic");
    Font font(&fontFamily, (REAL)SIZE, FontStyleRegular, UnitPixel);

    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    format.SetLineAlignment(StringAlignmentNear);

    g.DrawString(wtext.c_str(), -1, &font, PointF(0, 0), &format, &whiteBrush);

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

    cache[wtext] = result;
    return result;
}

// ============================================================
//  コメント弾幕パターン 1: 「w」 (高速フローコメント)
//  右から左へ高速で流れる。横に長い当たり判定を再現するため、文字全体を生成。
// ============================================================
static void ShotComment_W(sEnemyShotSet* pEnemyShotSet)
{
    constexpr double SCALE = 4.0;  // 拡大率

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const auto& offsets = GetTextPixels(L"w");
        for (size_t i = 0; i < offsets.size(); ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + offsets[i].first * SCALE;
            shot->y = pEnemyShotSet->y + offsets[i].second * SCALE;

            // 基本は左向き(DX_PI)、わずかに上下にブレさせる
            shot->muki = DX_PI + (GetRand(40) - 20) / 180.0 * DX_PI/10;
            shot->speed = 5.0 + GetRand(30) / 10.0/10; // 5.0 ~ 8.0

            // 素材: 小玉(白)
            shot->kind = img_enemyShotSmallBall[6];
            shot->margin = 240;

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  コメント弾幕パターン 2: 「8」 (炸裂コメント)
//  画面上部から扇状に炸裂するように飛んでくる。
// ============================================================
static void ShotComment_8(sEnemyShotSet* pEnemyShotSet)
{
    constexpr double SCALE = 5.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const auto& offsets = GetTextPixels(L"8");
        for (size_t i = 0; i < offsets.size(); ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + offsets[i].first * SCALE;
            shot->y = pEnemyShotSet->y + offsets[i].second * SCALE;

            // 自機狙いを中心に ±60度 の扇状に拡散
            double to_player = atan2(player.y - shot->y, player.x - shot->x);
            shot->muki = 5*pEnemyShotSet->muki -4*to_player;// (GetRand(120) - 60) / 180.0 * DX_PI;
            shot->speed = 3.5;

            // 素材: 中玉(橙)
            shot->kind = img_enemyShotMediumBall[8];
            shot->margin = 240;

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  コメント弾幕パターン 3: 「神」 (追尾コメント)
//  自機を執拗に狙ってくる高速で目立つ弾。
// ============================================================
static void ShotComment_God(sEnemyShotSet* pEnemyShotSet)
{
    constexpr double SCALE = 6.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const auto& offsets = GetTextPixels(L"神");
        for (size_t i = 0; i < offsets.size(); ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + offsets[i].first * SCALE;
            shot->y = pEnemyShotSet->y + offsets[i].second * SCALE;

            // 完全な自機狙い
            shot->muki = pEnemyShotSet->muki;
            shot->speed = 4.5;

            // 素材: 大玉(赤)
            shot->kind = img_enemyShotMediumBall[0];
            shot->margin = 240;

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  コメント弾幕パターン 4: 「？」 (低速・詰まりコメント)
//  画面を埋めるようにゆっくり流れてきて、視界を妨げる。
// ============================================================
static void ShotComment_Question(sEnemyShotSet* pEnemyShotSet)
{
    constexpr double SCALE = 4.5;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const auto& offsets = GetTextPixels(L"？");
        for (size_t i = 0; i < offsets.size(); ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + offsets[i].first * SCALE;
            shot->y = pEnemyShotSet->y + offsets[i].second * SCALE;

            // 左向きを中心に ±45度 の広い範囲にばら撒く
            shot->muki = DX_PI + (GetRand(90) - 45) / 180.0 * DX_PI/10;
            shot->speed = 1.5 + GetRand(20) / 10.0/10-1; // 1.5 ~ 3.5 (低速)

            // 素材: 菱形弾(マゼンタ)
            shot->kind = img_enemyShotDiamond[5];
            shot->margin = 240;

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  敵本体のパターン (ニコニコ動画モチーフ)
// ============================================================
void EnemyPat_NicoNico_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480。右上あたりで待機し、左右に揺れる動きをする
        enemy.x = 300.0;
        enemy.y = 80.0;
        enemy.maxHp = 200;
        enemy.hp = 200;
    }
    else {
        // 左右にゆっくり揺れる動き (周期 240フレーム)
        enemy.x = 300.0 + sin(count / 120.0 * DX_PI) * 100.0;
    }

    // --------------------------------------------------------
    // 弾幕発射管理
    // --------------------------------------------------------

    // 1. 「w」コメント流し (180フレーム周期)
    if (count % 180 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotComment_W;
        pEnemyShotSet->x = 500;
        pEnemyShotSet->y = player.y;
        pEnemyShotSet->muki = DX_PI; // 左向き
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 2. 「？」コメント詰まり (240フレーム周期, wとずらす)
    if (count % 240 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotComment_Question;
        pEnemyShotSet->x = 500; // 少し左から出す
        pEnemyShotSet->y = player.y + GetRand(40) - 20;
        pEnemyShotSet->muki = DX_PI;
        pEnemyShotSet->kind = 1;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 3. 「8」コメント炸裂 (300フレーム周期)
    if (count % 300 == 120) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotComment_8;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // 自機への角度を計算
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 2;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 4. 「神」コメント追尾 (360フレーム周期)
    if (count % 360 == 180) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 予告音

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotComment_God;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 30.0;
        // 自機への角度を計算
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 3;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}