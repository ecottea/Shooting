#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：残像花火
static void ShotZanshoHanabi(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 出現時：予告音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 30F後(0.5秒)に爆発
    if (pEnemyShotSet->count == 30) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 1. 外側：12方向の遅い中玉
        for (int i = 0; i < 12; i++) for (int j = 0; j < 5; j++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (DX_PI * 2.0 / 12.0) * (i + j % 2 * 0.5);
            pEnemyShot->speed = 1.5 + j * 0.5;
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白・中玉 7.0x7.0
            pEnemyShot->count = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 2. 内側：自機狙い5wayの速い小玉 (0.2秒後の演出は移動で表現するため同時生成)
        // プレイヤー位置は爆発瞬間のものを使用
        double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int k = -2; k <= 2; k++) for (int j = -5; j <= 5; j++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // ±12度ずつ 5way : -24, -12, 0, 12, 24
            pEnemyShot->muki = baseMuki + (k * 12.0 / 180.0 * DX_PI);
            pEnemyShot->speed = 3.5 + j * 0.3;
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤・小玉 2.5x2.5
            pEnemyShot->count = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動：countのインクリメントと画面外削除はメイン側で行うので移動のみ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 30F以前はまだ弾がない / 爆発後の弾だけ動かす
        if (pEnemyShotSet->count >= 30) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体：被弾で逃げるボス
void EnemyPat_Escape_MetaAI()
{
    static int prevHp;
    static int cooldown; // 連発防止クールダウン 0.8秒=48F
    static int escapeFrames; // 逃走残りフレーム
    static double destX, destY;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 20;
        prevHp = enemy.hp;
        cooldown = 0;
        escapeFrames = 0;
        destX = enemy.x;
        destY = enemy.y;
    }
    else {
        if (cooldown > 0) cooldown--;

        // 逃走中の移動
        if (escapeFrames > 0) {
            enemy.x += (destX - enemy.x) * 0.3;
            enemy.y += (destY - enemy.y) * 0.3;
            escapeFrames--;
            if (escapeFrames == 0) {
                enemy.x = destX;
                enemy.y = destY;
            }
        }
        else {
            // 通常時はゆっくり左右に揺れる
            enemy.x += 0.6 * sin(count * 0.03);
        }

        // 被弾検知：HPが減った瞬間に発動
        if (enemy.hp < prevHp && cooldown <= 0) {
            // 1. 最もプレイヤーから遠い上部4隅を選ぶ
            double candidates[4][2] = { {60.0, 40.0}, {420.0, 40.0}, {80.0, 110.0}, {400.0, 110.0} };
            int bestIdx = 0;
            double bestDist = -1.0;
            for (int i = 0; i < 4; i++) {
                double dx = candidates[i][0] - player.x;
                double dy = candidates[i][1] - player.y;
                double d2 = dx * dx + dy * dy;
                if (d2 > bestDist) {
                    bestDist = d2;
                    bestIdx = i;
                }
            }
            destX = candidates[bestIdx][0] + (GetRand(40) - 20); // GetRandは0..40なので -20..+20
            destY = candidates[bestIdx][1] + (GetRand(20) - 10);
            if (destX < 50.0) destX = 50.0;
            if (destX > 430.0) destX = 430.0;
            if (destY < 30.0) destY = 30.0;
            if (destY > 130.0) destY = 130.0;

            // 2. 現在地に残像弾幕セットを生成
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotZanshoHanabi;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = 0;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            // 3. 逃走開始
            escapeFrames = 18;
            cooldown = 48; // 0.8秒無敵
            prevHp = enemy.hp;
        }
        else if (enemy.hp < prevHp) {
            // クールダウン中でもHPは追従させて次回検知を正確にする
            prevHp = enemy.hp;
        }
    }
}