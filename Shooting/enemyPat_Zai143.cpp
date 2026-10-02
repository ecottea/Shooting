//============================================================
//  弾幕パターン:「実況コメント流」
//------------------------------------------------------------
//  ニコニコ動画モチーフ。コメント文字が弾となり右から左へ流れる。
//
//  Phase1 (hp > 140) : コメント流(ゆるめ) + リアコ弾
//  Phase2 (hp >  70) : コメント流(高頻度) + コテ弾(赤:高速直進/緑:揺らぎ)
//  Phase3 (hp >  20) : コメント流(最頻度) + 見えない「1コメ」黒弾 + コテ弾(強化)
//  Phase4 (hp <= 20) : フィニッシュ「5000万コメ」巨大文字弾
//
//  ※ count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//    画面外弾の消去はメインルーチン側で行う前提。
//  ※ GetRand(x) は 0〜x の x+1 種類を返すため、範囲計算に注意して使用。
//============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <map>
#include <string>
#include <vector>
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
//  文字列を32pxで描画し、白ピクセル座標(中心基準)を返す
//  ※ ストローク間引きのため 2px グリッドでサンプリング
// ============================================================
struct TextShape {
    std::vector<std::pair<double, double>> pts; // 中心基準オフセット
    double halfW = 0.0, halfH = 0.0;
};

static TextShape RenderTextShape(const std::wstring& text)
{
    InitGdiplusOnce();

    const int SIZE = 32;
    const int THIN = 1;   // 2px間引き(弾数節減)
    const int W = (int)text.size() * (SIZE + 8) + SIZE;
    const int H = SIZE * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic UI Light");
    Font font(&fontFamily, (REAL)SIZE, FontStyleRegular, UnitPixel);
    g.DrawString(text.c_str(), -1, &font, PointF(0, (REAL)(SIZE / 2)), &whiteBrush);

    BitmapData bmpData;
    Rect rect(0, 0, W, H);
    bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
    BYTE* argb = (BYTE*)bmpData.Scan0;

    std::vector<std::pair<int, int>> whitePixels;
    int minX = W, maxX = -1, minY = H, maxY = -1;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int idx = y * bmpData.Stride + x * 4;
            if (argb[idx + 3] == 255 && x % THIN == 0 && y % THIN == 0) {
                whitePixels.emplace_back(x, y);
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }
    bitmap.UnlockBits(&bmpData);

    TextShape shape;
    if (!whitePixels.empty()) {
        double cx = (minX + maxX) / 2.0;
        double cy = (minY + maxY) / 2.0;
        shape.halfW = (maxX - minX) / 2.0;
        shape.halfH = (maxY - minY) / 2.0;
        shape.pts.reserve(whitePixels.size());
        for (const auto& p : whitePixels)
            shape.pts.emplace_back(p.first - cx, p.second - cy);
    }
    return shape;
}

// 文字形状キャッシュ(同じコメントは2回目以降即取得)
static const TextShape& GetTextShape(const std::wstring& text)
{
    static std::map<std::wstring, TextShape> cache;
    auto it = cache.find(text);
    if (it != cache.end()) return it->second;
    return cache.emplace(text, RenderTextShape(text)).first->second;
}

// ============================================================
//  流れるコメント一覧(ニコニコ風)
// ============================================================
static const wchar_t* kComments[] = {
    L"ww", L"GJ", L"うまい!", L"神", L"888",
    L"きたぁぁぁ", L"これはひどい", L"1コメゲット",
    L"エコノミー回避", L"8888888888",       // ← プチ祭り
};
static const int kCommentNum = (int)(sizeof(kComments) / sizeof(kComments[0]));

// ============================================================
//  弾の追加(リンクリスト接続共通化)
// ============================================================
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* shot = new sEnemyShot;
    shot->x = x;
    shot->y = y;
    shot->muki = muki;
    shot->speed = speed;
    shot->kind = kind;
    shot->margin = 480;

    shot->prev = pSet->pEnemyShotHead->prev;
    shot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = shot;
    pSet->pEnemyShotHead->prev = shot;
    return shot;
}

