// enemyPat_Tmp.cpp
// 紅葉（もみじ）をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --------------------------------------------------
// 弾幕パターン：紅葉吹雪（ひらひら舞い落ちる葉）
// --------------------------------------------------
static void MapleLeafScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // この ShotSet の初回フレームだけ弾を生成する
    if (pEnemyShotSet->count == 0) {
        // 軽い効果音（葉が舞うイメージ）
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int SHOT_NUM = 10;   // 一度に出す葉の数

        for (int i = 0; i < SHOT_NUM; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置：敵の周囲にばらつかせる
            pEnemyShot->x = pEnemyShotSet->x + GetRand(80) - 40;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(30) - 15;

            // 基本は真下方向。±40度 の範囲でランダムに散らす
            double baseAngle = DX_PI / 2.0;   // 下向き
            double spread = (GetRand(80) - 40) * (DX_PI / 180.0);
            pEnemyShot->muki = baseAngle + spread;

            // 速度はゆっくりめ（葉がふわりと落ちる）
            pEnemyShot->speed = 0.8 + GetRand(70) / 100.0;

            // ---------- 紅葉らしい色とかたち ----------
            // 色：0:赤, 1:黄, 5:マゼンタ の3種類からランダム
            int colorIdx = GetRand(2);
            int color;
            if (colorIdx == 0) color = 0;  // 赤
            else if (colorIdx == 1) color = 1;  // 黄
            else                   color = 5;  // マゼンタ（紅葉の赤紫）

            // 形：小玉（まるい葉）と菱形（もみじの葉先）を混ぜる
            int type = GetRand(2);  // 0,1,2 の3通り
            if (type == 0) {
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            }
            else if (type == 1) {
                pEnemyShot->kind = img_enemyShotDiamond[color];
            }
            else {
                // 3通りめとして中玉を入れてもよいが、ここでは菱形に寄せる
                pEnemyShot->kind = img_enemyShotDiamond[color];
            }

            // 双方向リストに繋ぐ
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --------------------------------------------------
    // 毎フレームの弾移動＆“ひらひら”揺らぎ
    // --------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 直線移動（基本）
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 紅葉が風に舞うように、角度をゆるやかに揺らす
        // count と x 座標で位相をずらし、弾ごとに異なるふるまいにする
        pEnemyShot->muki += 0.04 * sin(pEnemyShot->count * 0.18 + pEnemyShot->x * 0.03);

        pEnemyShot = pEnemyShot->next;
    }
}

// --------------------------------------------------
// 敵本体のパターン
// --------------------------------------------------
void EnemyPat_Maple_DeepSeek()
{
    static int muki;      // 左右移動の向き

    // 初回だけ初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆっくり左右に動く
        enemy.x += 0.6 * muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    // 40フレームに一度、紅葉の葉をばらまく ShotSet を作成
    if (count % 10 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = MapleLeafScatter;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;  // 使わないが下向きを入れておく

        // ダミーヘッダで双方向リストを初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ゲーム全体の ShotSet リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}