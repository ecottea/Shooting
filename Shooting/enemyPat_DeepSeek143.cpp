// ============================================================
//  enemyPat_Tmp.cpp
//  弾幕パターン：ニコニコ動画風コメント弾幕
//
//  画面右外からコメント文字が右→左へ流れていく弾幕。
//  各文字は GDI+ でピクセル化し、1ピクセル = 1弾として扱う。
//
//  【使用素材】
//    弾画像 : img_enemyShotSmallBall[color]  （小玉 2.5x2.5）
//    色     : 0:赤, 4:青, 6:白 など 8 色から選択
//    効果音 : sound_enemyShot_light  (白コメント用)
//             sound_enemyShot_medium (青コメント用)
//             sound_enemyShot_heavy  (赤大コメント用)
//             sound_enemyCharge      (赤大コメントの予告音)
//
//  count / pEnemyShotSet->count / pEnemyShot->count のインクリメント
//  および画面外弾の消去はメインルーチン側で行われる前提。
// ============================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <utility>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// ============================================================
//  定数
// ============================================================
namespace {
    constexpr double kSpawnX = 520.0;  // コメント発生位置（画面右外）
    constexpr double kPixelScale = 2.0*2;    // グリフの拡大率
    constexpr double kCharExtra = 4.0;    // 文字間の追加余白
    constexpr double kShotMargin = 800.0;  // 画面外消去マージン（広めに取る）
    constexpr int    kFontSize = 20-4;     // フォントサイズ（px）
}

// ============================================================
//  GDI+ 初期化（この TU 用、1 回のみ）
// ============================================================
static void InitGdiplusTmp()
{
    static bool initialized = false;
    if (!initialized) {
        GdiplusStartupInput input;
        ULONG_PTR token;
        GdiplusStartup(&token, &input, nullptr);
        initialized = true;
    }
}

// ============================================================
//  1 文字分のグリフ情報
//    pixels  : 文字バウンディングボックス中心を原点とした白ピクセル座標
//    advance : 次の文字までの送り幅
// ============================================================
struct CharGlyph {
    std::vector<std::pair<double, double>> pixels;
    double advance = 0.0;
};

static const CharGlyph& GetGlyph(wchar_t ch)
{
    static std::unordered_map<wchar_t, CharGlyph> cache;
    auto it = cache.find(ch);
    if (it != cache.end()) return it->second;

    InitGdiplusTmp();

    const int W = kFontSize * 2;
    const int H = kFontSize * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush brush(Color(255, 255, 255, 255));
    FontFamily family(L"Yu Gothic UI Light");
    Font font(&family, (REAL)kFontSize, FontStyleRegular, UnitPixel);

    wchar_t buf[2] = { ch, 0 };
    g.DrawString(buf, -1, &font, PointF(0, 0), &brush);

    BitmapData data;
    Rect rect(0, 0, W, H);
    bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &data);
    BYTE* argb = (BYTE*)data.Scan0;

    std::vector<std::pair<int, int>> white;
    int minX = W, maxX = -1, minY = H, maxY = -1;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int idx = y * data.Stride + x * 4;
            if (argb[idx + 3] == 255) {  // アンチエイリアスOFFなので 0/255 の2値
                white.emplace_back(x, y);
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }
    bitmap.UnlockBits(&data);

    CharGlyph glyph;
    if (!white.empty()) {
        double cx = (minX + maxX) / 2.0;
        double cy = (minY + maxY) / 2.0;
        glyph.pixels.reserve(white.size());
        for (auto& p : white)
            glyph.pixels.emplace_back(p.first - cx, p.second - cy);
        glyph.advance = (maxX - minX) + 2.0;
    }
    else {
        // 空白文字など
        glyph.advance = kFontSize * 0.5;
    }

    auto res = cache.emplace(ch, std::move(glyph));
    return res.first->second;
}

// ============================================================
//  コメント定義テーブル
//    speed : px/frame
//    color : 0:赤, 4:青, 6:白
// ============================================================
struct CommentDef {
    const wchar_t* text;
    double         speed;
    int            color;
    int            sound;
};

static const CommentDef kComments[] = {
    { L"うぽつ",        3.0, 6, sound_enemyShot_light  },  // 白
    { L"888",           4.5, 6, sound_enemyShot_light  },  // 白
    { L"www",           4.0, 6, sound_enemyShot_light  },  // 白
    { L"ここすき",      3.5, 4, sound_enemyShot_medium },  // 青
    { L"乙",            2.5, 4, sound_enemyShot_medium },  // 青
    { L"ｷﾀ━(ﾟ∀ﾟ)━!!", 2.2, 0, sound_enemyShot_heavy  },  // 赤大
};
constexpr int kCommentCount = sizeof(kComments) / sizeof(kComments[0]);
constexpr int kBigCommentIndex = 5;

// ============================================================
//  コメント文字列を弾群として生成
// ============================================================
static void SpawnCommentBullets(sEnemyShotSet* pSet, const wchar_t* text,
    double speed, int color)
{
    double cursor = 0.0;
    for (const wchar_t* p = text; *p != L'\0'; ++p) {
        const CharGlyph& glyph = GetGlyph(*p);

        for (const auto& px : glyph.pixels) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x + cursor + px.first * kPixelScale;
            shot->y = pSet->y + px.second * kPixelScale;
            shot->muki = DX_PI;   // 左向き（右→左へ流れる）
            shot->speed = speed;
            shot->kind = img_enemyShotSmallBall[color];
            shot->margin = kShotMargin;
            shot->count = 0;

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
        cursor += glyph.advance * kPixelScale + kCharExtra;
    }
}

// ============================================================
//  弾幕パターン関数：コメントを右から左へ流し続ける
// ============================================================
static void ShotNicoComment(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int idx = pSet->kind;
        if (idx < 0 || idx >= kCommentCount) idx = 0;
        const CommentDef& def = kComments[idx];

        if (CheckSoundMem(def.sound)) StopSoundMem(def.sound);
        PlaySoundMem(def.sound, DX_PLAYTYPE_BACK);

        SpawnCommentBullets(pSet, def.text, def.speed, def.color);
    }

    // 全弾を左へ移動
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        if (p->x < -20) p->margin = -999;
        p = p->next;
    }
}

// ============================================================
//  コメントセット生成ヘルパ
// ============================================================
static void SpawnCommentSet(int commentIndex, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotNicoComment;
    pSet->x = kSpawnX;
    pSet->y = y;
    pSet->muki = DX_PI;
    pSet->kind = commentIndex;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_NicoNico_DeepSeek()
{
    static int muki = 1;

    // ---- 初期化 ＆ 移動（画面上部を左右に往復） ----
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (enemy.x < 80.0)  muki = 1;
        if (enemy.x > 400.0) muki = -1;
    }

    // ---- 通常コメント（白/青）を 90 フレームごとに発射 ----
    // ただし大コメントと同時にならないよう除外
    if (count > 0 && count % 25 == 0) {
        int    idx = GetRand(4);           // 0..4 の中からランダム
        double y = 60.0 + GetRand(360);  // 60..420 のレーン
        SpawnCommentSet(idx, y);
    }

    // ---- 大コメント（赤）270 フレーム周期 ----
    // その 90 フレーム前に予告音を鳴らす
    if (count > 0 && count % 200 == 45) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count > 0 && count % 200 == 105) {
        double y = 140.0 + GetRand(200);   // 140..340 の中央付近
        SpawnCommentSet(kBigCommentIndex, y);
    }
}