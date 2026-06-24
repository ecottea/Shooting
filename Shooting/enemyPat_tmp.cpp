// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------
// 音ゲーモチーフ：連符ノート（高密度連射）
// ---------------------------
static void ShotRapidNote(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 16発の小玉を扇形に発射
        for (int i = 0; i < 16; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 真下を中心に±30度の範囲で発射
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(60) - 30) * DX_PI / 180.0;
            pEnemyShot->speed = 3.5 + (GetRand(150) / 100.0); // 3.5~5.0

            int color = GetRand(7);
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------
// 音ゲーモチーフ：トリルノート（プレイヤーを狙う高速連射）
// ---------------------------
static void ShotTrillNote(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 8発の中玉をプレイヤー方向に高速発射
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // プレイヤー方向に微妙にばらつき
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(15) - 7.5) * DX_PI / 180.0;
            pEnemyShot->speed = 5.0 + (GetRand(100) / 100.0); // 5.0~6.0

            int color = GetRand(7);
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------
// 音ゲーモチーフ：グリッサンドノート（加速する弾）
// ---------------------------
static void ShotGlissandoNote(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 6発の大玉を発射（加速する）
        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(20) - 10) * DX_PI / 180.0;
            pEnemyShot->speed = 2.0 + (i * 0.5); // 2.0~4.5（発射ごとに速くなる）

            int color = GetRand(7);
            pEnemyShot->kind = img_enemyShotLargeBall[color];

            // 加速度を設定
            pEnemyShot->param_d[0] = 0.02 * (i + 1); // 加速度

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->speed += pEnemyShot->param_d[0]; // 加速
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------
// 音ゲーモチーフ：アルペジオ（円形に広がる弾）
// ---------------------------
static void ShotArpeggioNote(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme) == 1) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 24発の菱形弾を円形に発射
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * (DX_PI * 2 / 24); // 360度を24等分
            pEnemyShot->speed = 2.5 + (GetRand(100) / 100.0); // 2.5~3.5

            int color = i % 7;
            pEnemyShot->kind = img_enemyShotDiamond[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------
// 音ゲーモチーフ：トレモロ（振動する弾）
// ---------------------------
static void ShotTremoloNote(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 10発の鱗弾を発射（振動する）
        for (int i = 0; i < 10; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(30) - 15) * DX_PI / 180.0;
            pEnemyShot->speed = 3.0 + (GetRand(100) / 100.0); // 3.0~4.0

            int color = GetRand(7);
            pEnemyShot->kind = img_enemyShotScale[color];

            // 振動のパラメータ
            pEnemyShot->param_d[0] = 0.0; // 位相
            pEnemyShot->param_d[1] = 0.1 * (i + 1); // 振幅
            pEnemyShot->param_d[2] = 0.2 + (GetRand(100) / 1000.0); // 周期

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->param_d[0] += pEnemyShot->param_d[2]; // 位相を更新
        double offsetX = pEnemyShot->param_d[1] * sin(pEnemyShot->param_d[0]);
        double offsetY = pEnemyShot->param_d[1] * cos(pEnemyShot->param_d[0] * 0.5);
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki) + offsetX;
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki) + offsetY;
        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------
// 敵本体のパターン
// ---------------------------
void EnemyPat_Tmp()
{
    static int muki;
    static int phase = 0; // 0:連符, 1:トリル, 2:グリッサンド, 3:アルペジオ, 4:トレモロ
    static int step = 0; // 各フェーズ内のステップ

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
        step = 0;
    }
    else {
        // 敵は画面上部で高速に左右に往復移動
        enemy.x += 2.0 * muki;
        if (enemy.x < 20 || enemy.x > 460) muki *= -1;

        // 30フレームごとにステップを進める
        if (count % 30 == 0) {
            step++;
            // 6ステップごとにフェーズを切り替え
            if (step >= 6) {
                step = 0;
                phase = (phase + 1) % 5;
            }
        }
    }

    // フェーズごとに弾幕を発射
    switch (phase) {
    case 0: // 連符ノート
        if (count % 15 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotRapidNote;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 20.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        break;

    case 1: // トリルノート
        if (count % 10 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotTrillNote;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 20.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        break;

    case 2: // グリッサンドノート
        if (count % 20 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotGlissandoNote;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 20.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        break;

    case 3: // アルペジオ
        if (count % 40 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotArpeggioNote;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 20.0;
            pEnemyShotSet->muki = 0;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        break;

    case 4: // トレモロ
        if (count % 25 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotTremoloNote;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 20.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        break;
    }
}