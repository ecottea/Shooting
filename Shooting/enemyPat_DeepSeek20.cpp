// enemyPat_hourglass.cpp
// 砂時計をモチーフにした弾幕パターン
// ・砂粒に見立てたオレンジ色の弾が上下に広がり、
//   重力で落下しながら砂時計の形を描く
// ・敵機はゆっくり降下し、画面下端に着くと
//   上端にワープする（砂時計をひっくり返すイメージ）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//--------------------------------------------------------------
// 弾幕：重力つき扇形砂時計
//--------------------------------------------------------------
static void HourglassShot(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    const double gravity = 0.06;   // 下向き加速度

    if (pSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int    numPerFan = 5;                          // 片側の弾数
        const double spreadRad = 60.0 * DX_PI / 180.0;       // 扇形の開き角（60°）
        const double baseSpeed = 3.5;                        // 基準速度
        const double angleJitter = 5.0 * DX_PI / 180.0;        // 角度のランダム幅(±5°)
        const double speedJitter = 0.1;                        // 速度のランダム幅

        // 上向きの扇（上昇してから重力で落ちてくる）
        {
            const double centerAngle = -DX_PI / 2.0; // 真上
            for (int i = 0; i < numPerFan; ++i) {
                // 等間隔の角度にランダムな揺らぎを加える
                double angle = centerAngle - spreadRad / 2.0
                    + spreadRad * i / (numPerFan - 1);
                angle += (GetRand(10) - 5) * angleJitter / 5.0;

                pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;

                // 初速のばらつき
                double spd = baseSpeed + (GetRand(20) - 10) * 0.01;
                pShot->param_d[0] = spd * cos(angle); // vx
                pShot->param_d[1] = spd * sin(angle); // vy
                pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);

                // 砂粒らしく橙の小玉 or 中玉をランダムに選ぶ
                int ballType = GetRand(2); // 0 or 1
                pShot->kind = (ballType != 0)
                    ? img_enemyShotSmallBall[8]
                    : img_enemyShotMediumBall[8];

                // 双方向リストに追加
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }

        // 下向きの扇（最初から落下）
        {
            const double centerAngle = DX_PI / 2.0; // 真下
            for (int i = 0; i < numPerFan; ++i) {
                double angle = centerAngle - spreadRad / 2.0
                    + spreadRad * i / (numPerFan - 1);
                angle += (GetRand(10) - 5) * angleJitter / 5.0;

                pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;

                double spd = baseSpeed + (GetRand(20) - 10) * 0.01;
                pShot->param_d[0] = spd * cos(angle);
                pShot->param_d[1] = spd * sin(angle);
                pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);

                int ballType = GetRand(2);
                pShot->kind = (ballType != 0)
                    ? img_enemyShotSmallBall[8]
                    : img_enemyShotMediumBall[8];

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 毎フレームの移動（重力加速つき）
    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];
        pShot->param_d[1] += gravity;   // 下方向へ加速

        // 向きも現在の速度ベクトルに合わせて更新
        pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);

        pShot = pShot->next;
    }
}

//--------------------------------------------------------------
// 敵本体パターン：砂時計の首（落下→反転）
//--------------------------------------------------------------
void EnemyPat_Hourglass_DeepSeek()
{
    if (count == 1) {
        // 初期位置は画面上部中央
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // ゆっくりと降下
        enemy.y += 1.0;

        // 下端に着いたら「ひっくり返す」ように上部へワープ
        if (enemy.y > 440.0) {
            enemy.y = 40.0;
            // X座標はある程度ランダムに（画面中央付近）
            enemy.x = 120.0 + (double)GetRand(240); // 120～360
        }
    }

    // 6フレームごとに弾幕セットを生成
    if (count % 6 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = HourglassShot;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;   // このパターンでは未使用

        // ダミーヘッドの作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリストへ追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}