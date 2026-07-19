// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ワインダー弾幕：「螺旋の時計塔」
// ============================================================
// 概要：画面上部に配置された複数の「柱」から、螺旋状にうねる弾が
//       時計回り・反時計回りに交互に降り注ぐパターン。
// 各柱から発射される弾は、発射ごとに水平方向の発射角度をわずかに
// ずらすことで鞭のような軌道を描く（ワインダー）。
// ============================================================

// 柱の数
static const int NUM_PILLARS = 8;
// 角度変位の最大値（ラジアン）
static const double MAX_ANGLE_OFFSET = 30.0 / 180.0 * DX_PI;
// 角度変位の増分（ラジアン）
static const double ANGLE_STEP = 3.0 / 180.0 * DX_PI;

// ワインダー弾幕のショットセット処理
static void ShotWinder(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // kind: 柱のインデックス (0〜NUM_PILLARS-1)
    int pillarIdx = pEnemyShotSet->kind;

    // param_d[0] : 現在の角度オフセット
    // param_d[1] : 角度オフセットの増分方向 (+1 or -1)
    // param_i[0] : 発射間隔カウンタ

    // 初期化（初回呼び出し時）
    if (pEnemyShotSet->count == 0) {
        // 奇数柱は右にうねる、偶数柱は左にうねる
        if (pillarIdx % 2 == 0) {
            pEnemyShotSet->param_d[1] = 1.0;   // 右方向に増加
        }
        else {
            pEnemyShotSet->param_d[1] = -1.0;  // 左方向に増加
        }
        pEnemyShotSet->param_d[0] = 0.0;
        pEnemyShotSet->param_i[0] = 0;

        // 効果音（初回のみ）
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 一定間隔で弾を発射（柱ごとに位相をずらす）
    // param_i[0] で発射タイミングを管理
    int fireInterval = 6;  // 6フレームごとに発射
    int phaseOffset = pillarIdx * 3;  // 柱ごとに位相をずらす

    if ((pEnemyShotSet->count + phaseOffset) % fireInterval == 0) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        // 基本方向は下向き(90度 = PI/2) + 角度オフセット
        pEnemyShot->muki = DX_PI / 2.0 + pEnemyShotSet->param_d[0];
        pEnemyShot->speed = 2.0 + GetRand(50) / 100.0;  // 2.0〜2.5の速度

        // 弾の種類と色
        // 奇数柱：青系（シアン〜青）、偶数柱：赤系（赤〜橙）
        int colorIdx;
        int ballKindIdx = pillarIdx % 6;
        if (pillarIdx % 2 == 0) {
            // 偶数柱：赤系
            colorIdx = GetRand(2);  // 0:赤, 1:黄, 8:橙 の中から
            if (colorIdx == 2) colorIdx = 8;  // 2を8(橙)に変換
        }
        else {
            // 奇数柱：青系
            colorIdx = 3 + GetRand(2);  // 3:シアン, 4:青, 5:マゼンタ
        }

        switch (ballKindIdx) {
        case 0:
            pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
            break;
        case 1:
            pEnemyShot->kind = img_enemyShotMediumBall[colorIdx];
            break;
        case 2:
            pEnemyShot->kind = img_enemyShotLargeBall[colorIdx];
            break;
        case 3:
            pEnemyShot->kind = img_enemyShotBullet[colorIdx];
            break;
        case 4:
            pEnemyShot->kind = img_enemyShotScale[colorIdx];
            break;
        case 5:
            pEnemyShot->kind = img_enemyShotDiamond[colorIdx];
            break;
        }

        // 弾のパラメータに柱インデックスを記録（必要に応じて）
        pEnemyShot->param_i[0] = pillarIdx;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 角度オフセットを更新（往復動作）
        pEnemyShotSet->param_d[0] += ANGLE_STEP * pEnemyShotSet->param_d[1];

        // 最大角度に達したら方向を反転
        if (fabs(pEnemyShotSet->param_d[0]) >= MAX_ANGLE_OFFSET) {
            pEnemyShotSet->param_d[1] *= -1.0;
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

// ============================================================
//  敵本体のパターン：EnemyPat_Winder_Kimi
// ============================================================
// 「螺旋の時計塔」ワインダー弾幕を展開する敵本体の動作。
// 画面上部に8本の柱を等間隔で配置し、各柱から螺旋状の弾を発射。
// ============================================================

void EnemyPat_Winder_Kimi()
{
    static int muki;
    static int shot_count;
    static bool pillarsCreated = false;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;  // 画面上部に配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        pillarsCreated = false;
    }
    else {
        // 敵本体を左右にゆっくり移動
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 柱（ショットセット）を一度だけ生成
    if (!pillarsCreated) {
        double pillarSpacing = 480.0 / (NUM_PILLARS + 1);

        for (int i = 0; i < NUM_PILLARS; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotWinder;

            // 柱のX座標を等間隔で配置
            pEnemyShotSet->x = pillarSpacing * (i + 1);
            pEnemyShotSet->y = enemy.y - 20.0;  // 敵本体の少し下

            // 基本方向は下向き
            pEnemyShotSet->muki = DX_PI / 2.0;

            // kind に柱のインデックスを格納
            pEnemyShotSet->kind = i;

            // パラメータ初期化
            pEnemyShotSet->param_d[0] = 0.0;
            pEnemyShotSet->param_d[1] = 0.0;
            pEnemyShotSet->param_i[0] = 0;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        pillarsCreated = true;
    }
}