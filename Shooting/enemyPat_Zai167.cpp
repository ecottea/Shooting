// ============================================================
//  弾幕パターン:「顔書弾幕『へのへのもへじ』」
//  ------------------------------------------------------------
//  敵本体関数 : void EnemyPat_Henohenomoheji_Zai()
//
//  コンセプト:
//    ボスが画面上に自らの顔「へのへのもへじ」を弾で描いていく
//    5ストローク構成の弾幕。全て既存の弾種の組み合わせで表現する。
//
//  フェーズ構成 (1周期 CYCLE=1020f / 約17秒):
//    t=  60  フェーズ1  額と両目の「へ」: 山形カーテンが降ってくる
//    t= 240  フェーズ2  口の「の」    : 渦巻弾(途中で回転反転=笑う口)
//    t= 450  フェーズ3  輪郭の「も」  : 大型弾の四角い枠が回転しながら収縮
//    t= 780  フェーズ4  顔の完成      : 文字弾で描かれた顔が出現→瞬き→
//                                        にこにこ→爆散
//
//  使用素材(抜粋):
//    弾  : 中玉[0]赤=へカーテン / 小玉[5]マゼンタ=の渦巻 /
//          大玉[4]青=も枠 / 小玉[6]白=顔の線画
//          (銃弾・鱗弾・菱形弾・中楕円弾・短レーザーは不使用)
//    音  : sound_enemyCharge(予告) / medium / heavy / extreme
//
//  実装メモ:
//    - 専用素材の代わりに、サンプルの GDI+ 文字ピクセル化技術を流用し
//      「へ」「の」「も」を20pxフォントで描画→白ピクセル座標→
//      拡大配置することで小玉だけで顔の線画を作る。
//    - 同時最大弾数は約800(顔約650+他フェーズ)。プール4096に余裕あり。
//    - count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//      画面外弾の消去はメインルーチン側で行われる前提。
//      → このファイルでは増加参照のみ行い、リスト要素の削除はしない。
//    - GetRand(x) は 0〜x の x+1 種類を返す点に注意(使用箇所には注記あり)。
// ============================================================

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
//  指定文字を20pxで描画し、白ピクセルの座標を返す
//  (サンプルの SampleTextPixelsStr と同手法・1文字専用版)
// ============================================================
static std::vector<std::pair<double, double>> RenderCharPixels(wchar_t ch)
{
    std::vector<std::pair<double, double>> result;
    InitGdiplusOnce();

    const int SIZE = 20;   // フォントサイズ(ピクセル)
    const int W = SIZE * 2;
    const int H = SIZE * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));

    // アンチエイリアスOFF(単色描画)
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic UI Light");
    Font font(&fontFamily, (REAL)SIZE, FontStyleRegular, UnitPixel);

    std::wstring str(1, ch);
    g.DrawString(str.c_str(), (INT)str.length(), &font, PointF(0, 0), &whiteBrush);

    // ピクセルデータ取得
    BitmapData bmpData;
    Rect rect(0, 0, W, H);
    bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
    BYTE* argb = (BYTE*)bmpData.Scan0;

    std::vector<std::pair<int, int>> pixels;
    int minX = W, maxX = -1, minY = H, maxY = -1;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int idx = y * bmpData.Stride + x * 4;
            BYTE a = argb[idx + 3];
            if (a == 255) {
                pixels.emplace_back(x, y);
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }
    bitmap.UnlockBits(&bmpData);

    // バウンディングボックスの中心を原点に変換
    if (!pixels.empty()) {
        double cx = (minX + maxX) / 2.0;
        double cy = (minY + maxY) / 2.0;
        result.reserve(pixels.size());
        for (const auto& p : pixels) {
            result.emplace_back(p.first - cx, p.second - cy);
        }
    }
    return result;
}

// ============================================================
//  文字ピクセル座標のキャッシュ(初回アクセス時に自動生成)
// ============================================================
static const std::vector<std::pair<double, double>>& GetCharOffsets(wchar_t ch)
{
    static std::map<wchar_t, std::vector<std::pair<double, double>>> cache;
    auto it = cache.find(ch);
    if (it == cache.end()) {
        it = cache.emplace(ch, RenderCharPixels(ch)).first;
    }
    return it->second;
}

// ============================================================
//  弾を1発追加する共通ヘルパー
//  (countのインクリメントと画面外消去はメインルーチン側の仕様)
// ============================================================
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet,
    double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->margin = 240;

    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
    p->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = p;
    pEnemyShotSet->pEnemyShotHead->prev = p;
    return p;
}

