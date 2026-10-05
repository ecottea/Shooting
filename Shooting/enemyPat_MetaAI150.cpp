#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// サボテン弾幕「サボテン・タワー」
static void ShotCactusTower(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->param_i 割り当て
    // [1] 主幹の生成数
    // [2] 左腕の生成数
    // [3] 右腕の生成数
    // [4] 開花済みフラグ
    // [5] 枯れ落ちフラグ
    // pEnemyShotSet->param_d
    // [0] 左腕先端x [1] 左腕先端y
    // [2] 右腕先端x [3] 右腕先端y
    // [4] 主幹先端x [5] 主幹先端y

    // sEnemyShot->param_i[0] で弾種別
    // 0:茎(静止) 1:花 2:トゲ 3:花粉

    int cnt = pEnemyShotSet->count;

    if (cnt == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16; i++) {
            pEnemyShotSet->param_i[i] = 0;
            pEnemyShotSet->param_d[i] = 0.0;
        }
        // 初期先端位置 = 根元
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y + 7 * 12.0;
        pEnemyShotSet->param_d[2] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[3] = pEnemyShotSet->y + 11 * 12.0;
        pEnemyShotSet->param_d[4] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[5] = pEnemyShotSet->y;
    }

    //-------------------------------------------------
    // 1. 生成フェーズ
    //-------------------------------------------------
    // Phase 0: 主幹成長 0-71F
    if (cnt < 72) {
        if (cnt % 4 == 0 && pEnemyShotSet->param_i[1] < 18) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y + pEnemyShotSet->param_i[1] * 12.0;
            p->muki = 0.0;
            p->speed = 0.0;
            p->count = 0;
            p->kind = img_enemyShotMediumOval[2]; // 茎: 緑 中楕円弾 10.5x7.0
            for (int k = 0; k < 16; k++) { p->param_i[k] = 0; p->param_d[k] = 0.0; }
            p->param_i[0] = 0; // 茎

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;

            pEnemyShotSet->param_d[4] = p->x;
            pEnemyShotSet->param_d[5] = p->y;
            pEnemyShotSet->param_i[1]++;

            // 茎から左右にトゲを生やす
            for (int s = 0; s < 2; s++) {
                sEnemyShot* th = new sEnemyShot;
                th->x = p->x;
                th->y = p->y;
                th->muki = (s == 0 ? 0.0 : DX_PI) + (GetRand(20) - 10) / 180.0 * DX_PI;
                th->speed = 2.2 + GetRand(80) / 100.0;
                th->count = 0;
                th->kind = img_enemyShotBullet[1]; // トゲ: 黄 銃弾 5.0x2.0 針っぽい
                for (int k = 0; k < 16; k++) { th->param_i[k] = 0; th->param_d[k] = 0.0; }
                th->param_i[0] = 2;
                th->prev = pEnemyShotSet->pEnemyShotHead->prev;
                th->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = th;
                pEnemyShotSet->pEnemyShotHead->prev = th;
            }
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }
    // Phase 1: 腕分岐成長 72-179F
    else if (cnt < 180) {
        // 左腕 -135度
        if (cnt % 5 == 0 && pEnemyShotSet->param_i[2] < 10) {
            double ang = -3.0 * DX_PI / 4.0;
            double bx = pEnemyShotSet->param_d[0];
            double by = pEnemyShotSet->param_d[1];
            double nx = bx + 10.0 * cos(ang);
            double ny = by + 10.0 * sin(ang);

            sEnemyShot* p = new sEnemyShot;
            p->x = nx; p->y = ny; p->muki = 0; p->speed = 0; p->count = 0;
            p->kind = img_enemyShotMediumBall[2]; // 腕は少し細く見せるため中玉 緑
            for (int k = 0; k < 16; k++) { p->param_i[k] = 0; p->param_d[k] = 0.0; }
            p->param_i[0] = 0;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;

            pEnemyShotSet->param_d[0] = nx;
            pEnemyShotSet->param_d[1] = ny;
            pEnemyShotSet->param_i[2]++;

            // 腕の側面からトゲ
            for (int s = -1; s <= 1; s += 2) {
                sEnemyShot* th = new sEnemyShot;
                th->x = nx; th->y = ny;
                th->muki = ang + s * DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
                th->speed = 2.0 + GetRand(60) / 100.0;
                th->count = 0;
                th->kind = img_enemyShotScale[1]; // 黄 鱗弾で少し違うトゲ
                for (int k = 0; k < 16; k++) { th->param_i[k] = 0; th->param_d[k] = 0.0; }
                th->param_i[0] = 2;
                th->prev = pEnemyShotSet->pEnemyShotHead->prev;
                th->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = th;
                pEnemyShotSet->pEnemyShotHead->prev = th;
            }
        }
        // 右腕 -45度
        if (cnt % 5 == 2 && pEnemyShotSet->param_i[3] < 10) {
            double ang = -1.0 * DX_PI / 4.0;
            double bx = pEnemyShotSet->param_d[2];
            double by = pEnemyShotSet->param_d[3];
            double nx = bx + 10.0 * cos(ang);
            double ny = by + 10.0 * sin(ang);

            sEnemyShot* p = new sEnemyShot;
            p->x = nx; p->y = ny; p->muki = 0; p->speed = 0; p->count = 0;
            p->kind = img_enemyShotMediumBall[2];
            for (int k = 0; k < 16; k++) { p->param_i[k] = 0; p->param_d[k] = 0.0; }
            p->param_i[0] = 0;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;

            pEnemyShotSet->param_d[2] = nx;
            pEnemyShotSet->param_d[3] = ny;
            pEnemyShotSet->param_i[3]++;

            for (int s = -1; s <= 1; s += 2) {
                sEnemyShot* th = new sEnemyShot;
                th->x = nx; th->y = ny;
                th->muki = ang + s * DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
                th->speed = 2.0 + GetRand(60) / 100.0;
                th->count = 0;
                th->kind = img_enemyShotBullet[1];
                for (int k = 0; k < 16; k++) { th->param_i[k] = 0; th->param_d[k] = 0.0; }
                th->param_i[0] = 2;
                th->prev = pEnemyShotSet->pEnemyShotHead->prev;
                th->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = th;
                pEnemyShotSet->pEnemyShotHead->prev = th;
            }
        }
        // 全身からトゲ一斉放射
        if (cnt % 70 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            for (int i = 0; i < 20; i++) {
                sEnemyShot* th = new sEnemyShot;
                th->x = pEnemyShotSet->x;
                th->y = pEnemyShotSet->y + 80.0;
                th->muki = 2.0 * DX_PI * i / 20.0;
                th->speed = 2.5 + GetRand(40) / 100.0;
                th->count = 0;
                th->kind = img_enemyShotDiamond[1]; // 黄 菱形弾
                for (int k = 0; k < 16; k++) { th->param_i[k] = 0; th->param_d[k] = 0.0; }
                th->param_i[0] = 2;
                th->prev = pEnemyShotSet->pEnemyShotHead->prev;
                th->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = th;
                pEnemyShotSet->pEnemyShotHead->prev = th;
            }
        }
    }
    // Phase 2: 開花 180-359F
    else if (cnt < 360) {
        if (pEnemyShotSet->param_i[4] == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            double fx[3] = { pEnemyShotSet->param_d[4], pEnemyShotSet->param_d[0], pEnemyShotSet->param_d[2] };
            double fy[3] = { pEnemyShotSet->param_d[5], pEnemyShotSet->param_d[1], pEnemyShotSet->param_d[3] };
            for (int j = 0; j < 3; j++) {
                sEnemyShot* fl = new sEnemyShot;
                fl->x = fx[j]; fl->y = fy[j];
                fl->muki = atan2(player.y - fy[j], player.x - fx[j]);
                fl->speed = 0.4; // ゆっくり自機へ
                fl->count = 0;
                fl->kind = img_enemyShotLargeBall[5]; // 花: マゼンタ 大玉 20x20
                for (int k = 0; k < 16; k++) { fl->param_i[k] = 0; fl->param_d[k] = 0.0; }
                fl->param_i[0] = 1;
                fl->prev = pEnemyShotSet->pEnemyShotHead->prev;
                fl->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = fl;
                pEnemyShotSet->pEnemyShotHead->prev = fl;
            }
            pEnemyShotSet->param_i[4] = 1;
        }
    }
    // Phase 3: 枯れ落ち
    else {
        if (pEnemyShotSet->param_i[5] == 0) {
            pEnemyShotSet->param_i[5] = 1;
        }
    }

    //-------------------------------------------------
    // 2. 移動・花粉生成フェーズ（メインルーチンではcountの加算のみ行われる）
    //-------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 生成でリストが伸びても安全なように

        if (pShot->param_i[0] == 0) { // 茎
            if (pEnemyShotSet->param_i[5] == 1) {
                if (pShot->speed < 0.1) {
                    pShot->muki = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
                    pShot->speed = 1.2 + GetRand(120) / 100.0;
                }
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                pShot->speed += 0.06; // 重力落下っぽく加速
            }
        }
        else if (pShot->param_i[0] == 1) { // 花
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            if (pEnemyShotSet->count < 360 && pShot->count % 14 == 0) {
                // 花粉をばら撒く
                for (int k = 0; k < 3; k++) {
                    sEnemyShot* po = new sEnemyShot;
                    po->x = pShot->x;
                    po->y = pShot->y;
                    // GetRandは0〜xを返すので注意: 0〜100で101通り
                    po->muki = 2.0 * DX_PI * GetRand(100) / 101.0;
                    po->speed = 0.8 + GetRand(80) / 100.0;
                    po->count = 0;
                    po->kind = img_enemyShotSmallBall[5]; // 花粉: マゼンタ 小玉
                    for (int kk = 0; kk < 16; kk++) { po->param_i[kk] = 0; po->param_d[kk] = 0.0; }
                    po->param_i[0] = 3;
                    po->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    po->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = po;
                    pEnemyShotSet->pEnemyShotHead->prev = po;
                }
            }
        }
        else { // トゲ2 と花粉3 は通常移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

void EnemyPat_Cactus_MetaAI()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆら横移動でサボテンっぽい揺れ
        enemy.x += 0.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 400Fごとに新しいサボテンを生やす。前のが枯れ落ちるタイミングと被る
    if (count % 400 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCactusTower;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;
        for (int i = 0; i < 16; i++) {
            pEnemyShotSet->param_i[i] = 0;
            pEnemyShotSet->param_d[i] = 0.0;
        }
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}