// EnemyPat_Volcano.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  火山スプレー（小規模噴火）  patternFunc
//  - 火口から小さな溶岩弾が上方へ噴き出し、弧を描いて落下する
// ------------------------------------------------------------
static void VolcanoSpray(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 軽い噴火音
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 15発の溶岩弾を生成
        for (int i = 0; i < 15; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // 火口付近の位置にばらつき
            pShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pShot->y = pEnemyShotSet->y + GetRand(16) - 8;

            // 上方を中心に ±30度の広がり
            double baseAngle = -DX_PI / 2;  // 真上 (y軸上向き)
            double spread = (GetRand(60) - 30) / 180.0 * DX_PI;
            pShot->muki = baseAngle + spread;

            // 初速：そこそこ速い
            pShot->speed = 2.5 + GetRand(30) / 10.0 - 1.3; // 2.5～5.5

            // 色：赤(0)、橙(8)、黄(1) のいずれか
            int col = (GetRand(3) == 0) ? 0 : (GetRand(2) == 0 ? 8 : 1);
            pShot->kind = img_enemyShotSmallBall[col];

            // 初期角度を覚えておく（落下モーション用）
            pShot->param_d[0] = pShot->muki;

            pShot->margin = 999;

            // リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // ショットの更新（毎フレーム）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 経過フレーム数に応じて軌道変更（噴出 → 落下）
        int age = pShot->count;   // メインルーチンが自動インクリメント
        if (age < 20) {
            // 上昇中：初期角度を維持
            pShot->muki = pShot->param_d[0];
        }
        else if (age < 40) {
            // 弧を描いて徐々に真下へ向かう（線形補間）
            double t = (age - 20) / 20.0;
            double from = pShot->param_d[0];
            double to = from > -DX_PI / 2 ? DX_PI / 2 : -3 * DX_PI / 2;  // 真下
            pShot->muki = from + t * (to - from);
            pShot->speed += 0.03;     // 少し加速
        }
        else {
            // 完全に落下状態
            pShot->muki = DX_PI / 2;
            pShot->speed += 0.06;     // 重力を模した加速
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  大噴火（広範囲爆発）  patternFunc
//  - 火口から大量の溶岩弾が四方（主に上方）へ飛び散り、
//    一定時間後に全て落下する
// ------------------------------------------------------------
static void VolcanoEruption(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 重い噴火音
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 一度に50発
        for (int i = 0; i < 50; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->x = pEnemyShotSet->x + GetRand(60) - 30;
            pShot->y = pEnemyShotSet->y + GetRand(20) - 10;

            // 上方向を中心に左右に広がる角度（-90°±80°程度）
            double baseAngle = -DX_PI / 2;
            double spread = (GetRand(160) - 80) / 180.0 * DX_PI;
            pShot->muki = baseAngle + spread;

            // 初速は大きめ
            pShot->speed = 3.0 + GetRand(30) / 5.0;  // 3.0～9.0

            // 中玉や大玉、ダイヤ型も混ぜる
            int type = GetRand(3);  // 0:小玉,1:中玉,2:大玉,3:ダイヤ
            int col = GetRand(2) == 0 ? 0 : (GetRand(1) == 0 ? 8 : 1); // 赤/橙/黄
            switch (type) {
            case 0: pShot->kind = img_enemyShotSmallBall[col];  break;
            case 1: pShot->kind = img_enemyShotMediumBall[col]; break;
            case 2: pShot->kind = img_enemyShotLargeBall[col];  break;
            case 3: pShot->kind = img_enemyShotDiamond[col];    break;
            }

            pShot->param_d[0] = pShot->muki;   // 初期角度記憶
            pShot->param_d[1] = 1.0;           // 落下開始フラグ用 (0:未、1:済)

            pShot->margin = 999;

            // リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        int age = pShot->count;

        if (age < 25) {
            // 上昇・拡散期
            pShot->muki = pShot->param_d[0];
        }
        else if (age < 45) {
            // 頂点から落下へ遷移
            double t = (age - 25) / 20.0;
            double from = pShot->param_d[0];
            double to = from > -DX_PI / 2 ? DX_PI / 2 : -3 * DX_PI / 2;
            pShot->muki = from + t * (to - from);
            pShot->speed += 0.05;
        }
        else {
            // 全弾が真下へ加速
            pShot->muki = DX_PI / 2;
            pShot->speed += 0.08;
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体のパターン（火山）
// ------------------------------------------------------------
void EnemyPat_Volcano_DeepSeek()
{
    static int moveDir;   // 左右移動方向

    // 初回フレームの初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0;            // 画面上部に火口を構える
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
    }
    else {
        // ゆっくり左右に動く
        enemy.x += 0.5 * moveDir;
        if (count % 180 == 90) moveDir *= -1;
    }

    // ----- 小噴火（火口スプレー） : 10フレーム毎 -----
    if (count % 10 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = VolcanoSpray;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = -DX_PI / 2;  // 真上（Shot内では無視）

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ----- 大噴火 : 150フレーム毎 -----
    if (count % 150 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = VolcanoEruption;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 8.0;
        pSet->muki = -DX_PI / 2;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}