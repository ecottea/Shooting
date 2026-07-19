// enemyPat_Tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// 立体螺旋トンネル (3D Helix Tunnel)
// =============================================
static void ShotHelixTunnel(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int BULLETS_PER_LAYER = 16;     // 1周あたりの弾数
    const double HELIX_RADIUS = 90.0;     // 螺旋の半径
    const double ADVANCE_SPEED = 2.8;     // 手前への進行速度
    const double ROT_SPEED = 0.085;       // 回転速度（ラジアン/フレーム）

    if (pEnemyShotSet->count == 0)
    {
        // 効果音（中くらいのものを推奨）
        if (!CheckSoundMem(sound_enemyShot_medium))
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 2本の逆回転螺旋を生成
        for (int helix = 0; helix < 2; helix++)
        {
            double baseAngle = helix * DX_PI;  // 0 と π で逆方向にずらす

            for (int i = 0; i < BULLETS_PER_LAYER; i++)
            {
                pEnemyShot = new sEnemyShot;

                double angle = baseAngle + i * (DX_PI * 2.0 / BULLETS_PER_LAYER);
                double depth = (double)i / BULLETS_PER_LAYER; // 0.0(奥)〜1.0(手前)

                // 初期位置（奥から）
                pEnemyShot->x = pEnemyShotSet->x + cos(angle) * HELIX_RADIUS * 0.3;
                pEnemyShot->y = pEnemyShotSet->y - 80.0;  // 画面上部奥

                // パラメータを使って立体感を管理
                pEnemyShot->param_d[0] = angle;           // 現在の角度
                pEnemyShot->param_d[1] = depth;           // 深度（0.0〜1.0）
                pEnemyShot->param_d[2] = helix == 0 ? 1.0 : -1.0; // 回転方向

                // 遠近による見た目変化（kindで色・サイズを制御）
                int colorPhase = (int)(depth * 6.0); // 0〜5
                pEnemyShot->kind = colorPhase % 6;   // 赤→黄→緑→シアン→青→マゼンタ

                if (depth < 0.4)
                    pEnemyShot->kind = img_enemyShotSmallBall[colorPhase];   // 遠く：小玉
                else if (depth < 0.75)
                    pEnemyShot->kind = img_enemyShotMediumBall[colorPhase];  // 中間：中玉
                else
                    pEnemyShot->kind = img_enemyShotLargeBall[colorPhase];   // 手前：大玉

                pEnemyShot->margin = 50.0;  // 画面外判定を少し緩く

                // 双方向リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ==================== 毎フレーム更新 ====================
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        double& angle = pEnemyShot->param_d[0];
        double depth = pEnemyShot->param_d[1];
        double direction = pEnemyShot->param_d[2];

        // 螺旋進行
        angle += ROT_SPEED * direction;
        pEnemyShot->y += ADVANCE_SPEED;                     // 手前へ直進
        pEnemyShot->x = pEnemyShotSet->x + cos(angle) * HELIX_RADIUS * (0.6 + depth * 0.7);

        // 深度に応じた視覚効果（サイズはkindで決まっているので位置のみ）
        pEnemyShot = pEnemyShot->next;
    }
}

// =============================================
// 敵本体パターン
// =============================================
void EnemyPat_3D_Grok()
{
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else
    {
        // 左右往復移動
        enemy.x += 1.1 * (double)muki;
        if (count % 140 == 70) muki *= -1;
    }

    // 定期的に螺旋トンネルを発射
    if (count % 55 == 0)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHelixTunnel;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 双方向リストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}