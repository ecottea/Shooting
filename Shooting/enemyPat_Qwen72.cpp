// enemyPat_EternalForceBlizzard.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 色インデックス定義 (サンプルコードのコメントに基づく)
// 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
#define COLOR_CYAN  3
#define COLOR_BLUE  4
#define COLOR_WHITE 6

// 弾画像取得ヘルパー
// shape: 0=Small, 1=Medium, 2=Large, 3=Bullet, 4=Scale, 5=Diamond, 6=Oval, 7=Laser
static int GetShotImage(int shape, int color) {
    switch (shape) {
    case 0: return img_enemyShotSmallBall[color];
    case 1: return img_enemyShotMediumBall[color];
    case 2: return img_enemyShotLargeBall[color];
    case 3: return img_enemyShotBullet[color];
    case 4: return img_enemyShotScale[color];
    case 5: return img_enemyShotDiamond[color];
    case 6: return img_enemyShotMediumOval[color];
    case 7: return img_enemyShotLaser[color];
    }
    return img_enemyShotSmallBall[color];
}

// 双方向リスト接続ヘルパー
static void LinkShot(sEnemyShotSet* pSet, sEnemyShot* pShot) {
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ---------------------------------------------------------
// 弾幕パターン関数
// ---------------------------------------------------------

// 1. 氷花 (射出 -> 停止 -> 開花 -> 落下)
static void ShotIceFlower(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 種弾を8方向に発射
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = i * DX_PI / 4.0;
            pShot->speed = 2.5;
            pShot->kind = GetShotImage(1, COLOR_BLUE); // 中玉・青

            // param_i[0]: 状態 (0=移動, 1=停止, 2=落下)
            pShot->param_i[0] = 0;
            pShot->param_d[0] = 0.0; // タイマー

            LinkShot(pSet, pShot);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 0) { // 移動中
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->param_d[0] += 1.0;
            if (pShot->param_d[0] > 30.0) { // 30Fで停止
                pShot->param_i[0] = 1;
                pShot->speed = 0.0;
                pShot->param_d[0] = 0.0;
            }
        }
        else if (pShot->param_i[0] == 1) { // 停止中
            pShot->param_d[0] += 1.0;
            if (pShot->param_d[0] > 40.0) { // 40F停止後に開花
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                // 花びら弾を12方向に生成
                for (int k = 0; k < 12; k++) {
                    sEnemyShot* pChild = new sEnemyShot;
                    pChild->x = pShot->x;
                    pChild->y = pShot->y;
                    pChild->muki = k * DX_PI / 6.0;
                    pChild->speed = 1.2;
                    pChild->kind = GetShotImage(4, COLOR_CYAN); // 鱗弾・シアン
                    pChild->param_i[0] = 3;
                    LinkShot(pSet, pChild);
                }
                pShot->param_i[0] = 2; // 落下フェーズへ
                pShot->param_d[0] = 0.0;
            }
        }
        else if (pShot->param_i[0] == 2) { // 落下 (散る)
            pShot->y += 0.8;
            pShot->x += sin(pShot->param_d[0] / 10.0) * 0.5; // 揺らぎ
            pShot->param_d[0] += 1.0;
        }
        else if (pShot->param_i[0] == 3) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// 2. 横殴り吹雪 (サインカーブ)
static void ShotBlizzardSide(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 300Fで発射終了 (フェーズ切り替わり対策)
    if (pSet->count < 300) {
        if (pSet->count % 1 == 0) {
            sEnemyShot* pShot = new sEnemyShot;
            int side = (pSet->count / 12) % 2;
            pShot->x = (side == 0) ? -20 : 500;
            pShot->y = 70 + GetRand(340);
            pShot->muki = (side == 0) ? 0 : DX_PI;
            pShot->speed = 2.5 + GetRand(10) / 10.0;
            pShot->kind = GetShotImage(6, COLOR_WHITE); // 中楕円・白
            pShot->margin = 40;

            pShot->param_d[0] = GetRand(100); // 位相オフセット
            pShot->param_d[1] = pShot->y;    // 基準Y

            LinkShot(pSet, pShot);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->param_d[0] += 0.1;
        pShot->y = pShot->param_d[1] + sin(pShot->param_d[0]) * 40.0;

        pShot = pNext;
    }
}

// 3. 氷結螺旋
static void ShotSpiral(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 300Fで発射終了
    if (pSet->count < 300) {
        if (pSet->count % 2 == 0) {
            for (int i = 0; i < 3; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                double angle = (pSet->count / 6.0) * 0.2 + i * 2.0 * DX_PI / 3.0;
                pShot->muki = angle;
                pShot->speed = 1.8;
                pShot->kind = GetShotImage(5, COLOR_BLUE); // 菱形・青
                LinkShot(pSet, pShot);
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 敵本体
// ---------------------------------------------------------

void EnemyPat_EternalForceBlizzard_Qwen()
{
    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // ボス移動 (8の字軌道)
    double t = count / 120.0;
    enemy.x = 240.0 + sin(t) * 150.0;
    enemy.y = 80.0 + sin(t * 2.0) * 40.0;

    // フェーズ管理 (600F = 10秒サイクル)
    int phaseTime = count % 400;

    // Phase 1: 氷花 (0 - 300F)
    if (phaseTime < 200) {
        // 90フレームごとに氷花セットを生成
        if (count % 90 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotIceFlower;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->kind = 0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    // Phase 2: 吹雪 + 螺旋 (300 - 600F)
    else {
        // フェーズ切り替わりの瞬間に持続弾幕セットを生成
        if (phaseTime == 200) {
            // 吹雪セット生成
            sEnemyShotSet* pSetBlizzard = new sEnemyShotSet;
            pSetBlizzard->count = 0;
            pSetBlizzard->patternFunc = ShotBlizzardSide;
            pSetBlizzard->x = enemy.x;
            pSetBlizzard->y = enemy.y;
            pSetBlizzard->kind = 1;
            pSetBlizzard->pEnemyShotHead = new sEnemyShot;
            pSetBlizzard->pEnemyShotHead->prev = pSetBlizzard->pEnemyShotHead;
            pSetBlizzard->pEnemyShotHead->next = pSetBlizzard->pEnemyShotHead;
            pSetBlizzard->prev = enemyShotSetHead.prev;
            pSetBlizzard->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSetBlizzard;
            enemyShotSetHead.prev = pSetBlizzard;

            // 螺旋セット生成
            sEnemyShotSet* pSetSpiral = new sEnemyShotSet;
            pSetSpiral->count = 0;
            pSetSpiral->patternFunc = ShotSpiral;
            pSetSpiral->x = enemy.x;
            pSetSpiral->y = enemy.y;
            pSetSpiral->kind = 2;
            pSetSpiral->pEnemyShotHead = new sEnemyShot;
            pSetSpiral->pEnemyShotHead->prev = pSetSpiral->pEnemyShotHead;
            pSetSpiral->pEnemyShotHead->next = pSetSpiral->pEnemyShotHead;
            pSetSpiral->prev = enemyShotSetHead.prev;
            pSetSpiral->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSetSpiral;
            enemyShotSetHead.prev = pSetSpiral;
        }
    }
}