#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ミュラーリヤー錯視
// 同じ長さの線分に「内向き」と「外向き」の矢羽根を付け、
// 見た目の長さが違って見える錯視を弾幕として表現する。
// ============================================================

static const double MULLER_LINE_HALF = 84.0;
static const double MULLER_LINE_STEP = 12.0;
static const double MULLER_WING_ANGLE = DX_PI / 6.0;
static const double MULLER_WING_STEP = 9.0;
static const double MULLER_FALL_SPEED = 1.7;

// ------------------------------------------------------------
// 弾をリストへ追加
// ------------------------------------------------------------
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

    return pEnemyShot;
}

// ------------------------------------------------------------
// ミュラーリヤー錯視の線分を作成
// outward = true なら外向き矢印
// outward = false なら内向き矢印
// ------------------------------------------------------------
static void AddMullerLyerFigure(sEnemyShotSet* pEnemyShotSet, double centerX, double centerY, bool outward)
{
    // 線分本体
    // 銃弾を横向きに並べて一本の細い線に見せる。
    for (int i = 0; i < 15; i++) {
        double x = centerX - MULLER_LINE_HALF + MULLER_LINE_STEP * i;
        sEnemyShot* pEnemyShot = AddShot(
            pEnemyShotSet,
            x,
            centerY,
            0.0,
            0.0,
            img_enemyShotBullet[6]
        );

        // 速度0の錯視構成弾はパターン側で落下させる。
        pEnemyShot->param_i[0] = 1;
    }

    // 左端と右端の矢羽根
    const double tipX[2] = {
        centerX - MULLER_LINE_HALF,
        centerX + MULLER_LINE_HALF
    };

    for (int side = 0; side < 2; side++) {
        bool left = (side == 0);

        // 外向き:
        // 左端 -> 左方向
        // 右端 -> 右方向
        //
        // 内向き:
        // 左端 -> 右方向
        // 右端 -> 左方向
        double baseAngle;

        if (outward) {
            baseAngle = left ? DX_PI : 0.0;
        }
        else {
            baseAngle = left ? 0.0 : DX_PI;
        }

        for (int arm = -1; arm <= 1; arm += 2) {
            double wingAngle = baseAngle + MULLER_WING_ANGLE * arm;

            for (int step = 1; step <= 5; step++) {
                double distance = MULLER_WING_STEP * step;

                // 矢羽根を線端から伸ばす。
                double x = tipX[side] + cos(wingAngle) * distance;
                double y = centerY + sin(wingAngle) * distance;

                int wingColor = outward
                    ? img_enemyShotScale[5]  // マゼンタ
                    : img_enemyShotScale[3]; // シアン

                sEnemyShot* pEnemyShot = AddShot(
                    pEnemyShotSet,
                    x,
                    y,
                    wingAngle,
                    0.0,
                    wingColor
                );

                // muki は移動方向ではなく、矢羽根そのものの向きとして利用。
                // 実際の移動は下のパターン処理で行う。
                pEnemyShot->param_i[0] = 1;
            }
        }
    }
}

// ------------------------------------------------------------
// 錯視を構成する弾幕
// ------------------------------------------------------------
static void ShotMullerLyer(sEnemyShotSet* pEnemyShotSet)
{
    // 初期生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) {
            StopSoundMem(sound_enemyShot_medium);
        }
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 上段：外向き矢印
        AddMullerLyerFigure(
            pEnemyShotSet,
            pEnemyShotSet->x,
            pEnemyShotSet->y - 58.0,
            true
        );

        // 下段：内向き矢印
        AddMullerLyerFigure(
            pEnemyShotSet,
            pEnemyShotSet->x,
            pEnemyShotSet->y + 58.0,
            false
        );
    }

    // 一定時間後にプレイヤー狙いの弾を追加。
    // 錯視を眺めているだけでは済まないようにする。
    if (pEnemyShotSet->count == 34) {
        double sourceY;

        // 波ごとに狙う段を交互に変える。
        if (pEnemyShotSet->kind % 2 == 0) {
            sourceY = pEnemyShotSet->y - 58.0;
        }
        else {
            sourceY = pEnemyShotSet->y + 58.0;
        }

        double muki = atan2(
            player.y - sourceY,
            player.x - pEnemyShotSet->x
        );

        if (CheckSoundMem(sound_enemyShot_light)) {
            StopSoundMem(sound_enemyShot_light);
        }
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        AddShot(
            pEnemyShotSet,
            pEnemyShotSet->x,
            sourceY,
            muki,
            2.8,
            img_enemyShotMediumOval[5]
        );
    }

    // 錯視部分は全て同じ速度で下へ移動させる。
    // muki は矢羽根の描画方向に使うため、speedは0のままにする。
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pShot->y += MULLER_FALL_SPEED;
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_MullerLyer_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200;

        muki = 1;
        shot_count = 0;
    }
    else {
        // ボスは左右へゆっくり移動
        enemy.x += 1.8 * (double)muki;

        if (enemy.x >= 360.0) {
            enemy.x = 360.0;
            muki = -1;
        }
        else if (enemy.x <= 120.0) {
            enemy.x = 120.0;
            muki = 1;
        }
    }

    // ミュラーリヤー錯視の比較図形を一定間隔で投下
    if (count % 53 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMullerLyer;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 28.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
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