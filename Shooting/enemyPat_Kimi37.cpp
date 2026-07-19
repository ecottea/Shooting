// enemyPat_HelixStaircase.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：螺旋階段（Helix Staircase）
// 多層レイヤーの螺旋弾が奥から手前へ迫ってくる立体弾幕
static void ShotHelixStaircase(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double CENTER_X = pEnemyShotSet->x; // 生成時の敵位置を螺旋中心に
    const double CENTER_Y = pEnemyShotSet->y;
    const double BASE_RADIUS = 90.0;
    const int BULLETS_PER_LAYER = 12;
    const int LAYERS = 3;

    if (pEnemyShotSet->count == 0) {
        // 初回生成時：重めの効果音で存在感を出す
        if (!CheckSoundMem(sound_enemyShot_heavy)) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int layer = LAYERS - 1; layer >= 0; layer--) {
            double initZ = layer * 0.33; // 奥:0.00, 中:0.33, 手前:0.66

            for (int i = 0; i < BULLETS_PER_LAYER; i++) {
                pEnemyShot = new sEnemyShot;

                double baseAngle = (DX_PI * 2.0 / BULLETS_PER_LAYER) * i;
                // レイヤーごとに角度をずらし、螺旋状に見せる
                baseAngle += layer * (DX_PI / 8.0);

                pEnemyShot->param_d[0] = initZ;        // Z値：奥行き（0.0=奥 〜 1.0=手前）
                pEnemyShot->param_d[1] = baseAngle;    // 基準角度
                pEnemyShot->param_d[2] = 0.4 + initZ * 0.6; // 回転速度係数（手前ほど速い）
                pEnemyShot->param_d[3] = 0.4 + initZ * 0.9; // 描画スケール（手前ほど大きい）

                // レイヤーに応じた弾の種類と色
                // 奥:小玉・青系, 中:中玉・緑系, 手前:大玉・赤系
                int colorIdx;
                switch (layer) {
                case 0: // 奥：小玉（小さく薄く）
                    colorIdx = 4 + (i % 2); // 青〜シアン
                    pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
                    break;
                case 1: // 中：中玉
                    colorIdx = 2 + (i % 2); // 緑〜黄
                    pEnemyShot->kind = img_enemyShotMediumBall[colorIdx];
                    break;
                case 2: // 手前：大玉（迫力）
                    colorIdx = 0 + (i % 2); // 赤〜橙
                    pEnemyShot->kind = img_enemyShotLargeBall[colorIdx];
                    break;
                }

                // 初期位置を計算
                double r = BASE_RADIUS * (0.8 + initZ * 0.4);
                double theta = baseAngle;
                pEnemyShot->x = CENTER_X + r * cos(theta);
                pEnemyShot->y = CENTER_Y + r * sin(theta) * 0.35 + initZ * 150.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->muki = 0.0;

                // 影の情報（メインルーチン描画用）
                pEnemyShot->param_d[4] = 0.0; // 影Xオフセット
                pEnemyShot->param_d[5] = (1.0 - initZ) * 20.0; // 影Yオフセット（高いほど遠い）

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double z = pEnemyShot->param_d[0];
        double baseAngle = pEnemyShot->param_d[1];
        double speedFactor = pEnemyShot->param_d[2];

        // Z値を増加させて手前に近づける
        z += 0.0035;
        pEnemyShot->param_d[0] = z;

        // スケール更新
        double scale = 0.4 + z * 0.9;
        if (scale > 1.35) scale = 1.35;
        pEnemyShot->param_d[3] = scale;

        // 螺旋回転
        double angle = baseAngle + pEnemyShot->count * 0.012 * speedFactor;

        // 半径もZに応じて広がる
        double r = BASE_RADIUS * (0.8 + z * 0.5);

        // 位置更新
        pEnemyShot->x = CENTER_X + r * cos(angle);
        pEnemyShot->y = CENTER_Y + r * sin(angle) * 0.35 + z * 180.0;

        // 影のオフセット更新
        pEnemyShot->param_d[5] = (1.0 - z) * 25.0;

        // 透明度情報（メインルーチン参照用）
        int alpha = (int)(50 + z * 205);
        if (alpha > 255) alpha = 255;
        pEnemyShot->param_i[0] = alpha;

        // 完全に手前を超えたら画面外に追い出して消去を促す
        if (z > 1.25) {
            //pEnemyShot->y = 999.0;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_3D_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 200フレームごとに螺旋階段弾幕を発射
    if (count % 200 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHelixStaircase;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}