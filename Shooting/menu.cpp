// menu.cpp

#include "DxLib.h"
#include "gv.h"
#include "stateManager.h"
#include "menu.h"
#include "stageData.h"
#include "replay.h"
#include "imgSoundLoad.h"
#include "initial.h"
#include <string> 

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
// 説明エリアの開始Y座標
#define DESC_Y 335

// デフォルトフォントサイズ
#define FONT_CHAR_W 8
#define FONT_CHAR_H 16
#define TEXT_W (FONT_CHAR_W * 3)
#define TEXT_H FONT_CHAR_H

// 説明文の最大横幅
#define DESC_AREA_WIDTH  (SCREEN_W - 60)   // 左右マージン30pxずつ

static bool showReplayError = false;
static int replayErrorTimer = 0;

extern void iniGame();

// 文字列を指定幅で改行した行のリストを返す (ピクセル単位)
std::vector<std::string> WrapText(const char* text, int maxWidth)
{
    std::vector<std::string> lines;
    if (!text || !*text) {
        lines.push_back("");
        return lines;
    }

    // 1文字ずつ読みながら幅を計算していく (マルチバイト対応のためDxLibのGetDrawStringWidthを使用)
    const char* p = text;
    std::string currentLine;

    while (*p) {
        // 改行文字が来たらその行で確定
        if (*p == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
            p++;
            continue;
        }

        // 次の1文字のバイト長を取得（全角は2バイト、半角は1バイト）
        int charBytes = 1;
        if (((unsigned char)*p >= 0x81 && (unsigned char)*p <= 0x9F) ||
            ((unsigned char)*p >= 0xE0 && (unsigned char)*p <= 0xFC)) {
            charBytes = 2;  // Shift_JIS全角の先頭バイト
        }

        // 現在の行に1文字追加した場合の幅を試算
        std::string testLine = currentLine + std::string(p, charBytes);
        int testWidth = GetDrawStringWidth(testLine.c_str(), -1);

        if (testWidth > maxWidth && !currentLine.empty()) {
            // 幅を超えるので、現在の行を確定し、次の行へ
            lines.push_back(currentLine);
            currentLine.clear();
            // この文字は次の行の先頭に置くため、ループ先頭へ戻る（pは進めない）
            continue;
        }
        else {
            // 現在の行に追加可能
            currentLine.append(p, charBytes);
            p += charBytes;
        }
    }

    // 最後の行を追加
    if (!currentLine.empty() || lines.empty())
        lines.push_back(currentLine);

    return lines;
}

