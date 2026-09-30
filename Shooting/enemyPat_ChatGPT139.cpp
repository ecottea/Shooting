#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：台風「暴風圏」
//
// 外側へ広がる螺旋流と、外周から中心近くまで吸い込んでから
// 再び外へ吹き飛ぶ逆流を重ね、中央に「台風の目」を残す構成。
// 楕円弾は実際の移動ベクトルから muki を設定することで、
// 渦の流れに沿った向きに見えるようにする。
static void ShotTyphoon(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const int wave = pEnemyShotSet->param_i[0];
        const int rotationSign = (wave % 2 == 0) ? 1 : -1;

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 外側へ広がる主流。中玉で太い雨雲の帯を表現する。
        const int outwardCount = 18;
        for (int i = 0; i < outwardCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            const double angle = (DX_PI * 2.0 * i / outwardCount)
                + (wave % 2) * (DX_PI / outwardCount)
                + (GetRand(20) - 10) * 0.003;

            pEnemyShot->param_i[0] = 0; // 0: 外向き螺旋
            pEnemyShot->param_i[1] = i;
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 36.0 + GetRand(12);
            pEnemyShot->param_d[2] = 1.25 + GetRand(20) / 100.0;
            pEnemyShot->param_d[3] = rotationSign * (0.020 + GetRand(8) / 1000.0);
            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * pEnemyShot->param_d[1];
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * pEnemyShot->param_d[1];
            pEnemyShot->muki = angle + (rotationSign > 0 ? DX_PI * 0.5 : -DX_PI * 0.5);
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[(i + wave) % 2 == 0 ? 3 : 4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 外周から中心へ向かう逆流。小玉で細い風の筋を作る。
        const int inwardCount = 10;
        for (int i = 0; i < inwardCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            const double angle = (DX_PI * 2.0 * i / inwardCount)
                + DX_PI / inwardCount
                + (wave % 2) * 0.15
                + (GetRand(20) - 10) * 0.003;

            pEnemyShot->param_i[0] = 1; // 1: 内向き→反転して外向き
            pEnemyShot->param_i[1] = i;
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 300.0 + GetRand(40);
            pEnemyShot->param_d[2] = 1.85 + GetRand(15) / 100.0;
            pEnemyShot->param_d[3] = -rotationSign * (0.014 + GetRand(6) / 1000.0);
            pEnemyShot->param_d[4] = (pEnemyShot->param_d[1] - 76.0) / pEnemyShot->param_d[2];
            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * pEnemyShot->param_d[1];
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * pEnemyShot->param_d[1];
            pEnemyShot->muki = angle + (rotationSign > 0 ? DX_PI * 0.5 : -DX_PI * 0.5);
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotSmallBall[(i + wave) % 2 == 0 ? 3 : 4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 渦の輪郭を強調する楕円弾。少数だけ混ぜて流線を視認しやすくする。
        const int ovalCount = 6;
        for (int i = 0; i < ovalCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            const double angle = (DX_PI * 2.0 * i / ovalCount)
                + DX_PI / 6.0;

            pEnemyShot->param_i[0] = 2; // 2: 外周の強風帯
            pEnemyShot->param_i[1] = i;
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 112.0 + GetRand(24);
            pEnemyShot->param_d[2] = 1.60 + GetRand(15) / 100.0;
            pEnemyShot->param_d[3] = rotationSign * (0.030 + GetRand(8) / 1000.0);
            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * pEnemyShot->param_d[1];
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * pEnemyShot->param_d[1];
            pEnemyShot->muki = angle + (rotationSign > 0 ? DX_PI * 0.5 : -DX_PI * 0.5);
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumOval[(i + wave) % 2 == 0 ? 3 : 4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    const int rotationSign = (pEnemyShotSet->param_i[0] % 2 == 0) ? 1 : -1;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double t = (double)pShot->count;
        const double baseAngle = pShot->param_d[0];
        const double angularSpeed = pShot->param_d[3];
        const double centerX = pEnemyShotSet->x;
        const double centerY = pEnemyShotSet->y;

        double radius = pShot->param_d[1];
        double radiusSpeed = pShot->param_d[2];

        if (pShot->param_i[0] == 0) {
            // 外向き螺旋。中心から離れるほど流れが長くなる。
            radius += radiusSpeed * t;
        }
        else if (pShot->param_i[0] == 1) {
            // 外から目へ接近し、目の手前で反転して外へ吹き飛ぶ。
            const double turnTime = pShot->param_d[4];
            if (t < turnTime) {
                radius -= radiusSpeed * t;
            }
            else {
                radius = 76.0 + 1.45 * (t - turnTime);
            }
        }
        else {
            // 強風帯。やや速い角速度で外周をなぞる。
            radius += radiusSpeed * t;
        }

        // わずかな脈動を加えて、機械的な同心円にならないようにする。
        if (pShot->param_i[0] != 1) {
            radius += 5.0 * sin(t * 0.055 + pShot->param_d[0] * 2.0);
        }

        const double angle = baseAngle + angularSpeed * t;
        const double x = centerX + cos(angle) * radius;
        const double y = centerY + sin(angle) * radius;

        // 数フレーム先の位置から実際の流れ方向を求め、楕円弾の向きを合わせる。
        const double nextT = t + 1.0;
        double nextRadius = radius;
        if (pShot->param_i[0] == 0 || pShot->param_i[0] == 2) {
            nextRadius = pShot->param_d[1] + radiusSpeed * nextT
                + 5.0 * sin(nextT * 0.055 + pShot->param_d[0] * 2.0);
        }
        else {
            const double turnTime = pShot->param_d[4];
            if (nextT < turnTime) {
                nextRadius = pShot->param_d[1] - radiusSpeed * nextT;
            }
            else {
                nextRadius = 76.0 + 1.45 * (nextT - turnTime);
            }
        }
        const double nextAngle = baseAngle + angularSpeed * nextT;
        const double nextX = centerX + cos(nextAngle) * nextRadius;
        const double nextY = centerY + sin(nextAngle) * nextRadius;

        pShot->x = x;
        pShot->y = y;
        pShot->muki = atan2(nextY - y, nextX - x);

        pShot->speed = 0.0;
        (void)rotationSign;
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Typhoon_ChatGPT()
{
    static int muki;
    static int shot_count;
    static double stormCenterX;
    static double stormCenterY;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
        stormCenterX = 240.0;
        stormCenterY = 185.0;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        enemy.x += 0.90 * (double)muki;
        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            muki = 1;
        }
        else if (enemy.x > 400.0) {
            enemy.x = 400.0;
            muki = -1;
        }

        // 台風の目を少しずつプレイヤー側へ寄せる。
        const double targetX = player.x;
        const double targetY = 150.0 + player.y * 0.18;
        stormCenterX += (targetX - stormCenterX) * 0.012;
        stormCenterY += (targetY - stormCenterY) * 0.004;

        if (stormCenterX < 90.0) stormCenterX = 90.0;
        if (stormCenterX > 390.0) stormCenterX = 390.0;
        if (stormCenterY < 145.0) stormCenterY = 145.0;
        if (stormCenterY > 260.0) stormCenterY = 260.0;
    }

    // 定期的に回転方向を切り替え、台風らしい巻き込みと逆巻きを作る。
    const int cycle = count / 420;
    const int phase = count % 420;

    if (phase == 60) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
    if (phase == 1 && count > 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 予告後に連続した渦の帯を生成する。
    if (phase >= 60 && count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTyphoon;
        pEnemyShotSet->x = stormCenterX;
        pEnemyShotSet->y = stormCenterY;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->param_i[0] = cycle;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
