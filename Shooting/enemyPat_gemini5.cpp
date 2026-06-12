// enemyPat_waterfall.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：滝（激流と水しぶき、そして川の霧）
static void ShotWaterfall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ------------------------------------------------------------
    // 1. 弾の生成（滝の本体：上から下へ激しく注ぐ）
    // ------------------------------------------------------------
    if (pEnemyShotSet->count % 2 == 0) { // 2フレームに1回、激しく連射

        // 10フレームに1回、軽快な発射音を鳴らす
        if (pEnemyShotSet->count % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 1回につき5発の「水滴」を落とす
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;

            // 滝の幅を表現するために、敵のX座標から左右にランダムに散らす
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(440) - 220);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(20) - 10);;

            // やや下向き（真下が DX_PI / 2）に、わずかな傾きを持たせる
            pEnemyShot->muki = (DX_PI / 2.0) + ((GetRand(30) - 15) / 180.0 * DX_PI);

            // 初速（少し勢いよく飛び出させる）
            pEnemyShot->speed = (150 + GetRand(150)) / 100.0;

            // 弾の種類：水の色（3:シアン、4:青、6:白）をランダムにブレンド
            int rand_color = GetRand(2);
            int color = (rand_color == 0) ? 3 : (rand_color == 1) ? 4 : 6;

            // 形状：基本は水滴に見える「鱗弾」、時々「小玉」を混ぜる
            if (GetRand(3) == 0) {
                if (color == 6) color = 3;
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotScale[color];
            }

            // リストの末尾に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ------------------------------------------------------------
    // 2. 定期的な演出（滝壺の霧・広がる水蒸気）
    // ------------------------------------------------------------
    if (pEnemyShotSet->count % 60 == 0) {
        // 重めの音と共に、全方位にふんわり広がる水飛沫の輪を展開
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int way = 16; // 16方向
        for (int i = 0; i < way; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (360.0 / way * i) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.5; // 遅い速度でじわっと広がる
            pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアンの中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ------------------------------------------------------------
    // 3. 弾の移動処理（物理演算：重力と滝壺での跳ね返り）
    // ------------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 現在の「角度」と「速度」から、X成分・Y成分の速度ベクトルを取り出す
        double vx = pEnemyShot->speed * cos(pEnemyShot->muki);
        double vy = pEnemyShot->speed * sin(pEnemyShot->muki);

        // 重力加速度をY方向に加算（下に向かってどんどん加速する）
        vy += 0.12;

        // 【滝壺のギミック】画面下部（y = 420）に達した時の処理
        // 「y > 420」かつ「まだ下向きに落ちている(vy > 0)」場合のみ跳ね返す
        if (pEnemyShot->kind != img_enemyShotSmallBall[6] && pEnemyShot->y > 380.0 && vy > 0.0) {
            vy = -(1.0 + (GetRand(200) / 100.0)); // 力を上向き反転（放物線を描いて跳ね上がる）
            vx += (GetRand(200) - 100) / 40.0;    // 左右にランダムにしぶきを散らす

            if (GetRand(2) != 0) {
                vy -= 9;
            }

            // 跳ね返った瞬間に、弾の見た目を「水しぶき（白い小玉）」に変化させる！
            pEnemyShot->kind = img_enemyShotSmallBall[6];
        }

        if (pEnemyShot->kind == img_enemyShotSmallBall[6] && vy > 0) {
            double spd = sqrt(vx * vx + vy * vy);
            if (spd > 1.5) {
                vx /= spd; vy /= spd;
            }
        }

        // 座標を更新
        pEnemyShot->x += vx;
        pEnemyShot->y += vy;

        // 変化した速度ベクトルから、次のフレームのための speed と muki を逆算して書き戻す
        pEnemyShot->speed = sqrt(vx * vx + vy * vy);
        pEnemyShot->muki = atan2(vy, vx);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体の行動パターン（滝となるボス）
void EnemyPat_Waterfall_Gemini()
{
    // 初期配置とボスのステータス設定
    if (count == 1) {
        enemy.x = 240.0; // 画面中央上部
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 150; // 滝らしくタフに
    }
    else {
        // 滝自体がダイナミックに左右に少しだけ揺れる（サイン波で滑らかに移動）
        enemy.x = 240.0 + sin(count / 40.0) * 60.0;
    }

    // 4フレームに1回、滝の弾幕セットを供給し続ける
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWaterfall;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 真下向き

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}