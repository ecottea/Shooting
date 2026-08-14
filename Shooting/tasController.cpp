#include "tasController.h"
#include "DxLib.h"
#include "gv.h"
#include "stageData.h"
#include "initial.h"
#include "replay.h"
#include "stateManager.h"
#include "gameScreen.h"      // updateStars()
#include "enemy.h"
#include "enemyShot.h"
#include "player.h"
#include "playerShot.h"
#include "imgSoundLoad.h"


// TASモードの有効/無効を切り替える
bool g_isTasMode = 0;


void iniGameForTas()
{
	sPlayerShot* pPlayerShot, * pNextPlayerShot;
	sEnemyShotSet* pEnemyShotSet, * pNextEnemyShotSet;
	sEnemyShot* pEnemyShot, * pNextEnemyShot;

    //gameSeed = GetNowCount();
    SRand((int)gameSeed);

	count = 0;
	//replayKeyHistory.clear();
	replayActive = false;
	resetStars();

	player.x = 240.0;
	player.y = 400.0;

	pPlayerShot = playerShotHead.next;
	playerShotHead.next = &playerShotHead;
	playerShotHead.prev = &playerShotHead;

	while (pPlayerShot != &playerShotHead) {
		pNextPlayerShot = pPlayerShot->next;

		delete pPlayerShot;

		pPlayerShot = pNextPlayerShot;
	}

	pEnemyShotSet = enemyShotSetHead.next;
	enemyShotSetHead.next = &enemyShotSetHead;
	enemyShotSetHead.prev = &enemyShotSetHead;

	while (pEnemyShotSet != &enemyShotSetHead) {
		pNextEnemyShotSet = pEnemyShotSet->next;

		pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
		pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

		while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
			pNextEnemyShot = pEnemyShot->next;

			delete pEnemyShot;

			pEnemyShot = pNextEnemyShot;
		}
		delete pEnemyShotSet->pEnemyShotHead;
		delete pEnemyShotSet;

		pEnemyShotSet = pNextEnemyShotSet;
	}

	clearAllSparks();
	clearAllEnemyEngineFlames();
	clearAllForceParticles();
	clearAllPlayerEngineFlames();

    StateManager::currentState = Joutai::Game;
}

namespace {
    static int s_maxCount = 0;   // これまでに到達した最大count

    // 指定フレームまでゲーム状態を再計算（描画なし）
    void fastForwardToFrame(int targetFrame)
    {
        key[KEY_INPUT_NUMPAD4] = 0;
        key[KEY_INPUT_NUMPAD6] = 0;
        key[KEY_INPUT_NUMPAD8] = 0;
        key[KEY_INPUT_NUMPAD5] = 0;
        key[KEY_INPUT_V] = 1; // ゲーム開始時に V を押すのをクリアするのを忘れていたので辻褄合わせ
        key[KEY_INPUT_C] = 0;

        // 0 から targetFrame-1 までキー履歴を再適用
        for (int f = 0; f < targetFrame; ++f) {
            count++;
            uint8_t input = (f < (int)replayKeyHistory.size())
                ? replayKeyHistory[f]
                : 0;
            unpackReplayKey(input, key);

            // ゲームロジック実行（描画・サウンドなし）
            stageData[stageNum].patternFunc();
            enemyControl();
            enemyShotControl();
            enemyShotCalc();
            playerControl();
            playerShotControl();
            playerShotCalc();
            playerShotHit();
            //enemyShotHit();
            //enemyHit();
        }

        // うるさいので音を止める
        StopSoundMem(sound_enemyShot_noize);
        StopSoundMem(sound_enemyShot_light);
        StopSoundMem(sound_enemyShot_medium);
        StopSoundMem(sound_enemyShot_heavy);
        StopSoundMem(sound_enemyShot_extreme);
        StopSoundMem(sound_enemyCharge);
        StopSoundMem(sound_playerDestroyed);
        StopSoundMem(sound_playerShotHit_default);
        StopSoundMem(sound_playerShotHit_bossLowHP);
    }

} // namespace

void TAS_ResetMaxCount()
{
    s_maxCount = 0;
}

void TAS_UpdateMaxCount(int currentCount)
{
    if (currentCount > s_maxCount)
        s_maxCount = currentCount;
}

bool TAS_OnLoseState()
{
    if (!g_isTasMode) return false;

    // 最大値から10フレーム前（下限0）
    int target = s_maxCount - 10;
    if (target < 0) target = 0;

    // キー履歴を切り詰める
    if (target < (int)replayKeyHistory.size())
        replayKeyHistory.resize(target);

    // 状態を Game に戻す
    iniGameForTas();

    // ゲーム状態を目標フレームまで復元
    fastForwardToFrame(target);

    // count は fastForwardToFrame 内で設定済み
    return true;
}