// enemyPat_meteor.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 隕石本体。
// 大玉を中心に小玉・鱗弾をまとわせて岩塊に見せ、
// 飛来中は全体が移動し、寿命が来ると破片を四方へ散らす。
static void ShotMeteor(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    // 初回だけ隕石本体と外殻を生成。
    if (pEnemyShotSet->count == 0) {
        // 1. 大玉の核
        pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki;
        pShot->speed = 2.15 * 2;
        pShot->kind = img_enemyShotLargeBall[7]; // 黒
        pShot->param_i[0] = 0;                   // 核
        pShot->param_i[1] = 0;                   // 分裂済みフラグ
        pShot->param_d[0] = 0.0;                 // 初期角度
        pShot->margin = 120;
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;

        // 2. 外殻を構成する小玉・鱗弾
        const int shellCount = 10;
        for (int i = 0; i < shellCount; i++) {
            double a = 2.0 * DX_PI * i / shellCount;
            double r = 12.0 + (GetRand(10) - 5);

            pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + r * cos(a);
            pShot->y = pEnemyShotSet->y + r * sin(a);
            pShot->muki = pEnemyShotSet->muki;
            pShot->speed = 2.15 * 2;
            pShot->kind = (i % 2 == 0) ? img_enemyShotSmallBall[7]
                : img_enemyShotScale[1]; // 黒小玉＋黄鱗弾
            pShot->param_i[0] = 1;               // 外殻
            pShot->param_i[1] = 0;
            pShot->param_d[0] = a;
            pShot->margin = 120;
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 3. 予告音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 本体を進め、外殻は本体の周囲を少し揺らしながら追従。
    sEnemyShot* core = nullptr;
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            core = pShot;
            break;
        }
        pShot = pShot->next;
    }

    if (core != nullptr) {
        core->x += core->speed * cos(core->muki);
        core->y += core->speed * sin(core->muki);

        int shellIndex = 0;
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* next = pShot->next;

            if (pShot->param_i[0] == 1) {
                double a = pShot->param_d[0]
                    + 0.11 * sin((double)pEnemyShotSet->count * 0.07 + shellIndex * 0.6);
                double r = 12.0 + 2.0 * sin((double)pEnemyShotSet->count * 0.05 + shellIndex);

                pShot->x = core->x + r * cos(a);
                pShot->y = core->y + r * sin(a);
                pShot->muki = core->muki;
                pShot->speed = core->speed;
                shellIndex++;
            }

            pShot = next;
        }
    }

    // ある程度進んだところで隕石が崩壊し、破片を大量に放つ。
    if (pEnemyShotSet->count == 78) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        if (core != nullptr && core->param_i[1] == 0) {
            core->param_i[1] = 1;

            const int fragmentCount = 28 + 10;
            for (int i = 0; i < fragmentCount; i++) {
                double a = 2.0 * DX_PI * i / fragmentCount
                    + (GetRand(100) / 100.0) * 0.18;
                double speed = 1.4 + GetRand(160) / 100.0;

                sEnemyShot* fragment = new sEnemyShot;
                fragment->x = core->x;
                fragment->y = core->y;
                fragment->muki = a;
                fragment->speed = speed;
                fragment->kind = (i % 3 == 0) ? img_enemyShotMediumBall[7]
                    : img_enemyShotSmallBall[1];
                fragment->param_i[0] = 2; // 破片
                fragment->param_i[1] = 0;
                fragment->prev = pEnemyShotSet->pEnemyShotHead->prev;
                fragment->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = fragment;
                pEnemyShotSet->pEnemyShotHead->prev = fragment;
            }

            // 核と外殻をまとめて消し、破片だけを残す。
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                sEnemyShot* next = pShot->next;

                if (pShot->param_i[0] != 2) {
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                }

                pShot = next;
            }
        }
    }

    // 破片は加速しながら飛散。少しずつ外側へ開いていく。
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 2) {
            pShot->muki += 0.0018 * ((pShot->x < pEnemyShotSet->x) ? -1.0 : 1.0);
            pShot->speed += 0.008;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Meteor_ChatGPT()
{
    static int muki;
    static int meteor_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        meteor_count = 0;
    }
    else {
        enemy.x += 0.75 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 隕石を一定間隔で落とす。
    // 序盤は左右から斜めに、後半ほど正面から密度を上げる。
    if (count > 20 && count % 55 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMeteor;

        double spawnX;
        double targetX;

        if (meteor_count % 3 == 0) {
            spawnX = 20.0;
            targetX = 170.0 + GetRand(140);
        }
        else if (meteor_count % 3 == 1) {
            spawnX = 460.0;
            targetX = 170.0 + GetRand(140);
        }
        else {
            spawnX = 80.0 + GetRand(320);
            targetX = spawnX + (GetRand(160) - 80);
        }

        pEnemyShotSet->x = spawnX;
        pEnemyShotSet->y = -20.0;
        pEnemyShotSet->muki =
            atan2(300.0, targetX - spawnX);

        pEnemyShotSet->kind = meteor_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 後半は追加の巨大隕石を中央付近から落とす。
    if (count >= 520 && count % 85 == 20) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMeteor;
        pEnemyShotSet->x = 120.0 + GetRand(240);
        pEnemyShotSet->y = -20.0;
        pEnemyShotSet->muki =
            atan2(340.0, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = meteor_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
