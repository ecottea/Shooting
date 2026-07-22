// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：焔符「大文字送り火 -篝火の檻-」
static void ShotOomoji(sEnemyShotSet* pEnemyShotSet)
{
	// --- 【初期化処理】美しい「大」の字の骨組みを生成 ---
	if (pEnemyShotSet->count == 0) {
		if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
		PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

		// 書道のような自然なバランスと「払い」のカーブを表現するための6つの線分
		// { start_x, start_y, end_x, end_y }
		double strokes[6][4] = {
			{-95.0, -25.0,  95.0, -25.0}, // 1. 横棒 (一)：長くどっしりと
			{  0.0, -85.0,   0.0, -25.0}, // 2. 縦棒 (丨)：交点より上の部分
			{  0.0, -25.0, -35.0,  40.0}, // 3. 左払い上部：少し急な角度で降りる
			{-35.0,  40.0, -95.0,  95.0}, // 4. 左払い下部：外側へなだらかに流れる曲線美
			{  0.0, -25.0,  35.0,  40.0}, // 5. 右払い上部
			{ 35.0,  40.0,  95.0,  95.0}  // 6. 右払い下部
		};

		double spacing = 7.0; // 弾の配置間隔

		for (int i = 0; i < 6; i++) {
			double sx = strokes[i][0], sy = strokes[i][1];
			double ex = strokes[i][2], ey = strokes[i][3];
			double dist = sqrt((ex - sx) * (ex - sx) + (ey - sy) * (ey - sy));
			int num = (int)(dist / spacing);

			// この線分の基準となる角度（火の粉を飛ばすための法線計算に使う）
			double base_angle = atan2(ey - sy, ex - sx);

			for (int j = 0; j <= num; j++) {
				double t = (double)j / num;
				double lx = sx + (ex - sx) * t;
				double ly = sy + (ey - sy) * t;

				sEnemyShot* p = new sEnemyShot;
				p->param_i[7] = 1;          // 1: 構造弾フラグ
				p->param_d[0] = lx;         // ローカルX座標
				p->param_d[1] = ly;         // ローカルY座標
				p->param_d[2] = base_angle; // 線分の角度

				// 赤と橙を混ぜて燃え盛る火床を表現
				p->kind = (GetRand(1) == 0) ? img_enemyShotMediumBall[0] : img_enemyShotMediumBall[8];

				p->prev = pEnemyShotSet->pEnemyShotHead->prev;
				p->next = pEnemyShotSet->pEnemyShotHead;
				pEnemyShotSet->pEnemyShotHead->prev->next = p;
				pEnemyShotSet->pEnemyShotHead->prev = p;
			}
		}
	}

	// 時間経過による大文字の回転角度 theta
	double theta = pEnemyShotSet->count * 0.006;

	// 火の粉がはじけるSE（一定間隔で鳴らす）
	if (pEnemyShotSet->count % 12 == 0) {
		if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
		PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
	}

	// --- ギミック2: 大文字の交点からの自機狙い高速重炎弾 ---
	if (pEnemyShotSet->count % 45 == 0 && pEnemyShotSet->count > 0) {
		// 交点（ローカル座標 0.0, -25.0）の絶対座標を計算して、そこから発射する
		double cx = enemy.x + 25.0 * sin(theta);
		double cy = enemy.y - 25.0 * cos(theta);
		double target_angle = atan2(player.y - cy, player.x - cx);

		for (int i = -2; i <= 2; i++) {
			sEnemyShot* pSniper = new sEnemyShot;
			pSniper->param_i[7] = 0; // 通常弾
			pSniper->x = cx;
			pSniper->y = cy;
			pSniper->muki = target_angle + i * (10.0 / 180.0 * DX_PI);
			pSniper->speed = 3.6 - abs(i) * 0.3;

			if (i == 0) pSniper->kind = img_enemyShotLargeBall[0];
			else pSniper->kind = img_enemyShotMediumBall[8];

			pSniper->prev = pEnemyShotSet->pEnemyShotHead->prev;
			pSniper->next = pEnemyShotSet->pEnemyShotHead;
			pEnemyShotSet->pEnemyShotHead->prev->next = pSniper;
			pEnemyShotSet->pEnemyShotHead->prev = pSniper;
		}

		if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
		PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
	}

	// --- 【全登録弾の座標更新ループ】 ---
	sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
	while (pShot != pEnemyShotSet->pEnemyShotHead) {
		if (pShot->param_i[7] == 1) {
			// --- 構造弾（大文字の骨組み）の処理 ---
			double lx = pShot->param_d[0];
			double ly = pShot->param_d[1];

			// 2次元回転行列の適用
			double x_rot = lx * cos(theta) - ly * sin(theta);
			double y_rot = lx * sin(theta) + ly * cos(theta);

			pShot->x = enemy.x + x_rot;
			pShot->y = enemy.y + y_rot;

			// ギミック1: 骨組みの弾から一定確率で火の粉を発生させる
			// 全体の弾数に対して確率を調整 (1000分の8の確率で毎フレーム判定)
			if (GetRand(1000) < 16) {
				sEnemyShot* pSpark = new sEnemyShot;
				pSpark->param_i[7] = 0; // 通常弾
				pSpark->x = pShot->x;
				pSpark->y = pShot->y;

				// 線分の向きに対して垂直（外側）へ飛ばす
				double base_angle = pShot->param_d[2];
				double normal_angle = base_angle + ((GetRand(1) == 0) ? DX_PI / 2.0 : -DX_PI / 2.0);

				// 大文字の回転角を足し、少しランダムなブレを加える
				pSpark->muki = normal_angle + theta + (GetRand(20) - 10) / 180.0 * DX_PI;
				pSpark->speed = (80 + GetRand(140)) / 100.0;

				int col = GetRand(2);
				pSpark->kind = img_enemyShotSmallBall[(col == 0) ? 0 : (col == 1 ? 1 : 8)];

				// リストの末尾（head->prev）に挿入
				pSpark->prev = pEnemyShotSet->pEnemyShotHead->prev;
				pSpark->next = pEnemyShotSet->pEnemyShotHead;
				pEnemyShotSet->pEnemyShotHead->prev->next = pSpark;
				pEnemyShotSet->pEnemyShotHead->prev = pSpark;
			}
		}
		else {
			// --- 直進弾（火の粉・狙撃弾）の処理 ---
			pShot->x += pShot->speed * cos(pShot->muki);
			pShot->y += pShot->speed * sin(pShot->muki);
		}
		pShot = pShot->next;
	}
}

// 敵本体のパターン関数
void EnemyPat_Daimonji_Gemini()
{
	if (count == 1) {
		enemy.x = 240.0;
		enemy.y = 130.0;
		enemy.maxHp = enemy.hp = 200;

		sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
		pEnemyShotSet->count = 0;
		pEnemyShotSet->patternFunc = ShotOomoji;
		pEnemyShotSet->x = enemy.x;
		pEnemyShotSet->y = enemy.y;

		pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
		pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
		pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

		pEnemyShotSet->prev = enemyShotSetHead.prev;
		pEnemyShotSet->next = &enemyShotSetHead;
		enemyShotSetHead.prev->next = pEnemyShotSet;
		enemyShotSetHead.prev = pEnemyShotSet;
	}
	else {
		// ボス自体をサイン波で左右にゆっくりと揺らす
		enemy.x = 240.0 + 45.0 * sin(count * 0.012);
	}
}