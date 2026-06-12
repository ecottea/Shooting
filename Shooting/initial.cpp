#include "DxLib.h"
#include "gv.h"
#include "initial.h"
#include "imgSoundLoad.h"
#include "replay.h"
#include "backGround.h"

void ini()
{
	joutaiFlag = Joutai::Menu;
	cursor.x = 0;
	cursor.y = 0;
}

void iniGame()
{
	sPlayerShot *pPlayerShot, *pNextPlayerShot;
	sEnemyShotSet *pEnemyShotSet, *pNextEnemyShotSet;
	sEnemyShot *pEnemyShot, *pNextEnemyShot;

	count = 0;
	replayKeyHistory.clear();
	replayActive = false;
	resetStars();

	player.x = 240.0;
	player.y = 400.0;
	
	pPlayerShot = playerShotHead.next;
	playerShotHead.next = &playerShotHead;
	playerShotHead.prev = &playerShotHead;
	
	while(pPlayerShot != &playerShotHead){
		pNextPlayerShot = pPlayerShot->next;
		
		delete pPlayerShot;
		
		pPlayerShot = pNextPlayerShot;
	}
	
	pEnemyShotSet = enemyShotSetHead.next;
	enemyShotSetHead.next = &enemyShotSetHead;
	enemyShotSetHead.prev = &enemyShotSetHead;

	while(pEnemyShotSet != &enemyShotSetHead){
		pNextEnemyShotSet = pEnemyShotSet->next;
		
		pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
		pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

		while(pEnemyShot != pEnemyShotSet->pEnemyShotHead){
			pNextEnemyShot = pEnemyShot->next;
			
			delete pEnemyShot;
			
			pEnemyShot = pNextEnemyShot;
		}
		delete pEnemyShotSet->pEnemyShotHead;
		delete pEnemyShotSet;
		
		pEnemyShotSet = pNextEnemyShotSet;
	}
}

void startNewGame()
{
	// 現在時刻をシードとして使用
	gameSeed = GetNowCount();
	SRand((int)gameSeed);

	iniGame();
	joutaiFlag = Joutai::Game;
}

void setColor()
{
	colorWhite = GetColor(255, 255, 255);
	colorGray = GetColor(127, 127, 127);
	colorGreenBlue = GetColor(0, 255, 255);
}
