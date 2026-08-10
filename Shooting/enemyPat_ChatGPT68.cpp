// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#ifndef DX_PI
#define DX_PI 3.14159265358979323846
#endif

//----------------------------------------------------------
// 花火本体
//----------------------------------------------------------
static void ShotFirework(sEnemyShotSet* pEnemyShotSet)
{
    //------------------------------------------------------
    // 初期生成
    //------------------------------------------------------
    if (pEnemyShotSet->count == 0) {

        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;

        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;

        // 真上方向
        pShot->muki = -DX_PI / 2.0;

        // 打ち上げ速度
        pShot->speed = 4.8;

        // 白い中玉
        pShot->kind = img_enemyShotMediumBall[6];

        // 最高到達時間
        pShot->param_i[0] = 55 + GetRand(25);

        // 炸裂済みフラグ
        pShot->param_i[1] = 0;

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    //------------------------------------------------------
    // 更新
    //------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        //--------------------------------------------------
        // 上昇
        //--------------------------------------------------
        if (pShot->count < pShot->param_i[0]) {

            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;

            // 徐々に減速
            pShot->speed *= 0.982;
        }

        //--------------------------------------------------
        // 頂点で静止
        //--------------------------------------------------
        else if (pShot->count < pShot->param_i[0] + 12) {

            // 何もしない
        }

        //--------------------------------------------------
        // 炸裂開始
        //--------------------------------------------------
        else if (pShot->param_i[1] == 0) {

            pShot->param_i[1] = 1;

            if (CheckSoundMem(sound_enemyShot_extreme))
                StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            //--------------------------------------------------
            // 打ち上げ玉自身を外側リングの1発に変える
            //--------------------------------------------------
            pShot->muki = 0.0;
            pShot->speed = 3.9;
            pShot->kind = img_enemyShotSmallBall[8];
            pShot->param_i[0] = 0;

            //--------------------------------------------------
            // 外側リング（残り39発）
            //--------------------------------------------------
            for (int i = 1; i < 40; i++) {

                sEnemyShot* p = new sEnemyShot;

                double a = DX_PI * 2.0 * i / 40.0;

                p->x = pShot->x;
                p->y = pShot->y;
                p->muki = a;
                p->speed = 3.9;
                p->kind = img_enemyShotSmallBall[8];
                p->param_i[0] = 0;
                p->param_i[1] = 1;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }

            //--------------------------------------------------
            // 中間リング
            //--------------------------------------------------
            for (int i = 0; i < 24; i++) {

                sEnemyShot* p = new sEnemyShot;

                double a = DX_PI * 2.0 * i / 24.0;

                p->x = pShot->x;
                p->y = pShot->y;
                p->muki = a;
                p->speed = 2.7;
                p->kind = img_enemyShotMediumOval[1];
                p->param_i[0] = 1;
                p->param_i[1] = 1;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }

            //--------------------------------------------------
            // 内側リング
            //--------------------------------------------------
            for (int i = 0; i < 14; i++) {

                sEnemyShot* p = new sEnemyShot;

                double a = DX_PI * 2.0 * i / 14.0;

                p->x = pShot->x;
                p->y = pShot->y;
                p->muki = a;
                p->speed = 1.7;
                p->kind = img_enemyShotLargeBall[6];
                p->param_i[0] = 2;
                p->param_i[1] = 1;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }

        //--------------------------------------------------
        // リング弾・残光
        //--------------------------------------------------
        if (pShot->param_i[1]) {

            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;

            if (pShot->param_i[0] == 0)
                pShot->speed *= 0.992;
            else if (pShot->param_i[0] == 1)
                pShot->speed *= 0.994;
            else if (pShot->param_i[0] == 2)
                pShot->speed *= 0.996;
            else
                pShot->speed *= 0.985;

            //--------------------------------------------------
            // 残光
            //--------------------------------------------------
            if (pShot->param_i[0] == 0 &&
                pShot->count > 18 &&
                pShot->count % 12 == 0) {

                sEnemyShot* s = new sEnemyShot;

                s->x = pShot->x;
                s->y = pShot->y;
                s->muki = pShot->muki +
                    (GetRand(10) - 5) / 180.0 * DX_PI;
                s->speed = pShot->speed + 1.1;
                s->kind = img_enemyShotDiamond[8];
                s->param_i[0] = 3;
                s->param_i[1] = 1;

                s->prev = pEnemyShotSet->pEnemyShotHead->prev;
                s->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = s;
                pEnemyShotSet->pEnemyShotHead->prev = s;
            }
        }

        pShot = pShot->next;
    }
}

//----------------------------------------------------------
// 敵本体
//----------------------------------------------------------
void EnemyPat_Firework_ChatGPT()
{
    static int muki;
    static int firework_count;

    if (count == 1) {

        enemy.x = 240.0;
        enemy.y = 440.0;

        enemy.maxHp = enemy.hp = 20 * 60;

        muki = 1;
        firework_count = 0;

        player.y = 240;
    }
    else {

        //--------------------------------------------------
        // ボス移動
        //--------------------------------------------------
        enemy.x += 0.8 * muki;

        if (enemy.x < 80)
            muki = 1;

        if (enemy.x > 400)
            muki = -1;
    }

    enemy.hp--;


    //------------------------------------------------------
    // 花火発射
    //------------------------------------------------------
    if (count % 90 == 1) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;

        pEnemyShotSet->patternFunc = ShotFirework;

        //--------------------------------------------------
        // 花火位置
        //--------------------------------------------------
        pEnemyShotSet->x =
            enemy.x + (GetRand(80) - 40);

        pEnemyShotSet->y =
            enemy.y + 20;


        //--------------------------------------------------
        // 花火の種類
        //--------------------------------------------------
        pEnemyShotSet->kind = firework_count++;


        //--------------------------------------------------
        // 弾リスト初期化
        //--------------------------------------------------
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;

        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;


        //--------------------------------------------------
        // 登録
        //--------------------------------------------------
        pEnemyShotSet->prev =
            enemyShotSetHead.prev;

        pEnemyShotSet->next =
            &enemyShotSetHead;

        enemyShotSetHead.prev->next =
            pEnemyShotSet;

        enemyShotSetHead.prev =
            pEnemyShotSet;
    }
}