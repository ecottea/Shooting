#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

static const double CENTER_X = 240.0;
static const double CENTER_Y = 240.0;
static const double OUTER_RADIUS = 240.0 * 1.41421356237; // 480x480の外接円

// 引力の強さを計算 (ボス残り体力で増加)
static inline double GetPullSpeed() {
    double hpRate = 1.0 - (double)enemy.hp / (double)enemy.maxHp;
    if (hpRate < 0) hpRate = 0;
    if (hpRate > 1) hpRate = 1;
    return 0.5 + 1.7 * hpRate; // 0.5 -> 2.2
}

// A: 外接円から中央へ引き寄せられる赤中玉
static void ShotSubterraneanSun_A(sEnemyShotSet* pEnemyShotSet)
{
    double pull = GetPullSpeed();

    // 既存弾の移動: 常に中央向き、速度は引力と同じ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        double angToCenter = atan2(CENTER_Y - pShot->y, CENTER_X - pShot->x);
        pShot->muki = angToCenter;
        pShot->speed = pull;
        pShot->x += pull * cos(angToCenter);
        pShot->y += pull * sin(angToCenter);

        double dx = pShot->x - CENTER_X;
        double dy = pShot->y - CENTER_Y;
        if (dx * dx + dy * dy < 15.0 * 15.0) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        pShot = pNext;
    }

    // 中頻度生成、狭い間隔、基準角度は時間で回転
    if (pEnemyShotSet->count % 12 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseAngle = pEnemyShotSet->count * 0.015; // 時間経過で回転
        // たまに逆回転も混ぜる
        if ((pEnemyShotSet->count / 300) % 2 == 1) baseAngle = -baseAngle;

        const int num = 5;
        const double interval = 0.08; // 約4.5度

        for (int i = 0; i < num; i++) {
            sEnemyShot* ns = new sEnemyShot;
            double a = baseAngle + (i - num / 2) * interval;
            ns->x = CENTER_X + OUTER_RADIUS * cos(a);
            ns->y = CENTER_Y + OUTER_RADIUS * sin(a);
            ns->muki = 0.0;
            ns->speed = 0.0; // 仕様: 速度0で発生
            ns->kind = img_enemyShotMediumBall[0]; // 赤中玉
            ns->margin = 240.0; // 仕様
            ns->count = 0;

            ns->prev = pEnemyShotSet->pEnemyShotHead->prev;
            ns->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = ns;
            pEnemyShotSet->pEnemyShotHead->prev = ns;
        }
    }
}

// B: 中央から前後に撃つ赤小玉、引力無影響、回転+微小乱数
static void ShotSubterraneanSun_B(sEnemyShotSet* pEnemyShotSet)
{
    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }

    if (pEnemyShotSet->count % 3 == 0) { // 高頻度
        if (pEnemyShotSet->count % 12 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        double base = pEnemyShotSet->count * 0.04; // 時間で回転

        for (int k = 0; k < 2; k++) { // 前後
            sEnemyShot* ns = new sEnemyShot;
            ns->x = CENTER_X;
            ns->y = CENTER_Y;
            // GetRand(x)は0..xの x+1通りなので注意
            double jitter = (GetRand(20) - 10) / 180.0 * DX_PI; // -10度〜+10度
            double dir = (k == 0) ? 0.0 : DX_PI;
            ns->muki = base + dir + jitter;
            ns->speed = 3.2 + GetRand(40) / 100.0; // 一定+僅かなばらつき 3.2-3.6
            ns->kind = img_enemyShotSmallBall[0]; // 赤小玉
            ns->margin = 20.0;
            ns->count = 0;

            ns->prev = pEnemyShotSet->pEnemyShotHead->prev;
            ns->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = ns;
            pEnemyShotSet->pEnemyShotHead->prev = ns;
        }
    }
}

