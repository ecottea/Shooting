// stateManager.cpp
#include "stateManager.h"
#include "DxLib.h"
#include "imgSoundLoad.h" // currentBGMHandle, bgm_menu, loadStageBGM()
#include "initial.h"      // ini(), startNewGame()
#include "replay.h"       // startReplay(), replayActive
#include "stageData.h"    // stageData

int stageNum = 0;

sCursor cursor = { 0, 0, 0 };

Joutai StateManager::currentState = Joutai::Menu;

Joutai StateManager::GetState()
{
	return currentState;
}

bool StateManager::ChangeState(Joutai newState)
{
	// 同じ状態への遷移は何もしない
	if (currentState == newState)
		return false;

	// 新しい状態に応じた初期化処理
	switch (newState)
	{
	case Joutai::Menu:
		replayActive = false;
		if (currentBGMHandle != -1)
			StopSoundMem(currentBGMHandle);
		currentBGMHandle = bgm_menu;
		PlaySoundMem(bgm_menu, DX_PLAYTYPE_LOOP);
		break;

	case Joutai::Game:
		// 遅延ロード: ゲーム開始前にBGMを必要に応じてロード
		loadStageBGM(stageNum);
		// BGM 制御：現在のステージ用に切り替え
		{
			int bgmHandle = stageData[stageNum].bgmHandle;
			if (currentBGMHandle != bgmHandle)
			{
				if (currentBGMHandle != -1)
					StopSoundMem(currentBGMHandle);
				currentBGMHandle = bgmHandle;
				if (bgmHandle != -1)
					PlaySoundMem(bgmHandle, DX_PLAYTYPE_LOOP);
			}
		}
		startNewGame(); // iniGame + 乱数シード（joutaiFlag 代入は削除済み）
		break;

	case Joutai::Replay:
		// BGM 制御：ステージ BGM に切り替え
		if (!startReplay(stageNum))
			return false;  // リプレイファイル不存在など
		// 遅延ロード
		loadStageBGM(stageNum);
		{
			int bgmHandle = stageData[stageNum].bgmHandle;
			if (currentBGMHandle != bgmHandle)
			{
				if (currentBGMHandle != -1)
					StopSoundMem(currentBGMHandle);
				currentBGMHandle = bgmHandle;
				if (bgmHandle != -1)
					PlaySoundMem(bgmHandle, DX_PLAYTYPE_LOOP);
			}
		}
		break;

	case Joutai::Win:
	case Joutai::Lose:
		// 特別な処理なし（BGM はそのまま）
		break;
	}

	currentState = newState;
	return true;
}