#include "DxLib.h"
#include "gv.h"
#include "playerShot.h"
#include "stageData.h"
#include "imgSoundLoad.h"
#include "stateManager.h"

void playerShotControl()
{
	sPlayerShot* pPlayerShot;

	pPlayerShot = playerShotHead.next;
	while (pPlayerShot != &playerShotHead) {
		pPlayerShot->y -= 10.0;
		pPlayerShot = pPlayerShot->next;
	}
}

void playerShotCalc()
{
	sPlayerShot* pPlayerShot, * pNextPlayerShot;

	if (key[KEY_INPUT_V] % 6 == 1) {
		pPlayerShot = new sPlayerShot;

		pPlayerShot->x = player.x;
		pPlayerShot->y = player.y - 20;

		pPlayerShot->prev = playerShotHead.prev;
		pPlayerShot->next = &playerShotHead;
		playerShotHead.prev->next = pPlayerShot;
		playerShotHead.prev = pPlayerShot;
	}

	pPlayerShot = playerShotHead.next;
	while (pPlayerShot != &playerShotHead) {
		pPlayerShot->y -= 5.0;

		pNextPlayerShot = pPlayerShot->next;

		if (pPlayerShot->y < -18.0) {
			pPlayerShot->prev->next = pPlayerShot->next;
			pPlayerShot->next->prev = pPlayerShot->prev;
			delete pPlayerShot;
		}

		pPlayerShot = pNextPlayerShot;
	}
}

void playerShotDisp()
{
	sPlayerShot* pPlayerShot;

	pPlayerShot = playerShotHead.next;
	while (pPlayerShot != &playerShotHead) {
		DrawGraph((int)(pPlayerShot->x - (11-1)/2), (int)(pPlayerShot->y - (35-1)/2), imageData[img_playerShot].handle, TRUE);
		pPlayerShot = pPlayerShot->next;
	}
}

void playerShotHit()
{
	double x, y;
	sPlayerShot* pPlayerShot, * pNextPlayerShot;

	pPlayerShot = playerShotHead.next;
	while (pPlayerShot != &playerShotHead) {
		pNextPlayerShot = pPlayerShot->next;
		x = pPlayerShot->x - enemy.x;
		y = pPlayerShot->y - enemy.y;
		if (x * x + y * y < 60 * 60) {
			if (enemy.hp < 30) {
				PlaySoundMem(sound_playerShotHit_bossLowHP, DX_PLAYTYPE_BACK);
			}
			else {
				PlaySoundMem(sound_playerShotHit_default, DX_PLAYTYPE_BACK);
			}
			enemy.hp--;
			if (enemy.hp <= 0) {
				PlaySoundMem(sound_enemyDestroyed, DX_PLAYTYPE_BACK);
				StateManager::ChangeState(Joutai::Win);
			}
			pPlayerShot->prev->next = pPlayerShot->next;
			pPlayerShot->next->prev = pPlayerShot->prev;
			delete pPlayerShot;
		}

		pPlayerShot = pNextPlayerShot;
	}
}