void menuDraw()
{
    int i, x, y;

    // 背景
    DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(0, 0, 64), TRUE);
    DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(0, 128, 255), FALSE);

    // タイトル・操作説明
    DrawString(SCREEN_W / 2 - 52, 20, "STAGE SELECT", GetColor(255, 255, 0));
    DrawString(20, 46, "<7", GetColor(200, 200, 200));
    DrawString(SCREEN_W / 2 - 212, 46, "選択:テンキー4568  決定:V  リプレイ再生:R  終了:Q", GetColor(200, 200, 200));
    DrawString(SCREEN_W - 40, 46, "9>", GetColor(200, 200, 200));

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
    for (i = 0; i < GRID_COLS * GRID_ROWS; i++) {
        int idx = cursor.page * 100 + i;          // 実際のステージ番号
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        x = GRID_LEFT + col * CELL_W + (CELL_W - TEXT_W) / 2;
        y = GRID_TOP + row * CELL_H + (CELL_H - TEXT_H) / 2 - 30;

        // 有効ステージは白、範囲外は灰色
        unsigned int color = (idx < (int)stageData.size()) ? colorWhite : colorGray;
        DrawFormatString(x, y, color, "%3d", idx);   // 3桁右詰め表示
    }

    // ---------- 説明文エリア（自動改行＆高さ可変） ----------
    if (stageNum >= 0 && stageNum < (int)stageData.size()) {
        const int descAreaLeft = 30;
        const int descAreaWidth = DESC_AREA_WIDTH - 10;   // 570
        const int descAreaRight = descAreaLeft + descAreaWidth; // 610
        const int lineHeight = FONT_CHAR_H;       // 16

        // 説明文を指定幅で折り返し
        std::vector<std::string> descLines = WrapText(stageData[stageNum].description, descAreaWidth);

        // 説明エリアの縦幅を計算
        // タイトル行(1行) + 区切り線 + 説明行数分
        int titleHeight = lineHeight + 7;     // 適宜余白
        int sepHeight = 10;                 // 線の下マージン
        int descHeight = (int)descLines.size() * lineHeight;
        int bestTimeH = lineHeight + 5;     // ベストタイム行
        int areaInnerH = titleHeight + sepHeight + descHeight + bestTimeH;
        int descAreaTop = DESC_Y;
        int descAreaBottom = descAreaTop + areaInnerH;

        // 背景枠（高さを動的に）
        DrawBox(descAreaLeft, descAreaTop, descAreaRight, descAreaBottom,
            GetColor(0, 64, 128), TRUE);
        DrawBox(descAreaLeft, descAreaTop, descAreaRight, descAreaBottom,
            GetColor(128, 255, 255), FALSE);

        // ステージ見出し
        DrawFormatString(descAreaLeft + 5, descAreaTop + 5, GetColor(255, 255, 100),
            "★ %s", stageData[stageNum].stageId);

        // 区切り線
        int sepY = descAreaTop + titleHeight;
        DrawLine(descAreaLeft + 5, sepY, descAreaRight - 5, sepY, GetColor(100, 100, 150));

        // 詳細説明（複数行）
        int textY = sepY + 8;
        for (const auto& line : descLines) {
            DrawFormatString(descAreaLeft + 5, textY, GetColor(180, 200, 220), "%s", line.c_str());
            textY += lineHeight;
        }

        // 最短クリアタイム（説明の最終行の下に表示）
        int bestY = descAreaBottom - bestTimeH;
        if (stageData[stageNum].bestTime > 0) {
            DrawFormatString(descAreaRight - 180, bestY,
                GetColor(255, 200, 100), "Best: %6.2f",
                (double)stageData[stageNum].bestTime / 60.0);
        }
        else {
            DrawFormatString(descAreaRight - 180, bestY,
                GetColor(100, 100, 100), "Best: ---.--");
        }

        // リプレイエラーメッセージ（説明エリアのすぐ下に表示）
        if (showReplayError) {
            if (replayErrorTimer > 0) {
                DrawString(descAreaLeft + 5, descAreaBottom + 5,
                    "リプレイファイルが存在しません", GetColor(255, 100, 100));
                replayErrorTimer--;
                if (replayErrorTimer == 0) showReplayError = false;
            }
        }
    }
}

void moveCursor()
{
    // 変更なしのため省略（同じ内容）
    // V/R判定用のステージ番号を先に計算（移動前のカーソル位置）
    stageNum = cursor.page * 100 + cursor.y * 10 + cursor.x;

    if (key[KEY_INPUT_V] == 1 && stageNum < (int)stageData.size()) {
        StateManager::ChangeState(Joutai::Game);  // BGM制御・初期化は内部で実施
        return;
    }

    // moveCursor() 内の R キー処理
    if (key[KEY_INPUT_R] == 1 && stageNum < (int)stageData.size()) {
        if (!StateManager::ChangeState(Joutai::Replay)) {
            showReplayError = true;
            replayErrorTimer = 120;
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

    // ===== ページ切り替え（テンキー7/9） =====
    if (key[KEY_INPUT_NUMPAD7] == 1) {
        cursor.page = (cursor.page == 0) ? (int)stageData.size() / 100 : cursor.page - 1;
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
    }
    if (key[KEY_INPUT_NUMPAD9] == 1) {
        cursor.page = (cursor.page == (int)stageData.size() / 100) ? 0 : cursor.page + 1;
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
    }

    // 最後に、移動後のカーソル位置でステージ番号を更新（表示用）
    stageNum = cursor.page * 100 + cursor.y * 10 + cursor.x;
}