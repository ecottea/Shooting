// enemyPat_firework.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕：花火「昇華の瞬間」
// ------------------------------------------------------------
// 【3段階の流れ】
//  0-60  : 打ち上げ（レーザー予告線5本＋先端の蓄積玉）
//  61-150: 開花（5地点で同時爆発。誘導弾＋24way＋ランダム火花）
// 151-240: 残光（火花弾を一斉誘導化＋落下粉＋画面下端での二次開花）
// ------------------------------------------------------------

static void ShotFireworkAscensionBloom(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    sEnemyShot* pNewShot;

    // ============================================================
    // 初期化：打ち上げフェーズ
    // ============================================================
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = 0; // 現在フェーズ（0=打ち上げ）
        pEnemyShotSet->param_i[1] = 0; // 開花生成済みフラグ
        pEnemyShotSet->param_i[2] = 0; // 残光処理済みフラグ

        // 予告音（チャージ音）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 5本の予告線（短レーザー：白）と先端の蓄積玉（小玉：白）
        for (int i = 0; i < 5; i++) {
            double angle = -DX_PI / 2.0 + (i - 2) * 0.12; // 上方向を中心に扇状
            double sx = pEnemyShotSet->x + (i - 2) * 25.0;
            double sy = pEnemyShotSet->y;

            // 予告線本体（短レーザー 64.0x4.0、色：白[6]）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = sx;
            pEnemyShot->y = sy;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotLaser[6]; // 白
            pEnemyShot->param_i[0] = 100; // 識別：予告線
            pEnemyShot->param_i[1] = i;   // インデックス
            pEnemyShot->margin = 64;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 先端の蓄積玉（小玉 2.5x2.5、色：白[6]）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = sx;
            pEnemyShot->y = sy;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
            pEnemyShot->param_i[0] = 101; // 識別：先端玉
            pEnemyShot->param_i[1] = i;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ============================================================
    // フェーズ遷移：打ち上げ → 開花（count 60）
    // ============================================================
    if (pEnemyShotSet->param_i[0] == 0 && pEnemyShotSet->count >= 60 && pEnemyShotSet->param_i[1] == 0) {
        pEnemyShotSet->param_i[0] = 1; // 開花フェーズへ
        pEnemyShotSet->param_i[1] = 1; // 開花済み

        // 予告線・先端玉を高速で画面外に追い出す（メインルーチンで消去される）
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 100 || pShot->param_i[0] == 101) {
                pShot->speed = 20.0;
            }
            pShot = pShot->next;
        }

        // 開花効果音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 5地点で同時開花
        for (int i = 0; i < 5; i++) {
            double angle = -DX_PI / 2.0 + (i - 2) * 0.12;
            double bx = pEnemyShotSet->x + (i - 2) * 25.0 + 60.0 * cos(angle);
            double by = pEnemyShotSet->y + 60.0 * sin(angle);

            // --- 中心部：自機狙い誘導弾（大玉 20.0x20.0、色：赤[0]）3発 ---
            for (int j = 0; j < 3; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = bx;
                pEnemyShot->y = by;
                double aim = atan2(player.y - by, player.x - bx);
                pEnemyShot->muki = aim + (GetRand(20) - 10) / 180.0 * DX_PI;
                pEnemyShot->speed = 2.0 + j * 0.4;
                pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤
                pEnemyShot->param_i[0] = 200; // 識別：誘導弾

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // --- 中間層：24方向通常弾（中玉 7.0x7.0、色：黄[1]） ---
            // ※橙[8]を使いたい場合は img_enemyShotMediumBall[8] として直接指定可能
            for (int j = 0; j < 24; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = bx;
                pEnemyShot->y = by;
                pEnemyShot->muki = j * DX_PI / 12.0;
                pEnemyShot->speed = 1.5;
                pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄
                pEnemyShot->param_i[0] = 201; // 識別：通常弾

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // --- 外縁部：ランダム火花弾（小玉 2.5x2.5、色：黄[1]）20発 ---
            for (int j = 0; j < 20; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = bx;
                pEnemyShot->y = by;
                pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
                pEnemyShot->speed = 0.5 + GetRand(150) / 100.0;
                pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄
                pEnemyShot->param_i[0] = 202; // 識別：火花弾
                pEnemyShot->param_i[1] = 0;   // 誘導化フラグ（0=未誘導）

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ============================================================
    // フェーズ遷移：開花 → 残光（count 150）
    // ============================================================
    if (pEnemyShotSet->param_i[0] == 1 && pEnemyShotSet->count >= 150 && pEnemyShotSet->param_i[2] == 0) {
        pEnemyShotSet->param_i[0] = 2; // 残光フェーズへ
        pEnemyShotSet->param_i[2] = 1; // 残光処理済み

        // まだ画面内に残っている火花弾を一斉に自機へ誘導化
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 202 && pShot->param_i[1] == 0) {
                pShot->param_i[1] = 1; // 誘導化済み
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 2.0;
            }
            pShot = pShot->next;
        }
    }

    // ============================================================
    // 残光フェーズ：落下粉の生成（count 150〜210、10フレーム毎）
    // ============================================================
    if (pEnemyShotSet->param_i[0] == 2 && pEnemyShotSet->count >= 150 && pEnemyShotSet->count <= 210) {
        if ((pEnemyShotSet->count - 150) % 10 == 0) {
            for (int i = 0; i < 5; i++) {
                double angle = -DX_PI / 2.0 + (i - 2) * 0.12;
                double bx = pEnemyShotSet->x + (i - 2) * 25.0 + 60.0 * cos(angle);
                double by = pEnemyShotSet->y + 60.0 * sin(angle);

                // 落下粉（小玉 2.5x2.5、色：緑[2]）3発ずつ
                for (int j = 0; j < 3; j++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = bx + GetRand(30) - 15;
                    pEnemyShot->y = by + GetRand(20);
                    pEnemyShot->muki = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
                    pEnemyShot->speed = 1.0 + GetRand(100) / 100.0;
                    pEnemyShot->kind = img_enemyShotSmallBall[2]; // 緑
                    pEnemyShot->param_i[0] = 203; // 識別：落下粉

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }

    // ============================================================
    // 弾の更新処理
    // ============================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // --- 落下粉が画面下端に到達したら二次開花 ---
        if (pShot->param_i[0] == 203 && pShot->y > 470.0) {
            // 斜め4方向に二次開花（小玉 2.5x2.5、色：青[4]）
            for (int j = 0; j < 4; j++) {
                pNewShot = new sEnemyShot;
                pNewShot->x = pShot->x;
                pNewShot->y = 470.0;
                pNewShot->muki = j * DX_PI / 2.0 + DX_PI / 4.0; // 斜め4方向
                pNewShot->speed = 1.5;
                pNewShot->kind = img_enemyShotSmallBall[4]; // 青
                pNewShot->param_i[0] = 204; // 識別：二次開花弾

                pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
            }
            // 元の落下粉は画面外に追い出す（メインルーチンの自動消去で削除される）
            pShot->y = 9999.0;
            pShot->speed = 0.0;
        }

        // 座標更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Firework_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 180フレームごとに花火弾幕を生成
    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFireworkAscensionBloom;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0;
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