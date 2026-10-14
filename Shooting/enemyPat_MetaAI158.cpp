#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 二重スリット板：大玉で壁を作る
static void ShotWall(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const double baseY = pSet->y; // 130
        const double spacing = 28.0; // 大玉直径40を少し重ねる
        const double leftL = 162.0, leftR = 198.0; // 左スリット 36px幅
        const double rightL = 282.0, rightR = 318.0; // 右スリット 36px幅

        for (double x = 14.0; x < 480.0; x += spacing) {
            if ((x > leftL && x < leftR) || (x > rightL && x < rightR)) continue; // スリットは空ける
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = x + (GetRand(4) - 2); // GetRandは0..xなので注意
            pShot->y = baseY + (GetRand(6) - 3);
            pShot->muki = DX_PI / 2.0; // 真下へ
            pShot->speed = 0.35;
            pShot->kind = img_enemyShotLargeBall[4]; // 青・大玉(20x20) 円形で向き依存なし
            pShot->margin = 60.0;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// 二重スリットからの波 / 観測による収縮
static void ShotDoubleSlit(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // param_d: 0:左スリットX, 1:右スリットX, 2:基準Y, 3:波長λ, 4:スリット間隔d, 5:壁速度
        pSet->param_d[0] = 180.0;
        pSet->param_d[1] = 300.0;
        pSet->param_d[2] = 130.0;
        pSet->param_d[3] = 32.0; // λ 干渉縞のピッチを決める
        pSet->param_d[4] = 120.0; // d
        pSet->param_d[5] = 0.35;
    }

    double wallY = pSet->param_d[2] + pSet->param_d[5] * (double)pSet->count;
    bool observed = (player.y < wallY + 180.0); // 上半分に入ると観測扱い
    pSet->param_i[0] = observed ? 1 : 0;

    if (!observed) {
        // 波モード：半円状の波を2スリットから出す
        if (pSet->count % 12 == 0 && pSet->count <= 420) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            double slitX[2] = { pSet->param_d[0], pSet->param_d[1] };
            for (int s = 0; s < 2; s++) {
                double sx = slitX[s];
                double sy = wallY;
                for (int deg = 10; deg <= 170; deg += 9) {
                    double theta = deg * DX_PI / 180.0; // 0=右, 90=下
                    double phi = theta - DX_PI / 2.0; // 真下からのズレ
                    double delta = pSet->param_d[4] * sin(phi); // 経路差 d*sinφ
                    double interf = cos(2.0 * DX_PI * delta / pSet->param_d[3]); // -1~1

                    if (interf < -0.25) continue; // 暗縞は生成しない

                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = sx;
                    pShot->y = sy;
                    pShot->muki = theta + (GetRand(20) - 10) * DX_PI / 180.0 * 0.15;
                    pShot->speed = 1.3 + (interf + 1.0) * 0.45 + GetRand(15) / 100.0;
                    // 明るい所は白、暗い所はシアンで縞を強調
                    if (interf > 0.6) pShot->kind = img_enemyShotSmallBall[6]; // 白 強め
                    else if (interf > 0.2) pShot->kind = img_enemyShotSmallBall[1]; // 黄 中間
                    else pShot->kind = img_enemyShotSmallBall[3]; // シアン 弱め

                    pShot->prev = pSet->pEnemyShotHead->prev;
                    pShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pShot;
                    pSet->pEnemyShotHead->prev = pShot;
                }
            }
        }
    }
    else {
        // 粒子モード：観測されたので直進弾に収縮
        if (pSet->count % 12 == 0 && pSet->count <= 420) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            double slitX[2] = { pSet->param_d[0], pSet->param_d[1] };
            for (int s = 0; s < 2; s++) {
                double sx = slitX[s];
                double sy = wallY;
                double base = atan2(player.y - sy, player.x - sx);
                for (int k = -1; k <= 1; k++) {
                    if (GetRand(3) == 0) continue; // 少し間引いて2way気味に
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = sx;
                    pShot->y = sy;
                    pShot->muki = base + k * 0.12/3;
                    pShot->speed = 2.8 + GetRand(30) / 100.0+0.2;
                    pShot->kind = img_enemyShotSmallBall[0]; // 赤 粒子
                    pShot->prev = pSet->pEnemyShotHead->prev;
                    pShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pShot;
                    pSet->pEnemyShotHead->prev = pShot;
                }
            }
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

void EnemyPat_DoubleSlit_MetaAI()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.35 * (double)muki;
        if (count % 300 == 150) muki *= -1;
    }

    // 360Fごとに壁と波源を再生成して同期させる
    if (count % 360 == 1) {
        sEnemyShotSet* pWall = new sEnemyShotSet;
        pWall->count = 0;
        pWall->patternFunc = ShotWall;
        pWall->x = 240.0;
        pWall->y = 130.0;
        pWall->muki = DX_PI / 2.0;
        pWall->kind = 0;
        pWall->pEnemyShotHead = new sEnemyShot;
        pWall->pEnemyShotHead->prev = pWall->pEnemyShotHead;
        pWall->pEnemyShotHead->next = pWall->pEnemyShotHead;
        pWall->prev = enemyShotSetHead.prev;
        pWall->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWall;
        enemyShotSetHead.prev = pWall;

        sEnemyShotSet* pWave = new sEnemyShotSet;
        pWave->count = 0;
        pWave->patternFunc = ShotDoubleSlit;
        pWave->x = 240.0;
        pWave->y = 130.0;
        pWave->muki = DX_PI / 2.0;
        pWave->kind = 0;
        pWave->pEnemyShotHead = new sEnemyShot;
        pWave->pEnemyShotHead->prev = pWave->pEnemyShotHead;
        pWave->pEnemyShotHead->next = pWave->pEnemyShotHead;
        pWave->prev = enemyShotSetHead.prev;
        pWave->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWave;
        enemyShotSetHead.prev = pWave;
    }
}