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
static void InitGdiplusOnce_Tmp()
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
//  指定文字を32pxで描画し、白ピクセル座標をバウンディングボックス
//  中心基準に変換して返す
// ============================================================
static std::vector<std::pair<double, double>> SampleCharOffsets_Tmp(const wchar_t* str)
{
    InitGdiplusOnce_Tmp();

    const int SIZE = 32;
    const int W = SIZE * 2;
    const int H = SIZE * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic");
    Font font(&fontFamily, (REAL)SIZE, FontStyleRegular, UnitPixel);
    g.DrawString(str, -1, &font, PointF(0, 0), &whiteBrush);

    BitmapData bmpData;
    Rect rect(0, 0, W, H);
    bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
    BYTE* argb = (BYTE*)bmpData.Scan0;

    std::vector<std::pair<int, int>> whitePixels;
    int minX = W, maxX = -1, minY = H, maxY = -1;

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int idx = y * bmpData.Stride + x * 4;
            if (argb[idx + 3] == 255) {
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
//  文字ごとのピクセルオフセットキャッシュ
//  role: 0=へ, 1=の, 2=も, 3=じ
// ============================================================
static const std::vector<std::pair<double, double>>& GetCharOffsets_Tmp(int role)
{
    static std::vector<std::pair<double, double>> cache[4];
    static bool initialized[4] = { false, false, false, false };

    if (!initialized[role]) {
        const wchar_t* chars[4] = { L"へ", L"の", L"も", L"じ" };
        cache[role] = SampleCharOffsets_Tmp(chars[role]);
        initialized[role] = true;
    }
    return cache[role];
}

// ============================================================
//  弾を1つ生成してリスト末尾に繋ぐ
// ============================================================
static void SpawnShot_Tmp(sEnemyShotSet* set,
    double x, double y,
    double muki, double speed,
    int kind, int roleTag)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = 0;         // 0=描画中（停止）, 1=バースト済み
    p->param_i[1] = roleTag;   // 文字種/バースト種タグ
    p->param_d[0] = x;         // 生成位置
    p->param_d[1] = y;

    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
}

// ============================================================
//  弾幕：へのへのもへじ
//  0〜479F: 文字を順次描画（弾は停止）
//  480F   : 顔完成 → 全弾放射バースト＋目のリング・口の扇弾
//  480+20F: じの濁点2発目
// ============================================================
static void ShotHenonemoheji_Tmp(sEnemyShotSet* pEnemyShotSet)
{
    constexpr double CX = 240.0;               // 顔中心X
    constexpr double CY = 230.0;               // 顔中心Y
    constexpr int    BURST_FRAME = 480/2;
    constexpr int    CHEEK_DELAY = 20;

    struct DrawStep {
        int    frame;
        int    role;   // 0=へ, 1=の, 2=も, 3=じ
        double x, y;
        double scale;
        int    color;  // 0:赤,1:黄,2:緑,3:シアン,4:青,5:マゼンタ,6:白,7:黒,8:橙
    };
    static const DrawStep steps[] = {
        {   0, 0, 160.0, 140.0, 1.8, 6 },  // 左眉 へ (白の銃弾)
        {  60, 1, 160.0, 205.0, 2.0, 3 },  // 左目 の (シアンの小玉)
        { 120, 0, 320.0, 140.0, 1.8, 6 },  // 右眉 へ
        { 180, 1, 320.0, 205.0, 2.0, 3 },  // 右目 の
        { 240, 2, 240.0, 250.0, 1.8, 6 },  // 鼻   も (白の銃弾)
        { 300, 0, 240.0, 315.0, 2.2, 6 },  // 口   へ
        { 360, 3, 390.0, 260.0, 1.6, 5 },  // 頬   じ (マゼンタの小玉)
    };

    int c = pEnemyShotSet->count;

    // ---------- 描画フェーズ ----------
    for (const auto& st : steps) {
        if (c == st.frame/2) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            const auto& offs = GetCharOffsets_Tmp(st.role);

            int kind;
            if (st.role == 0 || st.role == 2) {
                kind = img_enemyShotBullet[st.color];      // へ・も : 銃弾（直線筆跡）
            }
            else {
                kind = img_enemyShotSmallBall[st.color];   // の・じ : 小玉（曲線）
            }

            // 3ピクセルおきに間引いてプール圧迫を防止
            for (size_t i = 0; i < offs.size(); i += 1) {
                double px = st.x + offs[i].first * st.scale;
                double py = st.y + offs[i].second * st.scale;
                SpawnShot_Tmp(pEnemyShotSet, px, py,
                    0.0, 0.0, kind, st.role);
            }
        }
    }

    // ---------- バーストフェーズ ----------
    if (c == BURST_FRAME) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 1) 描画済み全弾を顔中心から放射状に加速
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) {
                double dx = p->x - CX;
                double dy = p->y - CY;
                p->muki = atan2(dy, dx) + (GetRand(100) - 50) * 0.008;
                p->speed = 2.5 + (GetRand(100) - 50) * 0.02;
                p->param_i[0] = 1;
            }
            p = p->next;
        }

        // 2) 両目から8方向リング弾（中玉・シアン）
        for (int eye = 0; eye < 2; ++eye) {
            double ex = (eye == 0) ? 160.0 : 320.0;
            double ey = 205.0;
            for (int i = 0; i < 8*3; ++i) {
                SpawnShot_Tmp(pEnemyShotSet, ex, ey,
                    i * DX_PI * 2.0 / 8.0/3,
                    2.0,
                    img_enemyShotMediumBall[3],
                    10);
            }
        }

        // 3) 口から自機狙い5方向扇弾（銃弾・赤）
        double aim = atan2(player.y - 315.0, player.x - 240.0);
        for (int i = -2-2; i <= 2+2; ++i) {
            SpawnShot_Tmp(pEnemyShotSet, 240.0, 315.0,
                aim + i * 0.18,
                2.2,
                img_enemyShotBullet[0],
                20);
        }

        // 4) じの濁点1発目（菱形弾・マゼンタ）
        double aimC = atan2(player.y - 260.0, player.x - 390.0);
        SpawnShot_Tmp(pEnemyShotSet, 390.0, 260.0, aimC, 2.0,
            img_enemyShotDiamond[5], 30);
    }

    // ---------- じの濁点2発目（遅延発射） ----------
    if (c == BURST_FRAME + CHEEK_DELAY) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double aimC = atan2(player.y - 260.0, player.x - 400.0);
        SpawnShot_Tmp(pEnemyShotSet, 400.0, 260.0, aimC, 2.2,
            img_enemyShotDiamond[5], 30);
    }

    // ---------- 位置更新（停止弾は speed=0 なので動かない） ----------
    sEnemyShot* q = pEnemyShotSet->pEnemyShotHead->next;
    while (q != pEnemyShotSet->pEnemyShotHead) {
        q->x += q->speed * cos(q->muki);
        q->y += q->speed * sin(q->muki);
        q = q->next;
    }
}

// ============================================================
//  敵本体
// ============================================================
void EnemyPat_Henohenomoheji_DeepSeek()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 画面上部中央で静止（弾幕の「顔」はその下に描画される）
    enemy.x = 240.0;
    enemy.y = 40.0;

    if (count % 300 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHenonemoheji_Tmp;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}