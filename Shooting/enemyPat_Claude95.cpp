// enemyPat_KaizanRyoiki.cpp
//
// 「改竄領域 -チートコード顕現-」
// 反則(チート行為)をテーマにした4フェーズ弾幕パターン。
// 通常は書き換えないはずの変数(自機座標・自機弾座標・敵HP)を
// パターン側から直接改竄することで、「チートそのもの」を弾幕表現として再現する。
//
//   フェーズ1 起動シーケンス : ドットマトリクス文字弾で「CHEAT MODE ON」を実体化
//   フェーズ2 速度改竄       : 弾ごとに速度倍率を変えるスピードハック演出。一斉フリーズ演出付き
//   フェーズ3 座標改竄       : 自機座標を強制的にワープさせる檻ハック(自機弾の座標も道連れにする)
//   フェーズ4 HP偽装         : 敵HPをドット数字弾でスロット表示しつつ実変数も改竄。最後に露呈して1まで低下
//
// 専用画像素材は使用せず、すべて敵弾(img_enemyShot*)のみで表現している。
// 敵本体関数名は指定の通り EnemyPat_Violate_Claude() とする。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <string.h>

// ============================================================
// フェーズ定義(フレーム数は60fps基準)
// ============================================================
static constexpr int TEXT_CONVERGE_FRAMES = 40;   // 文字弾が収束するまでのフレーム数
static constexpr int TEXT_DISPERSE_FRAME = 130;  // 文字弾がここ(pShot->count)から飛散開始

static constexpr int PHASE2_START = 171;
static constexpr int PHASE2_END = 390;
static constexpr int PHASE2_PULSE_INTERVAL = 20;
static constexpr int PHASE2_FREEZE_START = 355; // 全弾一斉フリーズ開始(グローバルcount)
static constexpr int PHASE2_FREEZE_END = 367; // フリーズ解除(グローバルcount)

static constexpr int PHASE3_START = 391;
static constexpr int CAGE_CYCLE = 75;  // 檻ハック1サイクルのフレーム数
static constexpr int CAGE_CYCLE_COUNT = 3;   // 檻ハックの繰り返し回数
static constexpr int CAGE_FORM = 20;  // 檻形成
static constexpr int CAGE_HOLD = 25;  // 拘束(自機座標を強制固定)
static constexpr int CAGE_RELEASE = 15;  // 解放(放射状に飛散)
// 残り CAGE_CYCLE-(FORM+HOLD+RELEASE)=15 フレームは自由行動の隙間(GAP)
static constexpr int PHASE3_END = PHASE3_START + CAGE_CYCLE * CAGE_CYCLE_COUNT; // 616

static constexpr int PHASE4_START = PHASE3_END;                 // 616
static constexpr int PHASE4_FLICKER_COUNT = 6;
static constexpr int PHASE4_FLICKER_INTERVAL = 8;
static constexpr int PHASE4_9999_AT = PHASE4_START + PHASE4_FLICKER_COUNT * PHASE4_FLICKER_INTERVAL; // 664
static constexpr int PHASE4_9999_LIFE = 40;
static constexpr int PHASE4_CRASH_AT = PHASE4_9999_AT + PHASE4_9999_LIFE + 1; // 705
static constexpr int FINALE_TRIGGER = PHASE4_CRASH_AT + 90;                  // 795

static constexpr double DOT_SPACING = 6.0;  // ドット間隔
static constexpr double CHAR_SPACING = 34.0; // 文字間隔(中心間)

// ============================================================
// 5x7 ドットマトリクスフォント(このパターンで必要な文字のみ)
// ============================================================
namespace KaizanFont {
    struct Glyph { const char* row[7]; };
    struct Cell { int col; int row; };

    static const Glyph G_C = { {".###.","#....","#....","#....","#....","#....",".###."} };
    static const Glyph G_H = { {"#...#","#...#","#...#","#####","#...#","#...#","#...#"} };
    static const Glyph G_E = { {"#####","#....","#....","####.","#....","#....","#####"} };
    static const Glyph G_A = { {".###.","#...#","#...#","#####","#...#","#...#","#...#"} };
    static const Glyph G_T = { {"#####","..#..","..#..","..#..","..#..","..#..","..#.."} };
    static const Glyph G_M = { {"#...#","##.##","#.#.#","#...#","#...#","#...#","#...#"} };
    static const Glyph G_O = { {".###.","#...#","#...#","#...#","#...#","#...#",".###."} };
    static const Glyph G_D = { {"####.","#...#","#...#","#...#","#...#","#...#","####."} };
    static const Glyph G_N = { {"#...#","##..#","#.#.#","#..##","#...#","#...#","#...#"} };

