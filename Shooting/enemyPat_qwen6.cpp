// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include <math.h>

// ============================================================
// 新パターン：波と粒の境界
//  一定時間まではサインカーブで波打ちながら進み（波動性）、
//  境界を超えると振幅が0になり直進する（粒子性）。
// ============================================================
static void ShotWaveAndParticle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int num_shots = 21;
        double spread_angle = 180.0 * DX_PI / 180.0; // 180度扇状にばら撒く
        double start_angle = pEnemyShotSet->muki - spread_angle / 2.0;

        for (int i = 0; i < num_shots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = start_angle + (spread_angle * i / (num_shots - 1));
            pEnemyShot->speed = 2.2;

            // 波のときは鱗弾（色は青系:4）
            pEnemyShot->kind = img_enemyShotScale[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    int t = pEnemyShotSet->count;
    double boundary = 120.0; // 60フレームで波から粒に変わる（境界）
    double A0 = 10.0;        // 初期振幅
    int waves = 10;         // 境界までにうねる波の数
    double w = DX_PI * waves / boundary; // 角周波数 (t=boundaryでsinが0になるように調整)

    // 現在の振幅と前フレームの振幅を計算（線形減衰で境界で0になる）
    double decay = (t < boundary) ? (1.0 - t / boundary) : 0.0;
    double amp_curr = A0 * decay * sin(w * t);

    double decay_prev = (t > 0 && t - 1 < boundary) ? (1.0 - (t - 1) / boundary) : 0.0;
    double amp_prev = A0 * decay_prev * sin(w * (t - 1));

    // 垂直方向へのオフセット増分
    double d_amp = amp_curr - amp_prev;

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 【境界】を超えたら、弾の画像を「粒」に変更
        if (t == (int)boundary) {
            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青系の小玉に変化
        }

        // 進行方向への移動
        double dx = pEnemyShot->speed * cos(pEnemyShot->muki);
        double dy = pEnemyShot->speed * sin(pEnemyShot->muki);

        // 進行方向に対して垂直な単位ベクトル
        double nx = -sin(pEnemyShot->muki);
        double ny = cos(pEnemyShot->muki);

        // 位置更新（進行方向 + 垂直方向の波の増分）
        pEnemyShot->x += dx + nx * d_amp;
        pEnemyShot->y += dy + ny * d_amp;

        pEnemyShot = pEnemyShot->next;
    }
}

static void ShotBigBall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
		PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

		pEnemyShot = new sEnemyShot;
		pEnemyShot->x = pEnemyShotSet->x;
		pEnemyShot->y = pEnemyShotSet->y;
		pEnemyShot->muki = pEnemyShotSet->muki;
		pEnemyShot->speed = 1.0;
        pEnemyShot->kind = img_enemyShotLargeBall[5];

		pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
		pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
		pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
		pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
	}
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Namistubu_Qwen()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 150;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (count % 10 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;

        // ★新パターン「波と粒の境界」を適用
        pEnemyShotSet->patternFunc = ShotWaveAndParticle;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (count % 90 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;

        pEnemyShotSet->patternFunc = ShotBigBall;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}