// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：サンフラワー・スパイラル (ひまわり)
// ============================================================
static void ShotSunflower(sEnemyShotSet* pEnemyShotSet)
{
    int c = pEnemyShotSet->count;

    // 【フェーズ1: 芽吹き】(0〜59フレーム)
    // 中心から黄色い小粒弾がゆっくりと放射状に展開
    if (c < 60) {
        if (c == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        if (c % 6 == 0) {
            for (int i = 0; i < 8; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = (i / 8.0) * 2.0 * DX_PI + c * 0.05; // 全体がゆっくり回転
                pShot->speed = 1.5 + 0.3;
                pShot->kind = img_enemyShotSmallBall[1]; // 1:黄 (小玉)
                pShot->param_i[0] = 0; // 層ID: 0 (中心)
                pShot->margin = 480;

                // 循環二重連結リストに追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
    // 【フェーズ2: 開花】(60〜179フレーム)
    // 3層の花弁が異なる回転速度で螺旋状に広がる
    else if (c < 180) {
        // 内側: 6発の花弁（時計回り）
        if (c == 60) {
            for (int i = 0; i < 6; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = (i / 6.0) * 2.0 * DX_PI;
                pShot->speed = 1.8 + 0.3;
                pShot->kind = img_enemyShotMediumBall[8]; // 8:橙 (中玉)
                pShot->param_i[0] = 1; // 層ID: 1
                pShot->margin = 480;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
        // 中間: 12発の花弁（反時計回り）
        if (c == 90) {
            for (int i = 0; i < 12; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = (i / 12.0) * 2.0 * DX_PI + (DX_PI / 12.0); // 位相をずらす
                pShot->speed = 2.2 + 0.3;
                pShot->kind = img_enemyShotScale[8]; // 8:橙 (鱗弾)
                pShot->param_i[0] = 2; // 層ID: 2
                pShot->margin = 480;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
        // 外側: 18発の花弁（時計回り）
        if (c == 120) {
            for (int i = 0; i < 18; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = (i / 18.0) * 2.0 * DX_PI;
                pShot->speed = 2.6 + 0.3;
                pShot->kind = img_enemyShotMediumOval[8]; // 8:橙 (中楕円弾)
                pShot->param_i[0] = 3; // 層ID: 3
                pShot->margin = 480;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
    // 【フェーズ3: 満開・花粉散らし】(180〜239フレーム)
    // 花弁の間から白色の微粒子がランダムに飛び散る
    else if (c < 240) {
        if (c == 180) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        if (c % 2 == 0) {
            for (int i = 0; i < 6 * 2; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x + (GetRand(100) - 50); // 少し位置をばらけさせる
                pShot->y = pEnemyShotSet->y + (GetRand(100) - 50);
                // GetRand(359) は 0〜359 を返す (360種類)
                pShot->muki = GetRand(359) * (DX_PI / 180.0);
                pShot->speed = 2.0 + GetRand(300) / 100.0; // 2.0 〜 5.0 のランダム速度
                pShot->kind = img_enemyShotSmallBall[6]; // 6:白 (小玉)
                pShot->param_i[0] = 4; // 層ID: 4 (花粉)
                pShot->margin = 480;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
    // 【フェーズ4: 収束】(240〜299フレーム)
    // 全ての弾が敵本体に向かって逆回転で収束し、色が赤く変化する
    else if (c < 300) {
        if (c == 240) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                // 敵の現在位置に向かって角度を補正
                pShot->muki = atan2(enemy.y - pShot->y, enemy.x - pShot->x);
                pShot->speed = 6.0; // 収束速度
                pShot->kind = img_enemyShotSmallBall[0]; // 0:赤 (小玉)
                pShot->param_i[0] = 5; // 層ID: 5 (収束中)
                pShot = pShot->next;
            }
        }
    }
    // 【フェーズ5: パターン終了】(300フレーム以降)
    // 何もしない（メインルーチンが自動的に弾を画面外削除し、セットを管理します）

    // ============================================================
    // 全弾の移動処理と、螺旋回転の適用
    // ============================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 層IDに応じた回転処理（収束中(5)は回転させない）
        if (pShot->param_i[0] == 1) {
            pShot->muki += 0.04;  // 内側: 時計回り
        }
        else if (pShot->param_i[0] == 2) {
            pShot->muki -= 0.025; // 中間: 反時計回り
        }
        else if (pShot->param_i[0] == 3) {
            pShot->muki += 0.015; // 外側: 時計回り
        }

        // 通常の移動計算
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}


// ============================================================
// 敵本体のパターン制御
// ============================================================
void EnemyPat_Sunflower_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200; // ひまわりパターン用のHP
    }
    else {
        // 敵本体がゆっくりと揺れる演出（ひまわりが風に揺れるイメージ）
        enemy.x = 240.0 + sin(count * 0.02) * 80.0;
        enemy.y = 80.0 + cos(count * 0.03) * 40.0;
    }

    // パターン開始トリガー（出現後少し待ってから発動）
    if (count % 250 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSunflower;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 弾リストのヘッド初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}