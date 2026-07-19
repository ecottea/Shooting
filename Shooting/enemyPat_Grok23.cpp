// enemyPat_Tmp.cpp - Sierpinski Gasket Motif Danmaku Pattern
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ヘルパー: 弾を1発生成（色と種類をフラクタル的に意識して選択）
static void CreateBullet(sEnemyShotSet* pSet, double bx, double by, double dir, double spd, int level, int i, int j) {
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = bx;
    pShot->y = by;
    pShot->muki = dir;
    pShot->speed = spd;

    // レベルに応じた弾の種類と色を意図的に選択
    // 小さいレベル（外側）は大きな幾何弾、内側は細かい玉でフラクタル感を強調
    int kindBase;
    int colorIdx;

    if (level <= 1) {
        // 最外層：シャープで大きな三角感を出す → Diamond + 強い色
        kindBase = 5; // Diamond
        colorIdx = (i + j) % 3 == 0 ? 0 : (i % 2 == 0 ? 5 : 8); // 赤 / マゼンタ / 橙
    }
    else if (level == 2) {
        // 中層：Scale（鱗弾）で幾何学的なテクスチャ
        kindBase = 4; // Scale
        colorIdx = 3 + (level + i) % 3; // シアン〜青〜マゼンタ寄り
    }
    else {
        // 内層：細かいSmall/MediumBallで密度と再帰感
        kindBase = (i % 3 == 0) ? 0 : 1; // SmallBall or MediumBall
        colorIdx = (i * 7 + j * 3) % 7;   // 赤→黄→緑→シアン→青→マゼンタ→白 の循環
    }

    switch (kindBase) {
    case 0:
        pShot->kind = img_enemyShotSmallBall[colorIdx];
        break;
    case 1:
        pShot->kind = img_enemyShotMediumBall[colorIdx];
        break;
    case 4:
        pShot->kind = img_enemyShotScale[colorIdx];
        break;
    case 5:
        pShot->kind = img_enemyShotDiamond[colorIdx];
        break;
    default:
        pShot->kind = img_enemyShotSmallBall[colorIdx];
        break;
    }

    pShot->margin = 240;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// シェルピンスキー風弾幕パターン
static void ShotSierpinski(sEnemyShotSet* pEnemyShotSet) {
    if (pEnemyShotSet->count == 0) {
        // 重い発射音でインパクトを強調
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double cx = pEnemyShotSet->x;
        double cy = pEnemyShotSet->y;
        int maxLevel = 4;
        double baseSize = 325.0;

        for (int lev = 0; lev <= maxLevel; ++lev) {
            int div = (1 << lev);
            double subSize = baseSize / div * 0.92;

            for (int i = 0; i <= div; ++i) {
                for (int j = 0; j <= i; ++j) {
                    // シェルピンスキー三角グリッド配置
                    double tx = cx + (i - j * 0.5) * subSize;
                    double ty = cy + j * subSize * 0.8660254; // √3/2

                    double dx = player.x - tx;
                    double dy = player.y - ty;
                    double dist = hypot(dx, dy);
                    double dir = (dist > 1e-6) ? atan2(dy, dx) : pEnemyShotSet->muki;

                    // レベルが深いほど少し速く、内側が密集
                    double spd = 1.65 + lev * 0.55 + GetRand(45) / 120.0;

                    CreateBullet(pEnemyShotSet, tx, ty + lev * 3.5,
                        dir + (GetRand(44) - 22) / 180.0 * DX_PI * 0.12,
                        spd, lev, i, j);
                }
            }
        }
    }

    // 弾移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_Sierpinski_Grok() {
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 52.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 1.12 * (double)muki;
        if (enemy.x > 395 || enemy.x < 85) muki *= -1;
        enemy.y = 52.0 + 15.0 * sin(count * 0.028);
    }

    // 約48フレームごとに発射
    if (count % 48 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSierpinski;
        pSet->x = enemy.x - 100;
        pSet->y = enemy.y + 20.0 - 100;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
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