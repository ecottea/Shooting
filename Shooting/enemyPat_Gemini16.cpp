// enemyPat_igo.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =========================================================
// 弾幕パターン：布石と崩壊 (囲碁モチーフ)
// =========================================================
static void ShotIgoPattern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int setCount = pEnemyShotSet->count;

    // 【フェーズ1】碁盤に石を打つ (0〜180フレーム)
    // 10フレームごとに3つずつ石(弾)を盤面に配置していく
    if (setCount <= 180 && setCount % 10 == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;

            // 9x9の碁盤の交点をランダムに決定 (48px間隔, 画面端に余白48px)
            // GetRand(8) は 0〜8 の整数を返すので、ちょうど9x9のグリッドになる
            int gx = GetRand(8);
            int gy = GetRand(8);
            double targetX = 48.0 + gx * 48.0;
            double targetY = 48.0 + gy * 48.0;

            // 発射元（ボス）から目標の交点への角度と距離を計算
            double dx = targetX - pEnemyShotSet->x;
            double dy = targetY - pEnemyShotSet->y;
            pEnemyShot->muki = atan2(dy, dx);

            // 30フレームで目標座標にちょうど到達する初速を設定
            pEnemyShot->speed = sqrt(dx * dx + dy * dy) / 30.0;

            // 発射元の初期座標
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 石の色を決定（黒と白をランダムに配置）
            int color = (GetRand(1) == 0) ? 7 : 6; // 7:黒, 6:白
            pEnemyShot->kind = img_enemyShotLargeBall[color]; // 碁石らしさを出すため大玉を使用

            // リストへの追加処理
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 崩壊の瞬間に重い効果音を鳴らす
    if (setCount == 240) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 弾ごとの状態更新ループ
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 【フェーズ2】指定位置に到達したら停止する
        // pEnemyShot->count はメイン側でインクリメントされるため、発射後30Fの瞬間を捉える
        if (pEnemyShot->count == 30) {
            pEnemyShot->speed = 0.0; // 盤面に定着させて動かさない
        }

        // 【フェーズ3】盤面崩壊 (240フレーム目で一斉に動き出す)
        if (setCount == 240) {
            // 黒石(7)は自機を狙って殺到する（直接的な脅威）
            if (pEnemyShot->kind == img_enemyShotLargeBall[7]) {
                pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                pEnemyShot->speed = (150 + GetRand(150)) / 100.0; // 1.5〜3.0の速度
            }
            // 白石(6)は真下に向かって崩れ落ちる（移動を制限する障害物）
            else {
                pEnemyShot->muki = DX_PI / 2.0; // 真下
                pEnemyShot->speed = (100 + GetRand(100)) / 100.0; // 1.0〜2.0のランダムな速度
            }
        }

        // 弾の座標更新（速度が0の時はその場に留まる）
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// =========================================================
// 敵本体のパターン
// =========================================================
void EnemyPat_Go_Gemini()
{
    // 出現時の初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // ボスは画面上部でゆったりと左右に揺れ動く
    enemy.x = 240.0 + 80.0 * sin(count / 60.0);

    // 360フレーム（約6秒）周期で囲碁弾幕を展開する
    // 弾幕展開中(0〜240F)が終わった後、少しのインターバルを挟む設計
    if (count % 360 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotIgoPattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0; // 今回の弾幕では直接使用しない

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}