    static const Glyph G_0 = { {".###.","#...#","#..##","#.#.#","##..#","#...#",".###."} };
    static const Glyph G_1 = { {"..#..",".##..","..#..","..#..","..#..","..#..",".###."} };
    static const Glyph G_2 = { {".###.","#...#","....#","...#.","..#..",".#...","#####"} };
    static const Glyph G_3 = { {".###.","#...#","....#","..##.","....#","#...#",".###."} };
    static const Glyph G_4 = { {"...#.","..##.",".#.#.","#..#.","#####","...#.","...#."} };
    static const Glyph G_5 = { {"#####","#....","####.","....#","....#","#...#",".###."} };
    static const Glyph G_6 = { {"..##.",".#...","#....","####.","#...#","#...#",".###."} };
    static const Glyph G_7 = { {"#####","....#","...#.","..#..",".#...",".#...",".#..."} };
    static const Glyph G_8 = { {".###.","#...#","#...#",".###.","#...#","#...#",".###."} };
    static const Glyph G_9 = { {".###.","#...#","#...#",".####","....#","...#.",".##.."} };

    static const Glyph* GetLetterGlyph(char c)
    {
        switch (c) {
        case 'C': return &G_C;
        case 'H': return &G_H;
        case 'E': return &G_E;
        case 'A': return &G_A;
        case 'T': return &G_T;
        case 'M': return &G_M;
        case 'O': return &G_O;
        case 'D': return &G_D;
        case 'N': return &G_N;
        default:  return nullptr;
        }
    }

    static const Glyph* GetDigitGlyph(int d)
    {
        switch (d) {
        case 0: return &G_0;
        case 1: return &G_1;
        case 2: return &G_2;
        case 3: return &G_3;
        case 4: return &G_4;
        case 5: return &G_5;
        case 6: return &G_6;
        case 7: return &G_7;
        case 8: return &G_8;
        case 9: return &G_9;
        default: return nullptr;
        }
    }

    // グリフの点灯セルを列挙する。out は最低35要素確保しておくこと。
    static int GlyphCells(const Glyph* g, Cell* out)
    {
        int n = 0;
        for (int r = 0; r < 7; r++) {
            for (int c = 0; c < 5; c++) {
                if (g->row[r][c] == '#') {
                    out[n].col = c;
                    out[n].row = r;
                    n++;
                }
            }
        }
        return n;
    }
}

