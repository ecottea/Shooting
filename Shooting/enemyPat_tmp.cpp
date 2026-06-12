// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：火山噴火モチーフ
// 中心から多方向に、赤・橙・黄色の大小さまざまな弾を放射状に発射
static void ShotVolcanicEruption(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int numShots = 24; // 1回の噴出で発射する弾の数

    if (pEnemyShotSet->count == 0) {
        // 音は重めのものを使用
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < numShots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 360度全方位に均等に弾を配置
            double angleStep = 2.0 * DX_PI / numShots;
            pEnemyShot->muki = angleStep * i + (GetRand(30) - 15) / 180.0 * DX_PI; // 少しのランダム性を加える

            // ランダムな速度
            pEnemyShot->speed = (300 + GetRand(200)) / 100.0;

            // 弾の種類は小玉、中玉、大玉、鱗弾から選択
            int type = GetRand(3);
            // 色は赤(0)、黄(1)、マゼンタ(5:オレンジに近い)から選択
            int colorChoices[] = { 0, 1, 5 };
            int color = colorChoices[GetRand(2)];

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
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            }

            // リストへの追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
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

// 敵本体のパターン：火山
void EnemyPat_Tmp()
{
    static int direction; // 左右移動の方向 (-1 or 1)
    if (count == 1) {
        // 初期位置とHPを設定
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 300; // サンプルより少し硬め
        direction = 1;
    }
    else {
        // ゆっくり左右に往復移動
        enemy.x += 0.7 * (double)direction;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            direction *= -1;
        }
    }

    // 90フレームごとに火山噴火パターンを発動
    if (count % 90 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotVolcanicEruption;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // mukiはShotVolcanicEruption内で使用しないため、適当な値でOK
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}