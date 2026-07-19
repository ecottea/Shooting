// TwinBitingBlueMoons.cpp
// 双咬ノ蒼月（そうこうのそうげつ）
// 戻り弾幕パターン：月を敵弾として表現し、描画問題を解決

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotTwinMoons(sEnemyShotSet* pEnemyShotSet);

//----------------------------------------------------------
// 敵本体のパターン
//----------------------------------------------------------
void EnemyPat_Reverse_DeepSeek()
{
    static double moveMuki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        moveMuki = 1.0;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTwinMoons;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        enemy.x += 0.98 * moveMuki;
        if (count % 120 == 60) moveMuki *= -1.0;
    }
}

//----------------------------------------------------------
// 双咬ノ蒼月 パターン関数（月は敵弾として管理）
//----------------------------------------------------------
static void ShotTwinMoons(sEnemyShotSet* pEnemyShotSet)
{
    // 初回初期化
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_d[0] = 0.0;        // 左月の角度
        pEnemyShotSet->param_d[1] = DX_PI;      // 右月の角度
        pEnemyShotSet->param_i[0] = 0;          // 発射タイマー（即発射）
        pEnemyShotSet->param_i[1] = 0;          // 次に撃つ月（0:左,1:右）
    }

    // 月の回転
    const double rotSpeed = 0.01;
    pEnemyShotSet->param_d[0] += rotSpeed;
    pEnemyShotSet->param_d[1] += rotSpeed;
    while (pEnemyShotSet->param_d[0] > 2.0 * DX_PI) pEnemyShotSet->param_d[0] -= 2.0 * DX_PI;
    while (pEnemyShotSet->param_d[1] > 2.0 * DX_PI) pEnemyShotSet->param_d[1] -= 2.0 * DX_PI;

    const double radius = 80.0;

    // ----- 月（特殊敵弾）の存在チェックと補充 -----
    bool leftExist = false, rightExist = false;
    sEnemyShot* pCheck = pEnemyShotSet->pEnemyShotHead->next;
    while (pCheck != pEnemyShotSet->pEnemyShotHead) {
        if (pCheck->param_i[0] == 2) {   // 月の目印
            if (pCheck->param_i[1] == 0) leftExist = true;
            if (pCheck->param_i[1] == 1) rightExist = true;
        }
        pCheck = pCheck->next;
    }

    // 足りない月を追加
    for (int side = 0; side <= 1; ++side) {
        if ((side == 0 && leftExist) || (side == 1 && rightExist)) continue;

        sEnemyShot* pMoon = new sEnemyShot;
        pMoon->kind = img_enemyShotLargeBall[3];   // シアン中玉 → 蒼い光球
        pMoon->speed = 0.0;
        pMoon->count = 0;
        pMoon->margin = 9999.0;                     // 画面外に出ても消えない
        pMoon->param_i[0] = 2;                      // 月であるマーク
        pMoon->param_i[1] = side;                   // 0:左, 1:右
        // 位置は後で更新

        // リストに追加
        pMoon->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pMoon->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pMoon;
        pEnemyShotSet->pEnemyShotHead->prev = pMoon;
    }

    // ----- レーザー発射タイマー -----
    if (pEnemyShotSet->param_i[0] > 0)
        pEnemyShotSet->param_i[0]--;

    if (pEnemyShotSet->param_i[0] == 0) {
        int moonIdx = pEnemyShotSet->param_i[1];
        double angle = (moonIdx == 0) ? pEnemyShotSet->param_d[0] : pEnemyShotSet->param_d[1];
        double moonX = enemy.x + radius * cos(angle);
        double moonY = enemy.y + radius * sin(angle);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = moonX;
        pShot->y = moonY;
        pShot->muki = atan2(player.y - moonY, player.x - moonX);
        pShot->speed = 4.0;
        pShot->kind = img_enemyShotBullet[3];    // 往路：シアン銃弾
        pShot->count = 0;
        pShot->margin = 20.0;

        pShot->param_d[0] = moonX;
        pShot->param_d[1] = moonY;
        pShot->param_d[2] = 480.0 * 0.8;
        pShot->param_i[0] = 0;   // 往路
        pShot->param_i[1] = moonIdx;

        pShot->margin = 480;

        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;

        pEnemyShotSet->param_i[0] = 3;               // 次発まで0.8秒
        pEnemyShotSet->param_i[1] = 1 - moonIdx;      // 左右交互
    }

    // ----- 全敵弾の更新 -----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;

        if (pShot->param_i[0] == 2) {
            // 月：位置を追随させるだけ
            int side = pShot->param_i[1];
            double angle = (side == 0) ? pEnemyShotSet->param_d[0] : pEnemyShotSet->param_d[1];
            pShot->x = enemy.x + radius * cos(angle);
            pShot->y = enemy.y + radius * sin(angle);
        }
        else if (pShot->param_i[0] == 0) {
            // 往路レーザー
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            double dx = pShot->x - pShot->param_d[0];
            double dy = pShot->y - pShot->param_d[1];
            double dist = sqrt(dx * dx + dy * dy);
            if (dist >= pShot->param_d[2]) {
                // 反転、復路へ
                pShot->param_i[0] = 1;
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 5.0;
                pShot->kind = img_enemyShotBullet[8];   // 復路：橙

                if (!CheckSoundMem(sound_enemyShot_medium))PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 復路レーザー
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = nextShot;
    }
}