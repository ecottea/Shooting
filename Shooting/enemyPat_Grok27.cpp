// enemyPat_Tmp.cpp
// 手裏剣モチーフ弾幕「影星乱舞」実装
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotShurikenScatter(sEnemyShotSet* pEnemyShotSet);
static void ShotShurikenCross(sEnemyShotSet* pEnemyShotSet);
static void ShotRotatingRing(sEnemyShotSet* pEnemyShotSet);

// --------------------------------------------------
// 1. ばら撒き手裏剣（回転しながら直進）
// --------------------------------------------------
static void ShotShurikenScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num = 8;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double angle = pEnemyShotSet->muki + (i - num / 2.0) * (DX_PI * 2.0 / num) * 0.7;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 3.5 + (i % 3) * 0.4;

            // 菱形弾（手裏剣らしい形状）を使用
            pEnemyShot->kind = img_enemyShotDiamond[3]; // 青系など見やすい色

            // 自転用パラメータ
            pEnemyShot->param_i[0] = GetRand(1) ? 1 : -1; // 回転方向
            pEnemyShot->param_d[0] = 0.0; // 回転角度

            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 自転エフェクト用（描画側で使う想定）
        pEnemyShot->param_d[0] += pEnemyShot->param_i[0] * 0.25;

        pEnemyShot = pEnemyShot->next;
    }
}

// --------------------------------------------------
// 2. 大型手裏剣分裂＋十字攻撃
// --------------------------------------------------
static void ShotShurikenCross(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 大型手裏剣本体
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 4.2;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 大型で目立つ
        pEnemyShot->param_i[0] = 0; // 分裂フラグ
        pEnemyShot->param_d[0] = 0.0; // 進行距離

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot->param_d[0] += pEnemyShot->speed;

        // 一定距離進んだら分裂
        if (pEnemyShot->param_i[0] == 0 && pEnemyShot->param_d[0] > 180.0 && pEnemyShot->count <= 180) {
            pEnemyShot->param_i[0] = 1;
            if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 4; i++) {
                sEnemyShot* split = new sEnemyShot;
                split->x = pEnemyShot->x;
                split->y = pEnemyShot->y;
                split->muki = i * (DX_PI / 2.0);
                split->speed = 5.5;
                split->kind = img_enemyShotDiamond[1];
                split->count = pEnemyShot->count;

                split->prev = pEnemyShotSet->pEnemyShotHead->prev;
                split->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = split;
                pEnemyShotSet->pEnemyShotHead->prev = split;
            }
            // 本体は少し遅く消えるように速度を落とす
            pEnemyShot->speed = 0.8;
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// --------------------------------------------------
// 3. 回転リング（2重反転）
// --------------------------------------------------
static void ShotRotatingRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int rings = 2;
        for (int r = 0; r < rings; r++) {
            int num = 18;
            double baseSpeed = 2.8 + r * 0.6;
            double dir = (r % 2 == 0) ? 1.0 : -1.0;

            for (int i = 0; i < num; i++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = pEnemyShotSet->x;
                p->y = pEnemyShotSet->y;
                p->muki = i * (DX_PI * 2.0 / num);
                p->speed = baseSpeed;
                p->kind = (r == 0) ? img_enemyShotDiamond[4] : img_enemyShotScale[2];

                p->param_i[0] = i;                    // インデックス
                p->param_i[1] = num;                  // 総数
                p->param_d[0] = dir * 0.12;           // 回転速度
                p->param_d[1] = 80.0 + r * 45.0;      // 初期半径

                p->margin = 480;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 円運動 + 徐々に外側へ
        double angle = pEnemyShot->muki + pEnemyShotSet->count * pEnemyShot->param_d[0];
        double radius = pEnemyShot->param_d[1] + pEnemyShotSet->count * 0.9;

        pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
        pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);

        pEnemyShot = pEnemyShot->next;
    }
}

// --------------------------------------------------
// 敵本体パターン
// --------------------------------------------------
void EnemyPat_Shuriken_Grok()
{
    static int muki = 1;
    static int phase = 0;
    static int shotTimer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
        shotTimer = 0;
    }
    else {
        // 左右往復移動
        enemy.x += 1.8 * (double)muki;
        if (enemy.x < 80.0 || enemy.x > 400.0) muki *= -1;

        // 軽い上下動き
        enemy.y = 60.0 + sin(count / 40.0) * 15.0;
    }

    shotTimer++;

    // タイミングで異なる攻撃を展開
    if (shotTimer % 85 == 0) {
        // 散布手裏剣
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotShurikenScatter;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    if (shotTimer % 140 == 45) {
        // 大型十字分裂
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotShurikenCross;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 15.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x) + (GetRand(40) - 20) / 180.0 * DX_PI;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    if (shotTimer % 210 == 90) {
        // 回転リング
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotRotatingRing;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 25.0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}