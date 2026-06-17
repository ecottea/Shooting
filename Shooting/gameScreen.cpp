// backGround.cpp

#include "DxLib.h"
#include "stateManager.h"
#include "gameScreen.h"
#include "gv.h"
#include "stageData.h"
#include "imgSoundLoad.h"
#include <math.h>
#include "menu.h"
#include "replay.h"
#include <string> 
#include <vector> 

#define NUM_STARS 30

typedef struct {
    double x, y, speedY;
    int brightness, twinklePhase, color, size;
} Star;

static Star stars[NUM_STARS];
static int starsInitialized = 0;
static int fieldBG = -1;
static int sidePanelBG = -1;

// 戦闘フィールド背景を一枚の画像として生成
static void createFieldBG() {
    fieldBG = MakeScreen(480, 480, FALSE);   // 透過不要
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

// サイドパネル背景を一枚の画像として生成（動的要素を除く）
static void createSidePanelBG() {
    sidePanelBG = MakeScreen(160, 480, FALSE);
    int oldScreen = GetDrawScreen();
    SetDrawScreen(sidePanelBG);

    // パネル背景グラデーション（x=480～640 → 画像内では0～160）
    for (int i = 0; i < 160; i++) {
        int shade = (int)(60.0 * (double)(160 - i) / 160.0);
        int col = GetColor(shade, shade, shade);
        DrawLine(i, 0, i, 480, col);
    }
    // 左端の境界線
    DrawBox(0, 0, 1, 480, GetColor(100, 100, 100), TRUE);

    // タイトル
    DrawBox(10, 0, 140, 30, GetColor(0, 50, 100), TRUE);
    DrawFormatString(12, 3, colorWhite, "■ STAGE %d", stageNum);

    // ステージ情報ボックス
    DrawBox(5, 35, 155, 130, GetColor(30, 30, 50), TRUE);
    DrawBox(5, 35, 155, 36, GetColor(0, 80, 160), TRUE);
    if (stageNum >= 0 && stageNum < (int)stageData.size()) {
        DrawFormatString(10, 40, GetColor(200, 200, 255), "%s", stageData[stageNum].stageId);
        DrawFormatString(10, 65, colorWhite, "Best: %6.2f", (double)stageData[stageNum].bestTime / 60);
    }

    // BOSS ラベルと HP バー枠
    DrawLine(10, 115, 150, 115, GetColor(100, 100, 150));
    DrawFormatString(10, 118, colorWhite, "BOSS");
    DrawBox(10, 140, 150, 145, colorGreenBlue, FALSE);   // HPバー枠のみ（塗りは動的）

    SetDrawScreen(oldScreen);
}

void resetStars() {
    starsInitialized = 0;
}

void initStars() {
    if (starsInitialized) return;
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = GetRand(478);
        stars[i].y = GetRand(478);
        stars[i].speedY = 0.3 + (double)GetRand(120) / 100.0;
        stars[i].brightness = GetRand(255);
        stars[i].twinklePhase = GetRand(360);
        stars[i].size = GetRand(3) + 1;
        int r = 200 + GetRand(55);
        int g = 200 + GetRand(55);
        int b = 200 + GetRand(55);
        stars[i].color = GetColor(r, g, b);
    }
    starsInitialized = 1;
}

