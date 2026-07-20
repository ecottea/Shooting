// enemyPat_Swimmy.cpp
// スイミーモチーフ弾幕パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ======================
// 補助関数
// ======================
static void PlayShotSound(int strength) {
    int snd = sound_enemyShot_medium;
    if (strength >= 2) snd = sound_enemyShot_heavy;
    if (CheckSoundMem(snd)) StopSoundMem(snd);
    PlaySoundMem(snd, DX_PLAYTYPE_BACK);
}

// ======================
// パターン1: 小魚の散開泳行 (赤い魚群)
static void ShotSwimmyFishScatter(sEnemyShotSet* pSet) {
    sEnemyShot* pShot;
    if (pSet->count == 0) {
        PlayShotSound(1);
        int num = 12;
        for (int i = 0; i < num; i++) {
            pShot = new sEnemyShot;
            double angleOffset = (i - num / 2.0) * 0.25;
            pShot->x = pSet->x + GetRand(80) - 40;
            pShot->y = pSet->y + GetRand(30) - 15;
            pShot->muki = pSet->muki + angleOffset;
            pShot->speed = 1.8 + (GetRand(60) / 100.0);
            pShot->kind = img_enemyShotSmallBall[0]; // 赤小玉 (赤い魚)
            pShot->param_i[0] = 0; // 0 = 通常泳行
            pShot->param_d[0] = pSet->param_d[0]; // 群れID的なオフセット

            // 双方向リストに追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 更新
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double wave = sin(pSet->count * 0.1 + p->param_d[0]) * 0.8;
        p->muki += wave * 0.015; // 軽い蛇行
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ======================
// パターン2: 集合＆旋回 (群れ集合)
static void ShotSwimmyGather(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        PlayShotSound(1);
        int num = 16;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x + GetRand(120) - 60;
            pShot->y = pSet->y + GetRand(60) - 30;
            pShot->muki = GetRand(360) / 180.0 * DX_PI;
            pShot->speed = 2.2;
            pShot->kind = img_enemyShotSmallBall[0]; // 赤
            pShot->param_i[0] = 1; // 集合フェーズ
            pShot->param_d[0] = i * 0.4; // 個別オフセット

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        // 中心に向かう力 + 回転
        double dx = pSet->x - p->x;
        double dy = pSet->y - p->y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist > 5.0) {
            p->muki = atan2(dy, dx) + sin(pSet->count * 0.2 + p->param_d[0]) * 1.2;
        }
        p->speed = 2.0 + sin(pSet->count * 0.15) * 0.6;
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ======================
// パターン3: 巨大魚形成＆攻撃
static void ShotSwimmyBigFish(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        PlayShotSound(2);
        // 黒いスイミー本体
        sEnemyShot* swimmy = new sEnemyShot;
        swimmy->x = pSet->x;
        swimmy->y = pSet->y;
        swimmy->muki = pSet->muki;
        swimmy->speed = 2.5;
        swimmy->kind = img_enemyShotMediumBall[7]; // 黒
        swimmy->param_i[0] = 2; // リーダー
        swimmy->prev = pSet->pEnemyShotHead->prev;
        swimmy->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = swimmy;
        pSet->pEnemyShotHead->prev = swimmy;

        // 赤い魚群（体を構成）
        int num = 22;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double t = i / (double)num;
            pShot->x = pSet->x + cos(t * DX_PI * 2) * 45;
            pShot->y = pSet->y + sin(t * DX_PI * 2) * 25;
            pShot->muki = pSet->muki;
            pShot->speed = 1.8;
            pShot->kind = img_enemyShotSmallBall[0]; // 赤
            pShot->param_i[0] = 3; // 体構成部
            pShot->param_d[0] = t;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 更新
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 2) { // スイミー本体
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            // 軽く波打つ
            p->y += sin(pSet->count * 0.2) * 1.2;
        }
        else if (p->param_i[0] == 3) { // 体
            double t = p->param_d[0];
            // 巨大魚シルエットを維持しつつ左右移動
            double baseX = pSet->x + cos(pSet->count * 0.08) * 80;
            double baseY = pSet->y + sin(pSet->count * 0.12) * 30;
            p->x = baseX + cos(t * DX_PI * 2 + pSet->count * 0.1) * 48;
            p->y = baseY + sin(t * DX_PI * 2 + pSet->count * 0.1) * 28;
        }
        else {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }

        // 巨大魚の口から放射弾（一定間隔）
        if (pSet->count % 12 == 0 && p->param_i[0] == 3) {
            if (GetRand(3) == 0) {
                sEnemyShot* mouthShot = new sEnemyShot;
                mouthShot->x = p->x;
                mouthShot->y = p->y + 10;
                mouthShot->muki = pSet->muki + (GetRand(80) - 40) / 180.0 * DX_PI * 0.6;
                mouthShot->speed = 3.5 + GetRand(80) / 100.0;
                mouthShot->kind = img_enemyShotSmallBall[0];
                mouthShot->param_i[0] = 4;
                mouthShot->prev = pSet->pEnemyShotHead->prev;
                mouthShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = mouthShot;
                pSet->pEnemyShotHead->prev = mouthShot;
            }
        }
        p = p->next;
    }
}

// ======================
// 敵本体パターン
void EnemyPat_Swimmy_Grok() {
    static int phase = 0;
    static int muki = 1;
    static int shotCount = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        muki = 1;
        shotCount = 0;
    }

    // 敵本体移動（ゆったり左右）
    enemy.x += 1.1 * (double)muki;
    if (enemy.x < 100 || enemy.x > 380) muki *= -1;

    // 定期的に新しいShotSetを生成
    if (count % 90 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = shotCount++;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // フェーズごとに違う挙動
        if (phase == 0) {
            pSet->patternFunc = ShotSwimmyFishScatter;
            pSet->param_d[0] = GetRand(100) / 10.0; // 波オフセット
        }
        else if (phase == 1) {
            pSet->patternFunc = ShotSwimmyGather;
        }
        else {
            pSet->patternFunc = ShotSwimmyBigFish;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x) + (GetRand(60) - 30) / 180.0 * DX_PI;
        }

        // リストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        // フェーズ進行
        phase = (phase + 1) % 3;
    }
}