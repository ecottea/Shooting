#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  補助関数：弾をセットに追加
// ============================================================
static void AddShotToSet(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ============================================================
//  卍型レーザー（青小卍・赤大卍の無限循環）
// ============================================================
static void ShotMantsu(sEnemyShotSet* pSet)
{
    // 初回：卍を構成する短レーザー弾を生成
    if (pSet->pEnemyShotHead->next == pSet->pEnemyShotHead) {
        for (int arm = 0; arm < 4; arm++) {
            for (int rad = 0; rad < 3; rad++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotLaser[3];
                pShot->param_i[0] = arm; // 腕インデックス 0～3
                pShot->param_i[1] = rad; // 半径インデックス 0～2
                pShot->margin = 240;
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
        pSet->param_i[0] = 0;   // 0:青小逆回転, 1:赤大順回転
        pSet->param_d[0] = DX_PI / 4.0; // 現在の基準角度
    }

    // 120フレームごとに青小 ⇔ 赤大を切替
    if (pSet->count > 0 && pSet->count % 120 == 0) {
        pSet->param_i[0] = 1 - pSet->param_i[0];

        if (pSet->param_i[0] == 1) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        else {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }

    // 回転角度更新
    if (pSet->param_i[0] == 0) {
        pSet->param_d[0] -= 0.02; // 青小：逆时针
    }
    else {
        pSet->param_d[0] += 0.03; // 赤大：顺时针
    }

    // 各レーザー弾の位置・向き・種類を更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int arm = pShot->param_i[0];
        int rad = pShot->param_i[1];
        double baseAngle = pSet->param_d[0] + arm * DX_PI / 2.0;

        double radius;
        int color;
        if (pSet->param_i[0] == 0) {
            // 青小卍
            radius = (rad + 1) * 40.0;
            color = 3; // シアン
            if (rad == 2) {
                // 小さい時は第3段を中心に重ねて隠す
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = baseAngle;
                pShot->kind = img_enemyShotLaser[color];
                pShot = pShot->next;
                continue;
            }
        }
        else {
            // 赤大卍
            radius = (rad + 1) * 60.0;
            color = 0; // 赤
        }

        pShot->x = pSet->x + radius * cos(baseAngle);
        pShot->y = pSet->y + radius * sin(baseAngle);
        pShot->muki = baseAngle;
        pShot->kind = img_enemyShotLaser[color];
        pShot = pShot->next;
    }
}

// ============================================================
//  蝶弾（自機狙い＋折り返し）
// ============================================================
static void ShotButterfly(sEnemyShotSet* pSet)
{
    // 20フレームごとに3発の蝶弾を自機狙いで発射
    if (pSet->count % 20 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            double angle = atan2(player.y - pSet->y, player.x - pSet->x);
            angle += (i - 1) * 0.2; // -0.2, 0, +0.2 の拡散

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 3.0 + GetRand(100) / 100.0; // 3.0～4.0
            pShot->kind = img_enemyShotScale[5];        // マゼンタの鱗弾
            pShot->param_i[0] = 0; // 0:行き, 1:帰り
            pShot->margin = 120;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 移動と折り返し処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0 && pShot->count >= 60) {
            pShot->muki += DX_PI; // 進行方向を反転
            pShot->param_i[0] = 1;
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体：罔両「禅寺に棲む妖蝶」
// ============================================================
void EnemyPat_Zendera_DeepSeek()
{
    static sEnemyShotSet* pMantsuSet = nullptr;
    static sEnemyShotSet* pButterflySet = nullptr;

    if (count == 1) {
        // 画面は 480x480。八雲紫を上方やや中央に配置
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しない

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 卍レーザー用セット
        pMantsuSet = new sEnemyShotSet;
        pMantsuSet->count = 0;
        pMantsuSet->patternFunc = ShotMantsu;
        pMantsuSet->x = enemy.x;
        pMantsuSet->y = enemy.y;
        pMantsuSet->kind = 100;
        pMantsuSet->pEnemyShotHead = new sEnemyShot;
        pMantsuSet->pEnemyShotHead->prev = pMantsuSet->pEnemyShotHead;
        pMantsuSet->pEnemyShotHead->next = pMantsuSet->pEnemyShotHead;
        pMantsuSet->prev = enemyShotSetHead.prev;
        pMantsuSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMantsuSet;
        enemyShotSetHead.prev = pMantsuSet;

        // 蝶弾用セット
        pButterflySet = new sEnemyShotSet;
        pButterflySet->count = 0;
        pButterflySet->patternFunc = ShotButterfly;
        pButterflySet->x = enemy.x;
        pButterflySet->y = enemy.y;
        pButterflySet->kind = 101;
        pButterflySet->pEnemyShotHead = new sEnemyShot;
        pButterflySet->pEnemyShotHead->prev = pButterflySet->pEnemyShotHead;
        pButterflySet->pEnemyShotHead->next = pButterflySet->pEnemyShotHead;
        pButterflySet->prev = enemyShotSetHead.prev;
        pButterflySet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pButterflySet;
        enemyShotSetHead.prev = pButterflySet;
    }

    // セットの中心を敵の位置に追従させる
    if (pMantsuSet) {
        pMantsuSet->x = enemy.x;
        pMantsuSet->y = enemy.y;
    }
    if (pButterflySet) {
        pButterflySet->x = enemy.x;
        pButterflySet->y = enemy.y;
    }

    // 敵を左右にゆっくり動かす場合（必要なら有効化）
    // enemy.x = 240.0 + 40.0 * sin(count * 0.01);
    // enemy.y = 160.0 + 20.0 * cos(count * 0.013);
}