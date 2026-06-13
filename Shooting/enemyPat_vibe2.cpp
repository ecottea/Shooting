// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：波状回避弾幕
// 3つの同心円状に弾を展開し、プレイヤーを巻き込むように広がる
static void ShotWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 3層の波を作成
        for (int layer = 1; layer <= 3; layer++) {
            double angle_base = DX_PI * GetRand(179) / 180;

            // 各層に8個の弾を配置
            const int N = 160 - 30 * layer;
            for (int i = 0; i < N; i++) {
                pEnemyShot = new sEnemyShot;

                // 同心円状に配置
                double angle = angle_base + i * (DX_PI * 2 / N); // 8方向
                double radius = 60.0 * layer; // 層ごとに半径を変える

                pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
                pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);

                // 弾の進行方向：中心から外向き
                pEnemyShot->muki = angle;

                // 層ごとに速度を変える（内側ほど速い）
                pEnemyShot->speed = (4.0 - layer * 0.5);

                // 弾の種類：層ごとに変える
                int type, color;
                switch (layer) {
                case 1:
                    type = 0; // 小玉
                    color = 2; // 緑
                    break;
                case 2:
                    type = 1; // 中玉
                    color = 4; // 青
                    break;
                case 3:
                    type = 3; // 銃弾
                    color = 0; // 赤
                    break;
                }

                switch (type) {
                case 0:
                    pEnemyShot->kind = img_enemyShotSmallBall[color];
                    break;
                case 1:
                    pEnemyShot->kind = img_enemyShotMediumBall[color];
                    break;
                case 2:
                    pEnemyShot->kind = img_enemyShotLargeBall[color];
                    break;
                case 3:
                    pEnemyShot->kind = img_enemyShotBullet[color];
                    break;
                case 4:
                    pEnemyShot->kind = img_enemyShotScale[color];
                    break;
                case 5:
                    pEnemyShot->kind = img_enemyShotDiamond[color];
                    break;
                }

                // 弾をリストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Dodge_Vibe()
{
    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵は静止
    }

    // 90フレームごとに弾幕を発射
    if (count % 90 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWave;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0; // 方向は使用しない

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}