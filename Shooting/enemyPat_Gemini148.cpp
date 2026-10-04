#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：四天王奥義 「三歩必殺」
// ============================================================
static void ShotSanpoHissatsu(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --------------------------------------------------------
    // 0. 予告・溜め処理 (t = 1)
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    // 1. 【一歩目】(t = 60): ボス周囲に展開する青・シアンの高密度菱形弾幕
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 60) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        int way = 36+10;
        int ring = 3;

        for (int r = 0; r < ring; r++) {
            double speed = 1.6 + r * 0.3;
            for (int i = 0; i < way; i++) {
                double angle = baseAngle + (2.0 * DX_PI / way) * i + (r * 0.04);

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = speed;
                // 3:シアン, 4:青 の菱形弾
                pEnemyShot->kind = (r % 2 == 0) ? img_enemyShotDiamond[3] : img_enemyShotDiamond[4];
                pEnemyShot->param_i[0] = 1; // 一歩目の識別フラグ

                // リストへの追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // --------------------------------------------------------
    // 2. 【二歩目】(t = 110): 中間エリアへ広がるシアン・白の中楕円弾の波
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 110) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        int way = 44+10;
        int ring = 4;

        for (int r = 0; r < ring; r++) {
            double speed = 2.2 + r * 0.4;
            for (int i = 0; i < way; i++) {
                // GetRand(x) は 0～x の整数を返すため、わずかに角度をばらけさせる計算
                double randOffset = (GetRand(20) - 10) / 1000.0;
                double angle = baseAngle + (2.0 * DX_PI / way) * (i + 0.5) + randOffset;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = speed;
                // 3:シアン, 6:白 の中楕円弾
                pEnemyShot->kind = (r == 0) ? img_enemyShotMediumOval[3] : img_enemyShotMediumOval[6];
                pEnemyShot->param_i[0] = 2; // 二歩目の識別フラグ

                // リストへの追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // --------------------------------------------------------
    // 3. 【三歩目】(t = 160): 画面全体を覆い尽くす赤・橙の最密超高速波
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 160) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        int way = 56+10;
        int ring = 5;

        for (int r = 0; r < ring; r++) {
            double speed = 2.8 + r * 0.45;
            for (int i = 0; i < way; i++) {
                double angle = baseAngle + (2.0 * DX_PI / way) * i + (r * 0.02);

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = speed;
                // 0:赤 (鱗弾), 8:橙 (菱形弾)
                pEnemyShot->kind = (r % 2 == 0) ? img_enemyShotScale[0] : img_enemyShotDiamond[8];
                pEnemyShot->param_i[0] = 3; // 三歩目の識別フラグ

                // リストへの追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // --------------------------------------------------------
    // 4. 弾の移動制御
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 一歩目の菱形弾にじわじわと加速がかかる演出（密度の圧迫感を演出）
        if (pShot->param_i[0] == 1 && pShot->count > 20 && pShot->count < 80) {
            pShot->speed += 0.012;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_SanpoHissatsu_Gemini()
{
    static int muki;
    static int cycle_count;

    if (count == 1) {
        // ゲーム画面: 480x480
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200; // 規定値200
        muki = 1;
        cycle_count = 0;
    }
    else {
        // 攻撃合間の移動制御（弾幕展開時は威厳を保つため中央付近に定着）
        int cycle_time = count % 320;
        if (cycle_time > 220) {
            enemy.x += 0.7 * (double)muki;
            if (enemy.x < 140.0) { enemy.x = 140.0; muki = 1; }
            if (enemy.x > 340.0) { enemy.x = 340.0; muki = -1; }
        }
    }

    // 320フレーム（約5.3秒）周期で「三歩必殺」の弾幕セットを生成
    if (count % 320 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSanpoHissatsu;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = cycle_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}