#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotWaterfall(sEnemyShotSet* pEnemyShotSet)
{
	sEnemyShot* pEnemyShot;

	// 発生
	if (pEnemyShotSet->count == 0)
	{
		for (int i = -5; i <= 5; i++)
		{
			pEnemyShot = new sEnemyShot;

			pEnemyShot->x = pEnemyShotSet->x + i * 30.0;
			pEnemyShot->y = 0;

			pEnemyShot->muki = DX_PI / 2.0;
			pEnemyShot->speed = 2.5 + fabs(i) * 0.08;

			int color;

			if (i == 0)
				color = 6; // 白
			else if (abs(i) <= 2)
				color = 3; // シアン
			else
				color = 4; // 青

			pEnemyShot->kind = img_enemyShotSmallBall[color];

			pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
			pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
			pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
			pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
		}
	}

	// 移動
	pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

	while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
	{
		double t = (double)pEnemyShotSet->count;

		pEnemyShot->x +=
			cos(t * 0.05 + pEnemyShot->y * 0.02)
			* 0.35;

		pEnemyShot->y += pEnemyShot->speed;

		pEnemyShot = pEnemyShot->next;
	}
}

void EnemyPat_Waterfall_ChatGPT()
{
	if (count == 1)
	{
		enemy.x = 240.0;
		enemy.y = 60.0;

		enemy.maxHp = enemy.hp = 120;
	}

	enemy.x = 240.0 + sin(count * 0.01) * 80.0;

	if (count % 6 == 0)
	{
		sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

		pEnemyShotSet->count = 0;
		pEnemyShotSet->patternFunc = ShotWaterfall;

		pEnemyShotSet->x =
			enemy.x +
			sin(count * 0.03) * 120.0;

		pEnemyShotSet->y = enemy.y + 10.0;

		pEnemyShotSet->muki = DX_PI / 2.0;

		pEnemyShotSet->pEnemyShotHead = new sEnemyShot;

		pEnemyShotSet->pEnemyShotHead->prev =
			pEnemyShotSet->pEnemyShotHead;

		pEnemyShotSet->pEnemyShotHead->next =
			pEnemyShotSet->pEnemyShotHead;

		pEnemyShotSet->prev = enemyShotSetHead.prev;
		pEnemyShotSet->next = &enemyShotSetHead;

		enemyShotSetHead.prev->next = pEnemyShotSet;
		enemyShotSetHead.prev = pEnemyShotSet;

		if (count % 24 == 0)
		{
			if (CheckSoundMem(sound_enemyShot_light) == 1)
				StopSoundMem(sound_enemyShot_light);

			PlaySoundMem(
				sound_enemyShot_light,
				DX_PLAYTYPE_BACK);
		}
	}
}