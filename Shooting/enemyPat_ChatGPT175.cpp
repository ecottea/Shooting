#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：ペンデュラムウェーブ
// 中玉を振り子のおもり、小玉の点列を糸に見立てる。
// 各振り子は少しずつ異なる周期で揺れ、時間とともに位相がずれて
// 波のような形を作る。全振り子の位相が揃うと、再び同じ形に戻る。
// ============================================================
static void ShotPendulumWave(sEnemyShotSet* pEnemyShotSet)
{
    const int PENDULUM_COUNT = 13;
    const int STRING_DOT_COUNT = 5;

    // 1セットの初回実行時に、振り子13本分の弾を作成する。
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < PENDULUM_COUNT; i++) {
            // 糸を白い小玉の点列で表現する。
            for (int j = 1; j <= STRING_DOT_COUNT; j++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白・小玉
                pEnemyShot->param_i[0] = i; // 振り子番号
                pEnemyShot->param_i[1] = j; // 糸上の位置
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // おもりはシアンの中玉で表現する。
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン・中玉
            pEnemyShot->param_i[0] = i;
            pEnemyShot->param_i[1] = 0; // 0はおもり
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 周期差を整数比にすることで、一定時間後に全振り子の位相が揃う。
    // 13本の振り子は、基準の36周から48周まで少しずつ周期が変化する。
    const double WAVE_DURATION = 2160.0;
    const double BASE_CYCLES = 36.0;
    const double t = (double)pEnemyShotSet->count;
    const double pivotY = pEnemyShotSet->y + 0.16 * t*5;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int index = pShot->param_i[0];
        const int segment = pShot->param_i[1];

        const double cycles = BASE_CYCLES + index;
        const double angle = 0.52 * sin(2.0 * DX_PI * cycles * t / WAVE_DURATION);

        // 振り子の周期に合わせて糸の長さも変える。
        const double ratio = BASE_CYCLES / cycles;
        const double length = 78.0 * ratio * ratio;

        const double pivotX = pEnemyShotSet->x
            + 432.0 * (double)index / (double)(PENDULUM_COUNT - 1);
        const double bobX = pivotX + length * sin(angle);
        const double bobY = pivotY + length * cos(angle);

        if (segment == 0) {
            // おもり
            pShot->x = bobX;
            pShot->y = bobY;
        }
        else {
            // 糸の点列を支点とおもりの間に配置する。
            const double fraction = (double)segment / (double)(STRING_DOT_COUNT + 1);
            pShot->x = pivotX + (bobX - pivotX) * fraction;
            pShot->y = pivotY + (bobY - pivotY) * fraction;
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体の弾幕パターン
// ============================================================
void EnemyPat_PendulumWave_ChatGPT()
{
    const int WAVE_INTERVAL = 3000/20;
    const int WARNING_FRAMES = 60;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 24.0;
        enemy.maxHp = enemy.hp = 200;
    }

    if (count <= 0) return;

    const int cycle = (count - 1) % WAVE_INTERVAL;

    // 次のペンデュラムウェーブが始まることを予告する。
    if (cycle == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 予告後に13本の振り子を一斉に出現させる。
    if (cycle == WARNING_FRAMES) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPendulumWave;
        pEnemyShotSet->x = 24.0; // 左端の支点
        pEnemyShotSet->y = 28.0; // 支点の初期Y座標
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