// ============================================================
//  コメント弾の生成(文字 → 小玉の弾の集合)
//  短いコメントほど速く、長いコメントほど遅く流れる
// ============================================================
static void SpawnComment(sEnemyShotSet* pSet, const wchar_t* text,
    double y, double speedMul = 1.0)
{
    const TextShape& shape = GetTextShape(text);
    if (shape.pts.empty()) return;

    int len = (int)wcslen(text);
    const double SCALE = 2.0;
    double speed = 4.5 - len * 0.35;      // 長さ→速度変換
    if (speed < 1.4) speed = 1.4;
    speed *= speedMul;

    // 右端( x=500 付近)から全体が見えるように中心を決める
    double cx = 500.0 - shape.halfW * SCALE;
    if (cx < 100.0) cx = 100.0;

    // 安全弁: 1コメントの弾数上限(プール 4096 保護)
    size_t step = 1;
    while (shape.pts.size() / step > 500) step++;

    for (size_t i = 0; i < shape.pts.size(); i += step) {
        AddShot(pSet,
            cx + shape.pts[i].first * SCALE,
            y + shape.pts[i].second * SCALE,
            DX_PI, speed, img_enemyShotSmallBall[6]);   // 白 = コメント色
    }
}

// ============================================================
//  自セットの弾を移動(揺らぎ弾はサイン波で軌道が揺れる)
// ============================================================
static void MoveShots(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 1) {
            // 緑(匿名)弾: 基準角度 param_d[0] を中心に揺れる
            p->muki = p->param_d[0] + sin(p->count * 0.08 + p->param_d[1]) * 0.7;
        }
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  現在のフェーズを HP から算出
// ============================================================
static int CurrentPhase()
{
    if (enemy.hp > 140) return 1;
    if (enemy.hp > 70)  return 2;
    if (enemy.hp > 20)  return 3;
    return 4;
}

// ============================================================
//  弾幕①: うpコメント(右→左に流れる文字弾)
//  param_i[3] != 0 で「1コメ見えてる?」黒弾(見えない弾)を混入
// ============================================================
static void ShotPat_CommentStream(sEnemyShotSet* pSet)
{
    if (pSet->param_i[4] == CurrentPhase()) {
        if (pSet->count % pSet->param_i[0] == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            const wchar_t* text = kComments[pSet->param_i[1] % kCommentNum];
            pSet->param_i[1] += 1 + GetRand(2);   // 順送り+ランダム跳び

            double y = 36.0 + GetRand(400);       // 36〜436 の任意レーン
            SpawnComment(pSet, text, y);

            // Phase3: 黒い小玉がプレイヤーの高さを狙撃(低表示環境ではほぼ見えない)
            if (pSet->param_i[3] != 0) {
                for (int i = 0; i < 2; i++) {
                    double sy = player.y + (GetRand(80) - 40);
                    if (sy < 16.0)  sy = 16.0;
                    if (sy > 464.0) sy = 464.0;
                    AddShot(pSet, 500.0, sy, DX_PI,
                        3.0 + GetRand(15) / 10.0, img_enemyShotSmallBall[7]);
                }
            }
        }
    }
    MoveShots(pSet);
}

// ============================================================
//  弾幕②: リアコ弾(プレイヤーの位置に反応するコメント)
//  上空にいる →「花火で草」が上を流れて牽制
//  下段にいる →「うまい!」が速いコメントで襲いかかる
// ============================================================
static void ShotPat_Reaction(sEnemyShotSet* pSet)
{
    if (pSet->param_i[4] == CurrentPhase()) {
        if (pSet->count % pSet->param_i[0] == 0) {
            const wchar_t* text;
            double y;
            if (player.y < 200.0) {
                text = L"花火で草";
                y = 56.0 + GetRand(40);
            }
            else if (player.y > 280.0) {
                text = L"うまい!";
                y = player.y + (GetRand(60) - 30);
            }
            else {
                text = L"GJ";
                y = player.y + (GetRand(80) - 40);
            }
            if (y < 36.0)  y = 36.0;
            if (y > 444.0) y = 444.0;
            SpawnComment(pSet, text, y, 1.4);   // 反応弾はやや速め
        }
    }
    MoveShots(pSet);
}

