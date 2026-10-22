// ============================================================
//  雪の結晶モチーフ弾幕「六花・雪華晶」
//  敵本体: EnemyPat_Snowflake_DeepSeek
//  使用素材: 小玉(白) / 銃弾(シアン) / 大玉(白)
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾を1つ追加する（set の弾リスト末尾に挿入）
static void AddSnowShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, int centerFlag)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = centerFlag;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// 雪晶1層を生成（β度回転させた六回対称）
static void EmitSnowCrystalLayer(sEnemyShotSet* pSet, double betaDeg)
{
    const double cx = pSet->x;
    const double cy = pSet->y;
    const double beta = betaDeg * DX_PI / 180.0;
    const double deg60 = 60.0 * DX_PI / 180.0;
    const double deg55 = 55.0 * DX_PI / 180.0;

    for (int k = 0; k < 6; k++) {
        const double th = beta + k * deg60;
        const double dx = cos(th);
        const double dy = sin(th);

        // 主腕：小玉を点線状に並べる（外側ほど速くして膨張）
        for (int i = 0; i < 8; i++) {
            const double r = 0.8 + 0.45 * i;
            const double spd = 1.0 + 0.2 * i;
            AddSnowShot(pSet, cx + dx * r, cy + dy * r, th, spd,
                img_enemyShotSmallBall[6], 0); // 白
        }

        // 枝：銃弾を±55°方向へ（腕のノード位置から分岐）
        for (int j = 0; j < 2; j++) {
            const double r = (j == 0) ? 1.6 : 2.8;
            AddSnowShot(pSet, cx + dx * r, cy + dy * r, th + deg55, 0.9,
                img_enemyShotBullet[3], 0); // シアン
            AddSnowShot(pSet, cx + dx * r, cy + dy * r, th - deg55, 0.9,
                img_enemyShotBullet[3], 0);
        }

        // 腕の先端：大玉
        AddSnowShot(pSet, cx + dx * 4.4, cy + dy * 4.4, th, 1.8,
            img_enemyShotLargeBall[6], 0); // 白
    }
}

// 雪晶パターン本体
static void ShotSnowCrystal(sEnemyShotSet* pSet)
{
    switch (pSet->count) {
    case 0:
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        // 中心核（静止）
        AddSnowShot(pSet, pSet->x, pSet->y, 0.0, 0.0,
            img_enemyShotLargeBall[6], 1);
        EmitSnowCrystalLayer(pSet, 0.0);
        break;
    case 25:
        EmitSnowCrystalLayer(pSet, 30.0);
        break;
    case 50:
        EmitSnowCrystalLayer(pSet, 60.0);
        break;
    case 200:
        // 画面外に出ない中心核を削除
    {
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            sEnemyShot* next = p->next;
            if (p->param_i[0] == 1) {
                p->prev->next = p->next;
                p->next->prev = p->prev;
                delete p;
            }
            p = next;
        }
    }
    break;
    }

    // 弾の移動（中心核は speed=0 なので動かない）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体
void EnemyPat_Snowflake_DeepSeek()
{
    static int muki;
    static int shot_count;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しない
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.8 * (double)muki;
        if (enemy.x < 80.0)  muki = 1;
        if (enemy.x > 400.0) muki = -1;
    }

    // 90フレームごとに雪晶を1つ生成
    if (count % 90 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSnowCrystal;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}