// ============================================================
//  弾幕セットを1つ生成する共通ヘルパー
// ============================================================
static void SpawnSet(sEnemyShotSet::PatternFunc func,
    double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
//  フェーズ1:額と両目の「へ」
//  山形(へ字型 ︿)に並んだ中玉のカーテンが上から降ってくる。
//  端が開いているので、左右の裾を狙って抜けるのが基本の避け方。
// ============================================================
static void ShotHeCurtain(sEnemyShotSet* pEnemyShotSet)
{
    // 45f間隔で5波(T=0,45,90,135,180)
    if (pEnemyShotSet->count <= 180 && pEnemyShotSet->count % 45 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int    N = 17;       // 1波の弾数
        const double WIDTH = 220.0;   // 横幅
        const double HEIGHT = 70.0;   // 山の高さ

        // プレイヤー周辺を狙う。GetRand(120)は0〜120の121種なので
        // -60すると揺らぎは-60〜+60
        double cx = player.x + (GetRand(120) - 60);

        for (int i = 0; i < N; i++) {
            double u = -1.0 + 2.0 * i / (N - 1);            // -1.0〜1.0
            double x = cx + u * (WIDTH / 2.0);
            double y = -20.0 - (1.0 - fabs(u)) * HEIGHT;    // 中央が山頂(上)
            AddShot(pEnemyShotSet, x, y, DX_PI / 2.0, 2.2, img_enemyShotMediumBall[0]);
        }
    }

    // 共通の直線移動
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  フェーズ2:口の「の」
//  「の」はそのまま渦巻。口の位置から2方向の渦を巻く。
//  count==90で回転方向が反転し「口が動いて笑っている」ように見せる。
// ============================================================
static void ShotNoSpiral(sEnemyShotSet* pEnemyShotSet)
{
    // 渦の現在角度(param_d[0]は生成時に0初期化済み)
    double dir = (pEnemyShotSet->count < 90) ? 1.0 : -1.0;   // 反転=にっこり
    pEnemyShotSet->param_d[0] += dir * 0.22;

    if (pEnemyShotSet->count < 180) {
        double a = pEnemyShotSet->muki + pEnemyShotSet->param_d[0];
        for (int k = 0; k < 2; k++) {
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                a + k * DX_PI, 2.6, img_enemyShotSmallBall[5]);
        }
    }

    // 共通の直線移動
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  フェーズ3:輪郭の「も」
//  大型弾で作った四角い枠(輪郭)が画面の周上を流れながら
//  ゆっくり収縮してプレイヤーを締め付け、外側へ爆散して終わる。
//  弾は画面内(枠 20〜460px)に生成するので消去と競合しない。
// ============================================================
static void ShotMoFrame(sEnemyShotSet* pEnemyShotSet)
{
    const double CX = 240.0, CY = 240.0;   // 画面中心
    const int    N = 24;                    // 枠の弾数
    const double START_WH = 440.0;          // 初期の一辺の長さ
    const double SHRINK = 0.8;              // 収縮速度(px/f)
    const double MIN_WH = 240.0;            // 収縮の下限
    const int    BURST = 300;               // 爆散フレーム

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < N; i++) {
            // param_d[0] = 枠の周上の位置(ペリメータ距離)
            sEnemyShot* p = AddShot(pEnemyShotSet, CX, CY, 0.0, 0.0, img_enemyShotLargeBall[4]);
            p->param_d[0] = (4.0 * START_WH) * i / N;
        }
    }

    if (pEnemyShotSet->count == BURST) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count <= BURST) {
            // 枠の上を流れる(公転)＋収縮
            double wh = START_WH - SHRINK * pEnemyShotSet->count;
            if (wh < MIN_WH) wh = MIN_WH;

            p->param_d[0] += 1.5;                       // 枠に沿って流れる
            double s = fmod(p->param_d[0], 4.0 * wh);
            double x0 = CX - wh / 2.0, y0 = CY - wh / 2.0;

            if (s < wh) {                       // 上辺(左→右)
                p->x = x0 + s;
                p->y = y0;
            }
            else if (s < 2.0 * wh) {            // 右辺(上→下)
                p->x = x0 + wh;
                p->y = y0 + (s - wh);
            }
            else if (s < 3.0 * wh) {            // 下辺(右→左)
                p->x = x0 + wh - (s - 2.0 * wh);
                p->y = y0 + wh;
            }
            else {                              // 左辺(下→上)
                p->x = x0;
                p->y = y0 + wh - (s - 3.0 * wh);
            }

            if (pEnemyShotSet->count == BURST) {
                p->muki = atan2(p->y - CY, p->x - CX) + DX_PI;
                p->speed = 4.5;
            }
        }
        else {
            // 爆散後の直線移動
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// ============================================================
//  フェーズ4:顔の完成(フィニッシュ)
//  GDI+でドット化した「へのへのもへじ」を小玉(白)で再現し
//  画面中央下に浮かび上がらせる。
//    出現(45f 縮小表示から拡大) → 息づかい → 瞬き(目) →
//    にこにこ(口が弧を描いて動く) → 全弾爆散
// ============================================================

// 顔のレイアウト(FC=顔中心からのオフセットpx / 拡大率 / 役割)
// role: 0=その他 1=目(瞬きする) 2=口(にこにこ動く)
struct FacePart {
    wchar_t ch;
    double  ox, oy;
    double  scale;
    int     role;
};
static const FacePart kFaceParts[] = {
    { L'へ',   0.0, -58.0, 1.2, 0 },   // 額の「へ」:輪郭の上にちょこん
    { L'へ', -32.0, -12.0, 1.6, 1 },   // 左目
    { L'へ',  32.0, -12.0, 1.6, 1 },   // 右目
    { L'の',   0.0,  18.0, 1.6, 2 },   // 口
    { L'も',   0.0,  -4.0, 4.0, 0 },   // 顔の輪郭
};

static void ShotFaceFinish(sEnemyShotSet* pEnemyShotSet)
{
    const double FCX = pEnemyShotSet->x;   // 顔中心(生成時に240,250を指定)
    const double FCY = pEnemyShotSet->y;
    const int    APPEAR_END = 45;          // 出現アニメの終了フレーム
    const int    BLINK_BEGIN = 90;          // 瞬き開始
    const int    BLINK_END = 106;         // 瞬き終了
    const int    BURST = 150;          // 爆散フレーム

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (const auto& part : kFaceParts) {
            const auto& offsets = GetCharOffsets(part.ch);
            for (const auto& off : offsets) {
                double hx = FCX + part.ox * part.scale*2 + off.first * part.scale*2;
                double hy = FCY + part.oy * part.scale*2 + off.second * part.scale*2;

                sEnemyShot* p = AddShot(pEnemyShotSet, hx, hy, 0.0, 0.0,
                    img_enemyShotSmallBall[6]);
                p->param_i[0] = part.role;
                p->param_d[4] = hx;   // ホームポジション
                p->param_d[5] = hy;

                if (part.role == 1) {  // 目:瞬きの収縮中心
                    p->param_d[2] = FCX + part.ox * part.scale*2;
                    p->param_d[3] = FCY + part.oy * part.scale*2;
                }
                if (part.role == 2) {  // 口:顔中心回りの角度と半径
                    double dx = hx - FCX, dy = hy - FCY;
                    p->param_d[0] = atan2(dy, dx);
                    p->param_d[1] = sqrt(dx * dx + dy * dy);
                }
            }
        }
    }

    if (pEnemyShotSet->count == BURST) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count < BURST) {
            // 出現:15%の大きさから45fかけて浮かび上がる
            double appear = 1.0;
            if (pEnemyShotSet->count < APPEAR_END) {
                appear = 0.15 + 0.85 * pEnemyShotSet->count / APPEAR_END;
            }
            // 息づかい:ふわふわと膨らむ
            double breathe = 1.0 + 0.03 * sin(pEnemyShotSet->count * 0.12);

            double hx = p->param_d[4];
            double hy = p->param_d[5];

            if (p->param_i[0] == 2) {
                // 口:顔中心回りにゆっくり弧を描いて動く(にこにこ)
                double a = p->param_d[0] + 0.35 * sin(pEnemyShotSet->count * 0.15);
                hx = FCX + p->param_d[1] * cos(a);
                hy = FCY + p->param_d[1] * sin(a);
            }
            else if (p->param_i[0] == 1) {
                // 目:瞬きで目の中心に向かって閉じる
                double k = 1.0;
                if (pEnemyShotSet->count >= BLINK_BEGIN && pEnemyShotSet->count < BLINK_END) {
                    double t = (pEnemyShotSet->count - BLINK_BEGIN)
                        / (double)(BLINK_END - BLINK_BEGIN) * DX_PI;
                    k = 1.0 - 0.85 * sin(t);
                }
                hx = p->param_d[2] + (hx - p->param_d[2]) * k;
                hy = p->param_d[3] + (hy - p->param_d[3]) * k;
            }

            p->x = FCX + (hx - FCX) * appear * breathe;
            p->y = FCY + (hy - FCY) * appear * breathe;
        }
        else if (pEnemyShotSet->count == BURST) {
            // 顔が爆散!全弾が顔中心から放射状に飛ぶ
            p->muki = atan2(p->y - FCY, p->x - FCX);
            p->speed = 4.0;
        }
        else {
            // 爆散後の直線移動
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// ============================================================
//  敵本体のパターン(エントリポイント)
// ============================================================
void EnemyPat_Henohenomoheji_Zai()
{
    static int muki;
    const int CYCLE = 1020-200;   // 1周期 約17秒

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;   // 200で固定
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    int t = (count - 1) % CYCLE;

    // 周期の頭と顔出現直前に予告音
    if (t == 0)   PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    if (t == 720) PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

    // フェーズ1:「へ」カーテン
    if (t == 60) {
        SpawnSet(ShotHeCurtain, enemy.x, enemy.y + 10.0,
            atan2(player.y - enemy.y, player.x - enemy.x), 0);
    }
    // フェーズ2:「の」渦巻(口の位置から)
    if (t == 240) {
        SpawnSet(ShotNoSpiral, enemy.x, enemy.y + 20.0,
            atan2(player.y - enemy.y, player.x - enemy.x), 1);
    }
    // フェーズ3:「も」輪郭の枠(画面中心固定)
    if (t == 450) {
        SpawnSet(ShotMoFrame, 240.0, 240.0, 0.0, 2);
    }
    // フェーズ4:完成した「へのへのもへじ」(画面中央下・プレイヤー側)
    if (t == 780) {
        SpawnSet(ShotFaceFinish, 240.0, 250.0, 0.0, 3);
    }
}