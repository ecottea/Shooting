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
//   GDI+ 初期化
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
//   指定された1文字を描画し、その文字中心を原点とした白ピクセル座標を返す
// ============================================================
static std::vector<std::pair<double, double>> SampleSingleCharPixels(wchar_t ch)
{
    InitGdiplusOnce();

    const int SIZE = 26;          // フォントサイズ
    const int W = SIZE * 2;
    const int H = SIZE * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));    // 透明でクリア

    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic UI Light");
    Font font(&fontFamily, (REAL)SIZE, FontStyleRegular, UnitPixel);

    wchar_t s[2] = { ch, L'\0' };
    g.DrawString(s, -1, &font, PointF(0, 0), &whiteBrush);

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

    // 文字の中心を原点に変換
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
//   "へのへのもへじ" の各文字を人の顔のパーツ配置に合成する
// ============================================================
struct FacePartInfo {
    wchar_t ch;
    double partX;
    double partY;
};

static std::vector<std::pair<double, double>> GenerateFacePixels()
{
    // "へのへのもへじ" の各文字を人の顔に見える位置に配置
    FacePartInfo parts[7] = {
        { L'へ', -35.0, -40.0 }, // 1文字目：「へ」 (左眉)
        { L'の', -25.0, -15.0 }, // 2文字目：「の」 (左目)
        { L'へ',  35.0, -40.0 }, // 3文字目：「へ」 (右眉)
        { L'の',  25.0, -15.0 }, // 4文字目：「の」 (右目)
        { L'も',   0.0,  10.0 }, // 5文字目：「も」 (鼻)
        { L'へ',   0.0,  35.0 }, // 6文字目：「へ」 (口)
        { L'じ',   0.0,  60.0 }  // 7文字目：「じ」 (あご・輪郭)
    };

    std::vector<std::pair<double, double>> allPixels;
    for (int i = 0; i < 7; ++i) {
        auto charPixels = SampleSingleCharPixels(parts[i].ch);
        for (const auto& p : charPixels) {
            allPixels.emplace_back(p.first + parts[i].partX, p.second + parts[i].partY);
        }
    }
    return allPixels;
}

// ============================================================
//   顔パーツのオフセットリスト（初回アクセス時に自動生成）
// ============================================================
static const std::vector<std::pair<double, double>>& GetFaceOffsets()
{
    static std::vector<std::pair<double, double>> offsets;
    static bool initialized = false;
    if (!initialized) {
        offsets = GenerateFacePixels();
        initialized = true;
    }
    return offsets;
}

// ============================================================
//   弾幕パターン：人の顔の形をした「へのへのもへじ」展開＆解体アタック
// ============================================================
static void ShotFormationHenohenomohejiFace(sEnemyShotSet* pEnemyShotSet)
{
    constexpr double SCALE = 1.2*2;  // 全体の拡大率

    if (pEnemyShotSet->count == 0) {
        // チャージ・予告音の再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const auto& offsets = GetFaceOffsets();

        for (size_t i = 0; i < offsets.size(); ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + offsets[i].first * SCALE;
            shot->y = pEnemyShotSet->y + offsets[i].second * SCALE;

            shot->muki = 0.0;
            shot->speed = 0.0; // 最初はその場で静止

            // 素材の選択：基本は白小玉(6)、一定間隔で赤小玉(0)を混ぜてアクセントに
            // 弾の色一覧: 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
            if (i % 25 == 0) {
                shot->kind = img_enemyShotSmallBall[0]; // 赤
            }
            else {
                shot->kind = img_enemyShotSmallBall[6]; // 白
            }

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    // 【状態遷移】
    // 登場から約1秒間(60フレーム)は顔の形で静止してプレッシャーを与える
    if (pEnemyShotSet->count == 60) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            double angleToPlayer = atan2(player.y - p->y, player.x - p->x);
            // 自機方向をベースに、GetRand(40)（0〜40の41種類）を使って±20度のバラつきを持たせる
            p->muki = angleToPlayer + (GetRand(40) - 20) / 180.0 * DX_PI;
            // 速度は 1.5 〜 2.5 の間でランダム生成
            p->speed = (150 + GetRand(100)) / 100.0;
            p = p->next;
        }
    }

    // 弾の移動処理（speed が 0 の間はその場に静止、60フレーム目以降は移動）
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//   敵本体のパターン
// ============================================================
void EnemyPat_Henohenomoheji_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        enemy.x += 0.8 * (double)muki;
        if (enemy.x < 100.0 || enemy.x > 380.0) {
            muki *= -1;
        }
    }

    // 一定周期（180フレーム毎）で弾幕を展開
    if (count % 150 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFormationHenohenomohejiFace;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;
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