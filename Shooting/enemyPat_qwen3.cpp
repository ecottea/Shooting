// enemyPat_spectacular.cpp
// パターン：見栄えの良い弾幕（レインボー・マルチリング・バースト）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：マルチリングバースト
static void ShotSpectacular(sEnemyShotSet* pEnemyShotSet)
{
    // 1. メインの爆発 (40フレームごとに大輪の花を咲かせる)
    if (pEnemyShotSet->count % 60 == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int numDirections = 21; // 36方向 (10度刻み)
        int numRings = 4;       // 4つのリング (小, 中, 大, 鱗)

        for (int ring = 0; ring < numRings; ring++) {
            for (int i = 0; i < numDirections; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                
                // 角度 (リングごとに少しずらすとより複雑な動きになるが、ここでは揃えて美しさを重視)
                double angle = (i * 2.0 * 3.14159265 / numDirections);
                pShot->muki = angle;
                
                // リングごとに速度を変える (外側ほど速く)
                pShot->speed = 1.5 + ring * 1.0;

                // 色を角度と時間で変化させ、虹色が回転するように見せる
                int colorIndex = (i + pEnemyShotSet->count / 3) % 7;
                
                // リングごとに弾の種類を変える
                if (ring == 0)      pShot->kind = img_enemyShotSmallBall[colorIndex];
                else if (ring == 1) pShot->kind = img_enemyShotMediumBall[colorIndex];
                else if (ring == 2) pShot->kind = img_enemyShotLargeBall[colorIndex];
                else                pShot->kind = img_enemyShotScale[colorIndex];

                // リストに追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 2. 背景を彩るダイヤモンドの螺旋 (毎フレーム生成)
    if (pEnemyShotSet->count % 4 == 0) {
        int numDiamonds = 6;
        for (int i = 0; i < numDiamonds; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            
            // 回転する螺旋
            double angle = (pEnemyShotSet->count * 0.15) + (i * 2.0 * 3.14159265 / numDiamonds);
            pShot->muki = angle;
            pShot->speed = 2.8;
            
            // 色は虹色に
            int colorIndex = (pEnemyShotSet->count / 4 + i) % 8;
            pShot->kind = img_enemyShotDiamond[colorIndex];

            // リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
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
void EnemyPat_Beautiful_Qwen()
{
    static int muki;
    static sEnemyShotSet* pMyShotSet = nullptr;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = 150; // 見栄えが良いので長生きさせる
        enemy.hp = enemy.maxHp;
        muki = 1;
        pMyShotSet = nullptr;
    }
    else {
        // 画面中央で大きくゆっくりと左右に移動
        enemy.x += 0.8 * (double)muki;
        if (enemy.x < 120 || enemy.x > 360) muki *= -1;
        
        // 上下にもサインカーブで揺れ動く
        enemy.y = 80.0 + sin(count / 80.0) * 20.0;
    }

    // 出現後すぐに弾幕セットを生成
    if (count == 30 && pMyShotSet == nullptr) {
        pMyShotSet = new sEnemyShotSet;
        pMyShotSet->count = 0;
        pMyShotSet->patternFunc = ShotSpectacular;
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