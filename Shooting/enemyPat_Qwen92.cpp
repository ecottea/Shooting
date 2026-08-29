// enemyPat_meteor.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 隕石弾幕パターン用 補助関数
// ============================================================

// フェーズ1：予兆（警告弾）
static void Shot_Warning(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = DX_PI / 2.0; // 真下
        pShot->speed = 1.5;
        // 赤の大玉で警告を表現
        pShot->kind = img_enemyShotLargeBall[0];
        pShot->margin = 120;

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// フェーズ2：本降下（隕石本体と追従破片）
static void Shot_MeteorSingle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pMeteor = new sEnemyShot;
        pMeteor->x = pEnemyShotSet->x;
        pMeteor->y = pEnemyShotSet->y;
        // 下向き(DX_PI/2)を基準に、左右に最大30度ずらす
        // GetRand(60) は 0〜60 を返すので、-30〜30 の範囲になる
        pMeteor->muki = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;

        // 種類：70%で中型(橙)、30%で大型(赤)
        if (GetRand(9) < 7) {
            pMeteor->kind = img_enemyShotMediumBall[8]; // 橙の中玉
            pMeteor->speed = 3.5 + GetRand(20) / 10.0 - 1;  // 3.5 〜 5.5
        }
        else {
            pMeteor->kind = img_enemyShotLargeBall[0];  // 赤の大玉
            pMeteor->speed = 2.0 + GetRand(15) / 10.0 - 1;  // 2.0 〜 3.5 (大型は遅め)
        }
        pMeteor->margin = 120;

        pMeteor->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pMeteor->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pMeteor;
        pEnemyShotSet->pEnemyShotHead->prev = pMeteor;

        // 確率で予告音を再生（うるさくならないよう10%程度）
        if (GetRand(2) == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }
    else if (pEnemyShotSet->count == 12) {
        // 破片の生成（本体の後方に追従するように）
        sEnemyShot* pMeteor = pEnemyShotSet->pEnemyShotHead->next;
        if (pMeteor != pEnemyShotSet->pEnemyShotHead) {
            int fragmentCount = 3 + GetRand(3); // 3〜5個の破片
            for (int i = 0; i < fragmentCount; i++) {
                sEnemyShot* pFrag = new sEnemyShot;
                pFrag->x = pMeteor->x + (GetRand(40) - 20); // 横に少しばらけさせる
                pFrag->y = pMeteor->y - 15.0; // 少し上から
                // 本体の向きからさらに少しばらけさせる
                pFrag->muki = pMeteor->muki + (GetRand(40) - 20) / 180.0 * DX_PI;
                pFrag->speed = pMeteor->speed * (0.8 + GetRand(40) / 100.0); // 本体より少し遅め

                pFrag->kind = img_enemyShotSmallBall[1]; // 黄の小玉（破片）
                pFrag->margin = 120;

                pFrag->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pFrag->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pFrag;
                pEnemyShotSet->pEnemyShotHead->prev = pFrag;
            }
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 隕石らしい少しの不規則な横揺れを加える
        double wiggle = sin(pEnemyShotSet->count * 0.3 + pShot->x * 0.05) * 0.4;
        pShot->x += pShot->speed * cos(pShot->muki) + wiggle;
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// フェーズ3：爆発的散開（隕石衝突イメージ）
static void Shot_Explosion(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int num = 36 * 5;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = i * 2.0 * DX_PI / num;
            pShot->speed = 2.5 + GetRand(30) / 10.0 - 2; // 2.5 〜 5.5

            // 橙の小玉で爆発の破片を表現
            pShot->kind = img_enemyShotSmallBall[8];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Meteor_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵本体は画面上部で左右にゆっくり揺れる
        enemy.x = 240.0 + 100.0 * sin(count * 0.015);
    }

    const int T = 700;
    int countT = count % T;

    // フェーズ1：予兆 (120 < count <= 240)
    // 30フレーム(0.5秒)ごとに警告弾を降らせる
    if (countT > 120 && countT <= 240 && countT % 30 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Shot_Warning;
        pSet->x = 40.0 + (double)GetRand(400); // 40.0 〜 440.0 の範囲
        pSet->y = -20.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // フェーズ2：本降下 (240 < count <= 540)
    // 4フレームごとに隕石セットを生成
    if (countT > 240 && countT <= 540 && countT % 4 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Shot_MeteorSingle;
        pSet->x = (double)GetRand(480); // 0.0 〜 480.0
        pSet->y = -30.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // フェーズ3：爆発的散開 (count == 550)
    // 敵本体の位置から全方位に破片をばら撒く
    if (countT == 600) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Shot_Explosion;
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