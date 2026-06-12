// menu.cpp

#include "DxLib.h"
#include "gv.h"
#include "menu.h"
#include "stageData.h"
#include "replay.h"
#include "imgSoundLoad.h"
#include "initial.h"

#define SCREEN_W 640
#define SCREEN_H 480

// レイアウト設定
#define PANEL_X 30
#define PANEL_Y 70
#define PANEL_W 580
#define PANEL_H 240
#define CELL_W 56
#define CELL_H 24
#define GRID_COLS 10
#define GRID_ROWS 10
#define GRID_LEFT (PANEL_X + (PANEL_W - CELL_W * GRID_COLS) / 2)
#define GRID_TOP  (PANEL_Y + 30)
// 説明エリアのY座標（従来より上に移動）
#define DESC_Y 335

// デフォルトフォントサイズ
#define FONT_CHAR_W 8
#define FONT_CHAR_H 16
#define TEXT_W (FONT_CHAR_W * 2)
#define TEXT_H FONT_CHAR_H

static bool showReplayError = false;
static int replayErrorTimer = 0;

extern void iniGame();

void menuDraw()
{
    int i, x, y;

    // 背景
    DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(0, 0, 64), TRUE);
    DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(0, 128, 255), FALSE);

    // タイトル・操作説明
    DrawString(SCREEN_W / 2 - 52, 20, "STAGE SELECT", GetColor(255, 255, 0));
    DrawString(SCREEN_W / 2 - 212, 46, "選択:テンキー4568  決定:V  リプレイ再生:R  終了:Q", GetColor(200, 200, 200));

    // 選択パネル背景
    DrawBox(PANEL_X, PANEL_Y, PANEL_X + PANEL_W, PANEL_Y + PANEL_H,
        GetColor(0, 32, 64), TRUE);
    DrawBox(PANEL_X, PANEL_Y, PANEL_X + PANEL_W, PANEL_Y + PANEL_H,
        GetColor(64, 192, 255), FALSE);

    // カーソル描画
    {
        int cx = GRID_LEFT + cursor.x * CELL_W;
        int cy = GRID_TOP + cursor.y * CELL_H - 30;
        DrawBox(cx, cy, cx + CELL_W, cy + CELL_H, GetColor(0, 120, 180), TRUE);
        DrawBox(cx, cy, cx + CELL_W, cy + CELL_H, colorGreenBlue, FALSE);
    }

    // ステージ番号
    for (i = 0; i < (int)stageData.size(); i++) {
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        x = GRID_LEFT + col * CELL_W + (CELL_W - TEXT_W) / 2;
        y = GRID_TOP + row * CELL_H + (CELL_H - TEXT_H) / 2 - 30;
        DrawFormatString(x, y, colorWhite, "%2d", i);
    }
    for (i = (int)stageData.size(); i < GRID_COLS * GRID_ROWS; i++) {
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        x = GRID_LEFT + col * CELL_W + (CELL_W - TEXT_W) / 2;
        y = GRID_TOP + row * CELL_H + (CELL_H - TEXT_H) / 2 - 30;
        DrawFormatString(x, y, colorGray, "%2d", i);
    }

    // ---------- 説明文エリア ----------
    if (stageNum >= 0 && stageNum < (int)stageData.size()) {
        int descAreaTop = DESC_Y;            // 335
        int descAreaBottom = DESC_Y + 70;    // 405

        // 背景枠
        DrawBox(20, descAreaTop, SCREEN_W - 20, descAreaBottom,
            GetColor(0, 64, 128), TRUE);
        DrawBox(20, descAreaTop, SCREEN_W - 20, descAreaBottom,
            GetColor(128, 255, 255), FALSE);

        // ステージ見出し（強調：黄色＋★）
        DrawFormatString(30, descAreaTop + 7, GetColor(255, 255, 100),
            "★ %s", stageData[stageNum].stageId);

        // 区切り線
        DrawLine(30, descAreaTop + 25, SCREEN_W - 30, descAreaTop + 25,
            GetColor(100, 100, 150));

        // 詳細説明（控えめな水色）
        DrawFormatString(30, descAreaTop + 32, GetColor(180, 200, 220),
            "%s", stageData[stageNum].description);

        // 最短クリアタイム（右寄せで表示）
        if (stageData[stageNum].bestTime > 0) {
            DrawFormatString(SCREEN_W - 180, descAreaTop + 45,
                GetColor(255, 200, 100), "Best: %6.2f",
                (double)stageData[stageNum].bestTime / 60.0);
        }
        else {
            DrawFormatString(SCREEN_W - 180, descAreaTop + 45,
                GetColor(100, 100, 100), "Best: ---.--");
        }

        if (showReplayError) {
            if (replayErrorTimer > 0) {
                DrawString(30, DESC_Y + 80, "リプレイファイルが存在しません", GetColor(255, 100, 100));
                replayErrorTimer--;
                if (replayErrorTimer == 0) showReplayError = false;
            }
        }
    }
}

