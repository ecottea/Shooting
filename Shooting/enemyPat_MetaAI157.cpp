#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//============================================================
// Type-SN "重力崩壊と残骸星雲"
//============================================================

//---------- 吸い込み Phase1 ----------
static void ShotImplosion(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 12発を円周上に生成
        for (int i = 0; i < 12; i++) {
            sEnemyShot* p = new sEnemyShot;
            double baseAng = 2.0 * DX_PI * i / 12.0;
            double rndAng = (GetRand(20) - 10) / 180.0 * DX_PI;
            double ang = baseAng + rndAng;
            double radius = 260.0 + GetRand(120); // 260-380

            p->x = pSet->x + radius * cos(ang);
            p->y = pSet->y + radius * sin(ang);
            p->muki = atan2(pSet->y - p->y, pSet->x - p->x); // 中心へ
            p->speed = 2.2 + GetRand(100) / 100.0; // 2.2-3.2
            p->kind = img_enemyShotMediumBall[4]; // 中玉 青 = 収縮ガス
            // 中心座標を記憶
            p->param_d[0] = pSet->x;
            p->param_d[1] = pSet->y;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        // 中心へ加速しながら吸い込まれる
        pShot->speed *= 1.015;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 中心に十分近づいたら吸収されたとして消去
        double dx = pShot->x - pShot->param_d[0];
        double dy = pShot->y - pShot->param_d[1];
        if (dx * dx + dy * dy < 64.0) { // 8px以内
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        pShot = pNext;
    }
}

//---------- 本爆発 Flash Phase3 ----------
static void ShotFlash(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 36方向 x 3層 = 108発
        for (int layer = 0; layer < 3; layer++) {
            for (int i = 0; i < 36; i++) {
                sEnemyShot* p = new sEnemyShot;
                double ang = 2.0 * DX_PI * i / 36.0 + layer * 0.06;
                p->x = pSet->x;
                p->y = pSet->y;
                p->muki = ang;
                p->speed = 7.5 + layer * 1.8 + GetRand(50) / 100.0;
                // 白を中心に、層で色を変えると超新星の色温度に見える
                if (layer == 0) p->kind = img_enemyShotSmallBall[6]; // 白
                else if (layer == 1) p->kind = img_enemyShotSmallBall[3]; // シアン
                else p->kind = img_enemyShotSmallBall[4]; // 青
                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->speed *= 0.995; // 僅かに減速して残光感
        pShot = pShot->next;
    }
}

// 光芒用レーザー
static void ShotLightRay(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 16; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = 2.0 * DX_PI * i / 16.0;
            p->speed = 0.0;
            p->kind = img_enemyShotLaser[6]; // 短レーザー 白
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    // 18フレームだけ表示して消える
    if (pSet->count > 18) {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = pNext;
        }
    }
}

//---------- 残骸シェル 赤い衝撃波 ----------
static void ShotShell(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int num = pSet->param_i[0]; // 弾数
        double initSpeed = pSet->param_d[0];
        for (int i = 0; i < num; i++) {
            sEnemyShot* p = new sEnemyShot;
            double ang = 2.0 * DX_PI * i / num + pSet->param_d[1];
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = ang;
            p->speed = initSpeed;
            p->kind = img_enemyShotMediumBall[0]; // 中玉 赤 = 衝撃波本体
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->speed *= 0.985; // 膨張しながら減速 = SNRらしい挙動
        pShot = pShot->next;
    }
}

//---------- 内部フィラメント 黄色い破片 ----------
static void ShotFilament(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int n = 5 + GetRand(2); // 5-7発
        for (int i = 0; i < n; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            double base = 2.0 * DX_PI * GetRand(360) / 360.0;
            p->muki = base;
            p->param_d[0] = base; // 基準角度
            p->param_d[1] = GetRand(628) / 100.0; // 波の位相 0-6.28
            p->param_d[2] = 0.3 + GetRand(40) / 100.0; // 揺れ振幅
            p->speed = 1.4 + GetRand(120) / 100.0;
            p->kind = (GetRand(1) == 0) ? img_enemyShotScale[1] : img_enemyShotScale[8]; // 鱗弾 黄/橙
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // sin波で横揺れしながら進む = かに星雲のフィラメント
        double wave = sin(pSet->count * 0.18 + pShot->param_d[1]) * pShot->param_d[2]/3;
        double curMuki = pShot->param_d[0] + wave;
        // 僅かに自機狙いに曲げる
        double toPlayer = atan2(player.y - pShot->y, player.x - pShot->x);
        double diff = sin(toPlayer - pShot->param_d[0]);
        pShot->param_d[0] += diff * 0.008;

        pShot->x += pShot->speed * cos(curMuki);
        pShot->y += pShot->speed * sin(curMuki);
        pShot->muki = curMuki; // 見た目用(鱗弾の向き)
        pShot = pShot->next;
    }
}

//---------- パルサー 灯台ビーム ----------
static void ShotPulsar(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int k = 0; k < 2; k++) { // 2方向
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = pSet->param_d[0] + k * DX_PI; // 反対側
            p->speed = 3.3;
            p->kind = img_enemyShotSmallBall[3]; // 小玉 シアン = パルサー
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//============================================================
// 敵本体
//============================================================
void EnemyPat_Supernova_MetaAI()
{
    static double pulsarRot;
    static int phase;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
        pulsarRot = 0.0;
        phase = 0;
    }
    else {
        // 超新星なので中央でほぼ静止、わずかに脈動
        enemy.x = 240.0 + sin(count * 0.05) * 4.0;
        enemy.y = 120.0 + cos(count * 0.03) * 2.0;
    }

    //------ Phase1: 重力崩壊 1-150F ------
    if (count >= 1 && count <= 150 && count % 7 == 1) {
        if (count == 1) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotImplosion;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    //------ Phase2: 静寂 151-179F ------
    if (count == 155) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    //------ Phase3: 大爆発 180F ------
    if (count == 180) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 閃光
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotFlash;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        // 光芒レーザー
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotLightRay;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }

    //------ Phase4: 残骸膨張 240F以降 ------
    if (count >= 240) {
        // 衝撃波シェル: 60Fおきに1リング
        if (count % 60 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotShell;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->param_i[0] = 36; // 弾数
            pSet->param_d[0] = 2.8 + (GetRand(50) / 100.0); // 初速
            pSet->param_d[1] = GetRand(360) / 360.0 * DX_PI * 2.0; // 回転オフセット
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // フィラメント: 7Fおきに少量
        if (count % 7 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotFilament;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // パルサー: 5Fおきに回転ビーム
        if (count % 5 == 0) {
            pulsarRot += 0.18; // 約10度ずつ回転
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotPulsar;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->param_d[0] = pulsarRot;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}