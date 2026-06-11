// backGround.cpp

#include "DxLib.h"
#include "gv.h"
#include "backGround.h"
#include "stageData.h"
#include <math.h>

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
        if (joutaiFlag == Joutai::Game) {
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
    // サイドパネル画像を初回のみ生成
    if (sidePanelBG == -1) createSidePanelBG();

    // 静的背景（事前描画）を描画（描画先は (480,0)）
    DrawGraph(480, 0, sidePanelBG, FALSE);

    // ---------- 動的な要素を上書き描画 ----------

    // 現在のステージタイトル（毎フレーム stageNum を反映）
    // タイトル背景で古い文字を隠す（createSidePanelBG で描かれた部分を塗りつぶし）
    DrawBox(490, 0, 620, 30, GetColor(0, 50, 100), TRUE);
    DrawFormatString(492, 3, colorWhite, "■ STAGE %d", stageNum);

    // ステージ情報領域のタイトル・詳細を毎フレーム更新
    // 古い情報を背景色で塗りつぶしてから描画
    DrawBox(485, 36, 635, 115, GetColor(30, 30, 50), TRUE); // タイトル下の余白を含めて消去
    if (stageNum >= 0 && stageNum < (int)stageData.size()) {
        DrawFormatString(490, 40, GetColor(200, 200, 255), "%s", stageData[stageNum].stageId);
        DrawFormatString(490, 65, colorWhite, "Best: %6.2f", (double)stageData[stageNum].bestTime / 60);
    }

    // 経過時間
    DrawFormatString(490, 85, colorWhite, "Time: %6.2f", (double)count / 60);

    // ボス HP バーの塗り（半透明）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
    DrawBox(490, 140, 490 + 140 * enemy.hp / enemy.maxHp, 145, colorGreenBlue, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawFormatString(490, 148, colorWhite, "HP: %d / %d", enemy.hp, enemy.maxHp);

    // ゲーム中の操作ヒント
    if (joutaiFlag == Joutai::Game) {
        DrawString(495, 340, "ESC: Pause", colorWhite);
    }
}

// 新規追加：ポーズ・勝利・敗北時の半透明オーバーレイとメッセージ
void drawGameOverlay()
{
    int overlayColor;
    if (joutaiFlag == Joutai::Pause) {
        overlayColor = GetColor(10, 10, 30);
    }
    else if (joutaiFlag == Joutai::Win) {
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
    if (joutaiFlag == Joutai::Pause) {
        message = "Pause";
        msgColor = GetColor(255, 255, 100);
    }
    else if (joutaiFlag == Joutai::Win) {
        message = "You Win!!";
        msgColor = GetColor(255, 200, 50);
    }
    else {
        message = "You Lose...";
        msgColor = GetColor(255, 100, 100);
    }

    DrawBox(140, 200, 340, 280, GetColor(20, 20, 40), TRUE);
    DrawBox(140, 200, 340, 202, msgColor, TRUE);
    DrawString(240 - (int)strlen(message) * 8, 205, message, colorWhite);

    // ポーズ時のみ ESC : Resume を表示
    if (joutaiFlag == Joutai::Pause) {
        DrawString(155, 230, "ESC : Resume", colorWhite);
        DrawString(155, 250, "V   : Retry", colorWhite);
        DrawString(155, 270, "Q   : Stage Select", colorWhite);
    }
    else { // Joutai::Win または Joutai::Lose
        DrawString(155, 230, "V   : Retry", colorWhite);
        DrawString(155, 250, "Q   : Stage Select", colorWhite);
    }
}