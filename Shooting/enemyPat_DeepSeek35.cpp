// enemyPat_TwinSnake.cpp
// 弾幕パターン：双蛇交差ワインダー
// 左右の発射点が逆位相で水平振動しながら真下に弾を連射し、
// 中央で交差する二筋の蛇のような弾列を形成する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン関数（毎フレーム呼ばれる）
static void ShotTwinSnake(sEnemyShotSet* pEnemyShotSet)
{
    // 基本パラメータ
    const double AMPLITUDE    = 60.0;                 // 振動の振幅
    const double D            = 80.0;                 // 二つの発射点の間隔
    const double OMEGA        = 2.0 * DX_PI / 120.0;  // 1周期120フレームの角速度
    const int    EMIT_INTERVAL = 3;                   // 弾の発射間隔（フレーム）
    const double BULLET_SPEED = 3.0;                  // 弾速

    // 発射点がボス本体に追従するよう位置を毎フレーム更新
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;

    // EMIT_INTERVAL フレームごとに左右から1発ずつ発射
    if (pEnemyShotSet->count % EMIT_INTERVAL == 0) {
        double phase   = OMEGA * pEnemyShotSet->count;
        double left_x  = pEnemyShotSet->x - D / 2.0 + AMPLITUDE * sin(phase);
        double right_x = pEnemyShotSet->x + D / 2.0 - AMPLITUDE * sin(phase); // 逆位相

        // 左側の弾（赤色の小玉）
        sEnemyShot* pLeft = new sEnemyShot;
        pLeft->x      = left_x;
        pLeft->y      = pEnemyShotSet->y;
        pLeft->muki   = DX_PI / 2.0;                 // 真下
        pLeft->speed  = BULLET_SPEED;
        pLeft->kind   = img_enemyShotSmallBall[0];   // 色:赤
        pLeft->prev   = pEnemyShotSet->pEnemyShotHead->prev;
        pLeft->next   = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pLeft;
        pEnemyShotSet->pEnemyShotHead->prev       = pLeft;

        // 右側の弾（青色の小玉）
        sEnemyShot* pRight = new sEnemyShot;
        pRight->x     = right_x;
        pRight->y     = pEnemyShotSet->y;
        pRight->muki  = DX_PI / 2.0;
        pRight->speed = BULLET_SPEED;
        pRight->kind  = img_enemyShotSmallBall[4];   // 色:青
        pRight->prev  = pEnemyShotSet->pEnemyShotHead->prev;
        pRight->next  = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pRight;
        pEnemyShotSet->pEnemyShotHead->prev       = pRight;
    }

    // 登録済みの全弾を移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Winder_DeepSeek()
{
    static int  muki;
    static bool winderStarted = false;

    if (count == 1) {
        // 初期化
        enemy.x     = 240.0;
        enemy.y     = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki        = 1;
        winderStarted = false;
    }
    else {
        // 左右移動（一定周期で反転）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60)
            muki *= -1;
    }

    // ワインダー用の弾幕セットを一度だけ作成
    if (!winderStarted && count > 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count       = 0;
        pEnemyShotSet->patternFunc = ShotTwinSnake;
        pEnemyShotSet->x           = enemy.x;
        pEnemyShotSet->y           = enemy.y + 10.0;
        pEnemyShotSet->muki        = 0.0;
        pEnemyShotSet->kind        = 0;

        // 弾リストのダミーヘッド作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体の弾幕セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev       = pEnemyShotSet;

        // 効果音再生
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        winderStarted = true;
    }
}