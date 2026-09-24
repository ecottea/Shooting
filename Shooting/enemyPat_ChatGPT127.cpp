#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：終焉の立体ラッパ
// 既存素材だけで、細い管から広いベルへ広がるラッパを立体的に表現する。
// count / pEnemyShotSet->count / pEnemyShot->count の更新、および弾の削除はメインルーチン側で行う。

static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ラッパ本体
static void ShotTrumpet(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 1;

        // 管：細い口元からベルへ向かって徐々に太くなる輪郭
        const int sliceCount = 17;
        const int ringCount = 12;

        for (int slice = 0; slice < sliceCount; slice++) {
            for (int ring = 0; ring < ringCount; ring++) {
                sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet);
                pEnemyShot->param_i[0] = 0; // 管本体
                pEnemyShot->param_i[1] = slice;
                pEnemyShot->param_i[2] = ring;
                pEnemyShot->kind = (ring + slice) % 3 == 0
                    ? img_enemyShotMediumBall[8]
                    : img_enemyShotSmallBall[1];
                pEnemyShot->speed = 0.0;
            }
        }

        // 管の中央を走る楕円弾。向きを管の軸に合わせ、厚みを演出する。
        for (int slice = 0; slice < sliceCount - 1; slice++) {
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet);
            pEnemyShot->param_i[0] = 1; // 管の芯
            pEnemyShot->param_i[1] = slice;
            pEnemyShot->kind = img_enemyShotMediumOval[1];
            pEnemyShot->speed = 0.0;
        }

        // ベルの手前側と奥側に輪を作り、開口部の立体感を出す。
        for (int ringLayer = 0; ringLayer < 2; ringLayer++) {
            const int bellCount = 18;
            for (int i = 0; i < bellCount; i++) {
                sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet);
                pEnemyShot->param_i[0] = 2; // ベル外周
                pEnemyShot->param_i[1] = ringLayer;
                pEnemyShot->param_i[2] = i;
                pEnemyShot->kind = ringLayer == 0
                    ? img_enemyShotLargeBall[8]
                    : img_enemyShotMediumBall[1];
                pEnemyShot->speed = 0.0;
            }
        }

        // ベルの奥にある喉元を示す小さな輪。
        const int throatCount = 12;
        for (int i = 0; i < throatCount; i++) {
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet);
            pEnemyShot->param_i[0] = 3; // ベル内部
            pEnemyShot->param_i[1] = i;
            pEnemyShot->kind = img_enemyShotMediumBall[8];
            pEnemyShot->speed = 0.0;
        }
    }

    // ボス直下から画面中央方向へ伸びる、やや傾いたラッパ。
    // ラッパ本体はボスの現在位置に追従する。
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    const double t = (double)pEnemyShotSet->count;
    const double axis = DX_PI * 0.5 + 0.16 * sin(t * 0.020);
    const double pulse = 1.0 + 0.10 * sin(t * 0.075);
    const double lengthPulse = 1.0 + 0.05 * sin(t * 0.043 + 1.0);
    const double ax = cos(axis);
    const double ay = sin(axis);
    const double px = -sin(axis);
    const double py = cos(axis);

    // ラッパの口元。ベルだけ少し上下に呼吸する。
    const double mouthX = pEnemyShotSet->x;
    const double mouthY = pEnemyShotSet->y + 14.0;

    // 管本体の全体長
    const double trumpetLength = 245.0 * lengthPulse;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int type = pShot->param_i[0];

        if (type == 0) {
            const int slice = pShot->param_i[1];
            const int ring = pShot->param_i[2];
            const double u = 18.0 + trumpetLength * (double)slice / 16.0;

            // 管の半径をベル側ほど大きくする。
            const double k = (double)slice / 16.0;
            const double radius = 6.0 + 30.0 * pow(k, 1.45);
            const double phi = DX_PI * 2.0 * (double)ring / 12.0
                + t * 0.034 + slice * 0.055;

            // 奥行き成分を軸方向へ少し投影し、平面上でも立体的に見える輪郭を作る。
            const double depth = radius * 0.28 * sin(phi) * pulse;
            const double lateral = radius * cos(phi) * pulse;
            pShot->x = mouthX + ax * (u + depth) + px * lateral;
            pShot->y = mouthY + ay * (u + depth) + py * lateral;
            pShot->muki = axis + DX_PI * 0.5;
        }
        else if (type == 1) {
            const int slice = pShot->param_i[1];
            const double u = 22.0 + trumpetLength * (double)slice / 16.0;
            pShot->x = mouthX + ax * u;
            pShot->y = mouthY + ay * u;
            pShot->muki = axis;
        }
        else if (type == 2) {
            const int layer = pShot->param_i[1];
            const int i = pShot->param_i[2];
            const double bellU = trumpetLength + 14.0;
            const double bellRadius = 58.0 * pulse;
            const double layerScale = layer == 0 ? 1.0 : 0.70;
            const double phi = DX_PI * 2.0 * (double)i / 18.0
                - t * 0.045 + layer * 0.22;

            const double depth = bellRadius * layerScale * 0.34 * sin(phi);
            const double lateral = bellRadius * layerScale * cos(phi);
            pShot->x = mouthX + ax * (bellU + depth) + px * lateral;
            pShot->y = mouthY + ay * (bellU + depth) + py * lateral;
            pShot->muki = phi;
        }
        else if (type == 3) {
            const int i = pShot->param_i[1];
            const double throatU = trumpetLength - 10.0;
            const double throatRadius = 25.0 * pulse;
            const double phi = DX_PI * 2.0 * (double)i / 12.0
                + t * 0.060;
            const double depth = throatRadius * 0.20 * sin(phi);
            const double lateral = throatRadius * cos(phi);
            pShot->x = mouthX + ax * (throatU + depth) + px * lateral;
            pShot->y = mouthY + ay * (throatU + depth) + py * lateral;
            pShot->muki = phi;
        }

        pShot = pShot->next;
    }
}

