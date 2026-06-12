// main.cpp

#include "DxLib.h"
#include "gv.h"
#include "initial.h"
#include "fps.h"
#include "backGround.h"
#include "stageData.h"
#include "menu.h"
#include "imgSoundLoad.h"
#include "player.h"
#include "playerShot.h"
#include "enemy.h"
#include "enemyShot.h"
#include "fileOpenClose.h"
#include "getHitKeyStateAll2.h"
#include "replay.h"

constexpr int GAME_W = 640;
constexpr int GAME_H = 480;

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// ウィンドウモードの初期設定
	SetGraphMode(GAME_W, GAME_H, 32);
	ChangeWindowMode(TRUE);
	SetWindowSize(GAME_W, GAME_H);              // 初期サイズ
	loadWindowSettings();                       // 前回の設定で上書き
	SetWindowSizeChangeEnableFlag(TRUE, TRUE);  // 第2引数TRUEでバックバッファもリサイズに追従

	if (DxLib_Init() == -1) return -1;

	// 仮想画面を作成（この画面に全ゲーム描画を行う）
	int gameScreen = MakeScreen(GAME_W, GAME_H, TRUE);
	SetDrawScreen(gameScreen);

	fileOpen();
	imgSoundLoad();
	setColor();
	currentBGMHandle = bgm_menu;
	PlaySoundMem(bgm_menu, DX_PLAYTYPE_LOOP);
	ini();
	loadCursorPos();

	playerShotHead.prev = &playerShotHead;
	playerShotHead.next = &playerShotHead;
	enemyShotSetHead.prev = &enemyShotSetHead;
	enemyShotSetHead.next = &enemyShotSetHead;

	while (!ProcessMessage() && !ClearDrawScreen()) {
		int frameStart = GetNowCount();

		// キー入力取得（リプレイ再生中はリプレイデータを使う）
		if (joutaiFlag == Joutai::Replay) {
			if (!updateReplayInput()) {
				// リプレイデータ終了 → メニューへ
				replayActive = false;
				joutaiFlag = Joutai::Menu;
				if (currentBGMHandle != -1) StopSoundMem(currentBGMHandle);
				currentBGMHandle = bgm_menu;
				PlaySoundMem(bgm_menu, DX_PLAYTYPE_LOOP);
				continue;
			}
		}
		else {
			getHitKeyStateAll_2(key);
			// 通常ゲーム中はキー履歴を記録
			if (joutaiFlag == Joutai::Game) {
				replayKeyHistory.push_back(packReplayKey(key));
			}
		}

		if (joutaiFlag == Joutai::Menu) {
			moveCursor();
			menuDraw();
			if (key[KEY_INPUT_Q] == 1) {
				saveWindowSettings();
				break;
			}
		}
		else if (joutaiFlag == Joutai::Game || joutaiFlag == Joutai::Replay) {
			stageData[stageNum].patternFunc();
			enemyShotControl();
			enemyShotCalc();
			playerControl();
			playerShotControl();
			playerShotCalc();
			playerShotHit();
			enemyShotHit();
			enemyHit();

			backGround();
			playerDisp();
			playerShotDisp();
			enemyDisp();
			enemyShotDisp();
			drawSidePanel();

			// Qキーで即メニューに戻る
			if (key[KEY_INPUT_Q] == 1) {
				replayActive = false;
				joutaiFlag = Joutai::Menu;
				if (currentBGMHandle != -1) StopSoundMem(currentBGMHandle);
				currentBGMHandle = bgm_menu;
				PlaySoundMem(bgm_menu, DX_PLAYTYPE_LOOP);
			}
		}
		else if (joutaiFlag == Joutai::Win || joutaiFlag == Joutai::Lose) {
			// ベストタイム更新とリプレイ保存（Win時のみ）
			if (joutaiFlag == Joutai::Win && !replayActive) {
				unsigned int clearTime = count;
				if (clearTime < stageData[stageNum].bestTime) {
					stageData[stageNum].bestTime = clearTime;
					saveReplay(stageNum);
					fileClose();  // bestTime.json 更新
				}
			}

			backGround();
			playerDisp();
			playerShotDisp();
			enemyDisp();
			enemyShotDisp();
			drawSidePanel();
			drawGameOverlay();
			count--;

			if (key[KEY_INPUT_Q] == 1) {
				replayActive = false;
				joutaiFlag = Joutai::Menu;
				if (currentBGMHandle != -1) StopSoundMem(currentBGMHandle);
				currentBGMHandle = bgm_menu;
				PlaySoundMem(bgm_menu, DX_PLAYTYPE_LOOP);
			}
			else if (key[KEY_INPUT_V] == 1) {
				if (replayActive) {
					// リプレイを先頭から再生
					startReplay(stageNum);
				}
				else {
					startNewGame();
				}
			}
		}

		fpsTimeFunction();
		count++;

		// リサイズ対応の合成処理
		SetDrawScreen(DX_SCREEN_BACK);
		ClearDrawScreen();

		// GetWindowSize ではなく GetDrawScreenSize でバックバッファの実サイズを取得
		int winW, winH;
		GetDrawScreenSize(&winW, &winH);

		double scaleX = (double)winW / GAME_W;
		double scaleY = (double)winH / GAME_H;
		double scale = (scaleX < scaleY) ? scaleX : scaleY;
		int drawW = (int)(GAME_W * scale);
		int drawH = (int)(GAME_H * scale);
		int offsetX = (winW - drawW) / 2;
		int offsetY = (winH - drawH) / 2;

		DrawExtendGraph(offsetX, offsetY, offsetX + drawW, offsetY + drawH, gameScreen, TRUE);
		ScreenFlip();

		SetDrawScreen(gameScreen);

		// FPS 調整
		int elapsed = GetNowCount() - frameStart;
		if (elapsed < 17) WaitTimer(17 - elapsed);
	}

	iniGame();
	fileClose();

	DxLib_End();
	return 0;
}