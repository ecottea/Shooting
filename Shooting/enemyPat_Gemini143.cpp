#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <map>
#include <string>
#include <cmath>

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
//  文字列を画像化してピクセル座標（弾の配置位置）を抽出
// ============================================================
static std::vector<std::pair<double, double>> SampleTextPixelsStrMulti(const std::wstring& text, int size)
{
    InitGdiplusOnce();

    const int W = size * (int)text.length() * 2;
    const int H = size * 3;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0)); // 透明でクリア

    // アンチエイリアスOFF（くっきりしたドットの集合にするため）
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic UI");
    Font font(&fontFamily, (REAL)size, FontStyleBold, UnitPixel);
    g.DrawString(text.c_str(), -1, &font, PointF(0, 0), &whiteBrush);

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
                // 弾数の爆発を防ぎつつ、弾と弾の隙間をプレイヤーが抜けられるように
                // 市松模様（x + y が偶数）で間引いてサンプリングする
                if ((x + y) % 2 == 0) {
                    whitePixels.emplace_back(x, y);
                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                }
            }
        }
    }
    bitmap.UnlockBits(&bmpData);

    std::vector<std::pair<double, double>> result;
    if (!whitePixels.empty()) {
        double cx = (minX + maxX) / 2.0;
        double cy = (minY + maxY) / 2.0;
        result.reserve(whitePixels.size());
        // 重心を原点(0,0)とした相対座標に変換
        for (const auto& p : whitePixels) {
            result.emplace_back(p.first - cx, p.second - cy);
        }
    }
    return result;
}

// ============================================================
//  文字列ピクセルデータのキャッシュ（毎フレームの重い描画処理を回避）
// ============================================================
static const std::vector<std::pair<double, double>>& GetOffsetsText(const std::wstring& text, int size)
{
    static std::map<std::wstring, std::vector<std::pair<double, double>>> cache;
    std::wstring key = text + L"_" + std::to_wstring(size);
    if (cache.find(key) == cache.end()) {
        cache[key] = SampleTextPixelsStrMulti(text, size);
    }
    return cache[key];
}

// ============================================================
//  sEnemyShotSet 生成用ヘルパー関数
// ============================================================
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc patternFunc, double x, double y, double muki, double speed, int kind, int textId)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = patternFunc;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->param_d[0] = speed;
    pSet->kind = kind;
    pSet->param_i[0] = textId;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// ============================================================
