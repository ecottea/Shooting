// enemyPat_geometric.cpp
// パターン：幾何学的に美しい弾幕（回転する花）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：幾何学的花
static void ShotGeometricFlower(sEnemyShotSet* pEnemyShotSet)
{
    // 6フレームごとに弾を生成
    if (pEnemyShotSet->count % 12 == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numPetals = 5;         // 花びらの数
        int shotsPerPetal = 8;     // 1花びらあたりの弾数

        // 全体の回転角度（時間とともに変化）
        double globalRotation = pEnemyShotSet->count / 50.0;

        for (int p = 0; p < numPetals; p++) {
            // 花びらの基本角度
            double petalAngle = globalRotation + (p * 2.0 * 3.14159265 / numPetals);

            for (int i = 0; i < shotsPerPetal; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                // 花びら状に広がる角度
                double spreadAngle = petalAngle + (i - shotsPerPetal / 2.0) * 0.15;
                pEnemyShot->muki = spreadAngle;

                // 内側から外側へ速度を上げる
                pEnemyShot->speed = 1.5 + i * 0.4;

                // 色と弾の種類を調整（サンプル0の画像を使用）
                int color = (p + i / 2) % 8;
                if (i < 3) {
                    pEnemyShot->kind = img_enemyShotSmallBall[color];
                }
                else if (i < 6) {
                    pEnemyShot->kind = img_enemyShotMediumBall[color];
                }
                else {
                    pEnemyShot->kind = img_enemyShotScale[color];
                }

                // リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の移動処理
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Geometry_Qwen()
{
    static int muki;
    static sEnemyShotSet* pMyShotSet = nullptr;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        pMyShotSet = nullptr;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 100 || enemy.x > 380) muki *= -1;
    }

    // 出現後1秒で弾幕セットを生成
    if (count == 60 && pMyShotSet == nullptr) {
        pMyShotSet = new sEnemyShotSet;
        pMyShotSet->count = 0;
        pMyShotSet->patternFunc = ShotGeometricFlower;
        pMyShotSet->x = enemy.x;
        pMyShotSet->y = enemy.y + 20.0;
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
        pMyShotSet->y = enemy.y + 20.0;
    }
}