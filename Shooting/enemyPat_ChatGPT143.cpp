#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <utility>
#include <cmath>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

static void InitGdiplusOnce()
{
    static bool initialized = false;
    static ULONG_PTR token = 0;
    if (!initialized) {
        GdiplusStartupInput gdiInput;
        GdiplusStartup(&token, &gdiInput, nullptr);
        initialized = true;
    }
}

static std::vector<std::pair<double, double>> SampleCommentPixels(const wchar_t* text, int fontSize, int cellSize)
{
    InitGdiplusOnce();

    const int W = fontSize * 2 * 10 + 32;
    const int H = fontSize * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic UI Light");
    Font font(&fontFamily, (REAL)fontSize, FontStyleRegular, UnitPixel);
    g.DrawString(text, -1, &font, PointF(4.0f, 0.0f), &whiteBrush);

    BitmapData bmpData;
    Rect rect(0, 0, W, H);
    bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);

    BYTE* argb = (BYTE*)bmpData.Scan0;
    int minX = W, maxX = -1, minY = H, maxY = -1;

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const int idx = y * bmpData.Stride + x * 4;
            const BYTE a = argb[idx + 3];
            if (a == 255) {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }

    std::vector<std::pair<double, double>> result;

    if (maxX >= 0) {
        const double cx = (minX + maxX) / 2.0;
        const double cy = (minY + maxY) / 2.0;

        for (int y0 = minY; y0 <= maxY; y0 += cellSize) {
            for (int x0 = minX; x0 <= maxX; x0 += cellSize) {
                bool hit = false;

                for (int y = y0; y < y0 + cellSize && y <= maxY && !hit; ++y) {
                    for (int x = x0; x < x0 + cellSize && x <= maxX; ++x) {
                        const int idx = y * bmpData.Stride + x * 4;
                        if (argb[idx + 3] == 255) {
                            hit = true;
                            break;
                        }
                    }
                }

                if (hit) {
                    result.emplace_back(
                        x0 + cellSize * 0.5 - cx,
                        y0 + cellSize * 0.5 - cy
                    );
                }
            }
        }
    }

    bitmap.UnlockBits(&bmpData);
    return result;
}

static const std::vector<std::pair<double, double>>& GetCommentOffsets(int textId)
{
    static std::vector<std::pair<double, double>> offsets[7];
    static bool initialized[7] = {};

    static const wchar_t* texts[7] = {
        L"ｗｗｗｗｗ",
        L"88888888",
        L"それな",
        L"草",
        L"キターーー！",
        L"コメント弾幕",
        L"弾幕濃いよ！"
    };

    if (!initialized[textId]) {
        offsets[textId] = SampleCommentPixels(texts[textId], 28, 3);
        initialized[textId] = true;
    }

    return offsets[textId];
}

static void ShotComment(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count != 0) {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            p = p->next;
        }
        return;
    }

    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

    const int textId = pEnemyShotSet->param_i[0];
    const int colorId = pEnemyShotSet->param_i[1];
    const double scale = pEnemyShotSet->param_d[0];
    const double speed = pEnemyShotSet->param_d[1];

    const auto& offsets = GetCommentOffsets(textId);

    for (const auto& offset : offsets) {
        sEnemyShot* shot = new sEnemyShot;

        shot->x = pEnemyShotSet->x + offset.first * scale;
        shot->y = pEnemyShotSet->y + offset.second * scale;
        shot->muki = DX_PI;
        shot->speed = speed;
        shot->kind = img_enemyShotSmallBall[colorId];
        shot->margin = 480.0;

        shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        shot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = shot;
        pEnemyShotSet->pEnemyShotHead->prev = shot;
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

static void SpawnComment(
    double x,
    double y,
    int textId,
    int colorId,
    double scale,
    double speed)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotComment;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = DX_PI;
    pEnemyShotSet->kind = textId;
    pEnemyShotSet->param_i[0] = textId;
    pEnemyShotSet->param_i[1] = colorId;
    pEnemyShotSet->param_d[0] = scale;
    pEnemyShotSet->param_d[1] = speed;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

void EnemyPat_NicoNico_ChatGPT()
{
    static int moveDir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 42.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
    }
    else {
        enemy.x += 1.15 * (double)moveDir;

        if (enemy.x < 105.0) {
            enemy.x = 105.0;
            moveDir = 1;
        }
        if (enemy.x > 375.0) {
            enemy.x = 375.0;
            moveDir = -1;
        }
    }

    if (count % 32 == 0) {
        const int wave = (count / 32) % 7;
        const int lane = (count / 32) % 10;

        double y = 65.0 + lane * 43.0;
        if (y > 460.0) y = 460.0;

        const int colorTable[7] = { 6, 6, 3, 6, 1, 5, 6 };
        const double speedTable[3] = { 1.8, 2.3, 2.8 };

        SpawnComment(
            560.0,
            y,
            wave,
            colorTable[wave],
            (wave == 3) ? 2.2 : 1.45,
            speedTable[(count / 42) % 3]
        );
    }

    if (count % 168 == 84) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        SpawnComment(585.0, 112.0, 1, 1, 1.55, 2.65);
        SpawnComment(585.0, 238.0, 2, 3, 1.55, 2.25);
        SpawnComment(585.0, 364.0, 4, 5, 1.55, 2.95);
    }

    if (count % 336 == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        SpawnComment(610.0, 240.0, 5, 5, 2.35, 1.55);
    }
}
