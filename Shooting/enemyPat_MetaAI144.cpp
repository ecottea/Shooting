#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 絡み蔦 - 茎 0.6px/f + 葉 0.15px/f
static void ShotKaramiZuta(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 生成時
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 全体回転は pEnemyShotSet->muki に入っている
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 12方向 + 全体回転
            pEnemyShot->muki = pEnemyShotSet->muki + (DX_PI * 2.0 / 12.0) * i;
            pEnemyShot->speed = 0.6; // 1px以下

            pEnemyShot->kind = img_enemyShotMediumOval[2]; // 茎: 中楕円・緑

            pEnemyShot->param_i[0] = 0; // 0:茎 1:葉
            pEnemyShot->param_i[1] = 0; // 曲がりカウント用ではない、countで判定するので未使用

            // リスト追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム更新
    // 追加しながら回すので next を先に確保
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // 茎弾の処理
        if (pShot->param_i[0] == 0) {
            // 60フレーム(1秒)ごとに90度時計回りに曲がる
            // pShot->count はメインルーチンで自動インクリメントされる
            if (pShot->count != 0 && pShot->count % 60 == 0) {
                pShot->muki += DX_PI / 2.0/10;

                // 曲がり角から葉弾を2枚出す
                for (int k = 0; k < 2; k++) {
                    sEnemyShot* pLeaf = new sEnemyShot;
                    pLeaf->x = pShot->x;
                    pLeaf->y = pShot->y;
                    // 茎の進行方向から少し外側にばらける
                    // GetRand(x)は0〜xを返すので -20〜20 を作るには GetRand(40)-20
                    double rnd = (double)(GetRand(40) - 20) / 180.0 * DX_PI;
                    double offset = (k == 0) ? 0.35 : -0.35;
                    pLeaf->muki = pShot->muki + rnd + offset;

                    // 0.12〜0.18 px/f ランダム
                    pLeaf->speed = 0.12 + (double)GetRand(6) / 100.0;

                    pLeaf->kind = img_enemyShotScale[2]; // 葉: 鱗弾・緑
                    pLeaf->param_i[0] = 1; // 葉タイプ

                    pLeaf->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pLeaf->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pLeaf;
                    pEnemyShotSet->pEnemyShotHead->prev = pLeaf;
                }
            }
        }

        // 移動 共通 - 1px以下
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pNext;
    }
}

void EnemyPat_SlowBullet_MetaAI()
{
    static int dir;
    static double baseRot;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 300;
        dir = 1;
        baseRot = 0.0;
        shot_count = 0;
    }
    else {
        // 敵本体はゆっくり左右に
        enemy.x += 0.5 * (double)dir;
        if (count % 240 == 120) dir *= -1;

        // 蔦全体をゆっくり回転させる
        baseRot += 0.015; // 約0.86度/フレーム
    }

    // 120フレームごとに蔦の束を生成。低速で残留するため間隔は長め
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKaramiZuta;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = baseRot; // 全体回転
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}