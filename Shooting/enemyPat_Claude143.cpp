// ============================================================
//  弾幕：ニコニコ動画コメント弾幕「888888888」
//
//  右端から「コメント(文字の形に並んだ小玉の集合)」が左へ流れてくる。
//  文字の形は GDI+ で実際にフォントを描画し、白ピクセルの位置を弾にして作る。
//
//  【フェーズ構成】(1800フレーム=30秒で1周し、撃破されるまでループ)
//    フェーズ1 (  0〜 599)「ぽつぽつ流れる」  ランダムな行から1本ずつ
//    フェーズ2 (600〜1499)「コメント渋滞」    全行がコメントで埋まる。1行分だけ安全地帯が上下に動く
//    フェーズ3 (1500〜1799)「888888888」      「8」だけの超長文コメントが上下交互に高速で来る
//
//  【仕様メモ】
//    ・count / pEnemyShotSet->count / pEnemyShot->count の加算、画面外弾の消去はメインルーチン側
//    ・コメント1本 = ShotSet 1個。弾は画面右端に差しかかった分から順に生成する(遅延生成)
//      → 画面外で弾が待機しないので、プールを無駄に消費しない
//    ・安全地帯は「コメントが画面中央に到達する時刻」を基準に先読みして決める
//      → 安全地帯は弾と一緒に左へ流れてくるので、プレイヤーは右側から来る隙間を見て動ける
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <cwchar>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// ============================================================
//  定数 (短い名前はWindows SDKと衝突しやすいので NC_ を付ける)
// ============================================================
namespace {
    // --- 行(コメントが流れるレーン) ---
    constexpr int    NC_ROW_NUM = 10;      // 行数
    constexpr double NC_ROW_Y0 = 80.0;    // 0行目の中心Y
    constexpr double NC_ROW_PITCH = 40.0;    // 行の間隔
    constexpr double NC_SPAWN_X = 496.0;   // コメントの左端の初期X(画面右の外)
    constexpr double NC_CENTER_X = 240.0;   // 安全地帯の先読みの基準X
    constexpr double NC_SHOT_MARGIN = 48.0;    // 弾のmargin(画面外でも消されない余白)

    // --- 文字描画 ---
    constexpr int    NC_FONT_SIZE = 12;      // GDI+で描くフォントサイズ(px)
    constexpr double NC_SCALE = 3.0;     // 1ピクセルあたりの弾の間隔(px)

    // --- 時間 ---
    constexpr int NC_PHASE2_START = 600;
    constexpr int NC_PHASE3_START = 1500;
    constexpr int NC_CYCLE_LEN = 1800;

    // --- 速度 ---
    constexpr double NC_SPEED_P1 = 2.5;       // フェーズ1: 約3.5秒で画面を横断
    constexpr double NC_SPEED_P2 = 2.0;
    constexpr double NC_SPEED_P3 = 3.5;

    // --- フェーズ2の安全地帯 ---
    constexpr int    NC_GAP_SEG = 120;       // 安全地帯が次の行へ動く間隔(2秒)
    constexpr int    NC_GAP_RAMP = 40;        // 行移動にかける時間(この間は2行ぶん空く)
    constexpr int    NC_GAP_NSEG = 10;        // 安全地帯の経路テーブルの長さ
    constexpr double NC_GAP_HALF = 0.8;       // 安全地帯の中心から何行ぶん以内を空けるか

    // --- コメント同士の間隔(px) ---
    constexpr double NC_SPACING_MIN = 60.0;
    constexpr int    NC_SPACING_RAND = 60;

    // --- 弾の色(img_enemyShotSmallBall のインデックス) ---
    constexpr int NC_COL_RED = 0;
    constexpr int NC_COL_BLUE = 4;
    constexpr int NC_COL_WHITE = 6;

    // --- コメント文字列 ---
    const wchar_t* const NC_TEXT[] = {
        L"w",
        L"888888",
        L"うぽつ",
        L"かわいい",
        L"それな",
        L"草",
        L"神曲",
        L"wwwwwww",
        // 超長文コメント(「8」×27 = 画面幅いっぱい)
        L"888888888" L"888888888" L"888888888",
    };
    constexpr int NC_TEXT_NORMAL_NUM = 8;      // 通常コメントの数
    constexpr int NC_TEXT_LONG8 = 8;      // 超長文コメントのインデックス
    constexpr int NC_TEXT_NUM = 9;
}

