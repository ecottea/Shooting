#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ヘリンボーン・クロスステッチ
// 1回の発射で斜めの弾列を形成し、次の発射では傾きを反転させる。
// 赤とシアンの小玉を交互に使い、杉綾模様を視認しやすくする。
static void ShotHerringbone(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 25; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 横方向に並べた弾列を、発射ごとに左右反転して配置する。
            const double t = (i - 12) / 12.0;
            const double offsetX = t * 180.0;
            const double offsetY = offsetX * tan(pEnemyShotSet->param_d[0]);

            pEnemyShot->x = pEnemyShotSet->x + offsetX;
            pEnemyShot->y = pEnemyShotSet->y + offsetY;
            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->speed = 2.8;
            pEnemyShot->kind = (pEnemyShotSet->kind == 0)
                ? img_enemyShotSmallBall[0]   // 赤
                : img_enemyShotSmallBall[3];  // シアン

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // この弾列に属する弾を移動させる。
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Herringbone_ChatGPT()
{
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 100;
        shot_count = 0;
    }
    else {
        // 敵をゆっくり揺らし、弾列の発射位置にも緩やかな変化を与える。
        enemy.x = 240.0 + 28.0 * sin(count * 0.008);
        enemy.y = 55.0 + 8.0 * sin(count * 0.013);
    }

    // 一定間隔で弾列を発射する。
    if (count % 17 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHerringbone;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;

        // プレイヤー方向を基準にしつつ、射出方向をゆっくり揺らす。
        pEnemyShotSet->muki = atan2(
            player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x
        ) + 0.08 * sin(count * 0.012);

        // 弾列の傾きを交互に反転させ、時間経過で傾斜幅も変化させる。
        const double tilt = 0.23 + 0.08 * sin(count * 0.014);
        pEnemyShotSet->kind = shot_count % 2;
        pEnemyShotSet->param_d[0] = (pEnemyShotSet->kind == 0) ? tilt : -tilt;

        // 効果音は数列ごとに鳴らし、連射中に重なりすぎないようにする。
        if (shot_count % 4 == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
