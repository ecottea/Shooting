#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>

#pragma comment(lib,"gdiplus.lib")

using namespace Gdiplus;

//=========================================================
// GDI+ 初期化
//=========================================================

static void InitGdiplusOnce()
{
    static bool initialized = false;

    if (!initialized)
    {
        GdiplusStartupInput input;
        ULONG_PTR token;
        GdiplusStartup(&token, &input, nullptr);
        initialized = true;
    }
}

//=========================================================
// 文字輪郭サンプリング
//=========================================================

static std::vector<std::pair<double, double>>
SampleTextOutline(
    const wchar_t* text,
    const wchar_t* fontName,
    REAL fontSize,
    double interval)
{
    InitGdiplusOnce();

    FontFamily font(fontName);

    GraphicsPath path;
    StringFormat format;

    path.AddString(
        text,
        -1,
        &font,
        FontStyleRegular,
        fontSize,
        PointF(0, 0),
        &format);

    path.Flatten(nullptr, 0.25f);

    int pointCount = path.GetPointCount();

    if (pointCount < 2)
        return {};

    std::vector<PointF> pts(pointCount);
    std::vector<BYTE> types(pointCount);

    path.GetPathPoints(pts.data(), pointCount);
    path.GetPathTypes(types.data(), pointCount);

    RectF bounds;
    path.GetBounds(&bounds);

    double cx = bounds.X + bounds.Width * 0.5;
    double cy = bounds.Y + bounds.Height * 0.5;

    std::vector<std::pair<double, double>> result;

    PointF start = pts[0];
    PointF prev = pts[0];

    double remain = 0.0;

    auto SampleLine =
        [&](const PointF& next)
    {
        double dx = next.X - prev.X;
        double dy = next.Y - prev.Y;

        double len = sqrt(dx * dx + dy * dy);

        if (len > 0.0)
        {
            double ux = dx / len;
            double uy = dy / len;

            double pos = interval - remain;

            while (pos <= len)
            {
                result.emplace_back(
                    prev.X + ux * pos - cx,
                    prev.Y + uy * pos - cy);

                pos += interval;
            }

            remain = len - (pos - interval);
        }

        prev = next;
    };

    for (int i = 0; i < pointCount; i++)
    {
        BYTE type = types[i];
        BYTE pathType =
            type & PathPointTypePathTypeMask;

        if (pathType == PathPointTypeStart)
        {
            start = pts[i];
            prev = pts[i];

            result.emplace_back(
                pts[i].X - cx,
                pts[i].Y - cy);

            remain = 0.0;
        }
        else
        {
            SampleLine(pts[i]);
        }

        if (type & PathPointTypeCloseSubpath)
        {
            SampleLine(start);
        }
    }

    return result;
}

//=========================================================
// 使用する文字
//=========================================================

static std::vector<std::pair<double, double>>&
GetOffsets(int id)
{
    static bool initialized = false;

    static std::vector<std::pair<double, double>>
        offsets[8];

    if (!initialized)
    {
        const wchar_t* text[] =
        {
            L"夢",
            L"幻",
            L"弾",
            L"幕",
            L"東",
            L"方",
            L"星",
            L"華"
        };

        constexpr REAL fontSize = 340.0f;
        constexpr double interval = 16.0;

        const wchar_t* font =
            L"HGSoeiKakupoptai";

        for (int i = 0; i < 8; i++)
        {
            offsets[i] =
                SampleTextOutline(
                    text[i],
                    font,
                    fontSize,
                    interval);
        }

        initialized = true;
    }

    return offsets[id & 7];
}

//=========================================================
// 文字を書く→渦→崩壊
//=========================================================

