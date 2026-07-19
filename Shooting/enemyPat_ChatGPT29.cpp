// enemyPat_bubble.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotBubble(sEnemyShotSet* pEnemyShotSet)
{
	sEnemyShot* pEnemyShot;

	//----------------------------------------------------------
	// 初回：大きな泡を生成
	//----------------------------------------------------------
	if (pEnemyShotSet->count == 0)
	{
		if (CheckSoundMem(sound_enemyShot_medium) == 1)
			StopSoundMem(sound_enemyShot_medium);
		PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

		const int N = 24;

		for (int i = 0; i < N; i++)
		{
			double ang = DX_PI * 2.0 * i / N;
			double r = 35.0 + GetRand(25);

			pEnemyShot = new sEnemyShot;

			pEnemyShot->x = pEnemyShotSet->x + cos(ang) * r;
			pEnemyShot->y = pEnemyShotSet->y + sin(ang) * r;

			pEnemyShot->speed = 0.0;
			pEnemyShot->muki = ang;

			// 白い大玉＝泡
			pEnemyShot->kind = img_enemyShotLargeBall[6];

			// 初期位置保存
			pEnemyShot->param_d[0] = pEnemyShot->x;
			pEnemyShot->param_d[1] = pEnemyShot->y;

			// 漂う方向
			pEnemyShot->param_d[2] = ang;

			// 初期半径
			pEnemyShot->param_d[3] = r;

			// 寿命
			pEnemyShot->param_i[0] = 120 + GetRand(80);

			// 破裂済みフラグ
			pEnemyShot->param_i[1] = 0;

			pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
			pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
			pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
			pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
		}
	}

	//----------------------------------------------------------
	// 泡の更新
	//----------------------------------------------------------
	pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

	while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
	{
		if (pEnemyShot->kind != img_enemyShotSmallBall[6]) {
			//------------------------------------------------------
			// ゆっくり上昇
			//------------------------------------------------------
			pEnemyShot->param_d[1] -= 0.18;

			//------------------------------------------------------
			// 左右へふらふら
			//------------------------------------------------------
			double sway =
				sin((double)pEnemyShot->count * 0.05 +
					pEnemyShot->param_d[2]) * 6.0;

			//------------------------------------------------------
			// 少しずつ外側へ膨らむ
			//------------------------------------------------------
			double grow =
				pEnemyShot->param_d[3] +
				pEnemyShot->count * 0.05;

			pEnemyShot->x =
				pEnemyShotSet->x +
				cos(pEnemyShot->muki) * grow +
				sway;

			pEnemyShot->y =
				pEnemyShot->param_d[1] +
				sin(pEnemyShot->muki) * grow * 0.2;

			//------------------------------------------------------
			// 見た目だけ少し点滅
			//------------------------------------------------------
			if ((pEnemyShot->count / 10) & 1)
				pEnemyShot->kind = img_enemyShotLargeBall[6];
			else
				pEnemyShot->kind = img_enemyShotMediumBall[6];

			//------------------------------------------------------
			// 寿命で破裂
			//------------------------------------------------------
			if (!pEnemyShot->param_i[1] &&
				pEnemyShot->count >= pEnemyShot->param_i[0])
			{
				pEnemyShot->param_i[1] = 1;

				PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

				const int CHILD = 12;

				for (int i = 0; i < CHILD; i++)
				{
					sEnemyShot* pChild = new sEnemyShot;

					double ang =
						DX_PI * 2.0 * i / CHILD
						+ (GetRand(100) / 100.0 - 0.5) * 0.4;

					pChild->x = pEnemyShot->x;
					pChild->y = pEnemyShot->y;
					pChild->muki = ang;
					pChild->speed = 2.6 + GetRand(80) / 100.0;

					// 白い小玉
					pChild->kind = img_enemyShotSmallBall[6];

					// 漂うための乱数
					pChild->param_d[0] = (GetRand(628) / 100.0);

					pChild->param_i[1] = 1;

					pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
					pChild->next = pEnemyShotSet->pEnemyShotHead;
					pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
					pEnemyShotSet->pEnemyShotHead->prev = pChild;
				}

				// 元の泡は画面外へ飛ばして自動削除
				pEnemyShot->x = -1000.0;
				pEnemyShot->y = -1000.0;
				pEnemyShot->margin = 0.0;
			}
		}

		//------------------------------------------------------
		// 小泡の移動
		//------------------------------------------------------
		if (pEnemyShot->kind == img_enemyShotSmallBall[6])
		{
			pEnemyShot->speed *= 0.992;

			pEnemyShot->x +=
				cos(pEnemyShot->muki) * pEnemyShot->speed;

			pEnemyShot->y +=
				sin(pEnemyShot->muki) * pEnemyShot->speed;

			// 浮力
			pEnemyShot->y -= 0.03;

			// 少し蛇行
			pEnemyShot->x +=
				sin(
					pEnemyShot->count * 0.12 +
					pEnemyShot->param_d[0]
				) * 0.35;
		}

		pEnemyShot = pEnemyShot->next;
	}
}

//----------------------------------------------------------
// 敵本体
//----------------------------------------------------------
void EnemyPat_SoapBubbles_ChatGPT()
{
	static int dir;

	if (count == 1)
	{
		enemy.x = 240.0;
		enemy.y = 140.0;

		enemy.maxHp = 200;
		enemy.hp = 200;

		dir = 1;
	}
	else
	{
		enemy.x += dir * 0.8;

		if (enemy.x < 120.0) dir = 1;
		if (enemy.x > 360.0) dir = -1;
	}

	//------------------------------------------------------
	// 泡生成
	//------------------------------------------------------
	if (count % 150 == 30)
	{
		sEnemyShotSet* pSet = new sEnemyShotSet;

		pSet->count = 0;
		pSet->patternFunc = ShotBubble;
		pSet->x = enemy.x;
		pSet->y = enemy.y + 10.0;

		pSet->pEnemyShotHead = new sEnemyShot;
		pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
		pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

		pSet->prev = enemyShotSetHead.prev;
		pSet->next = &enemyShotSetHead;
		enemyShotSetHead.prev->next = pSet;
		enemyShotSetHead.prev = pSet;
	}
}