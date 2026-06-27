// enemyPat_Tmp.cpp
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
//  文字の輪郭を等間隔でサンプリング
// ============================================================
static std::vector<std::pair<double, double>> SampleTextOutline(
    const wchar_t* text,
    const wchar_t* fontName,
    REAL fontSize,
    double interval)
{
    InitGdiplusOnce();

    FontFamily fontFamily(fontName);
    GraphicsPath path;
    StringFormat fmt;
    path.AddString(text, -1, &fontFamily, FontStyleRegular, fontSize, PointF(0, 0), &fmt);

    // 0.25pxの精度で折れ線化
    path.Flatten(nullptr, 0.25f);

    int pointCount = path.GetPointCount();
    if (pointCount < 2) return {};

    std::vector<PointF> pts(pointCount);
    std::vector<BYTE> types(pointCount);
    path.GetPathPoints(pts.data(), pointCount);
    path.GetPathTypes(types.data(), pointCount);

    RectF bounds;
    path.GetBounds(&bounds);
    double cx = bounds.X + bounds.Width / 2.0;
    double cy = bounds.Y + bounds.Height / 2.0;

    std::vector<std::pair<double, double>> result;

    PointF subpathStart = pts[0];
    PointF prevPoint = pts[0];
    double leftover = 0.0;

    auto sampleLineTo = [&](const PointF& to) {
        double dx = to.X - prevPoint.X;
        double dy = to.Y - prevPoint.Y;
        double segLen = std::sqrt(dx * dx + dy * dy);

        if (segLen > 0.0) {
            double dirX = dx / segLen;
            double dirY = dy / segLen;
            double currentPos = interval - leftover;

            while (currentPos <= segLen) {
                double x = prevPoint.X + dirX * currentPos;
                double y = prevPoint.Y + dirY * currentPos;
                result.emplace_back(x - cx, y - cy);
                currentPos += interval;
            }
            leftover = segLen - (currentPos - interval);
        }
        prevPoint = to;
    };

    for (int i = 0; i < pointCount; ++i) {
        BYTE type = types[i];
        PointF pt = pts[i];
        BYTE pathType = type & PathPointTypePathTypeMask;

        if (pathType == PathPointTypeStart) {
            subpathStart = pt;
            prevPoint = pt;
            result.emplace_back(pt.X - cx, pt.Y - cy);
            leftover = 0.0;
        }
        else {
            sampleLineTo(pt);
        }

        if ((type & PathPointTypeCloseSubpath) == PathPointTypeCloseSubpath) {
            sampleLineTo(subpathStart);
        }
    }

    return result;
}

// ============================================================
//  4つの漢字の弾オフセットリスト（初回アクセス時に自動生成）
// ============================================================
static std::vector<std::pair<double, double>>& GetOffsetsTmp(int id)
{
    static std::vector<std::pair<double, double>> offsets[4];
    static bool initialized = false;
    if (!initialized) {
        // ボスらしく威圧感のある漢字4選
        const wchar_t* chars[] = { L"滅", L"斬", L"轟", L"烈" };

        // 画面サイズ(480x480)に合わせて、綺麗に見えるサイズと密度を調整
        constexpr REAL  fontSize = 300.0f;
        constexpr double interval = 10.0;
        const wchar_t* fontName = L"HGMinchoE";

        for (int i = 0; i < 4; ++i) {
            offsets[i] = SampleTextOutline(chars[i], fontName, fontSize, interval);
        }
        initialized = true;
    }
    return offsets[id];
}

