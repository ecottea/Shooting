#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 銃符「ルナティックガン」
// ・画面を横切る低速弾列で、通過できる隙間を作る。
// ・隙間の位置を少しずつ左右に変化させ、移動先を考えさせる。
// ・チャージ音の後、自機を狙う銃弾の密集した弾群を撃ち込む。
// count / sEnemyShotSet::count / sEnemyShot::count の更新と、
// 画面外の弾の削除はメインルーチン側に任せる。

static void AddLunaticGunShot(
    sEnemyShotSet* pEnemyShotSet,
    double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static sEnemyShotSet* CreateLunaticGunSet(
    double x, double y, double muki,
    sEnemyShotSet::PatternFunc patternFunc, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

static void PlayLunaticGunSound(int sound)
{
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);
}

// 自機狙いの銃弾群。発射時に決めた向きへ進むため、途中で追尾はしない。
static void ShotLunaticGunBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const int bulletCount = 13;
        const double spacing = 3.0;
        const double spread = 0.018;
        const double aim = pEnemyShotSet->muki;
        const double sideX = -sin(aim);
        const double sideY = cos(aim);
        const double randomBias = (GetRand(20) - 10) * 0.002;

        for (int i = 0; i < bulletCount; i++) {
            const int centered = i - bulletCount / 2;
            const double offset = centered * spacing;
            const double angle = aim + randomBias + centered * spread;
            const double speed = 5.1 + (GetRand(4) * 0.12);
            const double x = pEnemyShotSet->x + sideX * offset;
            const double y = pEnemyShotSet->y + sideY * offset;

            // 銃弾画像は進行方向に合わせて回転させる。
            AddLunaticGunShot(
                pEnemyShotSet, x, y, angle, speed,
                img_enemyShotBullet[0]);
            // 画像ハンドルは kind に、弾の向きは muki に保持される。
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 低速弾を横一列に配置して、左右に移動する通路を作る。
// param_d[0] は隙間の中心 X、param_i[0] は弾色の切り替え。
static void ShotLunaticGunWall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const double gapCenter = pEnemyShotSet->param_d[0];
        const double gapHalfWidth = 43.0;
        const double startX = 12.0;
        const double endX = 468.0;
        const double interval = 12.0;
        const int wallColor = pEnemyShotSet->param_i[0];
        const int bulletImage = wallColor == 0
            ? img_enemyShotMediumBall[3]  // シアン
            : img_enemyShotMediumBall[4]; // 青
        const double speed = wallColor == 0 ? 1.62 : 1.82;

        for (double x = startX; x <= endX; x += interval) {
            if (fabs(x - gapCenter) < gapHalfWidth) continue;

            AddLunaticGunShot(
                pEnemyShotSet, x, pEnemyShotSet->y,
                DX_PI / 2.0, speed, bulletImage);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体の弾幕パターン
void EnemyPat_LunaticGun_ChatGPT()
{
    static int wallIndex;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 変更しないこと
        wallIndex = 0;
    }
    else {
        // 画面中央付近をゆっくり往復する。
        enemy.x = 240.0 + 88.0 * sin((double)count * 0.012);
        enemy.y = 50.0 + 7.0 * sin((double)count * 0.025);
    }

    // チャージ音を先に鳴らし、少し間を置いて銃弾群を発射する。
    if (count % 90 == 1) {
        PlayLunaticGunSound(sound_enemyCharge);
    }
    if (count % 90 == 25) {
        const double shotX = enemy.x;
        const double shotY = enemy.y + 13.0;
        const double aim = atan2(player.y - shotY, player.x - shotX);

        PlayLunaticGunSound(sound_enemyShot_heavy);
        CreateLunaticGunSet(
            shotX, shotY, aim,
            ShotLunaticGunBurst, 0);
    }

    // 低速弾列を一定間隔で発射。隙間は行ごとに左右へ移動する。
    if (count % 28 == 8) {
        const double gapCenter = 240.0 + 170.0 * sin((double)wallIndex * 0.42);
        const int wallColor = wallIndex % 2;
        CreateLunaticGunSet(
            enemy.x, enemy.y + 22.0, DX_PI / 2.0,
            ShotLunaticGunWall, wallColor);

        // 作成した弾セットに、行ごとの隙間位置と色を記録する。
        sEnemyShotSet* pNewSet = enemyShotSetHead.prev;
        pNewSet->param_d[0] = gapCenter;
        pNewSet->param_i[0] = wallColor;
        wallIndex++;
    }
}
