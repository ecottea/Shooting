#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <array>
#include <map>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// ============================================================
//  アナグラムデータ
// ============================================================
static std::vector<std::array<std::string, 4>> anagram_data = {
    {{"清楚系描きたい", "せいそけいかきたい", "形態素解析", "けいたいそかいせき"}},
    {{"早よカキコしたれ", "はよかきこしたれ", "高橋是清", "たかはしこれきよ"}},
    {{"ヘソの意味、無い", "へそのいみない", "磯野波平", "いそのなみへい"}},
    {{"龍舞し、勝つ", "りゅうまいしかつ", "対馬海流", "つしまかいりゅう"}},
    {{"破格の三塁打", "はかくのさんるいだ", "春の大三角", "はるのだいさんかく"}},
    {{"臭すぎた歯間", "くさすぎたしかん", "高杉晋作", "たかすぎしんさく"}},
    {{"再三の誤ＢＡＮ", "さいさんのごばん", "最後の晩餐", "さいごのばんさん"}},
    {{"湿り気があった愛", "しめりけがあったあい", "雨上がり決死隊", "あめあがりけっしたい"}},
    {{"蚊、どうにかしたい", "かどうにかしたい", "二階堂高嗣", "にかいどうたかし"}},
    {{"麻生が艦これ", "あそうがかんこれ", "赤レンガ倉庫", "あかれんがそうこ"}},
    {{"吸い物、冷めへんかい？", "すいものさめへんかい", "かもめの水兵さん", "かもめのすいへいさん"}},
    {{"げ、先端臭い", "げせんたんくさい", "減反政策", "げんたんせいさく"}},
    {{"山に監禁、泣く", "やまにかんきんなく", "なかやまきんに君", "なかやまきんにくん"}},
    {{"四連着信", "よんれんちゃくしん", "クレヨンしんちゃん", "くれよんしんちゃん"}},
    {{"痛いし喘いどるん？", "いたいしあえいどるん", "アイドル新鋭隊", "あいどるしんえいたい"}},
    {{"おサボり禁止で描く", "おさぼりきんしでかく", "デオキシリボ核酸", "でおきしりぼかくさん"}}
};

// ============================================================
//  ユーティリティ
// ============================================================

static std::wstring StrToWStr(const std::string& str) {
    if (str.empty()) return L"";

    bool isUTF8 = true;
    int ascii_cnt = 0;
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = str[i];
        if (c <= 0x7F) { i++; ascii_cnt++; }
        else if (c >= 0xC2 && c <= 0xDF && i + 1 < str.length() && (str[i + 1] & 0xC0) == 0x80) i += 2;
        else if (c >= 0xE0 && c <= 0xEF && i + 2 < str.length() && (str[i + 1] & 0xC0) == 0x80 && (str[i + 2] & 0xC0) == 0x80) i += 3;
        else if (c >= 0xF0 && c <= 0xF4 && i + 3 < str.length() && (str[i + 1] & 0xC0) == 0x80 && (str[i + 2] & 0xC0) == 0x80 && (str[i + 3] & 0xC0) == 0x80) i += 4;
        else { isUTF8 = false; break; }
    }

    UINT cp = (isUTF8 && ascii_cnt < (int)str.length()) ? CP_UTF8 : CP_ACP;
    int size = MultiByteToWideChar(cp, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(cp, 0, &str[0], (int)str.size(), &wstr[0], size);
    return wstr;
}

static void InitGdiplusOnce() {
    static bool initialized = false;
    if (!initialized) {
        GdiplusStartupInput gdiInput;
        ULONG_PTR token;
        GdiplusStartup(&token, &gdiInput, nullptr);
        initialized = true;
    }
}

// 1文字のピクセル座標リストを取得 (左下詰め基準)
static std::vector<std::pair<double, double>> SampleSingleCharPixels(const std::wstring& text) {
    static std::map<std::wstring, std::vector<std::pair<double, double>>> cache;
    if (cache.count(text)) return cache[text];

    InitGdiplusOnce();

    const int SIZE = 20;
    const int W = SIZE * 2;
    const int H = SIZE * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"MS Gothic");
    Font font(&fontFamily, (REAL)SIZE, FontStyleRegular, UnitPixel);

    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    g.DrawString(text.c_str(), -1, &font, RectF(0, 0, (REAL)W, (REAL)H), &format, &whiteBrush);

    BitmapData bmpData;
    Rect rect(0, 0, W, H);
    bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
    BYTE* argb = (BYTE*)bmpData.Scan0;

    std::vector<std::pair<int, int>> whitePixels;
    int minX = W, maxX = -1, minY = H, maxY = -1;

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int idx = y * bmpData.Stride + x * 4;
            if (argb[idx + 3] > 128) {
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
        result.reserve(whitePixels.size());
        // 左下詰めに統一 (xはminX起点、yはmaxY起点)
        for (const auto& p : whitePixels) {
            result.emplace_back(p.first - minX, p.second - maxY);
        }
    }

    cache[text] = result;
    return result;
}

