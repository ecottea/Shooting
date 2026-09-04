// enemyPat_doppler.cpp
// 弾幕：音速接近・音速離脱（ドップラー効果）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// ドップラー波
// ボスの移動方向側は弾列を圧縮し、後方側は広げる。
// param_i[0] : 進行方向（-1 / +1）
// param_i[1] : 波の向き（1=進行方向側、-1=後方側）
// param_i[2] : 色相種別
// param_d[0] : 基準角
// param_d[1] : 弾列中心からの横オフセット
// ------------------------------------------------------------
static void ShotDoppler(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const int dir = pEnemyShotSet->param_i[0];
        const int side = pEnemyShotSet->param_i[1];
        const int wave = pEnemyShotSet->param_i[2];

        // 進行方向側は高密度、後方側は低密度。
        const double spacing = (side > 0) ? 11.0 : 25.0;
        const int n = (side > 0) ? 24 : 14;
        const double baseX = pEnemyShotSet->x + dir * pEnemyShotSet->param_d[1];
        const double baseY = pEnemyShotSet->y + 12.0;
        const double angle = pEnemyShotSet->param_d[0];

        for (int i = 0; i < n; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            const double offset = (i - (n - 1) * 0.5) * spacing;
            const double laneX = baseX + offset * 0.92;

            pShot->x = laneX;
            pShot->y = baseY;

            // 進行方向側ほど前へ伸びる斜めの波面、後方側は緩やかな波面。
            const double sideTilt = (side > 0) ? 0.26 : -0.18;
            const double local = offset / (n * spacing * 0.5);
            pShot->muki = angle + sideTilt * local + 0.04 * dir;
            pShot->speed = (side > 0) ? 2.55 : 2.25;

            // 進行方向側を明るい中玉、後方を小玉にして密度差を見やすくする。
            if (side > 0) {
                pShot->kind = img_enemyShotMediumBall[(wave + i) % 8];
            }
            else {
                pShot->kind = img_enemyShotSmallBall[(wave + i) % 8];
            }

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Doppler_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右往復運動。音源の移動方向が切り替わるたびに
        // 「圧縮側」と「伸長側」も入れ替わる。
        enemy.x += 1.15 * muki;
        if (enemy.x >= 415.0) muki = -1;
        if (enemy.x <= 65.0)  muki = 1;
    }

    const int T = 22;

    // 進行方向側：短い間隔で連続する圧縮波。
    if (count % T == 1) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDoppler;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = DX_PI / 2.0;
        pSet->kind = shot_count++;
        pSet->param_i[0] = muki;
        pSet->param_i[1] = 1;
        pSet->param_i[2] = pSet->kind % 8;
        pSet->param_d[0] = DX_PI / 2.0;
        pSet->param_d[1] = 18.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 後方側：遅れて現れる伸長波。圧縮波との間に隙間が生まれる。
    if (count % T == 1 + T / 2) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDoppler;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 2.0;
        pSet->muki = DX_PI / 2.0;
        pSet->kind = shot_count++;
        pSet->param_i[0] = muki;
        pSet->param_i[1] = -1;
        pSet->param_i[2] = pSet->kind % 8;
        pSet->param_d[0] = DX_PI / 2.0;
        pSet->param_d[1] = 8.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}