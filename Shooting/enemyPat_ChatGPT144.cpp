#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：静寂の雪原
// 画面いっぱいに雪が積もるように、低速の弾を横一列ずつ降らせる。
// 安全地帯は列ごとに少しずつ移動し、時間とともに雪原全体の隙間が変化する。

static void ShotSnowfield(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const int bulletCount = 18;
        const double screenWidth = 480.0;
        const double spacing = 27.0;

        // 列ごとに安全地帯の中心をゆっくり移動させる。
        const int phase = pEnemyShotSet->kind % 12;
        const double gapCenter = 60.0 + phase * 30.0;
        const double gapHalfWidth = 48.0;

        //if (phase == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        //}

        for (int i = 0; i < bulletCount; ++i) {
            // 左端から等間隔に並べ、移動する安全地帯だけを空ける。
            double x = 4.0 + spacing * i;
            if (x > screenWidth - 4.0) x = screenWidth - 4.0;

            if (fabs(x - gapCenter) < gapHalfWidth) {
                // 安全地帯の幅を維持するため、この位置には弾を置かない。
                continue;
            }

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x;
            pEnemyShot->y = pEnemyShotSet->y;

            // すべて1px/frame以下。ほぼ垂直に落ちながら、わずかに左右へ漂う。
            const double drift = ((i + phase) % 5 - 2) * 0.045;
            const double vertical = 0.42 + ((i * 7 + phase * 3) % 7) * 0.055;
            pEnemyShot->speed = sqrt(drift * drift + vertical * vertical);
            pEnemyShot->muki = atan2(vertical, drift);

            // 白い小玉を基本に、ところどころ白い楕円弾を混ぜる。
            // 楕円弾は落下方向に向きを合わせる。
            if ((i + phase) % 6 == 0) {
                pEnemyShot->kind = img_enemyShotMediumOval[6];
                pEnemyShot->param_i[0] = 1;
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[6];
                pEnemyShot->param_i[0] = 0;
            }

            pEnemyShot->param_d[0] = pEnemyShot->x;
            pEnemyShot->param_d[1] = pEnemyShot->y;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 中楕円弾は移動方向に合わせて向きを更新する。
        if (pShot->param_i[0] != 0) {
            pShot->muki = atan2(
                pShot->speed * sin(pShot->muki),
                pShot->speed * cos(pShot->muki)
            );
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SlowBullet_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 42.0;
        enemy.maxHp = enemy.hp = 250;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 上端を左右へゆっくり往復する。
        enemy.x += 0.72 * (double)muki;
        if (enemy.x < 70.0) {
            enemy.x = 70.0;
            muki = 1;
        }
        if (enemy.x > 410.0) {
            enemy.x = 410.0;
            muki = -1;
        }
    }

    // 雪の列を一定間隔で追加する。
    if (count % 18 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSnowfield;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // ときどき新しい雪雲が広がる予告音。
        //if (shot_count % 8 == 0) {
        //    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        //    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        //}
    }
}