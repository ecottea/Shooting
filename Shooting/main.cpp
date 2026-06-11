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

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	if (ChangeWindowMode(TRUE) != DX_CHANGESCREEN_OK || DxLib_Init() == -1) return -1;
	SetDrawScreen(DX_SCREEN_BACK);

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

	while (!ProcessMessage() && !ClearDrawScreen() && getHitKeyStateAll_2(key) == 0) {
		int frameStart = GetNowCount();

		if (joutaiFlag == Joutai::Menu) {
			moveCursor();
			menuDraw();
			if (key[KEY_INPUT_Q] == 1) break;
		}
		else if (joutaiFlag == Joutai::Game) {
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

			if (key[KEY_INPUT_ESCAPE] == 1) joutaiFlag = Joutai::Pause;
		}
		else if (joutaiFlag == Joutai::Win || joutaiFlag == Joutai::Lose) {
			backGround();
			playerDisp();
			playerShotDisp();
			enemyDisp();
			enemyShotDisp();
			drawSidePanel();
			drawGameOverlay();
			count--;

			if (key[KEY_INPUT_Q] == 1) {
				joutaiFlag = Joutai::Menu;
				if (currentBGMHandle != -1) StopSoundMem(currentBGMHandle);
				currentBGMHandle = bgm_menu;
				PlaySoundMem(bgm_menu, DX_PLAYTYPE_LOOP);
			}
			else if (key[KEY_INPUT_V] == 1) {
				iniGame();
				joutaiFlag = Joutai::Game;
			}
		}
		else if (joutaiFlag == Joutai::Pause) {
			backGround();
			playerDisp();
			playerShotDisp();
			enemyDisp();
			enemyShotDisp();
			drawSidePanel();
			drawGameOverlay();
			count--;

			if (key[KEY_INPUT_ESCAPE] == 1) {
				joutaiFlag = Joutai::Game;
			}
			else if (key[KEY_INPUT_V] == 1) {
				iniGame();
				joutaiFlag = Joutai::Game;
			}
			else if (key[KEY_INPUT_Q] == 1) {
				iniGame();
				joutaiFlag = Joutai::Menu;
				if (currentBGMHandle != -1) StopSoundMem(currentBGMHandle);
				currentBGMHandle = bgm_menu;
				PlaySoundMem(bgm_menu, DX_PLAYTYPE_LOOP);
			}
		}

		fpsTimeFunction();
		count++;
		ScreenFlip();

		int elapsed = GetNowCount() - frameStart;
		if (elapsed < 17) WaitTimer(17 - elapsed);
	}

	iniGame();
	fileClose();

	DxLib_End();
	return 0;
}