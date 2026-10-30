#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 残留インク弾（塗られた領域を表現する遅い弾）
static void ShotInkResidual(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 非常に遅くドリフトさせ、塗られた床のように見せる
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        // 寿命管理（一定時間後に加速して画面外へ）
        if (pShot->count > 180) {
            pShot->speed += 0.02;
        }
        pShot = pShot->next;
    }
}

// メインインク弾（着弾後に残留を残す）
static void ShotInkMain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num = 7 + GetRand(3);
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double baseMuki = pEnemyShotSet->muki;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseMuki + (GetRand(80) - 40) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.2 + GetRand(80) / 100.0;
            // シアン系の中玉をメインインクに使用
            pEnemyShot->kind = img_enemyShotMediumBall[3];
            pEnemyShot->param_i[0] = 1; // 残留発生フラグ
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

        // ある程度進んだら残留インクを残す
        if (pShot->param_i[0] == 1 && pShot->count > 25 && pShot->count % 8 == 0) {
            sEnemyShot* pRes = new sEnemyShot;
            pRes->x = pShot->x + GetRand(12) - 6;
            pRes->y = pShot->y + GetRand(12) - 6;
            pRes->muki = GetRand(360) / 180.0 * DX_PI;
            pRes->speed = 0.15 + GetRand(20) / 100.0;
            pRes->kind = img_enemyShotSmallBall[3]; // シアン小玉を残留インクに
            pRes->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRes->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRes;
            pEnemyShotSet->pEnemyShotHead->prev = pRes;
        }
        if (pShot->param_i[0] == 0 && pShot->count > 300) pShot->margin = -999;
        pShot = pShot->next;
    }
}

// 波状拡散インク
static void ShotInkWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(60) - 30);
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 5.5) * 0.18 + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.8 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotScale[3]; // 鱗弾を波状インクに
            pEnemyShot->param_i[0] = 1;
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

        if (pShot->param_i[0] == 1 && pShot->count > 20 && pShot->count % 6 == 0) {
            sEnemyShot* pRes = new sEnemyShot;
            pRes->x = pShot->x;
            pRes->y = pShot->y;
            pRes->muki = GetRand(360) / 180.0 * DX_PI;
            pRes->speed = 0.1 + GetRand(15) / 100.0;
            pRes->kind = img_enemyShotSmallBall[3];
            pRes->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRes->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRes;
            pEnemyShotSet->pEnemyShotHead->prev = pRes;
        }
        if (pShot->param_i[0] == 0 && pShot->count > 240) pShot->margin = -999;
        pShot = pShot->next;
    }
}

// 高密度軌跡塗り
static void ShotInkTrail(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.5 + GetRand(50) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumOval[3];
            pEnemyShot->param_i[0] = 1;
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

        // 軌跡に高密度で残留を置く
        if (pShot->param_i[0] == 1 && pShot->count % 4 == 0) {
            sEnemyShot* pRes = new sEnemyShot;
            pRes->x = pShot->x + GetRand(8) - 4;
            pRes->y = pShot->y + GetRand(8) - 4;
            pRes->muki = GetRand(360) / 180.0 * DX_PI;
            pRes->speed = 0.08 + GetRand(10) / 100.0;
            pRes->kind = img_enemyShotSmallBall[3];
            pRes->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRes->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRes;
            pEnemyShotSet->pEnemyShotHead->prev = pRes;
        }
        if (pShot->param_i[0] == 0 && pShot->count > 180) pShot->margin = -999;
        pShot = pShot->next;
    }
}

// 最終塗りつぶしばら撒き
static void ShotInkFinish(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 18; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(200) - 100;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(30) - 15;
            pEnemyShot->muki = (GetRand(120) + 30) / 180.0 * DX_PI; // 下方向中心
            pEnemyShot->speed = 1.5 + GetRand(150) / 100.0;
            pEnemyShot->kind = img_enemyShotDiamond[3];
            pEnemyShot->param_i[0] = 1;
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

        if (pShot->param_i[0] == 1 && pShot->count > 15 && pShot->count % 5 == 0) {
            sEnemyShot* pRes = new sEnemyShot;
            pRes->x = pShot->x;
            pRes->y = pShot->y;
            pRes->muki = GetRand(360) / 180.0 * DX_PI;
            pRes->speed = 0.12 + GetRand(18) / 100.0;
            pRes->kind = img_enemyShotSmallBall[3];
            pRes->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRes->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRes;
            pEnemyShotSet->pEnemyShotHead->prev = pRes;
        }
        if (pShot->param_i[0] == 0 && pShot->count > 60) pShot->margin = -999;
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Splatoon_Grok()
{
    static int muki;
    static int phase;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
    }
    else {
        // 左右移動
        enemy.x += 0.9 * (double)muki;
        if (enemy.x < 80.0) muki = 1;
        if (enemy.x > 400.0) muki = -1;
    }

    // フェーズ進行
    if (count == 120+100) phase = 1;
    if (count == 360+200) phase = 2;
    if (count == 600+300) phase = 3;
    if (count == 840+400) phase = 4;

    // フェーズに応じた弾幕生成
    if (phase == 0 && count % (25*5) == 1) {
        // 初期フェーズ：足元付近から塗り始める
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotInkMain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else if (phase == 1 && count % (18*5) == 1) {
        // 拡散フェーズ：波状に塗り広げ
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotInkWave;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI * 0.5 + (GetRand(40) - 20) / 180.0 * DX_PI;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else if (phase == 2 && count % (15*5) == 1) {
        // 制圧フェーズ：軌跡を濃く塗る
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotInkTrail;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else if (phase >= 3 && count % (12*5) == 1) {
        // フィニッシュ：画面下半分を埋め尽くす
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotInkFinish;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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
}
