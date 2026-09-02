// enemyPat_sunflower.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const double PI2 = DX_PI * 2.0;

// 弾幕：向日葵の輪舞
static void ShotSunflower(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 中央の「種」
        for (int i = 0; i < 20; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotLargeBall[1]; // 黄
            pShot->param_i[0] = 0;                  // 種
            pShot->param_i[1] = i;
            pShot->param_d[0] = pEnemyShotSet->x;
            pShot->param_d[1] = pEnemyShotSet->y;
            pShot->param_d[2] = (double)i / 20.0 * PI2;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 花びら。1枚を3発で表現し、12枚を円周上に配置する。
        for (int petal = 0; petal < 12; petal++) {
            for (int j = 0; j < 3; j++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->kind = img_enemyShotMediumOval[8]; // 橙
                pShot->param_i[0] = 1;                    // 花びら
                pShot->param_i[1] = petal;
                pShot->param_i[2] = j;
                pShot->param_d[0] = pEnemyShotSet->x;
                pShot->param_d[1] = pEnemyShotSet->y;
                pShot->param_d[2] = (double)petal / 12.0 * PI2;
                pShot->margin = 40;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 開花 → 一回転 → 崩壊 → 飛散
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double t = (double)pShot->count;
        const double spin = 0.0028 * t;

        if (pShot->param_i[0] == 0) {
            // 花芯：最初は密集し、その後に外へ弾ける。
            double r;
            if (t < 55.0) {
                r = 5.0 + 0.24 * t;
            }
            else if (t < 155.0) {
                r = 18.0 + 4.0 * sin(0.035 * t + pShot->param_d[2]);
            }
            else {
                r = 18.0 + 2.8 * (t - 155.0);
            }

            double a = pShot->param_d[2] + spin;
            pShot->x = pShot->param_d[0] + r * cos(a);
            pShot->y = pShot->param_d[1] + 0.82 * r * sin(a);
        }
        else if (pShot->param_i[0] == 1) {
            // 花びら：3列の弾を少し湾曲させて、12枚の花びらを作る。
            int j = pShot->param_i[2];
            double side = (double)(j - 1);
            double a = pShot->param_d[2] + spin;

            double baseR;
            if (t < 45.0) {
                baseR = 18.0 + 1.25 * t;
            }
            else if (t < 150.0) {
                baseR = 74.0 + 7.0 * sin(0.024 * t + side * 0.8);
            }
            else {
                baseR = 74.0 + 3.3 * (t - 150.0);
            }

            double curve = side * (0.10 + 0.012 * baseR);
            double rr = baseR + side * 19.0;
            double aa = a + curve;

            pShot->x = pShot->param_d[0] + rr * cos(aa);
            pShot->y = pShot->param_d[1] + 0.78 * rr * sin(aa);
            pShot->muki = aa;

            // 崩壊時だけ少し内向きの捻りを加えて、花びらがほどける感じを出す。
            if (t >= 150.0) {
                double twist = 0.0009 * (t - 150.0) * (double)((pShot->param_i[1] % 2) ? 1 : -1);
                double cx = pShot->param_d[0];
                double cy = pShot->param_d[1];
                double dx = pShot->x - cx;
                double dy = pShot->y - cy;
                double ca = cos(twist), sa = sin(twist);
                pShot->x = cx + dx * ca - dy * sa;
                pShot->y = cy + dx * sa + dy * ca;
            }
        }

        pShot = pShot->next;
    }

    // 花がほどける瞬間に、中心から小玉を一斉に散らす。
    if (pEnemyShotSet->count >= 155 && pEnemyShotSet->param_i[0] == 0) {
        pEnemyShotSet->param_i[0] = 1;

        for (int i = 0; i < 48; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotSmallBall[(i % 2) ? 1 : 8]; // 黄 / 橙
            pShot->muki = (double)i / 48.0 * PI2 + 0.04 * sin((double)i);
            pShot->speed = 2.1 + 0.35 * (double)(i % 5);
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->param_i[0] = 2;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 飛散用の小玉だけは通常移動。
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 2) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Sunflower_ChatGPT()
{
    static int dir;
    static int flowerCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        dir = 1;
        flowerCount = 0;
    }
    else {
        enemy.x += 0.75 * (double)dir;
        if (enemy.x < 70.0 || enemy.x > 410.0)
            dir *= -1;
    }

    // 花を重ねることで画面上に大きな向日葵畑を作る。
    if (count % 150 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSunflower;
        pSet->x = enemy.x;
        pSet->y = 125.0 + 38.0 * sin(0.014 * (double)count);
        pSet->kind = flowerCount++;
        pSet->param_i[0] = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}
