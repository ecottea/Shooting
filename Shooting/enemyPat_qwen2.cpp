// enemyPat_fun.cpp
// パターン：避けてて楽しい弾幕（螺旋＋狙い撃ち）
#include "DxLib.h"
#include "gv.h"
#include <math.h>

// 弾幕：螺旋と狙い撃ち
static void ShotFunPattern(sEnemyShotSet* pEnemyShotSet)
{
    // 1. 螺旋弾の生成 (毎フレーム)
    // 角度を少しずつずらしながら発射し、螺旋状の軌道を作る
    double spiralAngle = pEnemyShotSet->count * 0.15; // 回転速度
    
    // 2方向から同時に螺旋を出す（密度と迫力の向上）
    for (int i = 0; i < 2; i++) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = spiralAngle + i * 3.14159; // 反対方向にも出す
        pShot->speed = 2.2;
        
        // 色を時間とともに変化させ、虹色の螺旋にする
        int color = (pEnemyShotSet->count / 5) % 8;
        pShot->kind = img_enemyShotScale[color]; // 鱗弾は回転するので螺旋に最適

        // リストに追加
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // 2. プレイヤー狙い撃ちバーレット (20フレームごと)
    if (pEnemyShotSet->count % 20 == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK); // 重い効果音
        
        // プレイヤーの位置を計算
        double targetAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = targetAngle;
        pShot->speed = 4.5; // 高速で飛ぶ
        pShot->kind = img_enemyShotLargeBall[0]; // 赤いバーレット（視認性重視）

        // リストに追加
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // 3. 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Dodge_Qwen()
{
    static int muki;
    static sEnemyShotSet* pMyShotSet = nullptr;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = 200;
        enemy.hp = enemy.maxHp;
        muki = 1;
        pMyShotSet = nullptr;
    }
    else {
        // 左右に移動
        enemy.x += 1.2 * (double)muki;
        if (enemy.x < 80 || enemy.x > 400) muki *= -1;
        
        // 上下にサインカーブで移動（不規則な動きで狙い撃ちの角度を変える）
        enemy.y = 60.0 + sin(count / 60.0) * 30.0;
    }

    // 出現後1秒で弾幕セットを生成
    if (count == 60 && pMyShotSet == nullptr) {
        pMyShotSet = new sEnemyShotSet;
        pMyShotSet->count = 0;
        pMyShotSet->patternFunc = ShotFunPattern;
        pMyShotSet->x = enemy.x;
        pMyShotSet->y = enemy.y;
        pMyShotSet->muki = 0.0;

        pMyShotSet->pEnemyShotHead = new sEnemyShot;
        pMyShotSet->pEnemyShotHead->prev = pMyShotSet->pEnemyShotHead;
        pMyShotSet->pEnemyShotHead->next = pMyShotSet->pEnemyShotHead;

        pMyShotSet->prev = enemyShotSetHead.prev;
        pMyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMyShotSet;
        enemyShotSetHead.prev = pMyShotSet;
    }

    // 敵の位置に合わせて発射座標を更新
    if (pMyShotSet != nullptr) {
        pMyShotSet->x = enemy.x;
        pMyShotSet->y = enemy.y;
    }
}