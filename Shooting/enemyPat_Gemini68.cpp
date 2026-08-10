// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の状態管理用定数
enum eHanabiState {
    HANABI_LARGE = 0,   // Phase 1: 昇り大弾
    HANABI_SPARK,       // Phase 1: 軌道の火の粉
    HANABI_MEDIUM,      // Phase 2: 一次爆発（中弾・静止待機）
    HANABI_SMALL        // Phase 3: 二次爆発（小弾・枝垂れ落下）
};

// 弾幕処理：昇り曲導付 尺玉千輪菊
static void ShotHanabi(sEnemyShotSet* pEnemyShotSet)
{
    // Phase 1: 初期化（大弾の打ち上げ）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        // ターゲット（自機頭上付近）に向けて打ち上げ
        double targetX = player.x + (GetRand(60) - 30);
        double targetY = player.y - 120.0 + (GetRand(40) - 20);
        pEnemyShot->muki = atan2(targetY - pEnemyShot->y, targetX - pEnemyShot->x);
        pEnemyShot->speed = 6.5;
        pEnemyShot->kind = img_enemyShotLargeBall[1]; // 橙の大玉
        pEnemyShot->param_i[0] = HANABI_LARGE;
        pEnemyShot->margin = 100;

        // リストに追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        bool isErased = false;

        switch (pShot->param_i[0]) {
        case HANABI_LARGE: // 【Phase 1】昇り大弾
            pShot->speed *= 0.95; // 急減速
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 軌道上に火の粉を残す
            if (pShot->count % 2 == 0) {
                sEnemyShot* pSpark = new sEnemyShot;
                pSpark->x = pShot->x + (GetRand(10) - 5);
                pSpark->y = pShot->y + (GetRand(10) - 5);
                pSpark->muki = pShot->muki + DX_PI + (GetRand(60) - 30) / 180.0 * DX_PI;
                pSpark->speed = (50 + GetRand(100)) / 100.0;
                pSpark->kind = img_enemyShotDiamond[1]; // 黄色の菱形弾（火の粉）
                pSpark->param_i[0] = HANABI_SPARK;
                pSpark->margin = 240;

                pSpark->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSpark->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSpark;
                pEnemyShotSet->pEnemyShotHead->prev = pSpark;
            }

            // 目標地点到達（減速時）で大輪開花
            if (pShot->count >= 35 || pShot->speed < 0.3) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                int numMedium = 36; // 36方向の大輪
                for (int i = 0; i < numMedium; i++) {
                    sEnemyShot* pMedium = new sEnemyShot;
                    pMedium->x = pShot->x;
                    pMedium->y = pShot->y;
                    pMedium->muki = (2.0 * DX_PI / numMedium) * i;
                    pMedium->speed = 14.5;
                    pMedium->kind = (i % 2 == 0) ? img_enemyShotMediumBall[0] : img_enemyShotMediumBall[8]; // 赤と橙交互
                    pMedium->param_i[0] = HANABI_MEDIUM;
                    // 二次爆発までの時間差ディレイ（10〜50フレーム）
                    pMedium->param_i[1] = 10 + GetRand(40);
                    pMedium->margin = 240;

                    pMedium->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pMedium->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pMedium;
                    pEnemyShotSet->pEnemyShotHead->prev = pMedium;
                }

                // 大弾の消滅処理
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                isErased = true;
            }
            break;

        case HANABI_SPARK: // 火の粉処理
            pShot->speed *= 0.88;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki) + 0.2; // わずかに沈む

            if (pShot->count >= 15) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                isErased = true;
            }
            break;

        case HANABI_MEDIUM: // 【Phase 2】中弾（静止待機）
            pShot->speed *= 0.88; // 空中で静止
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 各弾の個別ディレイ満了で二次爆発（千輪咲き）
            if (pShot->count >= pShot->param_i[1]) {
                // サウンドの連打を抑えるため確率で再生
                if (GetRand(4) == 0) {
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }

                int numSmall = 4; // 4方向に小弾が分裂
                double baseAngle = (GetRand(360) / 180.0) * DX_PI;
                for (int i = 0; i < numSmall; i++) {
                    sEnemyShot* pSmall = new sEnemyShot;
                    pSmall->x = pShot->x;
                    pSmall->y = pShot->y;

                    double angle = baseAngle + (2.0 * DX_PI / numSmall) * i;
                    double initSpeed = (150 + GetRand(100)) / 100.0; // 1.5 ~ 2.5

                    // 速度ベクトルの成分を param_d に保持（重力計算用）
                    pSmall->param_d[0] = initSpeed * cos(angle); // vx
                    pSmall->param_d[1] = initSpeed * sin(angle); // vy

                    pSmall->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[4]; // シアン or 青
                    pSmall->param_i[0] = HANABI_SMALL;
                    pSmall->margin = 240;

                    pSmall->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSmall->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSmall;
                    pEnemyShotSet->pEnemyShotHead->prev = pSmall;
                }

                // 中弾の消滅処理
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                isErased = true;
            }
            break;

        case HANABI_SMALL: // 【Phase 3】小弾（枝垂れ落下）
            // 重力加速度を加算
            pShot->param_d[1] += 0.035; // 下向き加速度
            pShot->param_d[0] *= 0.99;  // 空気抵抗

            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];

            // 速度ベクトルに合わせて向きを保持
            pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
            break;
        }

        pShot = pNext;
    }
}

// 敵本体のパターン関数
void EnemyPat_Firework_Gemini()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期位置とパラメタ（画面サイズ 480x480）
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の緩やかな左右移動
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 160フレームごとに尺玉を打ち上げ
    if (count % 160 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHanabi;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}