static std::vector<int> GetAnagramMatch(const std::wstring& from, const std::wstring& to) {
    std::vector<int> match(from.length(), -1);
    std::vector<bool> used(to.length(), false);
    for (size_t i = 0; i < from.length(); ++i) {
        for (size_t j = 0; j < to.length(); ++j) {
            if (!used[j] && from[i] == to[j]) {
                match[i] = (int)j;
                used[j] = true;
                break;
            }
        }
    }
    return match;
}

static void DeleteGroupShots(sEnemyShotSet* pEnemyShotSet, const std::vector<int>& groupIds) {
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextP = p->next;
        bool match = false;
        for (int id : groupIds) {
            if (p->param_i[0] == id) match = true;
        }
        if (match) {
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
        }
        p = nextP;
    }
}

// 文字のピクセル弾を生成する
static void SpawnTextShots(sEnemyShotSet* pEnemyShotSet, const std::wstring& text, double baseY, int colorKind, int groupId, bool isAnagram = false, const std::vector<int>& matchIndices = {}, double targetY = 0) {
    const double SCALE = 2.0; // 文字の大きさを従来の2倍に拡大 (0.8 -> 1.6)

    int len = (int)text.length();
    // 文字数に応じて画面幅(480px)に収まるよう間隔を自動調整
    double SPACE = (len > 8) ? (460.0 / len) : 48.0;

    for (int i = 0; i < len; ++i) {
        std::wstring singleChar(1, text[i]);
        auto pixels = SampleSingleCharPixels(singleChar);

        // 文字の配置基準X座標 (センタリング)
        double charX = 240.0 + (i - (len - 1) / 2.0) * SPACE - (16.0 * SCALE / 2.0);

        double targetX = charX;
        if (isAnagram && i < (int)matchIndices.size() && matchIndices[i] != -1) {
            targetX = 240.0 + (matchIndices[i] - (len - 1) / 2.0) * SPACE - (16.0 * SCALE / 2.0);
        }

        for (auto& p : pixels) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = charX + p.first * SCALE;
            shot->y = baseY + p.second * SCALE;
            shot->muki = 0;
            shot->speed = 0;
            shot->kind = colorKind;

            shot->param_i[0] = groupId;
            shot->param_d[0] = targetX + p.first * SCALE;
            shot->param_d[1] = targetY + p.second * SCALE;
            shot->param_d[2] = 0.0;

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }
}

