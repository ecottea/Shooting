// enemyPat_SpiderWeb.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：クモの巣（Spider Web）
static void ShotSpiderWeb(sEnemyShotSet* pEnemyShotSet)
{
    // --- 1. 弾の生成フェーズ（0〜60フレーム） ---

    // 【放射状の骨組み（Spokes）】を生成
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 80フレーム後に特定の距離（240, 195, 150, 105, 60）でピタリと停止するように速度を計算してあります
        double spokeSpeeds[] = { 0.75, 1.3125, 1.875, 2.4375, 3.0, 285. / 80, 330. / 80, 375. / 80, 420. / 80, 465. / 80 };

        for (int i = 0; i < 16; i++) {
            double angle = i * (DX_PI * 2.0 / 16.0);
            for (int j = 0; j < 10; j++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = spokeSpeeds[j];
                pEnemyShot->kind = img_enemyShotDiamond[6]; // 白色の菱形弾（糸の張りを表現）

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 【同心円状の糸（Rings）】を生成
    // 15フレームごとに時間差で射出することで、80フレーム目でちょうど骨組みと交差します
    if (pEnemyShotSet->count % 15 == 0 && pEnemyShotSet->count <= 60) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 32; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * (DX_PI * 2.0 / 32.0); // 32方向で密度の高い円を描く
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白色の小玉（粘着糸を表現）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 2. 弾の移動・状態遷移フェーズ ---
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShotSet->count < 80) {
            // 【展開中】指定の位置まで放射状に広がる
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        else if (pEnemyShotSet->count == 80) {
            // 【完成・停止】クモの巣の形を維持してプレイヤーの退路を塞ぐ
            pEnemyShot->speed = 0.0;
        }
        else if (pEnemyShotSet->count > 160) {
            // 【崩壊・強襲】一斉に赤色に変わり、プレイヤーに向かって襲い掛かる
            if (pEnemyShotSet->count == 161) {
                if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                // プレイヤーの方向を計算
                double angleToPlayer = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                // 完全に自機狙いにすると塊になってしまうため、ランダムなブレ（-20度〜20度）を持たせる
                pEnemyShot->muki = angleToPlayer + (GetRand(40) - 20) / 180.0 * DX_PI;
                // 速度をランダムに再設定して波状攻撃にする（1.5 ～ 3.0）
                pEnemyShot->speed = (150 + GetRand(150)) / 100.0;
                // 危険を示す赤玉に変更
                pEnemyShot->kind = img_enemyShotSmallBall[0];
            }

            // 攻撃開始
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SpiderWeb_Gemini()
{
    static int moveDir;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;  // 画面上部
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
    }
    else {
        // ボスは画面上部をゆったりと左右に移動し続ける
        enemy.x += 1.0 * (double)moveDir;
        if (enemy.x > 380.0) moveDir = -1;
        if (enemy.x < 100.0) moveDir = 1;
    }

    // 240フレームごとに新しいクモの巣の基点を設置（ボスの移動軌跡に沿ってトラップが置かれる）
    if (count % 240 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiderWeb;

        // ボスの現在位置にクモの巣の中心を固定（追尾させないことで空間を制圧）
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;

        // ダミーのヘッドノード初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに繋ぐ
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}