void moveCursor()
{
    stageNum = cursor.y * 10 + cursor.x;

    if (key[KEY_INPUT_V] == 1 && stageNum < (int)stageData.size()) {
        if (currentBGMHandle != -1) StopSoundMem(currentBGMHandle);
        currentBGMHandle = stageData[stageNum].bgmHandle;
        PlaySoundMem(currentBGMHandle, DX_PLAYTYPE_LOOP);
        joutaiFlag = Joutai::Game;
        startNewGame();
        return;
    }

    // moveCursor() 内の R キー処理
    if (key[KEY_INPUT_R] == 1 && stageNum < (int)stageData.size()) {
        key[KEY_INPUT_NUMPAD4] = 0;
        key[KEY_INPUT_NUMPAD6] = 0;
        key[KEY_INPUT_NUMPAD8] = 0;
        key[KEY_INPUT_NUMPAD5] = 0;
        key[KEY_INPUT_V] = 1;
        key[KEY_INPUT_C] = 0;
        if (!startReplay(stageNum)) {
            showReplayError = true;
            replayErrorTimer = 120;  // 2秒間表示 (60fps想定)
        }
        return;
    }

    // 他のキー入力があればエラー表示を即座に消す（オプション）
    if (showReplayError) {
        if (key[KEY_INPUT_NUMPAD4] == 1 || key[KEY_INPUT_NUMPAD6] == 1 ||
            key[KEY_INPUT_NUMPAD8] == 1 || key[KEY_INPUT_NUMPAD5] == 1 ||
            key[KEY_INPUT_V] == 1 || key[KEY_INPUT_Q] == 1) {
            showReplayError = false;
        }
    }

    // テンキー6: 右
    if (key[KEY_INPUT_NUMPAD6] == 1 || (key[KEY_INPUT_NUMPAD6] % 4 == 0 && key[KEY_INPUT_NUMPAD6] > 18)) {
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
        if (cursor.x == 9) cursor.x = 0;
        else               cursor.x++;
    }
    // テンキー4: 左
    if (key[KEY_INPUT_NUMPAD4] == 1 || (key[KEY_INPUT_NUMPAD4] % 4 == 0 && key[KEY_INPUT_NUMPAD4] > 18)) {
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
        if (cursor.x == 0) cursor.x = 9;
        else               cursor.x--;
    }
    // テンキー5: 下
    if (key[KEY_INPUT_NUMPAD5] == 1 || (key[KEY_INPUT_NUMPAD5] % 4 == 0 && key[KEY_INPUT_NUMPAD5] > 18)) {
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
        if (cursor.y == 9) cursor.y = 0;
        else               cursor.y++;
    }
    // テンキー8: 上
    if (key[KEY_INPUT_NUMPAD8] == 1 || (key[KEY_INPUT_NUMPAD8] % 4 == 0 && key[KEY_INPUT_NUMPAD8] > 18)) {
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
        if (cursor.y == 0) cursor.y = 9;
        else               cursor.y--;
    }    
}