// ============================================================
//  アナグラム弾幕パターン
// ============================================================
static void ShotFormationAnagram(sEnemyShotSet* pEnemyShotSet)
{
    int& q_idx = pEnemyShotSet->param_i[0];
    int& state = pEnemyShotSet->param_i[1];
    int& timer = pEnemyShotSet->param_i[2];

    if (q_idx >= (int)anagram_data.size()) {
        return;
    }

    if (timer == 0) {
        std::wstring q_str = StrToWStr(anagram_data[q_idx][0]);
        std::wstring q_yomi = StrToWStr(anagram_data[q_idx][1]);
        std::wstring a_str = StrToWStr(anagram_data[q_idx][2]);
        std::wstring a_yomi = StrToWStr(anagram_data[q_idx][3]);

        if (state == 0) {
            // [問題] を画面上部やや上 (Y: 50.0) にシアン小弾で表示
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            SpawnTextShots(pEnemyShotSet, q_str, 90.0, img_enemyShotSmallBall[0], 1);
        }
        else if (state == 1) {
            // [問題の読み] を画面上部 (Y: 50.0) に白小弾で表示。移動先を画面最下部 (Y: 430.0) に設定
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            std::vector<int> match = GetAnagramMatch(q_yomi, a_yomi);
            SpawnTextShots(pEnemyShotSet, q_yomi, 90.0, img_enemyShotSmallBall[5], 2, true, match, 460.0);
        }
        else if (state == 2) {
            // [移動開始] 150フレームかけて画面最下部へ動かす
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[0] == 2) {
                    p->param_d[2] = 1.0;
                    p->param_d[3] = (p->param_d[0] - p->x) / 150.0;
                    p->param_d[4] = (p->param_d[1] - p->y) / 150.0;
                }
                p = p->next;
            }
        }
        else if (state == 3) {
            // [答え] を画面最下部 (Y: 430.0) に白小弾で表示
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            SpawnTextShots(pEnemyShotSet, a_str, 460.0, img_enemyShotSmallBall[0], 4);
        }
    }

    // 毎フレームの移動・軌跡処理
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->param_i[0] == 2 && p->param_d[2] == 1.0) {
            if (timer <= 150) {
                p->x += p->param_d[3];
                p->y += p->param_d[4];
            }
            else {
                p->x = p->param_d[0];
                p->y = p->param_d[1];
            }

            // 軌跡として低速の青小弾をばら撒く
            if (GetRand(300) == 0) {
                sEnemyShot* traj = new sEnemyShot;
                traj->x = p->x;
                traj->y = p->y;
                traj->muki = GetRand(360) * DX_PI / 180.0;
                traj->speed = (50 + GetRand(60)) / 100.0;
                traj->kind = img_enemyShotSmallBall[6]; // 青小弾
                traj->param_i[0] = 10;                 // 軌跡ID

                traj->prev = pEnemyShotSet->pEnemyShotHead->prev;
                traj->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = traj;
                pEnemyShotSet->pEnemyShotHead->prev = traj;
            }
        }
        else if (p->param_i[0] == 10) {
            // ばら撒き弾は消さずに画面外まで慣性移動
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }

    // タイマー更新とフェーズ遷移
    timer++;
    if (state == 0 && timer > 90) {
        DeleteGroupShots(pEnemyShotSet, { 1 });
        state = 1; timer = 0;
    }
    else if (state == 1 && timer > 90) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        state = 2; timer = 0;
    }
    else if (state == 2 && timer > 180) {
        // バラマキ弾(10)は消去対象から除外（画面外判定で消えるまで残る）
        DeleteGroupShots(pEnemyShotSet, { 2 });
        state = 3; timer = 0;
    }
    else if (state == 3 && timer > 120) {
        DeleteGroupShots(pEnemyShotSet, { 4 });
        state = 4; timer = 0;
    }
    else if (state == 4 && timer > 30) {
        q_idx++;
        state = 0; timer = 0;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Anagram()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 400;
        muki = 1;

        anagram_data = {
            {{"清楚系描きたい", "せいそけいかきたい", "形態素解析", "けいたいそかいせき"}},
            {{"早よカキコしたれ", "はよかきこしたれ", "高橋是清", "たかはしこれきよ"}},
            {{"ヘソの意味、無い", "へそのいみない", "磯野波平", "いそのなみへい"}},
            {{"龍舞し、勝つ", "りゅうまいしかつ", "対馬海流", "つしまかいりゅう"}},
            {{"破格の三塁打", "はかくのさんるいだ", "春の大三角", "はるのだいさんかく"}},
            {{"臭すぎた歯間", "くさすぎたしかん", "高杉晋作", "たかすぎしんさく"}},
            {{"再三の誤ＢＡＮ", "さいさんのごばん", "最後の晩餐", "さいごのばんさん"}},
            {{"湿り気があった愛", "しめりけがあったあい", "雨上がり決死隊", "あめあがりけっしたい"}},
            {{"蚊、どうにかしたい", "かどうにかしたい", "二階堂高嗣", "にかいどうたかし"}},
            {{"麻生が艦これ", "あそうがかんこれ", "赤レンガ倉庫", "あかれんがそうこ"}},
            {{"吸い物、冷めへんかい？", "すいものさめへんかい", "かもめの水兵さん", "かもめのすいへいさん"}},
            {{"げ、先端臭い", "げせんたんくさい", "減反政策", "げんたんせいさく"}},
            {{"山に監禁、泣く", "やまにかんきんなく", "なかやまきんに君", "なかやまきんにくん"}},
            {{"四連着信", "よんれんちゃくしん", "クレヨンしんちゃん", "くれよんしんちゃん"}},
            {{"痛いし喘いどるん？", "いたいしあえいどるん", "アイドル新鋭隊", "あいどるしんえいたい"}},
            {{"おサボり禁止で描く", "おさぼりきんしでかく", "デオキシリボ核酸", "でおきしりぼかくさん"}}
        };

        // シャッフル
        for (int i = (int)anagram_data.size() - 1; i > 0; --i) {
            int r = GetRand(i);
            std::swap(anagram_data[i], anagram_data[r]);
        }

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->alive = 99999; // 指示通り設定
        pSet->count = 0;
        pSet->patternFunc = ShotFormationAnagram;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->param_i[0] = 0; // q_idx
        pSet->param_i[1] = 0; // state
        pSet->param_i[2] = 0; // timer

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }
}