#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：マシンガン連射掃射
static void ShotMachineGun(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射位置を敵に追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 12.0;

    // 基本の狙い方向（プレイヤー方向）を毎フレーム更新
    pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

    // --- メイン連射（マシンガン本体） ---
    // 2フレームに1回、1〜2発の銃弾を発射
    if (pEnemyShotSet->count % 2 == 0) {
        // 軽い連射音（重複再生防止）
        if (pEnemyShotSet->count % 6 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        int shotNum = 1 + GetRand(1); // 1か2発
        for (int i = 0; i < shotNum; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 狙い方向 ± 小さなブレ（マシンガンの反動感）
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(16) - 8) / 180.0 * DX_PI;
            pEnemyShot->speed = 4.2 + GetRand(80) / 100.0; // 速め
            // 銃弾（細長い弾）・色は橙寄りで熱をイメージ
            pEnemyShot->kind = img_enemyShotBullet[8]; // 橙
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 掃射アクセント（左右端や一定間隔で中玉を扇状に） ---
    // 30フレームごとに扇状弾を追加
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 30 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 5発の扇状
        for (int i = -2; i <= 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * 12.0 / 180.0 * DX_PI;
            pEnemyShot->speed = 3.0 + GetRand(50) / 100.0; // 少し遅め
            // 中玉・色は赤で重みを出す
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_MachineGun_Grok()
{
    static int muki; // 左右移動方向

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;

        // マシンガン用ショットセットを1つ作成（以降ずっと連射）
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMachineGun;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 左右にゆっくり掃射する動き
        enemy.x += 1.1 * (double)muki;
        // 端に近づいたら反転（画面幅480、余白を考慮）
        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            muki = 1;
        }
        else if (enemy.x > 400.0) {
            enemy.x = 400.0;
            muki = -1;
        }
    }
}