// ============================================================
//  弾幕パターン：文字の形で飛ばし、回転させたあとに放射拡散
// ============================================================
static void ShotFormationStringTmp(sEnemyShotSet* pEnemyShotSet)
{
    const int soundLaunch = sound_enemyShot_heavy;
    const int soundScatter = sound_enemyShot_light;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(soundLaunch) == 1) StopSoundMem(soundLaunch);
        PlaySoundMem(soundLaunch, DX_PLAYTYPE_BACK);

        int charId = pEnemyShotSet->kind % 4; // 4文字のローテーション
        const auto& offsets = GetOffsetsTmp(charId);

        // 文字ごとに弾の色を変更 (0:赤, 3:シアン, 1:黄, 5:マゼンタ)
        int colorIdx = 0;
        switch (charId) {
        case 0: colorIdx = 0; break;
        case 1: colorIdx = 3; break;
        case 2: colorIdx = 1; break;
        case 3: colorIdx = 5; break;
        }
        const int imgKind = img_enemyShotDiamond[colorIdx];

        for (const auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->muki = pEnemyShotSet->muki;
            shot->speed = 1.2;     // 文字の移動速度（少し遅めにして回避しやすくする）
            shot->kind = imgKind;
            shot->margin = 200.0;  // 文字サイズが大きいため画面外自動消去を防止する大きめのマージン

            // 自由パラメータ(param)を利用して初期状態を保存
            shot->param_i[0] = 0;                // 0: 回転維持モード, 1: 放射拡散モード
            shot->param_d[0] = pEnemyShotSet->x; // 中心の初期位置X
            shot->param_d[1] = pEnemyShotSet->y; // 中心の初期位置Y
            shot->param_d[2] = off.first;        // 中心からの相対初期座標X
            shot->param_d[3] = off.second;       // 中心からの相対初期座標Y

            shot->x = pEnemyShotSet->x + off.first;
            shot->y = pEnemyShotSet->y + off.second;

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    // 毎フレームの移動および変形ロジック (インクリメント処理は行わない仕様)
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {

        if (p->param_i[0] == 0) {
            // 【モード0】回転維持モード（100フレームに達するまで形状を保って回転移動）
            if (p->count >= 100) {
                p->param_i[0] = 1; // 100フレーム経過で拡散モードへ移行

                // 現在の疑似的な文字の中心座標を計算
                double cx = p->param_d[0] + p->count * p->speed * cos(p->muki);
                double cy = p->param_d[1] + p->count * p->speed * sin(p->muki);

                // 中心点から各弾の現在位置への方向を算出し、外側へ広がる放射ベクトルにする
                p->muki = atan2(p->y - cy, p->x - cx);

                // 拡散速度は GetRand を使用してランダムなばらつきを持たせる
                // (100 + GetRand(150)) / 100.0 => 1.0 以上 2.5 以下のランダムな速度
                p->speed = (100 + GetRand(150)) / 100.0;

                // 拡散時の効果音（高密度のため、確率で鳴らして爆音化を防止）
                if (GetRand(30) == 0) {
                    if (CheckSoundMem(soundScatter) == 1) StopSoundMem(soundScatter);
                    PlaySoundMem(soundScatter, DX_PLAYTYPE_BACK);
                }
            }
            else {
                // 中心の現在位置を算出
                double cx = p->param_d[0] + p->count * p->speed * cos(p->muki);
                double cy = p->param_d[1] + p->count * p->speed * sin(p->muki);

                // 発射の波(kind)ごとに回転方向を交互に入れ替える
                double rotDirection = (pEnemyShotSet->kind % 2 == 0) ? 1.0 : -1.0;
                double angle = p->count * 0.015 * rotDirection; // 1フレームごとに少しずつ回転

                double rx = p->param_d[2];
                double ry = p->param_d[3];

                // 回転行列を適用して文字の形を完全に保ったまま回転させる
                p->x = cx + rx * cos(angle) - ry * sin(angle);
                p->y = cy + rx * sin(angle) + ry * cos(angle);
            }
        }

        // 【モード1】放射拡散モード（軌道変化後の直線移動）
        if (p->param_i[0] == 1) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }

        p = p->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Moji_Gemini()
{
    static int muki = 1;
    static int nextWaveFrame = 50;   // 最初の波を発射するフレーム
    static int waveId = 0;

    if (count == 1) {
        // 初期位置とHPの設定（ゲーム画面は 480x480）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200固定
        muki = 1;
        nextWaveFrame = 50;
        waveId = 0;
    }

    // 左右への緩やかな往復移動
    enemy.x += 0.8 * (double)muki;
    if (count % 160 == 80) muki *= -1;

    // 100フレーム間隔で新しい文字をプレイヤーに向けて撃ち出す
    if (count == nextWaveFrame) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFormationStringTmp;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;

        // プレイヤーの現在位置を狙う（自機狙い）
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = waveId; // どの漢字を出すかの識別ID

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリスト（enemyShotSetHead）の末尾へ追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        waveId++;
        nextWaveFrame += 100; // 次の発射までの間隔
    }
}