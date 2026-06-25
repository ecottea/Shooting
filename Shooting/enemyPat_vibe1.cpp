// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：螺旋状に展開する弾幕
static void ShotSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 24個の弾を螺旋状に展開
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            double baseAngle = (i * 15.0 + pEnemyShotSet->kind * 3.0) / 180.0 * DX_PI; // 15度間隔
            double offset = (pEnemyShotSet->count * 5.0) / 180.0 * DX_PI; // 時間経過で回転
            double angle = baseAngle + offset;
            double radius = 40.0 + (i % 3) * 20.0; // 半径に変化を付ける

            pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);
            pEnemyShot->muki = angle + DX_PI / 2.0; // 弾の進行方向（接線方向）
            pEnemyShot->speed = 1.5 + (i % 4) * 0.3; // 速度に変化を付ける

            // 弾の種類と色をランダムに選択
            int color = i % 6; // 7色
            pEnemyShot->kind = img_enemyShotLargeBall[color];

            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 螺旋の回転効果を維持
        if (pEnemyShot->count <= 300) pEnemyShot->muki += 0.02;

        pEnemyShot = pEnemyShot->next;
    }
}

// 弾幕：同心円状に展開する弾幕
static void ShotConcentricCircles(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double base_angle = GetRand(179) * DX_PI / 180;

        // 3つの同心円に分けて弾を配置
        for (int circle = 0; circle < 3; circle++) {
            int numShots = (8 + circle * 4) * 2; // 内側から8, 12, 16個
            double radius = 50.0 + circle * 30.0;

            for (int i = 0; i < numShots; i++) {
                pEnemyShot = new sEnemyShot;
                double angle = base_angle + (i * 360.0 / numShots) / 180.0 * DX_PI;

                pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
                pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);
                pEnemyShot->muki = angle + DX_PI; // 中心に向かう方向
                pEnemyShot->speed = 1.0 + circle * 0.5;

                // 弾の種類と色を固定（視覚的に美しく）
                int color = circle * 2 % 7; // 異なる色を割り当て
                pEnemyShot->kind = img_enemyShotSmallBall[color];

                // リンクリストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Geometry_Vibe()
{
    static int muki;
    static int phase = 0; // 弾幕のフェーズ管理

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
    }
    else {
        // 正弦波状に上下移動
        enemy.y = 200.0 + 40.0 * sin(count * 0.02);
    }

    // フェーズに応じて弾幕を切り替え
    if (count % 120 == 0) {
        phase = (phase + 1) % 2;
    }

    // 弾幕発射
    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;

        // フェーズに応じて弾幕を選択
        if (phase == 0) {
            pEnemyShotSet->kind = (count / 30) % 3;
            pEnemyShotSet->patternFunc = ShotSpiral;
        }
        else {
            pEnemyShotSet->patternFunc = ShotConcentricCircles;
        }

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}