// ============================================================
//  弾幕③: コテ弾(色付き弾)
//  赤(コテハン) = 高速・直進・読みやすい
//  緑(匿名)     = 低速だがサイン波で軌道が不規則
// ============================================================
static void ShotPat_Kotehan(sEnemyShotSet* pSet)
{
    if (pSet->param_i[4] == CurrentPhase()) {
        if (pSet->count % pSet->param_i[0] == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            double bx = enemy.x, by = enemy.y + 12.0;
            double base = atan2(player.y - by, player.x - bx);
            int n = (pSet->param_i[4] >= 3) ? 5 : 4;   // Phase3で増量

            // 赤色弾(コテハン): ばらけた高速直進
            for (int i = 0; i < n; i++) {
                double a = base + (GetRand(50) - 25) / 180.0 * DX_PI;
                AddShot(pSet, bx, by, a,
                    4.5 + GetRand(15) / 10.0, img_enemyShotSmallBall[0]);
            }
            // 緑色弾(匿名): 揺らぎ弾
            for (int i = 0; i < n; i++) {
                double a = base + (GetRand(120) - 60) / 180.0 * DX_PI;
                sEnemyShot* shot = AddShot(pSet, bx, by, a, 1.8,
                    img_enemyShotSmallBall[2]);
                shot->param_i[0] = 1;                   // 揺らぎフラグ
                shot->param_d[0] = a;                   // 基準角度
                shot->param_d[1] = GetRand(628) / 100.0;// 位相
            }
        }
    }
    MoveShots(pSet);
}

// ============================================================
//  弾幕④: フィニッシュ「5000万コメ」
//  巨大文字弾が画面を覆う。文字の白抜きの隙間を縫って避ける
// ============================================================
static void ShotPat_Finish(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const TextShape& shape = GetTextShape(L"5000万コメ");
        const double SCALE = 5.0;
        const double SPEED = 0.12;
        double cx = 500.0 - shape.halfW * SCALE + 30;
        double cy = 240.0;

        size_t step = 1;
        while (shape.pts.size() / step > 2500) step++;

        for (size_t i = 0; i < shape.pts.size(); i += step) {
            AddShot(pSet,
                cx + shape.pts[i].first * SCALE,
                cy + shape.pts[i].second * SCALE,
                DX_PI, SPEED, img_enemyShotMediumBall[6]);
        }
    }
    else if (pSet->count % 45 == 0 && pSet->count < 600) {
        // 支援弾: フィニッシュ中も赤い銃弾が追撃してくる
        double bx = enemy.x, by = enemy.y + 12.0;
        double a = atan2(player.y - by, player.x - bx);
        AddShot(pSet, bx, by, a, 2.6, img_enemyShotBullet[0]);
    }
    MoveShots(pSet);
}

// ============================================================
//  弾源セット生成
//  param_i[0]=発射間隔 / [1]=コメント索引 / [3]=黒弾フラグ / [4]=担当フェーズ
// ============================================================
static sEnemyShotSet* CreateShotSet(void (*func)(sEnemyShotSet*),
    int phase, int interval, int invisibleFlag)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;
    pSet->kind = phase;
    pSet->param_i[0] = interval;
    pSet->param_i[1] = 0;
    pSet->param_i[3] = invisibleFlag;
    pSet->param_i[4] = phase;
    pSet->alive = 9999;

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
//  敵本体のパターン
// ============================================================
void EnemyPat_NicoNico_Zai()
{
    static int phase;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200;   // 200で固定
        phase = 0;

        // 全フェーズ分の弾源を先に生成(各セットは自分のフェーズ中しか撃たない)
        CreateShotSet(ShotPat_CommentStream, 1, 130, 0);   // Phase1: コメント流(ゆるめ)
        CreateShotSet(ShotPat_Reaction, 1, 100, 0);   // Phase1: リアコ弾
        CreateShotSet(ShotPat_CommentStream, 2, 85, 0);   // Phase2: コメント流(高頻度)
        CreateShotSet(ShotPat_Kotehan, 2, 50, 0);   // Phase2: コテ弾
        CreateShotSet(ShotPat_CommentStream, 3, 55, 1);   // Phase3: +見えない弾
        CreateShotSet(ShotPat_Kotehan, 3, 34, 0);   // Phase3: コテ弾(強化)
    }
    else {
        // 左右に揺れながら移動(後半ほど振り幅が大きくなる)
        double amp = 60.0 + phase * 18.0;
        enemy.x = 240.0 + sin(count * 0.016) * amp;
        enemy.y = 48.0 + sin(count * 0.043) * 10.0;
    }

    int target = CurrentPhase();

    // フェーズ遷移検出 → フィニッシュ予告
    if (target != phase) {
        phase = target;
        if (phase == 4) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);   // 予告音
            CreateShotSet(ShotPat_Finish, 4, 0, 0);              // 巨大コメント弾
        }
    }
}