// ============================================================
// 共通ヘルパー: sEnemyShotSet を1つ生成してリストへ連結する
// (pEnemyShotHead の番人ノード初期化まで含めて行う)
// ============================================================
static sEnemyShotSet* SpawnShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki = 0.0)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;

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
// フェーズ1: 「CHEAT MODE ON」文字弾(画面外縁から収束→静止→放射飛散)
// ============================================================
static void PatTextGlyph(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const char* text = "CHEAT MODE ON";
        int len = (int)strlen(text);
        double totalWidth = len * CHAR_SPACING;
        double startX = pSet->x - totalWidth / 2.0;

        for (int idx = 0; idx < len; idx++) {
            char c = text[idx];
            if (c == ' ') continue;

            const KaizanFont::Glyph* g = KaizanFont::GetLetterGlyph(c);
            if (!g) continue;

            KaizanFont::Cell cells[35];
            int n = KaizanFont::GlyphCells(g, cells);
            double charX = startX + idx * CHAR_SPACING;

            for (int i = 0; i < n; i++) {
                sEnemyShot* pShot = new sEnemyShot;

                double tx = charX + cells[i].col * DOT_SPACING;
                double ty = pSet->y + cells[i].row * DOT_SPACING;

                // 収束開始位置: 画面外縁付近のランダムな1点(リプレイ再現性あり)
                double angle = GetRand(3600) / 3600.0 * 2.0 * DX_PI;
                double startDist = 260.0 + GetRand(120);
                double sx = tx + startDist * cos(angle);
                double sy = ty + startDist * sin(angle);

                pShot->x = sx;
                pShot->y = sy;
                pShot->muki = angle;
                pShot->speed = 0;
                pShot->param_d[0] = sx;
                pShot->param_d[1] = sy;
                pShot->param_d[2] = tx;
                pShot->param_d[3] = ty;
                pShot->kind = img_enemyShotSmallBall[6]; // 白
                pShot->margin = 480;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double sx = pShot->param_d[0], sy = pShot->param_d[1];
        double tx = pShot->param_d[2], ty = pShot->param_d[3];

        if (pShot->count < TEXT_CONVERGE_FRAMES) {
            double t = pShot->count / (double)TEXT_CONVERGE_FRAMES;
            double e = 1.0 - (1.0 - t) * (1.0 - t); // ease-out
            pShot->x = sx + (tx - sx) * e;
            pShot->y = sy + (ty - sy) * e;
        }
        else if (pShot->count < TEXT_DISPERSE_FRAME) {
            pShot->x = tx;
            pShot->y = ty;
        }
        else {
            double t = pShot->count - TEXT_DISPERSE_FRAME;
            double dist = 0.5 * t * t;
            double angle = (tx == pSet->x && ty == pSet->y)
                ? 0.0
                : atan2(ty - pSet->y, tx - pSet->x);
            pShot->muki = angle;
            pShot->x = tx + dist * cos(angle);
            pShot->y = ty + dist * sin(angle);
        }

        pShot = pShot->next;
    }
}

// ============================================================
// フェーズ2: スピードハック(弾ごとに速度倍率が違う環状弾+一斉フリーズ)
// ============================================================
static void PatSpeedHack(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int n = pSet->param_i[0]; // way数
        double baseAngle = pSet->muki;

        for (int i = 0; i < n; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = baseAngle + (2.0 * DX_PI) * i / n;

            int roll = GetRand(9); // 0-9
            double speedMult;
            int colorIdx;
            if (roll < 2) { speedMult = 0.2; colorIdx = 3; }      // 低速改竄: シアン(20%)
            else if (roll < 8) { speedMult = 1.0; colorIdx = 6; } // 通常速度: 白(60%)
            else { speedMult = 5.0; colorIdx = 0; }                // 高速改竄: 赤(20%)

            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 0;
            pShot->param_d[0] = angle;
            pShot->param_d[1] = 1.4 * speedMult; // 基礎速度(改竄済み)
            pShot->kind = img_enemyShotSmallBall[colorIdx];
            pShot->margin = 480;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double angle = pShot->param_d[0];
        double baseSpeed = pShot->param_d[1];

        // グローバルcountに同期した一斉フリーズ(巻き戻し風の一時停止)
        double frozen = 0.0;
        if (count > PHASE2_FREEZE_END) frozen = (double)(PHASE2_FREEZE_END - PHASE2_FREEZE_START);
        else if (count > PHASE2_FREEZE_START) frozen = (double)(count - PHASE2_FREEZE_START);

        double effective = pShot->count - frozen;
        if (effective < 0) effective = 0;

        double dist = baseSpeed * effective;
        pShot->x = pSet->x + dist * cos(angle);
        pShot->y = pSet->y + dist * sin(angle);

        pShot = pShot->next;
    }
}

// ============================================================
// 汎用: 自機狙いN-way(公平性担保のための通常弾)
// ============================================================
static void PatAimedFan(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int n = pSet->param_i[0];         // way数
        double spread = pSet->param_d[0]; // 全体の広がり(ラジアン)
        double baseAngle = atan2(player.y - pSet->y, player.x - pSet->x);

        for (int i = 0; i < n; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double t = (n <= 1) ? 0.0 : ((double)i / (n - 1) - 0.5);
            double angle = baseAngle + t * spread;

            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 2.6;
            pShot->param_d[0] = angle;
            pShot->kind = img_enemyShotBullet[0]; // 赤
            pShot->margin = 480;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double angle = pShot->param_d[0];
        pShot->x = pSet->x + pShot->speed * pShot->count * cos(angle);
        pShot->y = pSet->y + pShot->speed * pShot->count * sin(angle);
        pShot = pShot->next;
    }
}

// ============================================================
// フェーズ3: 檻ハック(自機ワープ座標に形成されるリング)
// ============================================================
static constexpr int CAGE_RING_N = 28;
static constexpr double CAGE_R_START = 220.0;
static constexpr double CAGE_R_HOLD = 42.0; // 自機が安全に収まる半径

static void PatCage(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < CAGE_RING_N; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = 2.0 * DX_PI * i / CAGE_RING_N;

            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 0;
            pShot->param_d[0] = angle;
            pShot->kind = img_enemyShotMediumBall[5]; // マゼンタ
            pShot->margin = 480;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double baseAngle = pShot->param_d[0];
        double rot = baseAngle + 0.01 * pShot->count; // 拘束感を出すゆるやかな回転
        double r;

        if (pShot->count < CAGE_FORM) {
            double t = pShot->count / (double)CAGE_FORM;
            double e = 1.0 - (1.0 - t) * (1.0 - t);
            r = CAGE_R_START + (CAGE_R_HOLD - CAGE_R_START) * e;
        }
        else if (pShot->count < CAGE_FORM + CAGE_HOLD) {
            r = CAGE_R_HOLD;
        }
        else {
            double t = pShot->count - (CAGE_FORM + CAGE_HOLD);
            r = CAGE_R_HOLD + 0.9 * t * t; // 解放: 放射状に加速飛散
        }

        pShot->x = pSet->x + r * cos(rot);
        pShot->y = pSet->y + r * sin(rot);

        pShot = pShot->next;
    }
}