//  弾幕パターン：横スクロールするテキスト群
// ============================================================
static void ShotNicoText(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        std::wstring textStr = L"";
        int fontSize = 24;
        double scale = 2.0;

        // pSet->param_i[0] で文字の種類を判定
        switch (pSet->param_i[0]) {
        case 0: textStr = L"うぽつ"; scale = 1.5; break;
        case 1: textStr = L"88888"; scale = 1.5; break;
        case 2: textStr = L"わくわく"; scale = 1.5; break;
        case 3: textStr = L"ｷﾀ━(ﾟ∀ﾟ)━!"; scale = 1.5; break;
        case 5: textStr = L"w"; scale = 1.0; fontSize = 16; break; // 高速用の小さい w
        case 6: textStr = L"W"; scale = 4.0; fontSize = 32; break; // 巨大な壁の W
        case 7: textStr = L"草"; scale = 2.0; fontSize = 24; break;
        }

        // 弾の大きさに合わせてタイピング音/発射音を鳴らし分ける
        if (pSet->param_i[0] == 5) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        else if (pSet->param_i[0] == 6) {
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        else {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        const auto& offsets = GetOffsetsText(textStr, fontSize);

        // 弾を生成してリストに繋ぐ
        for (auto& p : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x + p.first * scale;
            shot->y = pSet->y + p.second * scale;
            shot->muki = pSet->muki;
            shot->speed = pSet->param_d[0]; // 設定された速度を代入
            shot->kind = pSet->kind;
            shot->margin = 480;

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
    }

    // 弾の移動処理
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        if (p->x < -20) p->margin = -999;
        p = p->next;
    }
}

// ============================================================
//  弾幕パターン：途中で減速・滞留する赤字の警告テキスト
// ============================================================
static void ShotNicoTextWarning(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 出現する高さ(y)によってテキストを分岐
        std::wstring textStr = (pSet->y < 240) ? L"※上部弾幕注意" : L"※下部弾幕注意";
        int fontSize = 24;
        double scale = 2.0*3;

        const auto& offsets = GetOffsetsText(textStr, fontSize);

        for (auto& p : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x + p.first * scale;
            shot->y = pSet->y + p.second * scale;
            shot->muki = pSet->muki;
            shot->speed = pSet->param_d[0];
            shot->kind = pSet->kind;
            shot->margin = 600;

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
    }

    // 弾の移動処理（徐々に減速して画面内に滞留させる）
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (pSet->count < 60) {
            p->speed -= 0.08;
            if (p->speed < 0.25) p->speed = 0.25; // 完全に止めず、ごく僅かに左へ流し続ける
        }
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  敵本体のパターン関数（メインルーチンから呼ばれる）
// ============================================================
void EnemyPat_NicoNico_Gemini()
{
    // 初期化（敵本体は画面の右端外に鎮座させ、そこからコメントを流す）
    if (count == 1) {
        enemy.x = 520.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 2500;
    }
    enemy.hp--;

    // ----------------------------------------------------
    // フェーズ1：挨拶コメ (60 〜 700フレーム)
    // ----------------------------------------------------
    if (count >= 60 && count < 700) {
        // 45フレームに1回、ランダムな高さから白いテキストを流す
        if (count % 23 == 0) {
            double startY = 40.0 + GetRand(400);
            double speed = 2.0 + GetRand(100) / 100.0;
            int textId = GetRand(3); // 0, 1, 2, 3 (うぽつ、8888、わくわく、ｷﾀ━(ﾟ∀ﾟ)━!)

            CreateShotSet(ShotNicoText, 550.0, startY, DX_PI, speed, img_enemyShotSmallBall[6], textId);
        }
    }

    // ----------------------------------------------------
    // フェーズ2：赤字職人のエリア制限 (700フレームで予告、760と820で発射)
    // ----------------------------------------------------
    if (count == 700) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // ピピピッという予告音
    }
    if (count == 760) { // 上部を塞ぐ赤字
        CreateShotSet(ShotNicoTextWarning, 600.0, 80.0, DX_PI, 5.0, img_enemyShotSmallBall[0], 4);
    }
    if (count == 820) { // 下部を塞ぐ赤字
        CreateShotSet(ShotNicoTextWarning, 600.0, 400.0, DX_PI, 5.0, img_enemyShotSmallBall[0], 4);
    }

    // ----------------------------------------------------
    // フェーズ3：「ｗｗｗ」流星群 (1000 〜 2500フレーム)
    // ----------------------------------------------------
    if (count >= 1000 && count < 2500) {
        // 時間経過とともに発射間隔を短くし、密度を上げる
        int interval = 25 - (count - 1000) / 75;
        if (interval < 4) interval = 4;

        if (count % interval == 0) {
            // 赤字テキストの隙間（プレイヤーの回避エリア y=120〜360付近）を集中して狙う
            double startY = 120.0 + GetRand(240);

            int randType = GetRand(100);
            if (randType < 15) {
                // 巨大な「W」（15%）：低速で壁のように迫る緑の中楕円弾
                double speed = 1.0 + GetRand(10) / 10.0;
                CreateShotSet(ShotNicoText, 550.0, startY, DX_PI, speed, img_enemyShotMediumOval[2], 6);
            }
            else if (randType < 75) {
                // 小さい「w」（60%）：高速で隙間を突き抜ける緑の小玉
                double speed = 4.0 + GetRand(30) / 10.0;
                CreateShotSet(ShotNicoText, 550.0, startY, DX_PI, speed, img_enemyShotSmallBall[2], 5);
            }
            else {
                // 「草」（25%）：標準的な速度の緑の菱形弾
                double speed = 2.5 + GetRand(15) / 10.0;
                CreateShotSet(ShotNicoText, 550.0, startY, DX_PI, speed, img_enemyShotDiamond[2], 7);
            }
        }
    }
}