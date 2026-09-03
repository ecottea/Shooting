// enemyPat_Tmp.cpp
// トマト投げ祭りモチーフ弾幕「トマティーナ・カスケード」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 主弾：上部から落ちるトマト（大玉・赤）→ 一定時間後に破裂して小玉を放射
// ------------------------------------------------------------
static void ShotTomatoCascade(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 投げ音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 2〜4個のトマトを少し散らして落とす
        int num = 2 + GetRand(2);
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(100) - 50);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(30) - 15);
            // ほぼ真下、少し左右にブレ
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(50) - 25) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.4 + GetRand(120) / 100.0;

            // 大きい赤い玉をトマト本体に使用
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 0:赤

            // param_i[0] = 1 なら「破裂するトマト」、param_i[1] = 破裂フレーム
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_i[1] = 38 + GetRand(25);

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動と破裂処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除する可能性があるので先に保存

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 破裂判定（主弾のみ）
        if (pShot->param_i[0] == 1 && pShot->count >= pShot->param_i[1]) {
            // 果汁が飛び散る音（軽め）
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // 小さな赤い玉を放射状にばら撒く
            int n = 9 + GetRand(5); // 9〜13方向
            double baseAng = (GetRand(360)) / 180.0 * DX_PI; // 回転を少しランダムに
            for (int j = 0; j < n; j++) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = pShot->x;
                pNew->y = pShot->y;
                pNew->muki = baseAng + j * (2.0 * DX_PI / n) + (GetRand(16) - 8) / 180.0 * DX_PI;
                pNew->speed = 1.8 + GetRand(160) / 100.0;
                pNew->kind = img_enemyShotSmallBall[0]; // 0:赤の小玉
                pNew->param_i[0] = 0; // 破裂しない

                // 同じショットセットのリストに追加
                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }

            // 本体トマトをリストから外して削除
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }

        pShot = pNext;
    }
}

// ------------------------------------------------------------
// 側面からの投げ合い弾（中玉・赤、破裂なし）
// ------------------------------------------------------------
static void ShotTomatoSideThrow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 3〜5発を少し上下に散らして投げる
        int num = 3 + GetRand(2);
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(80) - 40);

            // 画面中央付近を狙う方向に少しバラつきを持たせる
            double targetX = 240.0 + (GetRand(120) - 60);
            double targetY = 280.0 + (GetRand(100) - 50);
            pEnemyShot->muki = atan2(targetY - pEnemyShot->y, targetX - pEnemyShot->x);
            pEnemyShot->speed = 2.8 + GetRand(100) / 100.0;

            // 中玉の赤
            pEnemyShot->kind = img_enemyShotMediumBall[0];
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 単純移動のみ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Tomatina_Grok()
{
    static int muki;
    static int sidePhase;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        sidePhase = 0;
    }
    else {
        // 上部をゆっくり左右に移動
        enemy.x += 1.1 * (double)muki;
        if (enemy.x > 390.0) {
            enemy.x = 390.0;
            muki = -1;
        }
        else if (enemy.x < 90.0) {
            enemy.x = 90.0;
            muki = 1;
        }
    }

    // ---- メインのトマト落下（カスケード）----
    // 定期的に上部から落とす
    if (count % 22 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTomatoCascade;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ---- 左右からの投げ合い ----
    // 左右交互に投げる
    if (count % 36 == 8) {
        // 左側から
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTomatoSideThrow;
        pEnemyShotSet->x = 10.0;
        pEnemyShotSet->y = 90.0 + GetRand(140);
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else if (count % 36 == 26) {
        // 右側から
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTomatoSideThrow;
        pEnemyShotSet->x = 470.0;
        pEnemyShotSet->y = 90.0 + GetRand(140);
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}