// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：大文字の送り火
static void ShotDaimonji(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->count はメインルーチンで毎フレーム自動で+1される仕様
    int c = pEnemyShotSet->count;
    double cx = 240.0;
    double cy = 240.0;

    if (c == 1) {
        // 符卡開始時の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 回転角度の計算 (フェーズ1はゆっくり回転)
    double angle = c * 0.015;

    // ============================================================
    // フェーズ1：結字 〜 回転する炎の結界 (0 <= count < 300)
    // ============================================================
    if (c < 300) {
        // 中心から環状の火弾 (数秒ごとに放射)
        if (c % 40 == 0) {
            for (int i = 0; i < 12; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = cx;
                pShot->y = cy;
                pShot->muki = (DX_PI * 2.0 / 12.0) * (double)i + (double)c * 0.05;
                pShot->speed = 2.0;
                pShot->kind = img_enemyShotSmallBall[8]; // 橙
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 横画（一）の両端から、筆の払いに沿うように扇状の炎弾を放射
        if (c % 12 == 0) {
            double x1 = cx + 160.0 * cos(angle);
            double y1 = cy + 160.0 * sin(angle);
            double x2 = cx - 160.0 * cos(angle);
            double y2 = cy - 160.0 * sin(angle);

            for (int i = 0; i < 5; i++) {
                // 端点1 (右下がり方向への扇状)
                sEnemyShot* pShot1 = new sEnemyShot;
                pShot1->x = x1;
                pShot1->y = y1;
                pShot1->muki = angle + DX_PI / 2.0 + ((double)i - 2.0) * 0.2;
                pShot1->speed = 2.5;
                pShot1->kind = img_enemyShotMediumBall[8]; // 橙
                pShot1->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot1->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot1;
                pEnemyShotSet->pEnemyShotHead->prev = pShot1;

                // 端点2 (左下がり方向への扇状)
                sEnemyShot* pShot2 = new sEnemyShot;
                pShot2->x = x2;
                pShot2->y = y2;
                pShot2->muki = angle - DX_PI / 2.0 + ((double)i - 2.0) * 0.2;
                pShot2->speed = 2.5;
                pShot2->kind = img_enemyShotMediumBall[0]; // 赤
                pShot2->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot2->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot2;
                pEnemyShotSet->pEnemyShotHead->prev = pShot2;
            }
        }

        // 撇（丿）・捺（乀）の先端から自機狙いの火球
        if (c % 30 == 0) {
            double angle_s = angle + DX_PI * 2.0 / 3.0; // 120度
            double angle_n = angle + DX_PI * 4.0 / 3.0; // 240度
            double xs = cx + 200.0 * cos(angle_s);
            double ys = cy + 200.0 * sin(angle_s);
            double xn = cx + 200.0 * cos(angle_n);
            double yn = cy + 200.0 * sin(angle_n);

            double muki_s = atan2(player.y - ys, player.x - xs);
            double muki_n = atan2(player.y - yn, player.x - xn);

            for (int i = 0; i < 3; i++) {
                sEnemyShot* pShotS = new sEnemyShot;
                pShotS->x = xs;
                pShotS->y = ys;
                pShotS->muki = muki_s + ((double)i - 1.0) * 0.15;
                pShotS->speed = 3.0;
                pShotS->kind = img_enemyShotLargeBall[0]; // 赤
                pShotS->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShotS->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShotS;
                pEnemyShotSet->pEnemyShotHead->prev = pShotS;

                sEnemyShot* pShotN = new sEnemyShot;
                pShotN->x = xn;
                pShotN->y = yn;
                pShotN->muki = muki_n + ((double)i - 1.0) * 0.15;
                pShotN->speed = 3.0;
                pShotN->kind = img_enemyShotLargeBall[1]; // 黄
                pShotN->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShotN->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShotN;
                pEnemyShotSet->pEnemyShotHead->prev = pShotN;
            }
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
    // ============================================================
    // フェーズ2：崩字 〜 筆跡の乱舞 (300 <= count < 600)
    // ============================================================
    else if (c < 600) {
        // 「一」の上下スイープ：横断する炎の壁
        if (c % 15 == 0) {
            double sweep_y = 240.0 + 160.0 * sin((double)c * 0.02);
            for (int i = 0; i < 16; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = 30.0 + (double)i * 26.0;
                pShot->y = sweep_y;
                // 進行方向は上向きまたは下向きにわずかに振らせる
                pShot->muki = -DX_PI / 2.0 + ((double)GetRand(40) - 20) / 100.0;
                pShot->speed = 2.0;
                pShot->kind = img_enemyShotLaser[0]; // 赤レーザー
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        // 「丿」「乀」の交差：ハサミのように駆け巡る筆跡
        if (c % 10 == 0) {
            double angle_s = (double)c * 0.04;
            double angle_n = -(double)c * 0.04 + DX_PI;
            double xs = 240.0 + 180.0 * cos(angle_s);
            double ys = 240.0 + 180.0 * sin(angle_s);
            double xn = 240.0 + 180.0 * cos(angle_n);
            double yn = 240.0 + 180.0 * sin(angle_n);

            for (int i = 0; i < 4; i++) {
                sEnemyShot* pShotS = new sEnemyShot;
                pShotS->x = xs;
                pShotS->y = ys;
                pShotS->muki = angle_s + DX_PI / 2.0 + ((double)i - 1.5) * 0.3;
                pShotS->speed = 2.8;
                pShotS->kind = img_enemyShotMediumOval[1]; // 黄楕円
                pShotS->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShotS->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShotS;
                pEnemyShotSet->pEnemyShotHead->prev = pShotS;

                sEnemyShot* pShotN = new sEnemyShot;
                pShotN->x = xn;
                pShotN->y = yn;
                pShotN->muki = angle_n + DX_PI / 2.0 + ((double)i - 1.5) * 0.3;
                pShotN->speed = 2.8;
                pShotN->kind = img_enemyShotMediumOval[0]; // 赤楕円
                pShotN->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShotN->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShotN;
                pEnemyShotSet->pEnemyShotHead->prev = pShotN;
            }
        }
    }
    // ============================================================
    // フェーズ3：余燼 〜 散華 (count >= 600)
    // ============================================================
    else if (c == 600) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        // 中心から全方位に青白い鱗弾を放射
        for (int i = 0; i < 36; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = 240.0;
            pShot->y = 240.0;
            pShot->muki = (DX_PI * 2.0 / 36.0) * (double)i;
            // GetRand(200) は 0〜200 の整数を返すため、/100.0 で 0.0〜2.0 の小数になる
            pShot->speed = 3.0 + (double)GetRand(200) / 100.0;
            pShot->kind = (i % 2 == 0) ? img_enemyShotScale[3] : img_enemyShotScale[6]; // シアン / 白
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
    else if (c < 720) {
        // 散華の持続：画面中央からわずかにずれた位置から白の菱形弾が渦を巻く
        if (c % 8 == 0) {
            for (int i = 0; i < 8; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = 240.0 + (double)(GetRand(40) - 20);
                pShot->y = 240.0 + (double)(GetRand(40) - 20);
                pShot->muki = (DX_PI * 2.0 / 8.0) * (double)i + (double)c * 0.1;
                pShot->speed = 2.2;
                pShot->kind = img_enemyShotDiamond[6]; // 白菱形
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 弾の移動処理
    // (countのインクリメントや画面外消去はメインルーチンで行われる仕様のため、ここでは移動のみ記述)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Daimonji_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0; // やや上めに配置し、画面中央で「大」の字が展開されるようにする
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵の動き：符卡中はゆっくりと左右に揺れながら、わずかに上下する
        enemy.x = 240.0 + 80.0 * sin((double)count * 0.01);
        enemy.y = 80.0 + 15.0 * sin((double)count * 0.02);
    }

    // 弾幕セットの生成はパターン開始時に1回だけ行う
    if (count % 780 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDaimonji;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}