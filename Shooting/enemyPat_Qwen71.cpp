// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// -----------------------------------------------------------------------------
// ヘルパー関数
// -----------------------------------------------------------------------------

// 弾の移動処理（共通）
static void UpdateShots(sEnemyShotSet* pSet) {
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾の生成とリスト接続（共通）
static sEnemyShot* CreateShot(sEnemyShotSet* pSet) {
    sEnemyShot* pShot = new sEnemyShot;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 弾幕セットの生成とリスト接続
static void AddEnemyShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki) {
    sEnemyShotSet* pNewSet = new sEnemyShotSet;
    pNewSet->count = 0;
    pNewSet->patternFunc = func;
    pNewSet->x = x;
    pNewSet->y = y;
    pNewSet->muki = muki;
    pNewSet->kind = 0;

    pNewSet->pEnemyShotHead = new sEnemyShot;
    pNewSet->pEnemyShotHead->prev = pNewSet->pEnemyShotHead;
    pNewSet->pEnemyShotHead->next = pNewSet->pEnemyShotHead;

    pNewSet->prev = enemyShotSetHead.prev;
    pNewSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pNewSet;
    enemyShotSetHead.prev = pNewSet;
}

// -----------------------------------------------------------------------------
// 弾幕パターン関数
// -----------------------------------------------------------------------------

// Phase 1-A: 火花 (Spark)
static void Shot_Spark(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12 * 5; i++) {
            sEnemyShot* pShot = CreateShot(pSet);
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = GetRand(628) / 100.0; // 0.00 ~ 6.28 ランダム方向
            pShot->speed = 1.5 + GetRand(15) / 10.0;
            pShot->kind = img_enemyShotSmallBall[1]; // 小玉:黄
        }
    }
    UpdateShots(pSet);
}

// Phase 1-B: 閃光 (Aim)
static void Shot_Aim(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = CreateShot(pSet);
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = pSet->muki; // 自機狙い
        pShot->speed = 4.0;
        pShot->kind = img_enemyShotMediumBall[6]; // 中玉:白
    }
    UpdateShots(pSet);
}

// Phase 2: 明滅リング (Flicker Ring)
static void Shot_FlickerRing(sEnemyShotSet* pSet) {
    // 活動期間: 8秒 (480フレーム)
    if (pSet->count < 480) {
        if (pSet->count == 0) {
            // 最初の点灯タイミングをセット
            pSet->param_d[0] = GetRand(20) + 10;
        }

        // 点灯タイミングになったら発射
        if (pSet->count >= pSet->param_d[0]) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            int num = 18 + GetRand(6) + 50; // 18~24発
            for (int i = 0; i < num; i++) {
                sEnemyShot* pShot = CreateShot(pSet);
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = (DX_PI * 2.0 / num) * i;
                pShot->speed = 2.5;
                pShot->kind = img_enemyShotScale[1]; // 鱗弾:黄
            }

            // 次の点灯タイミングをセット (ランダムな間隔でチカチカ)
            pSet->param_d[0] = pSet->count + GetRand(30) + 15;
        }
    }
    UpdateShots(pSet);
}

// Phase 3: フィラメント螺旋 (Filament Spiral)
static void Shot_Filament(sEnemyShotSet* pSet) {
    // 活動期間: 7秒 (420フレーム)
    if (pSet->count < 420 - 90) {
        if (pSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        // 4フレームに1回発射
        if (pSet->count % 1 == 0) {
            double angle = pSet->count * 0.08; // 回転速度
            for (int i = 0; i < 2; i++) { // 2条螺旋
                sEnemyShot* pShot = CreateShot(pSet);
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = angle + DX_PI * i;
                pShot->speed = 2.8;
                pShot->kind = img_enemyShotBullet[8]; // 銃弾:橙
            }
        }
    }
    UpdateShots(pSet);
}

// Phase 4: 破裂 (Burst)
static void Shot_Burst(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int num = 40 * 10;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = CreateShot(pSet);
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            // 全方位 + ランダムな揺らぎ
            pShot->muki = (DX_PI * 2.0 / num) * i + (GetRand(20) - 10) / 100.0;
            pShot->speed = 3.5 + GetRand(15) / 10.0;
            pShot->kind = img_enemyShotDiamond[3]; // 菱形:シアン
        }
    }
    UpdateShots(pSet);
}

// -----------------------------------------------------------------------------
// 敵本体のパターン
// -----------------------------------------------------------------------------

void EnemyPat_FlickeringLight_Qwen()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 移動ロジック: 不規則に揺れる
    enemy.x += cos((count - 1) * 0.02) * 1.5;
    enemy.y += cos((count - 1) * 0.03) * 0.8;

    // Phase 1: 不安定な点灯 (0.0 ~ 5.0秒)
    if (count < 300) {
        if (count % 20 == 0) {
            AddEnemyShotSet(Shot_Spark, enemy.x, enemy.y, 0);
        }
        if (count % 60 == 0) {
            double aim = atan2(player.y - enemy.y, player.x - enemy.x);
            AddEnemyShotSet(Shot_Aim, enemy.x, enemy.y, aim);
        }
    }
    // Phase 2: 明滅リング (5.0 ~ 13.0秒)
    else if (count == 300) {
        AddEnemyShotSet(Shot_FlickerRing, enemy.x, enemy.y, 0);
    }
    // Phase 3: フィラメントの渦 (13.0 ~ 20.0秒)
    else if (count == 780) {
        AddEnemyShotSet(Shot_Filament, enemy.x, enemy.y, 0);
    }
    // Phase 4: プチンと切れる瞬間 (20.0 ~ 25.0秒)
    else if (count == 1200 - 90) {
        AddEnemyShotSet(Shot_Burst, enemy.x, enemy.y, 0);
    }
}