// ベルから放たれる音波
static void ShotTrumpetWave(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const double bellAngle = DX_PI * 0.5 + 0.16 * sin((double)count * 0.020);
        const double bx = cos(bellAngle);
        const double by = sin(bellAngle);
        const double trumpetLength = 245.0 * (1.0 + 0.05 * sin((double)count * 0.043 + 1.0));
        const double bellU = trumpetLength + 14.0;

        const double bellX = enemy.x + bx * (bellU + 10.0);
        const double bellY = enemy.y + 14.0 + by * (bellU + 10.0);
        const double dir = atan2(player.y - bellY, player.x - bellX);
        const double wavePX = -sin(dir);
        const double wavePY = cos(dir);
        const double waveDX = cos(dir);
        const double waveDY = sin(dir);

        const int bulletCount = 24;
        for (int i = 0; i < bulletCount; i++) {
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet);
            pEnemyShot->param_i[0] = i;
            pEnemyShot->param_d[0] = bellX;
            pEnemyShot->param_d[1] = bellY;
            pEnemyShot->param_d[2] = waveDX;
            pEnemyShot->param_d[3] = waveDY;
            pEnemyShot->param_d[4] = wavePX;
            pEnemyShot->param_d[5] = wavePY;
            pEnemyShot->param_d[6] = DX_PI * 2.0 * (double)i / (double)bulletCount;
            pEnemyShot->kind = (i % 2 == 0)
                ? img_enemyShotSmallBall[6]
                : img_enemyShotMediumBall[6];
            pEnemyShot->speed = 0.0;
        }
    }

    const double t = (double)pEnemyShotSet->count;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double originX = pShot->param_d[0];
        const double originY = pShot->param_d[1];
        const double dirX = pShot->param_d[2];
        const double dirY = pShot->param_d[3];
        const double perpX = pShot->param_d[4];
        const double perpY = pShot->param_d[5];
        const double phase = pShot->param_d[6] + t * 0.075;

        // 前進しながら輪が膨らみ、少しねじれることで音波を立体的に見せる。
        const double forward = 2.35 * t;
        const double radius = 8.0 + 0.72 * t;
        const double depth = radius * 0.22 * sin(phase);
        const double lateral = radius * cos(phase);

        pShot->x = originX + dirX * (forward + depth) + perpX * lateral;
        pShot->y = originY + dirY * (forward + depth) + perpY * lateral;
        pShot->muki = phase;

        pShot = pShot->next;
    }
}

static void CreateTrumpetWave()
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotTrumpetWave;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 14.0;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = 0;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_GabrielsHorn_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 34.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ラッパ全体が画面内をゆっくり左右に揺れる。
        enemy.x += 0.90 * (double)muki;
        if (enemy.x < 110.0 || enemy.x > 370.0) muki *= -1;
    }

    // ラッパ本体は常にボスに追従する。
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTrumpet;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = DX_PI * 0.5;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 一定間隔でベルから大きな音波を放つ。
    if (count > 30 && count % 38 == 1) {
        CreateTrumpetWave();
    }
}