// C: 静止赤大玉で核を表現、HPで肥大
static void ShotSubterraneanSun_C(sEnemyShotSet* pEnemyShotSet)
{
    double hpRate = 1.0 - (double)enemy.hp / (double)enemy.maxHp;
    if (hpRate < 0) hpRate = 0;
    if (hpRate > 1) hpRate = 1;
    double targetR = 25.0 + 70.0 * hpRate; // 25 -> 95

    int curCnt = 0;
    for (sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next; p != pEnemyShotSet->pEnemyShotHead; p = p->next) curCnt++;

    int desired = (int)(20.0 + targetR * 1.5);
    int toSpawn = desired - curCnt;
    int spawnThisFrame = 0;
    if (pEnemyShotSet->count == 0) {
        spawnThisFrame = desired; // 初回は一括
    }
    else if (toSpawn > 0) {
        spawnThisFrame = toSpawn > 3 ? 3 : toSpawn;
    }

    for (int i = 0; i < spawnThisFrame; i++) {
        sEnemyShot* ns = new sEnemyShot;
        // 一様円盤分布: r = sqrt(rand)*R
        double ang = (double)GetRand(360) / 180.0 * DX_PI;
        double rr = (double)GetRand(1000) / 1000.0;
        double r = sqrt(rr) * targetR;
        ns->x = CENTER_X + r * cos(ang);
        ns->y = CENTER_Y + r * sin(ang);
        ns->muki = 0.0;
        ns->speed = 0.0;
        ns->kind = img_enemyShotLargeBall[0]; // 赤大玉
        ns->margin = 20.0;
        ns->count = 0;

        ns->prev = pEnemyShotSet->pEnemyShotHead->prev;
        ns->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = ns;
        pEnemyShotSet->pEnemyShotHead->prev = ns;
    }
}

void EnemyPat_SubterraneanSun_MetaAI()
{
    if (count == 1) {
        enemy.x = CENTER_X;
        enemy.y = CENTER_Y;
        enemy.maxHp = enemy.hp = 200;

        // --- A: 引力弾セット ---
        {
            sEnemyShotSet* s = new sEnemyShotSet;
            s->count = 0;
            s->patternFunc = ShotSubterraneanSun_A;
            s->x = CENTER_X;
            s->y = CENTER_Y;
            s->muki = 0;
            s->kind = 0;
            s->pEnemyShotHead = new sEnemyShot;
            s->pEnemyShotHead->prev = s->pEnemyShotHead;
            s->pEnemyShotHead->next = s->pEnemyShotHead;
            s->prev = enemyShotSetHead.prev;
            s->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = s;
            enemyShotSetHead.prev = s;
        }
        // --- B: 回転小玉セット ---
        {
            sEnemyShotSet* s = new sEnemyShotSet;
            s->count = 0;
            s->patternFunc = ShotSubterraneanSun_B;
            s->x = CENTER_X;
            s->y = CENTER_Y;
            s->muki = 0;
            s->kind = 0;
            s->pEnemyShotHead = new sEnemyShot;
            s->pEnemyShotHead->prev = s->pEnemyShotHead;
            s->pEnemyShotHead->next = s->pEnemyShotHead;
            s->prev = enemyShotSetHead.prev;
            s->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = s;
            enemyShotSetHead.prev = s;
        }
        // --- C: 核セット ---
        {
            sEnemyShotSet* s = new sEnemyShotSet;
            s->count = 0;
            s->patternFunc = ShotSubterraneanSun_C;
            s->x = CENTER_X;
            s->y = CENTER_Y;
            s->muki = 0;
            s->kind = 0;
            s->pEnemyShotHead = new sEnemyShot;
            s->pEnemyShotHead->prev = s->pEnemyShotHead;
            s->pEnemyShotHead->next = s->pEnemyShotHead;
            s->prev = enemyShotSetHead.prev;
            s->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = s;
            enemyShotSetHead.prev = s;
        }
    }
    else {
        // 常時: 画面中央への引力で自機を引っ張る
        double pullSpeed = GetPullSpeed();
        double angle = atan2(CENTER_Y - player.y, CENTER_X - player.x);
        double ratio = 3.0;
        spawnForceParticles(player.x, player.y, pullSpeed * cos(angle) * ratio, pullSpeed * sin(angle) * ratio);
        player.x += pullSpeed * cos(angle);
        player.y += pullSpeed * sin(angle);

        // 画面外に飛び出さないようにクランプ (ゲーム仕様に合わせて調整可)
        if (player.x < 4) player.x = 4;
        if (player.x > 476) player.x = 476;
        if (player.y < 4) player.y = 4;
        if (player.y > 476) player.y = 476;
    }
}