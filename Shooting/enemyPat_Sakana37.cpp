// enemyPat_3DRing.cpp
// 3D回転リング弾幕パターン（敵本体: EnemyPat_3D_Sakana）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 使える素材の一覧（enemyPat_sampleForAI.cpp より抜粋）
// 弾の種類一覧:
//   img_enemyShotSmallBall[i]    // 2.5 x 2.5
//   img_enemyShotMediumBall[i]   // 7.0 x 7.0
//   img_enemyShotLargeBall[i]    // 20.0 x 20.0
//   img_enemyShotBullet[i]       // 5.0 x 2.0
//   img_enemyShotScale[i]        // 4.0 x 3.0
//   img_enemyShotDiamond[i]      // 4.5 x 2.5
//   img_enemyShotMediumOval[i]   // 10.5 x 7.0
//   img_enemyShotLaser[i]        // 64.0 x 4.0
//
// 弾の色一覧（i=0〜8）:
//   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
//
// 効果音一覧:
//   sound_enemyShot_light
//   sound_enemyShot_medium
//   sound_enemyShot_heavy
//   sound_enemyShot_extreme

// ============================================================
// 3D回転リング弾幕（弾幕パターン関数）
// ============================================================
static void Shot3DRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回呼び出し時のみリングを生成
    if (pEnemyShotSet->count == 0)
    {
        // 効果音: 中くらいの重さの弾音
        if (!CheckSoundMem(sound_enemyShot_medium))
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // リングのパラメータ
        const int N = 16;           // 弾の数（リング一周）
        const double R = 60.0;      // リングの半径
        const double speed = 2.0;   // リングの前進速度
        const double omega = 0.1;   // 回転角速度（rad/frame）

        // リングの中心位置（敵の位置から少し下）
        const double cx = pEnemyShotSet->x;
        const double cy = pEnemyShotSet->y + 20.0;

        // リングを構成する弾を生成
        for (int i = 0; i < N; i++)
        {
            pEnemyShot = new sEnemyShot;

            // リング上の角度（2π * i / N）
            const double baseAngle = 2.0 * DX_PI * i / N;

            // 初期位置（xy平面の円）
            pEnemyShot->x = cx + R * cos(baseAngle);
            pEnemyShot->y = cy + R * sin(baseAngle);

            // 奥行き方向の回転用パラメータ（z軸相当）
            // param_d[0] を z の値として使う（-R〜+R）
            pEnemyShot->param_d[0] = R * sin(baseAngle);

            // 弾の向きはリングの中心方向（内側へ向ける）
            pEnemyShot->muki = atan2(cy - pEnemyShot->y, cx - pEnemyShot->x);

            // 弾の速さはリングの前進速度
            pEnemyShot->speed = speed;

            // 弾の種類と色
            // ここでは「中玉（MediumBall）」＋ 色インデックスで立体感を出す
            const int colorIndex = (i % 9); // 0〜8（赤〜橙）
            pEnemyShot->kind = img_enemyShotMediumBall[colorIndex];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // リング全体の回転用パラメータを pEnemyShotSet に保存
        pEnemyShotSet->param_d[0] = omega; // 回転角速度
        pEnemyShotSet->param_d[1] = R;     // リング半径
    }

    // 2回目以降のフレーム：弾の位置を更新（回転＋前進）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    const double omega = pEnemyShotSet->param_d[0];
    const double R = pEnemyShotSet->param_d[1];

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        // 現在のリング中心からの相対位置を計算
        const double dx = pEnemyShot->x - pEnemyShotSet->x;
        const double dy = pEnemyShot->y - pEnemyShotSet->y;

        // 現在の角度を取得
        double angle = atan2(dy, dx);

        // 角度を回転
        angle += omega;

        // 回転後の位置に更新
        pEnemyShot->x = pEnemyShotSet->x + R * cos(angle);
        pEnemyShot->y = pEnemyShotSet->y + R * sin(angle);

        // 奥行き（z）も同期して回転
        pEnemyShot->param_d[0] = R * sin(angle);

        // 弾の向きはリング中心方向に保つ
        pEnemyShot->muki = atan2(pEnemyShotSet->y - pEnemyShot->y,
            pEnemyShotSet->x - pEnemyShot->x);

        pEnemyShot = pEnemyShot->next;
    }

    // 前進（リング全体がプレイヤー方向へ）
    pEnemyShotSet->x += 2 * cos(pEnemyShotSet->muki);
    pEnemyShotSet->y += 2 * sin(pEnemyShotSet->muki);
}

// ============================================================
// 敵本体のパターン（EnemyPat_3D_Sakana）
// ============================================================
void EnemyPat_3D_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1)
    {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else
    {
        // 敵の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60)
            muki *= -1;
    }

    // 一定間隔で3D回転リング弾幕を発射
    if (count % 60 == 0)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = Shot3DRing;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        // 弾リストのダミーヘッドを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}