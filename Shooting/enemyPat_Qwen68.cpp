// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// プロトタイプ宣言
static void ShotFirework(sEnemyShotSet* pEnemyShotSet);

// 敵本体のパターン
void EnemyPat_Firework_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 左右移動 (サイン波)
        enemy.x = 240.0 + sin(count * 0.02) * 60.0;
    }

    // 花火打ち上げトリガー (約3秒に1回)
    if (count % 180 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFirework;

        // 画面下部から打ち上げるため、Y座標は画面外下端に設定
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = 480.0 + 40.0; // 画面外
        pEnemyShotSet->muki = -DX_PI / 2.0; // 上向き
        pEnemyShotSet->kind = GetRand(7); // 花火の色ベース (0-7)

        // ダミーヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}

// 弾幕：花火「三尺玉・三段咲き」
static void ShotFirework(sEnemyShotSet* pSet)
{
    // param_i[0]: Phase (0:上昇, 1:爆発・重力適用)

    sEnemyShot* pShot;

    if (pSet->count == 0) {
        // --- 1. 親弾（花火玉）の生成 ---
        pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = -DX_PI / 2.0; // 上
        pShot->speed = 5.0;
        pShot->margin = 50;

        // 親弾識別用フラグ (99)
        pShot->param_i[0] = 99;

        // 見た目は大玉、色は黄(1)
        pShot->kind = img_enemyShotLargeBall[1];

        // リストへ追加
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;

        // 打ち上げ音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // --- 毎フレームの更新処理 ---
    sEnemyShot* pCurrent = pSet->pEnemyShotHead->next;
    while (pCurrent != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pCurrent->next; // 削除に備えて次を保持

        if (pSet->param_i[0] == 0) {
            // Phase 0: 上昇フェーズ
            if (pCurrent->param_i[0] == 99) {
                // 親弾の移動
                pCurrent->y -= pCurrent->speed;

                // 尾曳き（火花）の生成
                if (pSet->count % 2 == 0) {
                    sEnemyShot* pTail = new sEnemyShot;
                    pTail->x = pCurrent->x + (GetRand(20) - 10) / 2.0;
                    pTail->y = pCurrent->y + 10.0; // 少し下から
                    pTail->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 100.0; // ほぼ下向き
                    pTail->speed = 1.0 + GetRand(10) / 10.0;
                    pTail->kind = img_enemyShotSmallBall[8]; // 橙
                    pTail->param_i[0] = 0; // 通常弾扱い

                    // リスト追加
                    pTail->prev = pSet->pEnemyShotHead->prev;
                    pTail->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pTail;
                    pSet->pEnemyShotHead->prev = pTail;
                }

                // 爆発判定 (画面上部 120px に達したら)
                if (pCurrent->y < 120.0) {
                    // 親弾削除
                    pCurrent->prev->next = pCurrent->next;
                    pCurrent->next->prev = pCurrent->prev;
                    delete pCurrent;

                    // 爆発座標
                    double ex = pSet->x;
                    double ey = 120.0;

                    // 自機方向（隙間を作るため）
                    double target_muki = atan2(player.y - ey, player.x - ex);

                    // --- 2. 花びら弾の生成 ---
                    int num_petals = 72;
                    double base_angle = GetRand(360) / 360.0 * DX_PI * 2.0;
                    int color_base = pSet->kind;

                    for (int j = 0; j < 5; j++) {
                        for (int i = 0; i < num_petals; i++) {
                            double angle = base_angle + i * (DX_PI * 2.0 / num_petals);

                            // 隙間判定 (自機方向 ±20度をスキップ)
                            double diff = angle - target_muki;
                            while (diff > DX_PI) diff -= DX_PI * 2.0;
                            while (diff < -DX_PI) diff += DX_PI * 2.0;
                            if (fabs(diff) < (20.0 / 180.0 * DX_PI)) continue;

                            sEnemyShot* pPetal = new sEnemyShot;
                            pPetal->x = ex;
                            pPetal->y = ey;
                            pPetal->muki = angle;
                            pPetal->speed = 2.5 + 1.5 * j;
                            pPetal->kind = img_enemyShotMediumBall[(color_base + i) % 8];
                            pPetal->param_i[0] = 1; // 重力対象フラグ
                            pPetal->margin = 480;

                            // リスト追加
                            pPetal->prev = pSet->pEnemyShotHead->prev;
                            pPetal->next = pSet->pEnemyShotHead;
                            pSet->pEnemyShotHead->prev->next = pPetal;
                            pSet->pEnemyShotHead->prev = pPetal;
                        }
                    }

                    // --- 3. 内側の星弾の生成 ---
                    for (int j = 0; j < 5; j++) {
                        for (int i = 0; i < 12; i++) {
                            double angle = base_angle + i * (DX_PI * 2.0 / 12);
                            sEnemyShot* pStar = new sEnemyShot;
                            pStar->x = ex;
                            pStar->y = ey;
                            pStar->muki = angle;
                            pStar->speed = 1.2 + 0.5 * j; // 遅め
                            pStar->kind = img_enemyShotSmallBall[6]; // 白
                            pStar->param_i[0] = 1; // 重力対象フラグ
                            pStar->margin = 480;

                            // リスト追加
                            pStar->prev = pSet->pEnemyShotHead->prev;
                            pStar->next = pSet->pEnemyShotHead;
                            pSet->pEnemyShotHead->prev->next = pStar;
                            pSet->pEnemyShotHead->prev = pStar;
                        }
                    }

                    // 爆発音
                    if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                    // Phase移行
                    pSet->param_i[0] = 1;
                }
            }
            else {
                // 尾曳き弾などの移動
                pCurrent->x += pCurrent->speed * cos(pCurrent->muki);
                pCurrent->y += pCurrent->speed * sin(pCurrent->muki);
            }
        }
        else if (pSet->param_i[0] == 1) {
            // Phase 1: 散り際 (重力適用)
            if (pCurrent->param_i[0] == 1) {
                // 重力加速度
                double gravity = 0.035;

                // 現在のベクトルを分解
                double vx = pCurrent->speed * cos(pCurrent->muki);
                double vy = pCurrent->speed * sin(pCurrent->muki);

                // 重力を加算
                vy += gravity;

                // 速度と向きを再計算
                pCurrent->speed = sqrt(vx * vx + vy * vy);
                pCurrent->muki = atan2(vy, vx);
            }

            // 移動
            pCurrent->x += pCurrent->speed * cos(pCurrent->muki);
            pCurrent->y += pCurrent->speed * sin(pCurrent->muki);
        }

        pCurrent = pNext;
    }
}