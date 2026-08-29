// enemyPat_Tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//=============================================================
// 流星群崩壊パターン用 弾幕関数
//-------------------------------------------------------------
// param_i[0] : 状態 (0=落下中, 1=崩壊済み)
// param_d[0] : 本体の現在x
// param_d[1] : 本体の現在y
// param_d[2] : 本体の向き
// param_d[3] : 本体の速度
//=============================================================
static void ShotMeteor(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //---------------------------------------------------------
    // 初回：隕石本体を1発生成
    //---------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 本体（大玉・橙寄り）
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 3.2 + GetRand(12) / 10.0;          // 3.2〜4.4
        pEnemyShot->kind = img_enemyShotLargeBall[8];          // 橙
        // 少しだけ赤寄りにばらけさせる
        if (GetRand(2) == 0) pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤
        pEnemyShot->margin = 120;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 状態初期化
        pEnemyShotSet->param_i[0] = 0;                         // 落下中
        pEnemyShotSet->param_d[0] = pEnemyShot->x;
        pEnemyShotSet->param_d[1] = pEnemyShot->y;
        pEnemyShotSet->param_d[2] = pEnemyShot->muki;
        pEnemyShotSet->param_d[3] = pEnemyShot->speed;
    }

    //---------------------------------------------------------
    // 毎フレーム：本体位置を更新＆尾を残す
    //---------------------------------------------------------
    if (pEnemyShotSet->param_i[0] == 0) {
        // 本体位置をパラメータで管理（リスト先頭の弾が本体）
        sEnemyShot* pMain = pEnemyShotSet->pEnemyShotHead->next;
        if (pMain != pEnemyShotSet->pEnemyShotHead) {
            pEnemyShotSet->param_d[0] = pMain->x;
            pEnemyShotSet->param_d[1] = pMain->y;
        }

        // 尾を一定間隔で生成（小玉）
        if (pEnemyShotSet->count % 3 == 0) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->param_d[0];
            pEnemyShot->y = pEnemyShotSet->param_d[1];
            // 尾はほぼ停止〜わずかに下へ
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.3 + GetRand(8) / 10.0;
            // 黄 or 白
            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[1] : img_enemyShotSmallBall[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 崩壊判定（時間 or 位置）
        if (pEnemyShotSet->count >= 55 + GetRand(25) ||
            pEnemyShotSet->param_d[1] > 280.0 + GetRand(60)) {

            pEnemyShotSet->param_i[0] = 1; // 崩壊状態へ

            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            double cx = pEnemyShotSet->param_d[0];
            double cy = pEnemyShotSet->param_d[1];

            //-------------------------------------------------
            // 放射状の小弾（8〜12方向）
            //-------------------------------------------------
            int num = 8 + GetRand(4);
            double base = GetRand(360) / 180.0 * DX_PI;
            for (int i = 0; i < num; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx;
                pEnemyShot->y = cy;
                pEnemyShot->muki = base + (DX_PI * 2.0) * i / num;
                pEnemyShot->speed = 2.0 + GetRand(15) / 10.0;
                // 赤・橙・黄をランダム
                int col = GetRand(2);
                if (col == 0)      pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤
                else if (col == 1) pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙
                else               pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            //-------------------------------------------------
            // 追加破片（中玉を斜め下方向に数発）
            //-------------------------------------------------
            for (int i = 0; i < 4 + GetRand(3); i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx + (GetRand(30) - 15);
                pEnemyShot->y = cy + (GetRand(20) - 10);
                pEnemyShot->muki = DX_PI / 2.0 + (GetRand(80) - 40) / 180.0 * DX_PI;
                pEnemyShot->speed = 2.8 + GetRand(18) / 10.0;
                // 中玉・橙or赤
                pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotMediumBall[8] : img_enemyShotMediumBall[0];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    //---------------------------------------------------------
    // 全弾の移動（メインルーチンでcountは増えるのでここでは動かさない）
    //---------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//=============================================================
// 敵本体パターン
//=============================================================
void EnemyPat_Meteor_Grok()
{
    static int muki;
    static int shot_timer;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_timer = 0;
    }
    else {
        // ゆるやかな左右移動
        enemy.x += 0.85 * (double)muki;
        if (enemy.x < 80.0)  muki = 1;
        if (enemy.x > 400.0) muki = -1;
        if (count % 150 == 75) muki *= -1; // たまに反転
    }

    // 一定間隔で隕石を投下
    if (count % 38 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMeteor;

        // 出現位置：画面上部やや外側から
        pEnemyShotSet->x = 40.0 + GetRand(400);
        pEnemyShotSet->y = -20.0 - GetRand(30);

        // 斜め下方向（-35°〜+35°程度）
        double angle = DX_PI / 2.0 + (GetRand(70) - 35) / 180.0 * DX_PI;
        pEnemyShotSet->muki = angle;

        pEnemyShotSet->kind = shot_timer++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}