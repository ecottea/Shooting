#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：天綱の七耀（北斗七星モチーフ）
// ============================================================

// 北斗七星の相対座標（中心を0,0とした場合の座標）
// 0:斗の先端(Alkaid), 1:柄の途中(Mizar), 2:柄の根元(Alioth), 
// 3:頭の根元(Megrez), 4:頭の下(Phecda), 5:頭の上(Merak), 6:頭の先端(Dubhe)
static const double BIG_DIPPER_X[7] = { 80.0,  40.0,   0.0, -40.0, -60.0, -20.0,  20.0 };
static const double BIG_DIPPER_Y[7] = { -40.0, -20.0,   0.0,  20.0,  60.0,  80.0,  60.0 };

static void Shot_BigDipper(sEnemyShotSet* pEnemyShotSet)
{
    int phase = pEnemyShotSet->param_i[0];

    // フェーズ遷移
    if (phase == 0 && pEnemyShotSet->count >= 210) {
        phase = 1;
        pEnemyShotSet->param_i[0] = 1;
    }
    else if (phase == 1 && pEnemyShotSet->count >= 600+300) {
        phase = 2;
        pEnemyShotSet->param_i[0] = 2;
    }
    else if (phase == 2 && pEnemyShotSet->count >= 800+9999) {
        phase = 3;
        pEnemyShotSet->param_i[0] = 3;
    }

    // 回転角度の更新 (フェーズ1以降で反時計回りに回転)
    if (phase >= 1 && phase <= 2) {
        pEnemyShotSet->param_d[0] += 0.02;
    }
    double angle = pEnemyShotSet->param_d[0];

    // 1. 全星の現在座標を計算して param_d に保存 (1+idx*2 にX, 2+idx*2 にY)
    for (int i = 0; i < 7; ++i) {
        double ox = BIG_DIPPER_X[i];
        double oy = BIG_DIPPER_Y[i];
        double sx = pEnemyShotSet->x + ox * cos(angle) - oy * sin(angle);
        double sy = pEnemyShotSet->y + ox * sin(angle) + oy * cos(angle);
        pEnemyShotSet->param_d[1 + i * 2] = sx;
        pEnemyShotSet->param_d[2 + i * 2] = sy;
    }

    // 2. 弾リストの移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // param_i[0] が 0〜6 の場合のみ、対応する星の位置に座標を強制上書き（回転表現）
        if (pShot->param_i[0] >= 0 && pShot->param_i[0] <= 6) {
            int idx = pShot->param_i[0];
            pShot->x = pEnemyShotSet->param_d[1 + idx * 2];
            pShot->y = pEnemyShotSet->param_d[2 + idx * 2];
        }
        else {
            // その他の弾（撒き弾、レーザーなど、param_i[0] == -1）は通常移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // 3. フェーズごとの弾生成処理
    if (phase == 0) {
        // 【フェーズ0: 点灯】 30フレーム(0.5秒)ごとに星を1つずつ出現させる
        if (pEnemyShotSet->count % 30 == 0 && pEnemyShotSet->param_i[1] < 7) {
            int idx = pEnemyShotSet->param_i[1];
            sEnemyShot* pStar = new sEnemyShot;
            pStar->param_i[0] = idx; // 星のインデックスを記録（座標上書き対象）
            pStar->kind = img_enemyShotSmallBall[6]; // 白の小玉
            pStar->speed = 0.0;
            pStar->x = pEnemyShotSet->param_d[1 + idx * 2];
            pStar->y = pEnemyShotSet->param_d[2 + idx * 2];

            pStar->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pStar->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pStar;
            pEnemyShotSet->pEnemyShotHead->prev = pStar;

            pEnemyShotSet->param_i[1]++;
        }
    }
    else if (phase == 1) {
        // 【フェーズ1: 天の回転】 星が回転しながら弾を撒き、線が繋がる

        // 撒き弾：各星から進行方向(接線方向)へ青い中玉を撒く
        if (pEnemyShotSet->count % 10 == 0) {
            for (int i = 0; i < 7; ++i) {
                sEnemyShot* pSub = new sEnemyShot;
                pSub->param_i[0] = -1; // 星の座標上書き対象外にする
                pSub->x = pEnemyShotSet->param_d[1 + i * 2];
                pSub->y = pEnemyShotSet->param_d[2 + i * 2];
                // 接線方向(angle + 90度) + 小さなばらつき
                pSub->muki = angle + DX_PI / 2.0 + (GetRand(20) - 10) / 100.0;
                pSub->speed = 3.0;
                pSub->kind = img_enemyShotMediumBall[4]; // 青の中玉

                pSub->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSub->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSub;
                pEnemyShotSet->pEnemyShotHead->prev = pSub;
            }
        }

        // 星座の線：星と星の間を白い短レーザーで繋ぐ
        if (pEnemyShotSet->count % 15 == 0) {
            int connections[][2] = { {0,1}, {1,2}, {2,3}, {3,4}, {4,5}, {5,6} };
            for (int i = 0; i < 6; ++i) {
                int idx1 = connections[i][0];
                int idx2 = connections[i][1];
                double x1 = pEnemyShotSet->param_d[1 + idx1 * 2];
                double y1 = pEnemyShotSet->param_d[2 + idx1 * 2];
                double x2 = pEnemyShotSet->param_d[1 + idx2 * 2];
                double y2 = pEnemyShotSet->param_d[2 + idx2 * 2];

                sEnemyShot* pLaser = new sEnemyShot;
                pLaser->param_i[0] = -1; // 星の座標上書き対象外にする
                pLaser->x = x1;
                pLaser->y = y1;
                pLaser->muki = atan2(y2 - y1, x2 - x1);
                pLaser->speed = 20.0; // 高速で飛ばしてすぐに画面外へ消去させる
                pLaser->kind = img_enemyShotLaser[6]; // 白の短レーザー

                pLaser->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pLaser->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pLaser;
                pEnemyShotSet->pEnemyShotHead->prev = pLaser;
            }
        }
    }
    else if (phase == 2) {
        // 【フェーズ2: 招揺(とどめ)】 回転が止まり、斗の先端(星0)がプレイヤーを指し示して攻撃
        if (pEnemyShotSet->count % 8 == 0) {
            double x0 = pEnemyShotSet->param_d[1 + 0 * 2];
            double y0 = pEnemyShotSet->param_d[2 + 0 * 2];

            sEnemyShot* pAtk = new sEnemyShot;
            pAtk->param_i[0] = -1; // 星の座標上書き対象外にする
            pAtk->x = x0;
            pAtk->y = y0;
            pAtk->muki = atan2(player.y - y0, player.x - x0);
            pAtk->speed = 6.0;
            pAtk->kind = img_enemyShotLaser[0]; // 赤の短レーザー

            pAtk->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pAtk->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pAtk;
            pEnemyShotSet->pEnemyShotHead->prev = pAtk;
        }
    }
}

// 敵本体のパターン
void EnemyPat_BigDipper_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 120.0; // やや上めに配置し、回転の中心を画面中央寄りにする
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // 弾幕セットの初期化
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = Shot_BigDipper;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = 0; // phase
        pEnemyShotSet->param_i[1] = 0; // 点灯済み星の数
        pEnemyShotSet->param_d[0] = 0.0; // 回転角度

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵本体の左右移動
        enemy.x += 0.8 * (double)muki;
        if (count % 180 == 90) {
            muki *= -1;
        }

        // 弾幕セットの位置を敵本体に追従させる（北極星のように中心を維持）
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == Shot_BigDipper) {
                pSet->x = enemy.x;
                pSet->y = enemy.y;
            }
            pSet = pSet->next;
        }
    }
}