void backGround()
{
    initStars();

    // 背景画像を初回のみ生成
    if (fieldBG == -1) createFieldBG();

    // 戦闘フィールド背景（一枚画像）を描画
    DrawGraph(0, 0, fieldBG, FALSE);

    // 星の更新と描画（動的）
    for (int i = 0; i < NUM_STARS; i++) {
        if (StateManager::GetState() == Joutai::Game || StateManager::GetState() == Joutai::Replay) {
            stars[i].y += stars[i].speedY;
            if (stars[i].y > 480.0) {
                stars[i].y -= 480.0;
                stars[i].x = GetRand(478);
            }
        }

        double twinkle = 0.5 + 0.5 * sin((double)(count * 2 + stars[i].twinklePhase) / 60.0);
        int alpha = (int)(stars[i].brightness * (0.5 + 0.5 * twinkle));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        int cx = (int)stars[i].x;
        int cy = (int)stars[i].y;
        int half = stars[i].size;
        DrawLine(cx - half, cy, cx + half, cy, stars[i].color);
        DrawLine(cx, cy - half, cx, cy + half, stars[i].color);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void drawSidePanel()
{
    if (sidePanelBG == -1) createSidePanelBG();
    DrawGraph(480, 0, sidePanelBG, FALSE);

    const int panelLeft = 490;
    const int panelRight = 630;
    const int descMaxWidth = 120;
    const int lineHeight = 16;
    const int halfLine = lineHeight / 2;   // 8px

    // タイトル
    DrawBox(panelLeft - 1, 0, panelRight, 30, GetColor(0, 50, 100), TRUE);
    DrawFormatString(panelLeft + 2, 3, colorWhite, "■ STAGE %d", stageNum);

    // 動的情報の背景をクリア
    DrawBox(485, 36, 635, 480, GetColor(30, 30, 50), TRUE);

    // stageId
    int currentY = 40;
    DrawFormatString(panelLeft, currentY, GetColor(200, 200, 255), "%s", stageData[stageNum].stageId);
    currentY += lineHeight + halfLine;

    // 説明文
    std::vector<std::string> descLines = WrapText(stageData[stageNum].description, descMaxWidth);
    int linesToShow = (int)descLines.size();
    for (int i = 0; i < linesToShow; i++) {
        DrawFormatString(panelLeft, currentY, GetColor(180, 200, 220), "%s", descLines[i].c_str());
        currentY += lineHeight;
    }

    // BestTime (1行あける)
    currentY += lineHeight;
    DrawFormatString(panelLeft, currentY, colorWhite, "Best: %6.2f", (double)stageData[stageNum].bestTime / 60);
    currentY += lineHeight;

    // Time
    DrawFormatString(panelLeft, currentY, colorWhite, "Time: %6.2f", (double)count / 60);
    currentY += lineHeight;

    // ---- BOSS 区切り線（上下に0.5行のスペース） ----
    currentY += halfLine;                                       // 線の上のスペース
    DrawLine(panelLeft, currentY, panelLeft + 140, currentY, GetColor(100, 100, 150));
    currentY += 1 + halfLine;                                   // 線の下のスペース

    // BOSS ラベル
    DrawFormatString(panelLeft, currentY, colorWhite, "BOSS");
    currentY += 18;

    // HP バー
    DrawBox(panelLeft, currentY, panelLeft + 140, currentY + 5, colorGreenBlue, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
    DrawBox(panelLeft, currentY, panelLeft + 140 * enemy.hp / enemy.maxHp, currentY + 5, colorGreenBlue, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    currentY += 10;
    DrawFormatString(panelLeft, currentY, colorWhite, "HP: %d / %d", enemy.hp, enemy.maxHp);
    currentY += lineHeight * 2;    // HP表示の下は1行分のスペース

    // Q: Stage Select (常に表示、上に1行分のスペースを確保済み)
    DrawString(panelLeft + 5, currentY, "Q: Stage Select", colorWhite);
    currentY += lineHeight * 2;

    // リプレイ中はさらに2行下に表示
    if (replayActive) {
        DrawString(panelLeft + 5, currentY, "<Replay Mode>", GetColor(255, 255, 128));
    }
}

// ポーズ・勝利・敗北時の半透明オーバーレイとメッセージ
void drawGameOverlay()
{
    int overlayColor;
    if (StateManager::GetState() == Joutai::Win) {
        overlayColor = GetColor(30, 20, 10);
    }
    else { // Joutai::Lose
        overlayColor = GetColor(30, 10, 10);
    }

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
    DrawBox(0, 0, 480, 480, overlayColor, TRUE);
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

    DrawBox(140, 200, 340, 300, GetColor(20, 20, 40), TRUE);
    DrawBox(140, 200, 340, 202, msgColor, TRUE);
    DrawString(240 - (int)strlen(message) * 8, 205, message, colorWhite);

    if (!replayActive) DrawString(155, 230, "V   : Retry",  colorWhite);
    else               DrawString(155, 230, "R   : Replay", colorWhite);
    DrawString(155, 250, "N   : Next Stage", colorWhite);
    DrawString(155, 270, "Q   : Stage Select", colorWhite);
}