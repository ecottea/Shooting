// enemyPat_tmp.cpp
// ボンバーマン風「十字爆風の連鎖弾幕」パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 十字方向の数
static const int CROSS_DIR_COUNT = 4;

// 十字方向の角度（上・右・下・左）
static const double CROSS_DIRS[CROSS_DIR_COUNT] = {
    -DX_PI / 2.0,  // 上
    0.0,           // 右
    DX_PI / 2.0,   // 下
    DX_PI          // 左
};

// 連鎖爆発の段階数
static const int CHAIN_PHASE_MAX = 2;

// 弾幕：十字爆風＋連鎖爆発
static void ShotCrossChain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回フレーム（pEnemyShotSet->count == 0）で初期配置
    if (pEnemyShotSet->count == 0) {
        // 効果音：中〜重い爆発音を選ぶ
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 連鎖段階を param_i[0] に保存（0: 中心爆発, 1〜: 連鎖）
        pEnemyShotSet->param_i[0] = 0;

        // 中心爆発の弾を配置
        for (int i = 0; i < CROSS_DIR_COUNT; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = CROSS_DIRS[i];
            pEnemyShot->speed = 4.0; // 基本速度

            // 弾の種類と色
            // 中心爆発は中玉＋赤（0）で炎っぽく
            pEnemyShot->kind = img_enemyShotMediumBall[0];

            // 連鎖段階を param_i[0] に保存
            pEnemyShot->param_i[0] = 0;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの弾移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 弾を移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 連鎖爆発の判定
        int phase = pEnemyShot->param_i[0];
        if (phase < CHAIN_PHASE_MAX) {
            // 一定距離進んだら連鎖発生
            double dist = hypot(pEnemyShot->x - pEnemyShotSet->x,
                pEnemyShot->y - pEnemyShotSet->y);
            if (dist > 160.0 * (phase + 1)) {
                // 連鎖段階を進める
                pEnemyShot->param_i[0]++;

                // 連鎖爆発の弾を追加
                for (int i = 0; i < CROSS_DIR_COUNT; i++) {
                    sEnemyShot* chainShot = new sEnemyShot;
                    chainShot->x = pEnemyShot->x;
                    chainShot->y = pEnemyShot->y;
                    chainShot->muki = CROSS_DIRS[i];
                    chainShot->speed = 2.0; // 連鎖は少し遅め

                    // 連鎖が進むほど色を橙（8）に近づける
                    int colorIdx = (phase == 0) ? 0 : 8; // 0:赤, 8:橙
                    chainShot->kind = img_enemyShotMediumBall[colorIdx];

                    // 連鎖段階を保存
                    chainShot->param_i[0] = phase + 1;

                    // リストに追加
                    chainShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    chainShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = chainShot;
                    pEnemyShotSet->pEnemyShotHead->prev = chainShot;
                }

                // 必要なら連鎖時の効果音も再生（例：phase が大きいほど重い音）
                if (phase == 0) {
                    if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                }
            }
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（指定どおりの関数名）
void EnemyPat_Bomberman_Sakana()
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
        // 左右にゆっくり移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で弾幕セットを生成
    if (count % 25 == 0) { // 60フレームごとに発射
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCrossChain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // このパターンでは向きは未使用
        pEnemyShotSet->kind = shot_count++;

        // 弾リストのダミーヘッド
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}