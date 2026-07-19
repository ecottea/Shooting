// enemyPat_Match.cpp
// 弾幕: マッチ「一斉着火」
//
// マッチ箱に並んだマッチ棒(軸弾)が擦れながら伸び、
// 一定間隔で隣から隣へ着火が連鎖していく。
// 全ての軸が燃え尽きたところで中心から一斉フレアを放つ。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {
    const int    SPOKE_NUM = 16;    // マッチ棒(軸弾)の本数
    const int    PREPARE_FRAME = 40;    // Phase1: 擦り(着火準備)の長さ
    const int    IGNITE_INTERVAL = 4;     // 隣の軸への着火伝播間隔
    const int    BURST_WAY = 35;     // 着火時に展開する火花弾のWAY数
    const double SHAFT_SPEED = 1.2;   // 軸弾(未着火)の伸びる速さ
    const double BURN_SPEED = 0.5;   // 軸弾(着火後)の漂う速さ
    const double BURST_SPEED = 2.3;   // 火花弾の速さ

    const int LAST_IGNITE_FRAME = PREPARE_FRAME + (SPOKE_NUM - 1) * IGNITE_INTERVAL; // 最後の軸が着火するフレーム
    const int FLARE_FRAME = LAST_IGNITE_FRAME + 10;                            // 一斉フレアを放つフレーム
    const int CYCLE_LEN = FLARE_FRAME + 60;                                  // 1周期の長さ(以降ループ)
}

// 軸弾(マッチ棒)＋着火時の火花弾をまとめて扱うパターン関数
static void ShotMatchIgnite(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < SPOKE_NUM; i++) {
            pEnemyShot = new sEnemyShot;
            double baseAngle = pEnemyShotSet->muki + DX_PI * 2.0 * i / SPOKE_NUM;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle;
            pEnemyShot->speed = SHAFT_SPEED;
            pEnemyShot->kind = img_enemyShotBullet[7]; // 未着火 = 黒

            // param_d[0]: 擦れ揺れの位相(個体差ランダム)
            // param_d[1]: 軸の基準角度
            // param_i[0]: 着火するまでの遅延フレーム(隣から連鎖)
            // param_i[1]: 着火済みフラグ(0=未着火, 1=着火済み)
            // param_i[2]: 種別(0=軸弾, 1=火花弾)
            pEnemyShot->param_d[0] = GetRand(628) / 100.0;
            pEnemyShot->param_d[1] = baseAngle;
            pEnemyShot->param_i[0] = PREPARE_FRAME + i * IGNITE_INTERVAL;
            pEnemyShot->param_i[1] = 0;
            pEnemyShot->param_i[2] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pEnemyShot->next; // 着火時にリスト末尾へ挿入するため先に保持

        if (pEnemyShot->param_i[2] == 0) {
            // ---- 軸弾(マッチ棒) ----
            if (pEnemyShot->param_i[1] == 0) {
                // Phase1: 擦り。摩擦でわずかに角度が揺れながら伸びる
                double wobble = sin((pEnemyShot->count + pEnemyShot->param_d[0]) * 0.3) * 0.03;
                pEnemyShot->muki = pEnemyShot->param_d[1] + wobble;
                pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
                pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

                if (pEnemyShot->count >= pEnemyShot->param_i[0]) {
                    if (CheckSoundMem(sound_enemyShot_noize) == 1) StopSoundMem(sound_enemyShot_noize);
                    PlaySoundMem(sound_enemyShot_noize, DX_PLAYTYPE_BACK);
                    
                    // Phase2: 着火。炎色の火花弾を扇状に展開する
                    pEnemyShot->param_i[1] = 1;
                    pEnemyShot->kind = img_enemyShotBullet[8]; // 橙(着火直後)
                    pEnemyShot->speed = BURN_SPEED;             // 着火後は減速して漂う

                    for (int w = 0; w < BURST_WAY; w++) {
                        sEnemyShot* pFlame = new sEnemyShot;
                        double flameAngle = pEnemyShot->param_d[1]
                            + (w - (BURST_WAY - 1) / 2.0) * (DX_PI / 48.0);

                        pFlame->x = pEnemyShot->x;
                        pFlame->y = pEnemyShot->y;
                        pFlame->muki = flameAngle;
                        pFlame->speed = BURST_SPEED + GetRand(60) / 100.0;
                        pFlame->param_i[2] = 1; // 種別: 火花弾

                        // 赤・橙・黄をランダムに混ぜ炎の揺らめきを表現
                        switch (GetRand(2)) {
                        case 0: pFlame->kind = img_enemyShotSmallBall[0]; break; // 赤
                        case 1: pFlame->kind = img_enemyShotSmallBall[8]; break; // 橙
                        case 2: pFlame->kind = img_enemyShotSmallBall[1]; break; // 黄
                        }

                        pFlame->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pFlame->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pFlame;
                        pEnemyShotSet->pEnemyShotHead->prev = pFlame;
                    }
                }
            }
            else {
                // Phase3: 燃え尽き。着火後の軸はゆっくり漂いながら赤へ変色する
                pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
                pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
                if (pEnemyShot->count >= pEnemyShot->param_i[0] + 20) {
                    pEnemyShot->kind = img_enemyShotBullet[0]; // 赤(燃え尽き際)
                }
            }
        }
        else {
            // ---- 火花弾(着火時に展開した炎) ----
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pNext;
    }
}

// 一斉フレア(全方位)を扱うパターン関数
static void ShotFinalFlare(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int FLARE_NUM = 32;
        for (int i = 0; i < FLARE_NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI * 2.0 * i / FLARE_NUM;
            pEnemyShot->speed = 3.0 + (i % 2) * 0.6; // 二重リングにして密度を上げる
            pEnemyShot->kind = img_enemyShotLargeBall[8]; // 橙の大玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン: マッチ「一斉着火」
void EnemyPat_Match_Claude()
{
    if (count == 1) {
        // ゲーム画面は480x480
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // わずかに左右へ揺れる
        enemy.x = 240.0 + 30.0 * sin(count * 0.01);
    }

    // 周期の頭でマッチ棒(軸弾)を一斉発生させる
    if (count % CYCLE_LEN == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMatchIgnite;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0; // 軸の基準角度

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 全軸着火完了後、一斉フレアを放つ
    if (count % CYCLE_LEN == FLARE_FRAME) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFinalFlare;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}