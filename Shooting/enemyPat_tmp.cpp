// enemyPat_comet.cpp
// 彗星をモチーフにした弾幕パターン
// 敵本体の関数名：EnemyPat_Tmp()

#include "DxLib.h"
#include "gv.h"
#include <math.h>

// ------------------------------------------------------------
// 彗星の尾を表現するショットセット用関数
// ------------------------------------------------------------
static void CometTrail(sEnemyShotSet* pEnemyShotSet)
{
    // 初回フレームで自機方向を記憶し、効果音を鳴らす
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 発生源を敵本体の現在位置に追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // 一定間隔で尾となる小弾を追加（120フレームで打ち止め）
    if (pEnemyShotSet->count % 4 == 0 && pEnemyShotSet->count < 120) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki;
        pShot->speed = 2.5;                       // 移動速度
        pShot->kind = img_enemyShotSmallBall[3];  // シアン（氷の彗星をイメージ）

        // 双方向リストの末尾に挿入
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // すべての弾を移動
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン：彗星のように横移動しながら尾を射出
// ------------------------------------------------------------
void EnemyPat_Tmp()
{
    static int muki;   // 横移動の向き (1 or -1)

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 1.2 * muki;
        if (enemy.x < 50.0 || enemy.x > 430.0)
            muki *= -1;
    }

    // 120カウントごとに彗星の尾を放つショットセットを生成
    if (count % 120 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = CometTrail;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;   // 実際の方向は CometTrail 内で設定

        // 双方向リストのヘッダノードを作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルなショットセットリストに登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}