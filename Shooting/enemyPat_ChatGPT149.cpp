#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：マシンガン
// 連射中は発射角を少しずつずらし、銃身が反動で左右に振れるように見せる。
// 小型の銃弾を連射し、連射の最後に中型弾を一発だけ撃つ。
static void ShotMachineGun(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // param_i[0] : 連射済みの弾数
        // param_d[0] : 連射開始時に狙った基準角度
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_d[0] = atan2(
            player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x
        );
    }

    // 連射中はボスの位置に発射位置を追従させる。
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 12.0;

    const int BURST_SHOTS = 12+6;
    const int SHOT_INTERVAL = 3;

    // 一定間隔で銃弾を連射する。
    if (pEnemyShotSet->param_i[0] < BURST_SHOTS &&
        pEnemyShotSet->count % SHOT_INTERVAL == 0) {

        const int shotIndex = pEnemyShotSet->param_i[0];

        // 連射が進むほど少しずつ射角をずらして扇状に広げる。
        const double angleStep = 0.014*1.5;
        const double center = (BURST_SHOTS - 1) * 0.5;
        double muki = pEnemyShotSet->param_d[0] + (shotIndex - center) * angleStep;

        // 銃身が反動で左右に跳ねるよう、発射位置を左右へ交互にずらす。
        const double muzzleOffset = (shotIndex % 2 == 0) ? -3.0 : 3.0;
        const double sideX = -sin(pEnemyShotSet->param_d[0]) * muzzleOffset;
        const double sideY = cos(pEnemyShotSet->param_d[0]) * muzzleOffset;

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x + sideX;
        pEnemyShot->y = pEnemyShotSet->y + sideY;
        pEnemyShot->muki = muki;
        pEnemyShot->speed = 5.5;
        pEnemyShot->kind = img_enemyShotBullet[0];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0]++;
    }

    // 連射の最後に、重い一撃を混ぜてマシンガンの締めにする。
    if (pEnemyShotSet->param_i[0] == BURST_SHOTS &&
        pEnemyShotSet->param_i[1] == 0) {

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->param_d[0];
        pEnemyShot->speed = 3.8;
        pEnemyShot->kind = img_enemyShotLargeBall[8];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[1] = 1;
    }

    {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_MachineGun_ChatGPT()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.9 * (double)muki;
        if (enemy.x < 70.0) {
            enemy.x = 70.0;
            muki = 1;
        }
        else if (enemy.x > 410.0) {
            enemy.x = 410.0;
            muki = -1;
        }
    }

    // ボスが移動しながら、短いマシンガン連射を繰り返す。
    if (count % 75 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMachineGun;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = atan2(
            player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x
        );
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}