#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// A: 狭い間隔と広い間隔が交互に並ぶ、回転する10way全方位弾。
static void ShotLunaticGun_RotatingFan(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 18度、54度を交互に加算すると、狭い隙間と広い隙間が交互に並ぶ。
        double angle = pEnemyShotSet->muki;
        const double narrowGap = DX_PI / 10.0;
        const double wideGap = DX_PI * 3.0 / 10.0;

        for (int i = 0; i < 10; ++i) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青い中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            angle += (i % 2 == 0) ? narrowGap : wideGap;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// B: 1辺に8発並ぶ逆三角形の銃弾塊。各塊は発射時に決めた方向へ進む。
static void ShotLunaticGun_Triangle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const int sideCount = 8;
        const double spacing = 4.5*2;
        const double angle = pEnemyShotSet->muki;
        const double dirX = cos(angle);
        const double dirY = sin(angle);
        const double sideX = -dirY;
        const double sideY = dirX;

        // ボス側を広い辺、進行方向側を頂点にした三角形（8+7+...+1発）。
        for (int row = 0; row < sideCount; ++row) {
            const int rowCount = sideCount - row;
            const double forward = row * spacing;
            for (int col = 0; col < rowCount; ++col) {
                const double lateral = (col - (rowCount - 1) / 2.0) * spacing;
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + dirX * forward + sideX * lateral;
                pEnemyShot->y = pEnemyShotSet->y + dirY * forward + sideY * lateral;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 4.2;
                pEnemyShot->kind = img_enemyShotBullet[0]; // 赤い銃弾

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾セットをリスト末尾へ追加する。弾数カウンタ類はメインルーチン側で管理する。
static sEnemyShotSet* CreateLunaticGunShotSet(
    sEnemyShotSet::PatternFunc patternFunc,
    double x,
    double y,
    double angle,
    int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = angle;
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

// 敵本体の弾幕：銃符「ルナティックガン」
void EnemyPat_LunaticGun_ChatGPT()
{
    static double fanAngle;
    static int moveDirection;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200; // 変更しないこと
        fanAngle = 0.0;
        moveDirection = 1;

    }
    else {
        // ゆっくり左右へ移動し、狙い弾の方向に変化を付ける。
        enemy.x += 0.55 * moveDirection;
        if (enemy.x >= 360.0) {
            enemy.x = 360.0;
            moveDirection = -1;
        }
        else if (enemy.x <= 120.0) {
            enemy.x = 120.0;
            moveDirection = 1;
        }
    }

    // Aは10フレームごとに撃ち続け、発射ごとに少しずつ時計回りへ回転。
    if (count % 10 == 1) {
        CreateLunaticGunShotSet(
            ShotLunaticGun_RotatingFan,
            enemy.x,
            enemy.y,
            fanAngle,
            0);
        fanAngle += 0.085;
        if (fanAngle >= DX_PI * 2.0) fanAngle -= DX_PI * 2.0;

        // 効果音の連打を避け、数回の射撃ごとに軽い発射音を鳴らす。
        if (count % 40 == 1) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // Bは三角形弾列を5発ずつ2組、組同士はほぼ間を空けずに発射。
    // 2組を撃ち終えた後は間隔を空け、同じリズムを繰り返す。
    const int cycle = (count - 1) % 210;
    const int launchFrames[10] = { 0, 8, 16, 24, 32, 36, 44, 52, 60, 68 };
    for (int i = 0; i < 10; ++i) {
        if (cycle == launchFrames[i]) {
            const double baseAngle = atan2(player.y - enemy.y, player.x - enemy.x);
            // 各三角形にわずかな角度差を付け、狙いを少し散らす。
            const double aimAngle = baseAngle + (GetRand(100) - 50) * 0.0009*10;
            CreateLunaticGunShotSet(
                ShotLunaticGun_Triangle,
                enemy.x,
                enemy.y,
                aimAngle,
                i);

            if (i == 0 || i == 5) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }
            break;
        }
    }
}
