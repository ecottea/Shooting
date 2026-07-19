// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ピラミッド・ディセント
static void ShotPyramid(sEnemyShotSet* pEnemyShotSet)
{
    int phase = pEnemyShotSet->param_i[0];
    double cx = pEnemyShotSet->x;
    double cy = pEnemyShotSet->y;

    // 初期化：ピラミッドの辺を構成する弾を生成
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        int num_per_side = 20; // 1辺あたりの弾数
        double R = pEnemyShotSet->param_d[0]; // ピラミッドのサイズ（中心から頂点の距離）
        double W = R * sqrt(3.0) / 2.0;       // 底辺の半分

        // 頂点の相対座標
        double ax = 0.0, ay = -R;             // 頂上
        double bx = -W, by = R / 2.0;         // 左下
        double cx2 = W, cy2 = R / 2.0;        // 右下

        // 弾生成ラムダ
        auto create_shot = [&](double x, double y, int kind) {
            sEnemyShot* p = new sEnemyShot;
            p->x = cx + x;
            p->y = cy + y;
            p->muki = 0;
            p->speed = 0.0; // 位置は毎フレーム計算して上書きするため0にしておく
            p->kind = kind;
            p->count = 0;
            p->param_d[0] = x; // 中心からの相対座標を保持
            p->param_d[1] = y;
            p->margin = 999;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        };

        // 3つの辺に沿って弾を配置
        for (int i = 0; i < num_per_side; i++) {
            double t = (double)i / num_per_side;
            // 左斜面 (黄)
            create_shot(ax + t * (bx - ax), ay + t * (by - ay), img_enemyShotMediumBall[1]);
            // 底辺 (橙)
            create_shot(bx + t * (cx2 - bx), by + t * (cy2 - by), img_enemyShotMediumBall[8]);
            // 右斜面 (黄)
            create_shot(cx2 + t * (ax - cx2), cy2 + t * (ay - cy2), img_enemyShotMediumBall[1]);
        }
    }

    // フェーズごとの更新処理
    if (phase == 1) {
        // 【フェーズ1：降下】
        pEnemyShotSet->y += 1.5; // 降下速度
        cy = pEnemyShotSet->y;

        // 構成弾の位置を更新（メインルーチンの移動を上書き）
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            p->x = cx + p->param_d[0];
            p->y = cy + p->param_d[1];
            p = p->next;
        }
    }
    else if (phase == 2) {
        // 【フェーズ2：停止・射撃】
        // 30フレームごとに辺から自機に向けて弾を発射
        if (pEnemyShotSet->count % 30 == 0) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->kind != img_enemyShotSmallBall[3]) {
                    double muki = atan2(player.y - p->y, player.x - p->x);

                    sEnemyShot* shot = new sEnemyShot;
                    shot->x = p->x;
                    shot->y = p->y;
                    shot->muki = muki;
                    shot->speed = 3.0;
                    shot->kind = img_enemyShotSmallBall[3]; // シアン
                    shot->count = 0;
                    shot->margin = 999;

                    shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    shot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = shot;
                    pEnemyShotSet->pEnemyShotHead->prev = shot;
                }

                p = p->next;
            }
        }

        // 構成弾の位置は固定（cy は変わらないが、念のため更新）
        //sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        //while (p != pEnemyShotSet->pEnemyShotHead) {
        //    p->x = cx + p->param_d[0];
        //    p->y = cy + p->param_d[1];
        //    p = p->next;
        //}
    }
    else if (phase == 3) {
        // 【フェーズ3：崩壊】
        // 構成弾を砂のように散らし、落下させる
        double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->count == 360) {
                // 崩壊開始時にランダムな初速度と角度（下方向）を与える
                p->speed = 1.0 + GetRand(20) / 10.0;
                // 真下(DX_PI/2)から±30度の範囲
                p->muki = (GetRand(60) - 30) / 180.0 * DX_PI + base_muki;
            }
            // 重力加速
            p->speed += 0.1;
            // 位置更新はメインルーチンに任せる (speed と muki が設定されているため)
            p = p->next;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Pyramid_Qwen()
{
    static int muki;
    static sEnemyShotSet* pPyramid = nullptr;

    if (count == 1) {
        // 敵の初期設定
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵本体の移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (count % 400 == 1) {
        // 弾幕セットの初期設定
        pPyramid = new sEnemyShotSet;
        pPyramid->count = 0;
        pPyramid->patternFunc = ShotPyramid;
        pPyramid->x = 240.0;
        pPyramid->y = -50.0; // 画面外上から開始
        pPyramid->muki = 0.0;
        pPyramid->kind = 0;

        pPyramid->param_i[0] = 1;       // Phase 1: 降下
        pPyramid->param_d[0] = 120.0;   // ピラミッドのサイズ (R)

        pPyramid->pEnemyShotHead = new sEnemyShot;
        pPyramid->pEnemyShotHead->prev = pPyramid->pEnemyShotHead;
        pPyramid->pEnemyShotHead->next = pPyramid->pEnemyShotHead;

        // リストに登録
        pPyramid->prev = enemyShotSetHead.prev;
        pPyramid->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pPyramid;
        enemyShotSetHead.prev = pPyramid;
    }
    else if (count % 400 == 120) {
        pPyramid->param_i[0] = 2; // Phase 2: 停止・射撃
        pPyramid->y = 120.0;      // 停止位置
    }
    else if (count % 400 == 360) {
        pPyramid->param_i[0] = 3; // Phase 3: 崩壊
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
}