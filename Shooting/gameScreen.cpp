// gameScreen.cpp

#define _CRT_SECURE_NO_WARNINGS

#include "DxLib.h"
#include "stateManager.h"
#include "gameScreen.h"
#include "gv.h"
#include "stageData.h"
#include "imgSoundLoad.h"
#include "menu.h"
#include "replay.h"
#include "player.h"
#include "recordController.h"
#include "masterpieceViewer.h"
#include "effectRand.h"
#include <math.h>
#include <string> 
#include <vector> 
#include <ctype.h>   // isdigit 用


#define NUM_STARS 30

typedef struct {
    double x, y, speedY;
    int brightness, twinklePhase, color, size;
} Star;

static Star stars[NUM_STARS];
static int starsInitialized = 0;
static int fieldBG = -1;
static int leftSidePanelBG = -1;
static int rightSidePanelBG = -1;
static int lastStageForSidePanel = -1;   // 生成時の stageNum

// 情報領域の背景色
static const int INFO_BG_COLOR = GetColor(30, 30, 50);

// 戦闘フィールド背景を一枚の画像として生成
static void createFieldBG() {
    fieldBG = MakeScreen(480, 480, FALSE);
    int oldScreen = GetDrawScreen();
    SetDrawScreen(fieldBG);

    // 宇宙空間のグラデーション
    for (int i = 0; i < 480; i++) {
        int shade = (int)(20.0 + 50.0 * (1.0 - (double)i / 480.0));
        int col = GetColor(shade, shade, shade + 20);
        DrawBox(0, i, 480, i + 1, col, TRUE);
    }

    // グリッド線（半透明）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 20);
    int gridColor = GetColor(255, 255, 255);
    for (int x = 80; x < 480; x += 80) {
        DrawLine(x, 0, x, 479, gridColor);
    }
    for (int y = 80; y < 480; y += 80) {
        DrawLine(0, y, 479, y, gridColor);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    SetDrawScreen(oldScreen);
}

static int descLineCount = 0;               // 説明文の行数（静的）
static int stageDescLineCount = 0;          // 長い説明文の行数（静的）

// ステージIDから作成者名と数字を抽出する
static void parseStageId(const char* stageId, char* creator, int* number) {
    int len = (int)strlen(stageId);
    int numStart = len;
    while (numStart > 0 && isdigit((unsigned char)stageId[numStart - 1])) {
        numStart--;
    }
    if (numStart < len) {
        *number = atoi(stageId + numStart);
        strncpy(creator, stageId, numStart);
        creator[numStart] = '\0';
    }
    else {
        *number = 0;
        strcpy(creator, stageId);
    }
}

typedef struct {
    int playCountY;
    int bestTimeY;
    int timeY;
    int bossSeparatorY;
    int bossLabelY;
    int hpBarY;
    int replayY;
} SidePanelLayout;

static SidePanelLayout computeSidePanelLayout() {
    SidePanelLayout layout;
    const int lineHeight = 16;
    const int halfLine = lineHeight / 2;
    const int quarterLine = lineHeight / 4;

    int y = 40;
    y += descLineCount * 20;
    y += lineHeight;  // 空行

    layout.playCountY = y;
    y += lineHeight;

    y += lineHeight;  // Play Count と Best の間
    layout.bestTimeY = y;
    y += lineHeight;

    layout.timeY = y;
    y += lineHeight;

    y += halfLine;
    layout.bossSeparatorY = y;
    y += 1 + halfLine;

    layout.hpBarY = y;
    y += 10; // HPバー高さ

    layout.bossLabelY = y;
    y += lineHeight;

    y += lineHeight;  // HPバー下の余白

    layout.replayY = y;
    return layout;
}

static void createLeftSidePanelBG() {
    if (leftSidePanelBG != -1) DeleteGraph(leftSidePanelBG);

    const int panelWidth = 187;
    leftSidePanelBG = MakeScreen(panelWidth, 480, FALSE);
    int oldScreen = GetDrawScreen();
    SetDrawScreen(leftSidePanelBG);

    // ---- 背景描画（既存の createSidePanelBG と同様のグラデーション等）----
    // 1. 背景縦グラデーション
    for (int y = 0; y < 480; y++) {
        int t = y * 100 / 479;
        int r = 45 - (45 - 25) * t / 100;
        int g = 45 - (45 - 28) * t / 100;
        int b = 65 - (65 - 42) * t / 100;
        DrawLine(0, y, panelWidth, y, GetColor(r, g, b));
    }
    // 2. 左側アクセントバー
    for (int y = 0; y < 480; y++) {
        int t = y * 100 / 479;
        int r = 0;
        int g = 180 - 120 * t / 100;
        int b = 255 - 75 * t / 100;
        DrawLine(0, y, 2, y, GetColor(r, g, b));
    }
    // 3. 枠
    DrawBox(0, 0, panelWidth - 1, 479, GetColor(120, 140, 180), FALSE);
    DrawBox(2, 2, panelWidth - 3, 477, GetColor(70, 90, 120), FALSE);
    // 4. ヘッダー背景
    for (int y = 0; y < 30; y++) {
        int t = y * 100 / 29;
        int r = 10 + t * 20 / 100;
        int g = 40 + t * 30 / 100;
        int b = 80 + t * 30 / 100;
        DrawLine(0, y, panelWidth, y, GetColor(r, g, b));
    }
    DrawLine(0, 30, panelWidth, 30, GetColor(100, 200, 255));
    // 5. 情報エリア背景
    for (int y = 36; y < 480-5; y++) {
        int t = (y - 36) * 100 / (479 - 36);
        int r = 22 - t * 8 / 100;
        int g = 25 - t * 10 / 100;
        int b = 40 - t * 15 / 100;
        DrawLine(5, y, panelWidth - 5, y, GetColor(r, g, b));
    }
    // 6. 格子パターン
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 8);
    int gridColor = GetColor(255, 255, 255);
    for (int x = 5; x < panelWidth - 5; x += 20) {
        DrawLine(x, 36, x, 479, gridColor);
    }
    for (int y = 36; y < 480; y += 20) {
        DrawLine(5, y, panelWidth - 6, y, gridColor);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ---- 固定コンテンツ ----

    const int panelLeft = 10;
    const int descMaxWidth = panelWidth - 20;

    // タイトル
    char creator[64];
    int stageNumber;
    parseStageId(stageData[stageNum].stageId, creator, &stageNumber);
    if (strcmp(creator, "Zai") == 0) strcpy(creator, "Z.ai");
    DrawFormatString(panelLeft, 3, GetColor(255, 255, 255),
        "STAGE %d(%s)", stageNumber, creator);

    // 短い説明文
    int currentY = 40;
    std::vector<std::string> descLines = WrapText(stageData[stageNum].description, descMaxWidth + 15);
    descLineCount = (int)descLines.size();
    for (int i = 0; i < descLineCount; i++) {
        DrawFormatString(panelLeft, currentY, GetColor(180, 200, 220),
            "%s", descLines[i].c_str());
        currentY += 20;
    }

    // BOSS区切り線とHPバー枠
    SidePanelLayout layout = computeSidePanelLayout();
    DrawBox(panelLeft, layout.hpBarY,
        panelLeft + descMaxWidth, layout.hpBarY,
        GetColor(0, 255, 255), FALSE);

    // 傑作選コメント（静的描画）
    if (masterpieceMode && g_masterpieceComment) {
        int lineHeight = 16;
        currentY += 180;
        if (stageData[stageNum].stageId == "Grok67") currentY += 36;
        DrawString(panelLeft, currentY, "[一言コメント]", GetColor(180, 200, 220));
        currentY += 20;
        const int commentAreaWidth = GAME_AREA_X - 10;
        std::vector<std::string> commentLines = WrapText(g_masterpieceComment, commentAreaWidth);
        for (const auto& line : commentLines) {
            DrawString(panelLeft, currentY, line.c_str(), GetColor(255, 255, 255));
            currentY += lineHeight + 2;
        }
    }

    SetDrawScreen(oldScreen);
}

static void createRightSidePanelBG() {
    if (rightSidePanelBG != -1) DeleteGraph(rightSidePanelBG);

    const int panelWidth = 187;
    rightSidePanelBG = MakeScreen(panelWidth, 480, FALSE);
    int oldScreen = GetDrawScreen();
    SetDrawScreen(rightSidePanelBG);

    // 背景は左パネルと同様（コピー）でもよいが、少し色を変えてもよい
    // ここでは左パネルと同じ背景を描画
    for (int y = 0; y < 480; y++) {
        int t = y * 100 / 479;
        int r = 45 - (45 - 25) * t / 100;
        int g = 45 - (45 - 28) * t / 100;
        int b = 65 - (65 - 42) * t / 100;
        DrawLine(0, y, panelWidth, y, GetColor(r, g, b));
    }
    for (int y = 0; y < 480; y++) {
        int t = y * 100 / 479;
        int r = 0;
        int g = 180 - 120 * t / 100;
        int b = 255 - 75 * t / 100;
        DrawLine(0, y, 2, y, GetColor(r, g, b));
    }
    DrawBox(0, 0, panelWidth - 1, 479, GetColor(120, 140, 180), FALSE);
    DrawBox(2, 2, panelWidth - 3, 477, GetColor(70, 90, 120), FALSE);
    
    // 5. 情報エリア背景（濃紺グラデーション）※ヘッダーなしなのでy=0から開始
    for (int y = 0+5; y < 480-5; y++) {
        int t = y * 100 / 479;
        int r = 22 - t * 8 / 100;
        int g = 25 - t * 10 / 100;
        int b = 40 - t * 15 / 100;
        DrawLine(5, y, panelWidth - 5, y, GetColor(r, g, b));
    }

    // 6. 格子パターン（低アルファ・決定的）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 8);
    int gridColor = GetColor(255, 255, 255);
    for (int x = 5; x < panelWidth - 5; x += 20) {
        DrawLine(x, 0, x, 479, gridColor);
    }
    for (int y = 0; y < 480; y += 20) {
        DrawLine(5, y, panelWidth - 6, y, gridColor);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ---- 長い説明文を表示 ----

    const int panelLeft = 5;
    const int descMaxWidth = panelWidth - 5;

    char creator[64];
    int stageNumber;
    parseStageId(stageData[stageNum].stageId, creator, &stageNumber);
    if (strcmp(creator, "Zai") == 0) strcpy(creator, "Z.ai");

    int y = 5;
    DrawFormatString(panelLeft, y, GetColor(180, 200, 220),
        "[%sによる説明]", creator);
    y += 20;

    // ここでフォントサイズを切り替える
    int defaultFontSize = GetFontSize();
    int lineHeight = 20;

    // 最終ステージの stageDescription より長い場合は小さめにする
    if (strlen(stageData[stageNum].stageDescription) >
        strlen(stageData[stageData.size() - 1].stageDescription)) {
        SetFontSize(defaultFontSize - 2);
        lineHeight -= 1;
    }

    std::vector<std::string> stageDescLines =
        WrapText(stageData[stageNum].stageDescription, descMaxWidth);
    stageDescLineCount = (int)stageDescLines.size();

    for (int i = 0; i < stageDescLineCount; i++) {
        DrawFormatString(panelLeft, y, GetColor(255, 255, 255),
            "%s", stageDescLines[i].c_str());
        y += lineHeight;
    }

    // 描画後に元のフォントサイズへ戻す
    SetFontSize(defaultFontSize);

    SetDrawScreen(oldScreen);
}

void resetStars() {
    starsInitialized = 0;
}

void initStars() {
    if (starsInitialized) return;
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = effectRandInt(478);
        stars[i].y = effectRandInt(478);
        stars[i].speedY = 0.3 + (double)effectRandInt(120) / 100.0;
        stars[i].brightness = effectRandInt(255);
        stars[i].twinklePhase = effectRandInt(360);
        stars[i].size = effectRandInt(3) + 1;
        int r = 200 + effectRandInt(55);
        int g = 200 + effectRandInt(55);
        int b = 200 + effectRandInt(55);
        stars[i].color = GetColor(r, g, b);
    }
    starsInitialized = 1;
}

void updateStars()
{
    initStars();
    // 現在の状態に応じて星を移動（Game/Replay中のみ）
    if (StateManager::GetState() == Joutai::Game || StateManager::GetState() == Joutai::Replay) {
        for (int i = 0; i < NUM_STARS; i++) {
            stars[i].y += stars[i].speedY;
            if (stars[i].y > 480.0) {
                stars[i].y -= 480.0;
                stars[i].x = effectRandInt(478);
            }
        }
    }
}

void backGround()
{
    initStars();
    updateStars();

    if (fieldBG == -1) createFieldBG();
    DrawGraph(GAME_AREA_X + 0, 0, fieldBG, FALSE);

    for (int i = 0; i < NUM_STARS; i++) {
        double twinkle = 0.5 + 0.5 * sin((double)(count * 2 + stars[i].twinklePhase) / 60.0);
        int alpha = (int)(stars[i].brightness * (0.5 + 0.5 * twinkle));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        int cx = (int)stars[i].x;
        int cy = (int)stars[i].y;
        int half = stars[i].size;
        DrawLine(GAME_AREA_X + cx - half, cy, GAME_AREA_X + cx + half, cy, stars[i].color);
        DrawLine(GAME_AREA_X + cx, cy - half, GAME_AREA_X + cx, cy + half, stars[i].color);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 特殊演出
void special_performance() {
    static int img_ChatGPT33 = -1;
    static int img_Zai48 = -1;
    static int img_Kimi59_1 = -1;
    static int img_Kimi59_2 = -1;
    static int img_Kimi59_3 = -1;
    static int img_Gemini81_1 = -1;
    static int img_Gemini81_2 = -1;
    static int img_Gemini81_3 = -1;
    static int drawX = 0, drawY = 0;
    static int drawH = 0, drawW = 0;

    if (stageData[stageNum].stageId == "ChatGPT33") {
        if (count == 120) {
            if (img_ChatGPT33 == -1) {
                img_ChatGPT33 = LoadGraph("assets/images/ChatGPT33.png");
                if (img_ChatGPT33 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_ChatGPT33, &imgW, &imgH);
                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);
                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        if (count >= 120 && img_ChatGPT33 != -1) {
            DrawGraph(drawX, drawY, img_ChatGPT33, TRUE);
        }
    }
    else if (stageData[stageNum].stageId == "Zai48") {
        if (count == 120) {
            if (img_Zai48 == -1) {
                img_Zai48 = LoadGraph("assets/images/Zai48.png");
                if (img_Zai48 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_Zai48, &imgW, &imgH);
                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);
                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        if (count >= 120 && img_Zai48 != -1) {
            DrawGraph(drawX, drawY, img_Zai48, TRUE);
        }
    }
    else if (stageData[stageNum].stageId == "Kimi59") {
        const int T1 = 120;
        if (count == T1) {
            if (img_Kimi59_1 == -1) {
                img_Kimi59_1 = LoadGraph("assets/images/Kimi59_1.png");
                if (img_Kimi59_1 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_Kimi59_1, &imgW, &imgH);
                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);
                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        const int T2 = 240;
        if (count >= T1 && count < T1 + T2 && img_Kimi59_1 != -1) {
            DrawGraph(drawX, drawY, img_Kimi59_1, TRUE);
        }

        if (count == T1 + T2) {
            if (img_Kimi59_2 == -1) {
                img_Kimi59_2 = LoadGraph("assets/images/Kimi59_2.png");
                if (img_Kimi59_2 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_Kimi59_2, &imgW, &imgH);
                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);
                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        const int T3 = 240;
        if (count >= T1 + T2 && count < T1 + T2 + T3 && img_Kimi59_2 != -1) {
            DrawGraph(drawX, drawY, img_Kimi59_2, TRUE);
        }

        if (count == T1 + T2 + T3) {
            if (img_Kimi59_3 == -1) {
                img_Kimi59_3 = LoadGraph("assets/images/Kimi59_3.png");
                if (img_Kimi59_3 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_Kimi59_3, &imgW, &imgH);
                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);
                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }            
        }

        if (count >= T1 + T2 + T3 && img_Kimi59_3 != -1) {
            static int centerX = 0, centerY = 0;
            if (count == T1 + T2 + T3) {
                int w, h;
                GetGraphSize(img_Kimi59_3, &w, &h);
                centerX = drawX + w / 2;
                centerY = drawY + h / 2 - 30;
            }

            for (int t = T1 + T2 + T3; t <= count; t += 20) {
                // 1. 拡大率：t に比例して線形に増加
                double scale = 1.0 + (t - (T1 + T2 + T3)) * 0.002;   // 1フレームあたり0.01ずつ拡大

                // 2. 回転角度：t に応じた疑似乱数（毎回同じ値になる決定論的なハッシュ）
                //    例: 大きめの素数を使って疑似的にランダムな角度を生成
                unsigned int hash = (unsigned int)(t * 2654435761u); // Knuth's multiplicative hash
                double angle = hash % 360 * (DX_PI / 180.0);      // 0～360°をラジアンに

                // 描画
                DrawRotaGraph(centerX, centerY, scale, angle, img_Kimi59_3, TRUE);
            }

            if ((count - (T1 + T2 + T3)) % 20 == 0) {
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
            }
        }
    }
    else if (stageData[stageNum].stageId == "Gemini81") {
        const int T1 = 120;
        if (count == T1) {
            if (img_Gemini81_1 == -1) {
                img_Gemini81_1 = LoadGraph("assets/images/Gemini81_1.png");
            }

            if (img_Gemini81_1 != -1) {
                int imgW, imgH;
                GetGraphSize(img_Gemini81_1, &imgW, &imgH);

                int scrW, scrH;
                GetScreenState(&scrW, &scrH, NULL);

                drawX = (scrW - imgW) / 2;
                drawY = (scrH - imgH) / 2;
            }

            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        const int T2 = 240;
        if (count >= T1 && count < T1 + T2 && img_Gemini81_1 != -1) {
            DrawGraph(drawX, drawY, img_Gemini81_1, TRUE);
        }

        if (count == T1 + T2) {
            if (img_Gemini81_2 == -1) {
                img_Gemini81_2 = LoadGraph("assets/images/Gemini81_2.png");
                if (img_Gemini81_2 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_Gemini81_2, &imgW, &imgH);

                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);

                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        const int T3 = 240;
        if (count >= T1 + T2 && count < T1 + T2 + T3 && img_Gemini81_2 != -1) {
            DrawGraph(drawX, drawY, img_Gemini81_2, TRUE);
        }

        if (count == T1 + T2 + T3) {
            if (img_Gemini81_3 == -1) {
                img_Gemini81_3 = LoadGraph("assets/images/Gemini81_3.png");
                if (img_Gemini81_3 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_Gemini81_3, &imgW, &imgH);

                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);

                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        const int T4 = 360;
        if (count >= T1 + T2 + T3 && count < T1 + T2 + T3 + T4 && img_Gemini81_3 != -1) {
            DrawGraph(drawX, drawY, img_Gemini81_3, TRUE);
        }
    }
}

void foreGround() {
    // 距離の閾値（自機のY座標）
    const double NEAR_Y = 40.0;    // このY座標以下で完全透明
    const double FAR_Y = 100.0;    // このY座標以上で完全不透明

    int titleAlpha = 255;
    if (player.y <= FAR_Y) {
        if (player.y <= NEAR_Y) {
            titleAlpha = 0;
        }
        else {
            double t = (player.y - NEAR_Y) / (FAR_Y - NEAR_Y);
            titleAlpha = (int)(255 * t);
        }
    }

    if (titleAlpha <= 0) return;

    // 文字サイズを設定
    const int FONT_SIZE = 16;               // 好みのサイズに変更
    int BORDER_WIDTH = FONT_SIZE / 10;      // 縁取り太さをフォントサイズから自動計算（ここでは2px相当）
    if (BORDER_WIDTH < 1) BORDER_WIDTH = 1;

    // 縁取り文字の描画設定
    const int TITLE_X = 10;
    int TITLE_Y = is_tate ? 20 : 10;
    const int BORDER_COLOR = GetColor(0, 0, 0);             // 縁取り色（黒）
    const int TEXT_COLOR = GetColor(255, 255, 255);         // 文字色（白）
    const char* titleText = stageData[stageNum].stageTitle; // 実際のステージタイトル

    // デフォルトのフォントサイズを保存してから設定
    int defaultFontSize = GetFontSize();   // DxLib 3.20以降で使用可。なければ無視してOK
    SetFontSize(FONT_SIZE);

    if (titleAlpha < 255) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, titleAlpha / 6);
    }

    // 縁取り（BORDER_WIDTHだけずらす）
    DrawFormatString(GAME_AREA_X + TITLE_X - BORDER_WIDTH, TITLE_Y, BORDER_COLOR, "%s", titleText);
    DrawFormatString(GAME_AREA_X + TITLE_X + BORDER_WIDTH, TITLE_Y, BORDER_COLOR, "%s", titleText);
    DrawFormatString(GAME_AREA_X + TITLE_X, TITLE_Y - BORDER_WIDTH, BORDER_COLOR, "%s", titleText);
    DrawFormatString(GAME_AREA_X + TITLE_X, TITLE_Y + BORDER_WIDTH, BORDER_COLOR, "%s", titleText);

    if (titleAlpha < 255) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, titleAlpha);
    }

    // 本体の文字を描画
    DrawFormatString(GAME_AREA_X + TITLE_X, TITLE_Y, TEXT_COLOR, "%s", titleText);

    if (titleAlpha < 255) {
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // フォントサイズを元に戻す（必要に応じて）
    SetFontSize(defaultFontSize);

    // 特殊演出
    special_performance();

    // 縦動画モード
    if (is_tate) {
        int barX = 5;
        int barY = 2;
        int barWidth = 470;
        int barHeight = 14;

        // 背景（半透明の黒でバー部分を塗りつぶし）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
        DrawBox(GAME_AREA_X + barX, barY, GAME_AREA_X + barX + barWidth, barY + barHeight, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 枠（暗めの赤）
        DrawBox(GAME_AREA_X + barX, barY, GAME_AREA_X + barX + barWidth, barY + barHeight, GetColor(180, 40, 40), FALSE);

        // 内部のHPゲージ（明るい赤）
        int hpWidth = (int)(barWidth * (double)enemy.hp / enemy.maxHp);
        if (hpWidth > 0) {
            DrawBox(GAME_AREA_X + barX + 1, barY + 1, GAME_AREA_X + barX + hpWidth - 1, barY + barHeight - 1, GetColor(255, 60, 60), TRUE);
        }

        // HP数値（白文字で中央付近に）
        SetFontSize(14);
        DrawFormatString(GAME_AREA_X + barX + 140, barY - 1, GetColor(255, 255, 255),
            "BOSS HP: %d / %d", enemy.hp, enemy.maxHp);
        SetFontSize(defaultFontSize);  // foreGround内で定義済みのデフォルトサイズに戻す

        if (stageData[stageNum].stageId == "handmade8") {
			int bulletCount = 0;
			int setCount = 0;
			sEnemyShotSet* pSet = enemyShotSetHead.next;
			while (pSet != &enemyShotSetHead) {
				++setCount;
				sEnemyShot* pShot = pSet->pEnemyShotHead->next;
				while (pShot != pSet->pEnemyShotHead) {
					++bulletCount;
					pShot = pShot->next;
				}
				pSet = pSet->next;
			}
			// FPS表示の少し上 (y=436) に表示。FPSは通常 (565,460) 付近。
			DrawFormatString(GAME_AREA_X + 360, 460, GetColor(255, 255, 255), "Bullets: %d", bulletCount);
        }
    }
}

void drawSidePanel()
{
    // 左パネル背景
    if (leftSidePanelBG == -1 || stageNum != lastStageForSidePanel) {
        createLeftSidePanelBG();
    }
    DrawGraph(0, 0, leftSidePanelBG, FALSE);

    // 右パネル背景
    if (rightSidePanelBG == -1 || stageNum != lastStageForSidePanel) {
        createRightSidePanelBG();
    }
    DrawGraph(GAME_W - 187, 0, rightSidePanelBG, FALSE);

    lastStageForSidePanel = stageNum;

    // ===== 左パネルの動的要素 =====
    const int panelLeftScreen = 10;               // 左パネル内のX(画面座標では +0)
    const int panelContentWidth = 187 - 20;       // 167px
    const int panelRightScreen = panelLeftScreen + panelContentWidth;
    const int lineHeight = 16;

    SidePanelLayout layout = computeSidePanelLayout();

    // プレイ回数
    DrawFormatString(panelLeftScreen, layout.playCountY + 1,
        GetColor(255, 255, 255), "Play Count: %u",
        stageData[stageNum].playCount);
    
    // BestTime
    if (stageData[stageNum].bestTime >= 59999) {
        DrawFormatString(panelLeftScreen, layout.bestTimeY + 1,
            GetColor(255, 255, 255), "BestTime: --.--");
    }
    else {
        DrawFormatString(panelLeftScreen, layout.bestTimeY + 1,
            GetColor(255, 255, 255), "BestTime: %5.2f",
            (double)stageData[stageNum].bestTime / 60);
    }

    // Time
    DrawFormatString(panelLeftScreen, layout.timeY + 1,
        GetColor(255, 255, 255), "    Time: %5.2f",
        (double)count / 60);

    // HPバー
    DrawBox(panelLeftScreen, layout.hpBarY,
        panelRightScreen, layout.hpBarY + 5,
        GetColor(10, 12, 18), TRUE);
    if (enemy.maxHp <= 0) enemy.maxHp = 1;
    int hpFill = panelContentWidth * enemy.hp / enemy.maxHp;
    if (hpFill > panelContentWidth) hpFill = panelContentWidth;
    if (hpFill < 0) hpFill = 0;
    for (int x = 0; x < hpFill; x++) {
        int ratio = x * 100 / panelContentWidth;
        int r = 255;
        int g = 255 - 155 * ratio / 100;
        int b = 255 - 255 * ratio / 100;
        DrawLine(panelLeftScreen + x, layout.hpBarY,
            panelLeftScreen + x, layout.hpBarY + 4,
            GetColor(r, g, b));
    }
    DrawBox(panelLeftScreen, layout.hpBarY,
        panelRightScreen, layout.hpBarY + 5,
        GetColor(0, 255, 255), FALSE);

    // BOSS HP
    DrawFormatString(panelLeftScreen, layout.bossLabelY + 1,
        GetColor(255, 255, 255), "HP: %d / %d",
        enemy.hp, enemy.maxHp);

    // HP残量警告
    if (enemy.maxHp > 0 && enemy.hp < enemy.maxHp / 5 && (count / 10) % 2 == 0) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 60);
        DrawBox(panelLeftScreen, layout.hpBarY,
            panelRightScreen, layout.hpBarY + 5,
            GetColor(255, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // リプレイモード表示
    if (replayActive) {
        DrawString(panelLeftScreen, layout.replayY + 1,
            "<Replay Mode>", GetColor(255, 255, 128));

        int y = layout.replayY + lineHeight * 2;

        if (stageData[stageNum].stageId == "Grok67") {
            DrawString(panelLeftScreen, y, "TAS でも無理！", GetColor(255, 255, 128));
            y += lineHeight;
            DrawString(panelLeftScreen, y, "無敵モード ON", GetColor(255, 255, 128));
            isMuteki = true;
        }
        if (stageData[stageNum].stageId == "Claude67") {
            isMuteki = false;
        }
        if (stageData[stageNum].stageId == "DeepSeek67"
            || stageData[stageNum].stageId == "ChatGPT67"
            || stageData[stageNum].stageId == "Gemini67"
            || stageData[stageNum].stageId == "Zai67"
            || stageData[stageNum].stageId == "DeepSeek85"
            || stageData[stageNum].stageId == "Qwen85"
            || stageData[stageNum].stageId == "Gemini85"
            || stageData[stageNum].stageId == "Grok98"
            || stageData[stageNum].stageId == "Claude98"
            || stageData[stageNum].stageId == "Kimi98")
            {
            DrawString(panelLeftScreen, y, "人力では無理！", GetColor(255, 255, 128));
            y += lineHeight;
            DrawString(panelLeftScreen, y, "TAS プレイです", GetColor(255, 255, 128));
        }        
    }

    // デバッグ用（無敵モード時）
    if (isMuteki) {
        int bulletCount = 0;
        //int setCount = 0;
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            //++setCount;
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                ++bulletCount;
                pShot = pShot->next;
            }
            pSet = pSet->next;
        }
        DrawFormatString(panelLeftScreen, 416, GetColor(255, 255, 0), "<Invincible Mode>");
        DrawFormatString(panelLeftScreen, 436, GetColor(255, 255, 255), "Bullets: %d", bulletCount);
    }
}

// 勝利・敗北時の半透明オーバーレイとメッセージ（変更なし）
void drawGameOverlay()
{
    int overlayColor;
    if (StateManager::GetState() == Joutai::Win) {
        overlayColor = GetColor(30, 20, 10);
    }
    else {
        overlayColor = GetColor(30, 10, 10);
    }

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
    DrawBox(GAME_AREA_X + 0, 0, GAME_AREA_X + 480, 480, overlayColor, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const char* message;
    int msgColor;
    if (StateManager::GetState() == Joutai::Win) {
        message = "You Win!!";
        msgColor = GetColor(255, 200, 50);
    }
    else {
        message = "You Lose...";
        msgColor = GetColor(255, 100, 100);
    }

    DrawBox(GAME_AREA_X + 140, 200, GAME_AREA_X + 340, 300, GetColor(20, 20, 40), TRUE);
    DrawBox(GAME_AREA_X + 140, 200, GAME_AREA_X + 340, 202, msgColor, TRUE);
    DrawString(GAME_AREA_X + 240 - (int)strlen(message) * 8, 205, message, GetColor(255, 255, 255));

    if (!replayActive) DrawString(GAME_AREA_X + 155, 230, "V   : Retry", GetColor(255, 255, 255));
    else               DrawString(GAME_AREA_X + 155, 230, "R   : Replay", GetColor(255, 255, 255));
    DrawString(GAME_AREA_X + 155, 250, "N   : Next Stage", GetColor(255, 255, 255));
    DrawString(GAME_AREA_X + 155, 270, "Q   : Quit", GetColor(255, 255, 255));
}