// ============================================================
// フェーズ4: HP偽装のドット数字弾(4桁固定・0埋め)
//   isFinal=0: 短命(param_i[1]で指定)で自己タイマー飛散するスロット表示用
//   isFinal=1: グローバルcountがFINALE_TRIGGERに達するまで静止し続ける確定表示用
// ============================================================
static void PatDigitGlyph(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int value = pSet->param_i[0];
        int life = pSet->param_i[1];
        int isFinal = pSet->param_i[2];

        char buf[5];
        buf[0] = '0' + (value / 1000) % 10;
        buf[1] = '0' + (value / 100) % 10;
        buf[2] = '0' + (value / 10) % 10;
        buf[3] = '0' + value % 10;
        buf[4] = '\0';

        double originX = pSet->x;
        double originY = pSet->y;
        double totalWidth = 4 * CHAR_SPACING;
        double startX = originX - totalWidth / 2.0;

        for (int idx = 0; idx < 4; idx++) {
            const KaizanFont::Glyph* g = KaizanFont::GetDigitGlyph(buf[idx] - '0');
            if (!g) continue;

            KaizanFont::Cell cells[35];
            int n = KaizanFont::GlyphCells(g, cells);
            double charX = startX + idx * CHAR_SPACING;

            for (int i = 0; i < n; i++) {
                sEnemyShot* pShot = new sEnemyShot;

                double tx = charX + cells[i].col * DOT_SPACING;
                double ty = originY + cells[i].row * DOT_SPACING;
                double angle = (tx == originX && ty == originY)
                    ? 0.0
                    : atan2(ty - originY, tx - originX);

                pShot->x = tx;
                pShot->y = ty;
                pShot->muki = angle;
                pShot->speed = 0;
                pShot->param_d[0] = tx;
                pShot->param_d[1] = ty;
                pShot->param_d[2] = angle;
                pShot->param_i[0] = life;
                pShot->param_i[1] = isFinal;
                pShot->kind = isFinal ? img_enemyShotDiamond[0] : img_enemyShotDiamond[5]; // 確定:赤/仮:マゼンタ
                pShot->margin = 480;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double tx = pShot->param_d[0];
        double ty = pShot->param_d[1];
        double angle = pShot->param_d[2];
        int life = pShot->param_i[0];
        int isFinal = pShot->param_i[1];

        if (!isFinal) {
            if (pShot->count < life) {
                pShot->x = tx;
                pShot->y = ty;
            }
            else {
                double t = pShot->count - life;
                double dist = 0.7 * t * t;
                pShot->x = tx + dist * cos(angle);
                pShot->y = ty + dist * sin(angle);
            }
        }
        else {
            if (count < FINALE_TRIGGER) {
                pShot->x = tx;
                pShot->y = ty;
            }
            else {
                double t = count - FINALE_TRIGGER;
                double dist = 0.6 * t * t;
                pShot->x = tx + dist * cos(angle);
                pShot->y = ty + dist * sin(angle);
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Violate_Claude()
{
    static double warpTargetX;
    static double warpTargetY;

    // ---- 初期化 ----
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200;
        warpTargetX = 240.0;
        warpTargetY = 300.0;

        // フェーズ1: 起動シーケンス(CHEAT MODE ONの実体化)
        SpawnShotSet(PatTextGlyph, 240.0, 140.0);
    }

    // ---- 敵本体: フェーズ2中だけ短時間の座標グリッチを起こす(演出) ----
    enemy.x = 240.0;
    enemy.y = 90.0;
    if (count >= PHASE2_START && count < PHASE2_END) {
        int window = (count - PHASE2_START) % 45;
        static int glitchDx = 0, glitchDy = 0;
        if (window == 0) {
            glitchDx = GetRand(30) - 15;
            glitchDy = GetRand(20) - 10;
        }
        if (window < 3) {
            enemy.x += glitchDx;
            enemy.y += glitchDy;
        }
    }

    // ---- フェーズ2: 速度改竄(スピードハック) ----
    if (count >= PHASE2_START && count < PHASE2_END &&
        (count - PHASE2_START) % PHASE2_PULSE_INTERVAL == 0) {

        int pulse = (count - PHASE2_START) / PHASE2_PULSE_INTERVAL;
        double baseAngle = GetRand(3600) / 3600.0 * 2.0 * DX_PI;

        sEnemyShotSet* pRing = SpawnShotSet(PatSpeedHack, enemy.x, enemy.y + 15.0, baseAngle);
        pRing->param_i[0] = 20; // 20way

        if (pulse % 2 == 1) {
            sEnemyShotSet* pAim = SpawnShotSet(PatAimedFan, enemy.x, enemy.y + 15.0);
            pAim->param_i[0] = 3;
            pAim->param_d[0] = 0.5;
        }
    }

    // ---- フェーズ3: 座標改竄(自機ワープハック) ----
    if (count >= PHASE3_START && count < PHASE3_END) {
        int lc = count - PHASE3_START;
        int withinCycle = lc % CAGE_CYCLE;

        // サイクル開始: 新しい強制ワープ先を決定し、檻リングを形成開始
        if (withinCycle == 0) {
            warpTargetX = 70.0 + GetRand(340);  // 70〜410
            warpTargetY = 190.0 + GetRand(220); // 190〜410
            SpawnShotSet(PatCage, warpTargetX, warpTargetY);
        }

        // 強制ワープ発動の瞬間: 自機座標を直接書き換える(反則そのもの)
        if (withinCycle == CAGE_FORM) {
            double dx = warpTargetX - player.x;
            double dy = warpTargetY - player.y;

            // 自機ショットの座標も道連れにワープさせる
            sPlayerShot* pPS = playerShotHead.next;
            while (pPS != &playerShotHead) {
                pPS->x += dx;
                pPS->y += dy;
                pPS = pPS->next;
            }

            player.x = warpTargetX;
            player.y = warpTargetY;
        }

        // 拘束中: 毎フレーム座標を強制的に固定し続け、操作を無効化する
        if (withinCycle >= CAGE_FORM && withinCycle < CAGE_FORM + CAGE_HOLD) {
            player.x = warpTargetX;
            player.y = warpTargetY;
        }

        // 解放後の隙間(GAP)に自機狙い攻撃を差し込み、緊張を継続させる
        if (withinCycle == CAGE_FORM + CAGE_HOLD + CAGE_RELEASE + 5) {
            sEnemyShotSet* pAim = SpawnShotSet(PatAimedFan, enemy.x, enemy.y + 15.0);
            pAim->param_i[0] = 5;
            pAim->param_d[0] = 0.7;
        }
    }

    // ---- フェーズ4: HP偽装(改竄の露呈) ----
    if (count >= PHASE4_START && count < PHASE4_9999_AT &&
        (count - PHASE4_START) % PHASE4_FLICKER_INTERVAL == 0) {

        int val = GetRand(9999);
        sEnemyShotSet* pDigit = SpawnShotSet(PatDigitGlyph, 240.0, 130.0);
        pDigit->param_i[0] = val;
        pDigit->param_i[1] = PHASE4_FLICKER_INTERVAL;
        pDigit->param_i[2] = 0; // isFinal = false
    }

    if (count == PHASE4_9999_AT) {
        enemy.maxHp = enemy.hp = 9999; // 数値改竄を実変数へもそのまま反映

        sEnemyShotSet* pDigit = SpawnShotSet(PatDigitGlyph, 240.0, 130.0);
        pDigit->param_i[0] = 9999;
        pDigit->param_i[1] = PHASE4_9999_LIFE;
        pDigit->param_i[2] = 0;
    }

    if (count == PHASE4_CRASH_AT) {
        enemy.maxHp = 200;
        enemy.hp = 1; // 改竄が露呈し、本来より弱体化した状態で終わる

        sEnemyShotSet* pDigit = SpawnShotSet(PatDigitGlyph, 240.0, 130.0);
        pDigit->param_i[0] = 1;
        pDigit->param_i[1] = 0;  // isFinal=1のため未使用
        pDigit->param_i[2] = 1;  // isFinal = true
    }

    if (count == FINALE_TRIGGER) {
        sEnemyShotSet* pBurst = SpawnShotSet(PatAimedFan, enemy.x, enemy.y + 15.0);
        pBurst->param_i[0] = 5;
        pBurst->param_d[0] = 0.8;
    }
}