static void ShotText(sEnemyShotSet* pSet)
{
    const auto& offsets = GetOffsets(pSet->kind);

    //-------------------------------------------------
    // Phase0 初期化
    //-------------------------------------------------

    if (pSet->count == 0)
    {
        pSet->param_i[0] = 0;      // phase
        pSet->param_i[1] = 0;      // 次に生成する点
        pSet->param_i[2] = (int)offsets.size();
        pSet->param_i[3] = 0;

        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);

        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    //-------------------------------------------------
    // Phase0
    // 文字を書く
    //-------------------------------------------------

    if (pSet->param_i[0] == 0)
    {
        constexpr int CREATE_PER_FRAME = 5;

        for (int i = 0; i < CREATE_PER_FRAME; i++)
        {
            if (pSet->param_i[1] >= pSet->param_i[2])
                break;

            auto p = offsets[pSet->param_i[1]++];

            sEnemyShot* shot = new sEnemyShot;

            shot->x = pSet->x + p.first;
            shot->y = pSet->y + p.second;

            shot->speed = 0.0;
            shot->muki = 0.0;

            shot->kind = img_enemyShotSmallBall[pSet->kind % 9];
            shot->margin = 260.0;

            shot->param_d[0] = hypot(p.first, p.second);
            shot->param_d[1] = atan2(p.second, p.first);

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;

            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }

        if (pSet->param_i[1] >= pSet->param_i[2])
        {
            pSet->param_i[0] = 1;
            pSet->param_i[3] = 40;

            PlaySoundMem(sound_enemyShot_heavy,
                DX_PLAYTYPE_BACK);
        }
    }

    //-------------------------------------------------
    // Phase1
    // 完成して停止
    //-------------------------------------------------

    else if (pSet->param_i[0] == 1)
    {
        pSet->param_i[3]--;

        if (pSet->param_i[3] <= 0)
        {
            pSet->param_i[0] = 2;
        }
    }

    //-------------------------------------------------
    // Phase2
    // 渦回転
    //-------------------------------------------------

    else if (pSet->param_i[0] == 2)
    {
        sEnemyShot* p = pSet->pEnemyShotHead->next;

        while (p != pSet->pEnemyShotHead)
        {
            p->param_d[1] += 0.03;
            p->param_d[0] *= 0.998;

            p->x = pSet->x +
                cos(p->param_d[1]) * p->param_d[0];

            p->y = pSet->y +
                sin(p->param_d[1]) * p->param_d[0];

            p = p->next;
        }

        if (pSet->count > 240)
        {
            pSet->param_i[0] = 3;

            PlaySoundMem(sound_enemyShot_extreme,
                DX_PLAYTYPE_BACK);

            sEnemyShot* q = pSet->pEnemyShotHead->next;

            while (q != pSet->pEnemyShotHead)
            {
                q->muki =
                    atan2(player.y - q->y,
                        player.x - q->x)
                    + (GetRand(180) - 90)
                    * DX_PI / 180.0;

                q->speed =
                    2.5 + GetRand(100) / 100.0;

                q = q->next;
            }
        }
    }

    //-------------------------------------------------
    // Phase3
    // 崩壊
    //-------------------------------------------------

    else
    {
        sEnemyShot* p = pSet->pEnemyShotHead->next;

        while (p != pSet->pEnemyShotHead)
        {
            p->x +=
                cos(p->muki) * p->speed;

            p->y +=
                sin(p->muki) * p->speed;

            p = p->next;
        }
    }
}

//=========================================================
// 敵本体
//=========================================================

void EnemyPat_Moji_ChatGPT()
{
    static int dir;
    static int wave;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 100.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        dir = 1;
        wave = 0;
    }

    //-------------------------------------------------
    // 左右移動
    //-------------------------------------------------

    enemy.x += dir * 0.9;

    if (enemy.x < 80)
        dir = 1;

    if (enemy.x > 400)
        dir = -1;

    //-------------------------------------------------
    // 文字を発射
    //-------------------------------------------------

    if (count % 150 == 60)
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;

        pSet->patternFunc = ShotText;

        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;

        pSet->muki =
            atan2(
                player.y - pSet->y,
                player.x - pSet->x);

        // 使用する文字
        pSet->kind = wave++;

        pSet->pEnemyShotHead =
            new sEnemyShot;

        pSet->pEnemyShotHead->prev =
            pSet->pEnemyShotHead;

        pSet->pEnemyShotHead->next =
            pSet->pEnemyShotHead;

        // 自由パラメータ初期化
        for (int i = 0; i < 4; i++)
        {
            pSet->param_i[i] = 0;
            pSet->param_d[i] = 0.0;
        }

        //-------------------------------------------------
        // リストへ追加
        //-------------------------------------------------

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}
