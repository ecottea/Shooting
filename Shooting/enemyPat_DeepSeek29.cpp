// EnemyPat_Bubble.cpp
// 泡沫の輪舞曲 ～Iridescent Bubble Rondo～

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//-----------------------------------------------------------
// 親泡・子泡・飛沫弾を管理するパターン関数
//-----------------------------------------------------------
static void BubbleParentPattern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回呼び出しで親泡を1つ生成
    if (pEnemyShotSet->count == 0) {
        // 生成音（軽め）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;       // 自機狙い
        pEnemyShot->speed = 0.5;                      // ゆっくり（約30px/秒）
        pEnemyShot->kind = img_enemyShotLargeBall[3]; // シアン大玉（泡らしく）
        pEnemyShot->count = 0;
        pEnemyShot->param_i[0] = 0;                   // 種類: 0=親泡
        pEnemyShot->param_d[0] = pEnemyShotSet->muki; // 基本角度
        pEnemyShot->param_d[1] = GetRand(360) * DX_PI / 180.0; // 蛇行の位相ずらし
        pEnemyShot->margin = 999;

        // リストに追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // セット内の全弾を更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pEnemyShot->next; // 削除に備えて次を保持

        if (pEnemyShot->param_i[0] == 0) {
            //--- 親泡：自機狙い＋正弦波で蛇行 -----------------
            double baseAngle = pEnemyShot->param_d[0];
            double phase = pEnemyShot->param_d[1];
            double amp = 30.0 * 0.1;                    // 振幅
            double period = 90.0;                    // 1.5秒周期
            double wobble = amp * sin(2.0 * DX_PI * pEnemyShot->count / period + phase);
            double perp = baseAngle + DX_PI / 2.0; // 進行方向の垂直

            pEnemyShot->x += pEnemyShot->speed * cos(baseAngle) + wobble * cos(perp);
            pEnemyShot->y += pEnemyShot->speed * sin(baseAngle) + wobble * sin(perp);

            // 3秒後（180F）に子泡をばらまく
            if (pEnemyShot->count == 180) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                for (int i = 0; i < 20; i++) {
                    sEnemyShot* child = new sEnemyShot;
                    child->x = pEnemyShot->x;
                    child->y = pEnemyShot->y;
                    if (i < 8) {
                        // 8方向均等
                        child->muki = (double)i / 8.0 * 2.0 * DX_PI;
                    }
                    else {
                        // 残り12個は自機方向に偏らせたランダム
                        double toPlayer = atan2(player.y - child->y, player.x - child->x);
                        child->muki = toPlayer + (GetRand(120) - 60) / 180.0 * DX_PI;
                    }
                    child->speed = 1.5;                       // 低速で漂う
                    child->kind = img_enemyShotMediumBall[3]; // シアン中玉
                    child->count = 0;
                    child->param_i[0] = 1;                    // 種類: 子泡
                    child->param_d[0] = child->muki;          // 初期方向を記憶
                    child->param_d[1] = GetRand(360) * DX_PI / 180.0;
                    child->margin = 999;

                    child->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    child->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = child;
                    pEnemyShotSet->pEnemyShotHead->prev = child;
                }
            }

            // 5秒後（300F）に破裂して飛沫弾
            if (pEnemyShot->count >= 300) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                double toPlayer = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                int numSplash = 20;
                for (int i = 0; i < numSplash; i++) {
                    sEnemyShot* splash = new sEnemyShot;
                    splash->x = pEnemyShot->x;
                    splash->y = pEnemyShot->y;
                    if (i < 8) {
                        splash->muki = (double)i / 8.0 * 2.0 * DX_PI;
                    }
                    else {
                        splash->muki = toPlayer + (GetRand(120) - 60) / 180.0 * DX_PI;
                    }
                    splash->speed = 4.0;                          // やや速め
                    splash->kind = img_enemyShotSmallBall[GetRand(8)]; // 虹色ランダム
                    splash->count = 0;
                    splash->param_i[0] = 2;                       // 種類: 飛沫弾
                    splash->margin = 999;

                    splash->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    splash->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = splash;
                    pEnemyShotSet->pEnemyShotHead->prev = splash;
                }

                // 破裂した親泡を削除
                pEnemyShot->prev->next = pEnemyShot->next;
                pEnemyShot->next->prev = pEnemyShot->prev;
                delete pEnemyShot;
                pEnemyShot = nextShot;
                continue;
            }
        }
        else if (pEnemyShot->param_i[0] == 1) {
            //--- 子泡：ゆらゆら漂う（正弦波で蛇行） -----------
            double dir = pEnemyShot->param_d[0];
            double phase = pEnemyShot->param_d[1];
            double amp = 20.0 * 0.1;
            double period = 60.0; // 1秒周期でふわふわ
            double wobble = amp * sin(2.0 * DX_PI * pEnemyShot->count / period + phase);
            double perp = dir + DX_PI / 2.0;

            pEnemyShot->x += pEnemyShot->speed * cos(dir) + wobble * cos(perp);
            pEnemyShot->y += pEnemyShot->speed * sin(dir) + wobble * sin(perp);
        }
        else if (pEnemyShot->param_i[0] == 2) {
            //--- 飛沫弾：直線等速 ----------------------------
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = nextShot;
    }
}

//-----------------------------------------------------------
// 敵本体パターン（泡沫の輪舞曲）
//-----------------------------------------------------------
void EnemyPat_SoapBubbles_DeepSeek()
{
    static int muki;
    static int waveTimer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        waveTimer = 0;
    }
    else {
        // 横方向にゆっくり往復
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1; // 3秒ごとに反転
    }

    // 2秒（120F）ごとに親泡を自機狙いで発射
    if (count % 120 == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = BubbleParentPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 20秒（1200F）ごとに画面外上端から親泡8個の波
    waveTimer++;
    if (waveTimer >= 500) {
        waveTimer = 0;
        for (int i = 0; i < 4; i++) {
            sEnemyShotSet* pWave = new sEnemyShotSet;
            pWave->count = 0;
            pWave->patternFunc = BubbleParentPattern;
            pWave->x = 30.0 + i * 120.0;  // 均等に配置
            pWave->y = -20.0;            // 画面上部の外
            pWave->muki = DX_PI / 2.0;   // 真下へ
            pWave->kind = 0;

            pWave->pEnemyShotHead = new sEnemyShot;
            pWave->pEnemyShotHead->prev = pWave->pEnemyShotHead;
            pWave->pEnemyShotHead->next = pWave->pEnemyShotHead;

            pWave->prev = enemyShotSetHead.prev;
            pWave->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pWave;
            enemyShotSetHead.prev = pWave;
        }
    }
}