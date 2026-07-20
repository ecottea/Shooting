// stateManager.cpp
#include "stateManager.h"
#include "DxLib.h"
#include "imgSoundLoad.h" // currentBGMHandle, bgm_menu, loadStageBGM()
#include "initial.h"      // ini(), startNewGame()
#include "replay.h"       // startReplay(), replayActive
#include "stageData.h"    // stageData
#include "gv.h"

// BGMオンオフ制御定数 (true:再生する, false:再生しない)
static constexpr bool BGM_ENABLED = true;

int stageNum = 0;
sCursor cursor = { 0, 0, 0 };
Joutai StateManager::currentState = Joutai::None;

// 録画モード用グローバル変数
bool recordingMode = false;   // (true:録画モード, false:通常モード)
int  replayLoopCount = 8;    // 連続リプレイ録画個数


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
		if constexpr (BGM_ENABLED) {
			if (currentBGMHandle != -1) {
				StopSoundMem(currentBGMHandle);
			}
			currentBGMHandle = bgm_menu;
			PlaySoundMem(bgm_menu, DX_PLAYTYPE_LOOP);
		}
		break;

	case Joutai::Game:
		// 遅延ロード: ゲーム開始前にBGMを必要に応じてロード
		loadStageBGM(stageNum);
		// BGM 制御：現在のステージ用に切り替え
		if constexpr (BGM_ENABLED) {
			int bgmHandle = stageData[stageNum].bgmHandle;
			if (currentBGMHandle != bgmHandle) {
				if (currentBGMHandle != -1) {
					StopSoundMem(currentBGMHandle);
				}
				currentBGMHandle = bgmHandle;
				if (bgmHandle != -1) {
					PlaySoundMem(bgmHandle, DX_PLAYTYPE_LOOP);
				}
			}
		}
		startNewGame(); // iniGame + 乱数シード（joutaiFlag 代入は削除済み）
		break;

	case Joutai::Replay:
		// BGM 制御：ステージ BGM に切り替え
		if (!startReplay(stageNum))
			return false;  // リプレイファイル不存在など
		key[KEY_INPUT_NUMPAD4] = 0;
		key[KEY_INPUT_NUMPAD6] = 0;
		key[KEY_INPUT_NUMPAD8] = 0;
		key[KEY_INPUT_NUMPAD5] = 0;
		key[KEY_INPUT_V] = 1;
		key[KEY_INPUT_C] = 0;
		// 遅延ロード
		loadStageBGM(stageNum);
		if constexpr (BGM_ENABLED) {
			int bgmHandle = stageData[stageNum].bgmHandle;
			if (currentBGMHandle != bgmHandle) {
				if (currentBGMHandle != -1) {
					StopSoundMem(currentBGMHandle);
				}
				currentBGMHandle = bgmHandle;
				if (bgmHandle != -1) {
					PlaySoundMem(bgmHandle, DX_PLAYTYPE_LOOP);
				}
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