// ============================================================
//  状態
// ============================================================
static int g_ncRowBusyUntil[NC_ROW_NUM];   // 各行に次のコメントを出せるようになる count
static int g_ncGapRow[NC_GAP_NSEG];        // フェーズ2の安全地帯の行(区間ごと)

// ============================================================
//  GDI+ 初期化
// ============================================================
static void NcInitGdiplus()
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
//  コメント(文字列→弾の座標リスト)
// ============================================================
struct NcPixel {
    int ox, oy;                 // 左端を0、縦は文字の中心を0とした整数座標(拡大前)
};

struct NcComment {
    std::vector<NcPixel> px;    // ox の昇順(左から順に並ぶ)
    double width = 0.0;         // 拡大後の横幅(px)
};

// 文字列を描画し、白ピクセルの座標を ox 昇順で返す
static NcComment NcBuildComment(const wchar_t* text)
{
    NcInitGdiplus();
    NcComment result;

    const int len = (int)wcslen(text);
    const int W = NC_FONT_SIZE * (len + 1);
    const int H = NC_FONT_SIZE * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    {
        Graphics g(&bitmap);
        g.Clear(Color(0, 0, 0, 0));
        // アンチエイリアスOFF(単色描画)
        g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        // MS ゴシックは小さいサイズで埋め込みビットマップが使われ、1pxの線できれいに出る
        FontFamily family(L"MS Gothic");
        const FontFamily* pFamily = &family;
        if (family.GetLastStatus() != Ok) pFamily = FontFamily::GenericSansSerif();
        Font font(pFamily, (REAL)NC_FONT_SIZE, FontStyleRegular, UnitPixel);
        g.DrawString(text, -1, &font, PointF(0, 0), &whiteBrush);
    }

    BitmapData bmpData;
    Rect rect(0, 0, W, H);
    bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
    const BYTE* argb = (const BYTE*)bmpData.Scan0;

    std::vector<std::pair<int, int>> white;
    int minX = W, maxX = -1, minY = H, maxY = -1;
    // x が外側のループ → x昇順に並ぶ
    for (int x = 0; x < W; ++x) {
        for (int y = 0; y < H; ++y) {
            const int idx = y * bmpData.Stride + x * 4;
            if (argb[idx + 3] == 255) {
                white.emplace_back(x, y);
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }
    bitmap.UnlockBits(&bmpData);

    if (white.empty()) return result;

    const int cy = (minY + maxY) / 2;
    result.px.reserve(white.size());
    for (const auto& p : white) {
        result.px.push_back({ p.first - minX, p.second - cy });
    }
    result.width = (maxX - minX + 1) * NC_SCALE;
    return result;
}

// 全コメントのキャッシュ(初回アクセス時に生成)
static const NcComment& NcGetComment(int idx)
{
    static std::vector<NcComment> cache;
    if (cache.empty()) {
        cache.reserve(NC_TEXT_NUM);
        for (int i = 0; i < NC_TEXT_NUM; i++) {
            cache.push_back(NcBuildComment(NC_TEXT[i]));
        }
    }
    return cache[idx];
}

// ============================================================
//  効果音
// ============================================================
static void NcPlaySe(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ============================================================
//  弾幕：コメント1本ぶん
//    ShotSet の使い方
//      x, y        : コメントの左端X(初期値)と行の中心Y
//      muki        : 進行方向(左 = π)
//      kind        : コメント文字列のインデックス
//      param_i[0]  : 次に生成する弾の番号
//      param_i[1]  : 弾の色
//      param_d[0]  : 速度
// ============================================================
static void NcShotComment(sEnemyShotSet* pSet)
{
    const NcComment& cd = NcGetComment(pSet->kind);
    const double v = pSet->param_d[0];
    const double traveled = v * pSet->count;   // コメントがここまでに進んだ距離
    const int total = (int)cd.px.size();

    // 画面右端(NC_SPAWN_X)に差しかかった列の弾を順に生成
    while (pSet->param_i[0] < total && cd.px[pSet->param_i[0]].ox * NC_SCALE <= traveled) {
        const NcPixel& p = cd.px[pSet->param_i[0]];

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x + p.ox * NC_SCALE - traveled;
        pShot->y = pSet->y + p.oy * NC_SCALE;
        pShot->muki = pSet->muki;
        pShot->speed = v;
        pShot->kind = img_enemyShotSmallBall[pSet->param_i[1]];
        pShot->margin = NC_SHOT_MARGIN;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;

        pSet->param_i[0]++;
    }

    // 移動
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// コメント1本を発射(ShotSetを作ってリストに繋ぐ)
static void NcSpawnComment(int text, int row, int color, double speed)
{
    if (NcGetComment(text).px.empty()) return;   // 文字の描画に失敗した場合は何もしない

    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = NcShotComment;
    pSet->x = NC_SPAWN_X;
    pSet->y = NC_ROW_Y0 + NC_ROW_PITCH * row;
    pSet->muki = DX_PI;
    pSet->kind = text;
    pSet->param_i[0] = 0;
    pSet->param_i[1] = color;
    pSet->param_d[0] = speed;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// コメントが画面を横断し終えるまで、その行を占有する時間(フレーム)
static int NcFullTravelFrames(double width, double speed)
{
    return (int)((NC_SPAWN_X + width + NC_SHOT_MARGIN) / speed) + 1;
}

// ============================================================
//  フェーズ2の安全地帯
// ============================================================
// フェーズ2開始から t フレーム後の安全地帯の行(小数。行移動中は途中の値)
static double NcGapRowAt(double t)
{
    if (t < 0.0) t = 0.0;
    int s = (int)(t / NC_GAP_SEG);
    double local = t - (double)s * NC_GAP_SEG;
    if (s >= NC_GAP_NSEG) {
        s = NC_GAP_NSEG - 1;
        local = NC_GAP_SEG;
    }
    if (s > 0 && local < NC_GAP_RAMP) {
        return g_ncGapRow[s - 1] + (g_ncGapRow[s] - g_ncGapRow[s - 1]) * local / NC_GAP_RAMP;
    }
    return g_ncGapRow[s];
}

// 行 row のコメントが画面中央を通過する時間帯 [tStart, tEnd] の間に、
// その行が安全地帯になっているか
static bool NcRowBlockedByGap(int row, double tStart, double tEnd)
{
    for (int k = 0; k <= 4; k++) {
        const double t = tStart + (tEnd - tStart) * k / 4.0;
        if (fabs((double)row - NcGapRowAt(t)) < NC_GAP_HALF) return true;
    }
    return false;
}

// 安全地帯の経路を決める(最初はプレイヤーのいる行から、以降は±1〜2行ずつ動く)
static void NcMakeGapTable()
{
    int r0 = (int)floor((player.y - NC_ROW_Y0) / NC_ROW_PITCH + 0.5);
    if (r0 < 0) r0 = 0;
    if (r0 > NC_ROW_NUM - 1) r0 = NC_ROW_NUM - 1;
    g_ncGapRow[0] = r0;

    for (int s = 1; s < NC_GAP_NSEG; s++) {
        const int mag = 1 + GetRand(1);                 // 1 or 2
        const int sgn = (GetRand(1) == 0) ? -1 : 1;     // 上 or 下
        int next = g_ncGapRow[s - 1] + mag * sgn;
        if (next < 0 || next >= NC_ROW_NUM) next = g_ncGapRow[s - 1] - mag * sgn;
        g_ncGapRow[s] = next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_NicoNico_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.maxHp = enemy.hp = 300; // 200で固定
        for (int i = 0; i < NC_ROW_NUM; i++) g_ncRowBusyUntil[i] = 0;
        NcGetComment(0);              // 文字データを先に作っておく(初回の処理落ち防止)
    }

    // 本体は上部でゆっくり左右に揺れる(コメント主のイメージ)
    enemy.x = 240.0 + 90.0 * sin(count * DX_PI * 2.0 / 480.0);
    enemy.y = 40.0;

    const int cyc = (count - 1) % NC_CYCLE_LEN;   // 30秒周期の中での経過フレーム

    // ------------------------------------------------------------
    //  フェーズ3：「888888888」 超長文コメントが上下交互に高速で来る
    // ------------------------------------------------------------
    if (cyc >= NC_PHASE3_START) {
        const int ph3 = cyc - NC_PHASE3_START;

        // 予告音(0.5秒後に発射)
        if (ph3 == 0) NcPlaySe(sound_enemyCharge);

        if (ph3 % 30 == 29) {
            const int k = ph3 / 30;
            const int half = NC_ROW_NUM / 2;
            const double w = NcGetComment(NC_TEXT_LONG8).width;

            for (int t = 0; t < 6; t++) {
                // 偶数本目は上半分、奇数本目は下半分の行から
                const int row = (k % 2 == 0) ? GetRand(half - 1) : half + GetRand(half - 1);
                if (count < g_ncRowBusyUntil[row]) continue;

                NcSpawnComment(NC_TEXT_LONG8, row, NC_COL_WHITE, NC_SPEED_P3);
                g_ncRowBusyUntil[row] = count + NcFullTravelFrames(w, NC_SPEED_P3);
                NcPlaySe(sound_enemyShot_medium);
                break;
            }
        }
    }
    // ------------------------------------------------------------
    //  フェーズ2：コメント渋滞 (全行が埋まり、1行分だけ安全地帯が動く)
    // ------------------------------------------------------------
    else if (cyc >= NC_PHASE2_START) {
        const int ph2 = cyc - NC_PHASE2_START;
        const double v = NC_SPEED_P2;

        if (ph2 == 0) {
            NcMakeGapTable();
            // 全行が同じタイミングで始まって壁にならないよう、出始めをずらす
            for (int r = 0; r < NC_ROW_NUM; r++) {
                const int d = count + GetRand(60);
                if (g_ncRowBusyUntil[r] < d) g_ncRowBusyUntil[r] = d;
            }
        }
        if (ph2 % 30 == 0) NcPlaySe(sound_enemyShot_light);

        for (int r = 0; r < NC_ROW_NUM; r++) {
            if (count < g_ncRowBusyUntil[r]) continue;

            const int text = GetRand(NC_TEXT_NORMAL_NUM - 1);
            const double w = NcGetComment(text).width;

            // このコメントが画面中央を通る時間帯に、この行が安全地帯なら出さない
            const double tStart = ph2 + (NC_SPAWN_X - NC_CENTER_X) / v;
            const double tEnd = tStart + w / v;
            if (NcRowBlockedByGap(r, tStart, tEnd)) {
                g_ncRowBusyUntil[r] = count + 4;   // 少し待って別の文字で再挑戦
                continue;
            }

            const int color = (GetRand(7) == 0) ? NC_COL_BLUE : NC_COL_WHITE;
            NcSpawnComment(text, r, color, v);
            g_ncRowBusyUntil[r] = count + (int)((w + NC_SPACING_MIN + GetRand(NC_SPACING_RAND)) / v) + 1;
        }

        // 渋滞の始まりの予告音(フェーズ2に入る直前はフェーズ1側で鳴らす)
    }
    // ------------------------------------------------------------
    //  フェーズ1：ぽつぽつ流れる
    // ------------------------------------------------------------
    else {
        if (cyc == NC_PHASE2_START - 30) NcPlaySe(sound_enemyCharge);

        if (cyc % 30 == 0) {
            for (int t = 0; t < 4; t++) {
                const int row = GetRand(NC_ROW_NUM - 1);
                if (count < g_ncRowBusyUntil[row]) continue;

                const int text = GetRand(NC_TEXT_NORMAL_NUM - 1);
                // 1/6 の確率で赤コメ(運営コメ)：高速
                const bool isRed = (GetRand(5) == 0);
                const int color = isRed ? NC_COL_RED : NC_COL_WHITE;
                const double v = isRed ? NC_SPEED_P1 * 1.6 : NC_SPEED_P1;

                NcSpawnComment(text, row, color, v);
                g_ncRowBusyUntil[row] = count + NcFullTravelFrames(NcGetComment(text).width, v);
                NcPlaySe(sound_enemyShot_light);
                break;
            }
        }
    }
}