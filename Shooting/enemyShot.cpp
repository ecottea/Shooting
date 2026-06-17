#include "DxLib.h"
#include "gv.h"
#include "enemyShot.h"
#include "imgSoundLoad.h"
#include "stateManager.h"

void enemyShotControl(){
	sEnemyShotSet *pEnemyShotSet;
	
	pEnemyShotSet = enemyShotSetHead.next;
	while(pEnemyShotSet != &enemyShotSetHead){
		sEnemyShotSet* pNext = pEnemyShotSet->next;

		pEnemyShotSet->patternFunc(pEnemyShotSet);

		pEnemyShotSet = pNext; //次へ
	}
}

void enemyShotCalc()
{
	sEnemyShotSet *pEnemyShotSet, *pNextEnemyShotSet;
	sEnemyShot *pEnemyShot, *pNextEnemyShot;

	pEnemyShotSet = enemyShotSetHead.next;
	while(pEnemyShotSet != &enemyShotSetHead){
		pEnemyShotSet->count++; //処理
		
		pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
		while(pEnemyShot != pEnemyShotSet->pEnemyShotHead){
			pEnemyShot->count++; //処理
			
			pNextEnemyShot = pEnemyShot->next; //避難

			if (pEnemyShot->x < -pEnemyShot->margin || pEnemyShot->x > 480.0 + pEnemyShot->margin //画面外の弾は削除
				|| pEnemyShot->y < -pEnemyShot->margin || pEnemyShot->y > 480.0 + pEnemyShot->margin) {
				pEnemyShot->prev->next = pEnemyShot->next;
				pEnemyShot->next->prev = pEnemyShot->prev;
				delete pEnemyShot;
			}
			
			pEnemyShot = pNextEnemyShot; //次へ
		}
		
		pNextEnemyShotSet = pEnemyShotSet->next; //避難
		
		if(pEnemyShotSet->pEnemyShotHead->next==pEnemyShotSet->pEnemyShotHead && pEnemyShotSet->count>10){ //空のセットは削除
			delete pEnemyShotSet->pEnemyShotHead;
			
			pEnemyShotSet->prev->next = pEnemyShotSet->next;
			pEnemyShotSet->next->prev = pEnemyShotSet->prev;
			delete pEnemyShotSet;
		}
		
		pEnemyShotSet = pNextEnemyShotSet; //次へ
	}
}

void enemyShotDisp()
{
	sEnemyShotSet *pEnemyShotSet;
	sEnemyShot *pEnemyShot;

	pEnemyShotSet = enemyShotSetHead.next;
	while(pEnemyShotSet != &enemyShotSetHead){
		pEnemyShot = pEnemyShotSet->pEnemyShotHead->next; //処理
		while(pEnemyShot != pEnemyShotSet->pEnemyShotHead){
			if (imageData[pEnemyShot->kind].no_rotate) {
				DrawRotaGraph((int)pEnemyShot->x, (int)pEnemyShot->y, imageData[pEnemyShot->kind].mag, 0.0, imageData[pEnemyShot->kind].handle, TRUE, FALSE);
			}
			else {
				DrawRotaGraph((int)pEnemyShot->x, (int)pEnemyShot->y, imageData[pEnemyShot->kind].mag, pEnemyShot->muki, imageData[pEnemyShot->kind].handle, TRUE, FALSE);
			}
			pEnemyShot = pEnemyShot->next; //次へ
		}
		
		pEnemyShotSet = pEnemyShotSet->next; //次へ
	}
}

void enemyShotHit()
{
	// Bキーが押されている間は無敵（当たり判定をしない）
	if (CheckHitKey(KEY_INPUT_B) == 1) {
		return;
	}

	double x, y, r;
	
	sEnemyShotSet *pEnemyShotSet;
	sEnemyShot *pEnemyShot;

	pEnemyShotSet = enemyShotSetHead.next;
	while(pEnemyShotSet != &enemyShotSetHead){
		pEnemyShot = pEnemyShotSet->pEnemyShotHead->next; //処理
		while(pEnemyShot != pEnemyShotSet->pEnemyShotHead){
			x = player.x - pEnemyShot->x;
			y = player.y - pEnemyShot->y;
			r = imageData[pEnemyShot->kind].radius + 2.3;
			if(x*x + y*y < r*r){
				PlaySoundMem(sound_playerDestroyed, DX_PLAYTYPE_BACK);
				StateManager::ChangeState(Joutai::Lose);
			}
			 
			pEnemyShot = pEnemyShot->next; //次へ
		}
		
		pEnemyShotSet = pEnemyShotSet->next; //次へ
	}
}
