#include "DxLib.h"
#include "gv.h"
#include "enemyShot.h"
#include "imgSoundLoad.h"
#include "stateManager.h"
#include "player.h"

void enemyShotControl(){
	sEnemyShotSet *pEnemyShotSet;
	
	pEnemyShotSet = enemyShotSetHead.next;
	while(pEnemyShotSet != &enemyShotSetHead){
		sEnemyShotSet* pNext = pEnemyShotSet->next;

		pEnemyShotSet->patternFunc(pEnemyShotSet);

		pEnemyShotSet = pNext;
	}
}

void enemyShotCalc()
{
	sEnemyShotSet *pEnemyShotSet, *pNextEnemyShotSet;
	sEnemyShot *pEnemyShot, *pNextEnemyShot;

	pEnemyShotSet = enemyShotSetHead.next;
	while(pEnemyShotSet != &enemyShotSetHead){
		pEnemyShotSet->count++;
		
		pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
		while(pEnemyShot != pEnemyShotSet->pEnemyShotHead){
			pEnemyShot->count++;
			
			pNextEnemyShot = pEnemyShot->next;

			if (pEnemyShot->x < -pEnemyShot->margin || pEnemyShot->x > 480.0 + pEnemyShot->margin //画面外の弾は削除
				|| pEnemyShot->y < -pEnemyShot->margin || pEnemyShot->y > 480.0 + pEnemyShot->margin) {
				pEnemyShot->prev->next = pEnemyShot->next;
				pEnemyShot->next->prev = pEnemyShot->prev;
				delete pEnemyShot;
			}
			
			pEnemyShot = pNextEnemyShot;
		}
		
		pNextEnemyShotSet = pEnemyShotSet->next;
		
		if(pEnemyShotSet->pEnemyShotHead->next==pEnemyShotSet->pEnemyShotHead && pEnemyShotSet->count>60){ //空のセットは削除
			delete pEnemyShotSet->pEnemyShotHead;
			
			pEnemyShotSet->prev->next = pEnemyShotSet->next;
			pEnemyShotSet->next->prev = pEnemyShotSet->prev;
			delete pEnemyShotSet;
		}
		
		pEnemyShotSet = pNextEnemyShotSet;
	}
}

void enemyShotDisp()
{
	sEnemyShotSet* pEnemyShotSet = enemyShotSetHead.next;
	while (pEnemyShotSet != &enemyShotSetHead) {
		sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
		while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
			if (!imageData[pEnemyShot->kind].rotatable) {
				DrawRotaGraph((int)pEnemyShot->x, (int)pEnemyShot->y,
					imageData[pEnemyShot->kind].mag, 0.0,
					imageData[pEnemyShot->kind].handle, TRUE, FALSE);
			}
			else {
				DrawRotaGraph((int)pEnemyShot->x, (int)pEnemyShot->y,
					imageData[pEnemyShot->kind].mag, pEnemyShot->muki,
					imageData[pEnemyShot->kind].handle, TRUE, FALSE);
			}

			pEnemyShot = pEnemyShot->next;
		}

		pEnemyShotSet = pEnemyShotSet->next;
	}
}

void enemyShotHit()
{
	sEnemyShotSet* pEnemyShotSet = enemyShotSetHead.next;
	while (pEnemyShotSet != &enemyShotSetHead) {
		sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
		while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
			// プレイヤーとの相対座標
			double dx = player.x - pEnemyShot->x;
			double dy = player.y - pEnemyShot->y;

			// 弾の半径（画像に登録されたもの＋マージン）
			double rx = imageData[pEnemyShot->kind].radiusX + imageData[img_player].radiusX;
			double ry = imageData[pEnemyShot->kind].radiusY + imageData[img_player].radiusY;

			// 弾が回転する場合は逆回転してローカル座標系に変換
			if (imageData[pEnemyShot->kind].rotatable) {
				double c = cos(-pEnemyShot->muki);
				double s = sin(-pEnemyShot->muki);
				double lx = dx * c - dy * s;
				double ly = dx * s + dy * c;
				dx = lx;
				dy = ly;
			}

			// 楕円の内側かで簡易判定
			if ((dx * dx) / (rx * rx) + (dy * dy) / (ry * ry) < 1.0) {
				if (CheckSoundMem(sound_playerDestroyed)) StopSoundMem(sound_playerDestroyed);
				PlaySoundMem(sound_playerDestroyed, DX_PLAYTYPE_BACK);

				if (!isMuteki) {
					StateManager::ChangeState(Joutai::Lose);
				}
			}

			pEnemyShot = pEnemyShot->next;
		}
		pEnemyShotSet = pEnemyShotSet->next;
	}
}
