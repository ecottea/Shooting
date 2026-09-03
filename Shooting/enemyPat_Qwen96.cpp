// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：トマト投げ祭り
static void ShotTomatoFestival(sEnemyShotSet* pEnemyShotSet)
{
    // セット生成時(count==0)に効果音を再生
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 1. 弾の生成処理 (count==0 のみで初期生成)
    if (pEnemyShotSet->count == 0) {
        int type = pEnemyShotSet->kind;

        if (type == 0) {
            // 【基本直線弾】小玉(赤)
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = pEnemyShotSet->muki;
            pShot->speed = 3.0;
            pShot->kind = img_enemyShotSmallBall[0]; // 0:赤
            pShot->param_i[0] = 0; // 移動タイプ 0:直線

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        else if (type == 1) {
            // 【放物線弾】中玉(赤) - 投げられた軌道
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y - 20.0; // 少し上から投げる

            // 120フレーム程度でプレイヤーのX座標付近に到達するよう計算
            pShot->param_d[0] = (player.x - pEnemyShotSet->x) / 120.0; // X速度
            pShot->param_d[1] = -5.0;  // 初期Y速度(上向き)
            pShot->param_d[2] = 0.08;  // 重力加速度
            pShot->kind = img_enemyShotMediumBall[0]; // 0:赤
            pShot->param_i[0] = 1; // 移動タイプ 1:放物線
            pShot->margin = 240;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        else if (type == 2) {
            // 【拡散弾】鱗弾(赤) - 地面で跳ね返って広がるイメージ
            for (int i = 0; i < 5; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                // 基準角度から -0.6, -0.3, 0, 0.3, 0.6 ラジアンずらす
                pShot->muki = pEnemyShotSet->muki + (i - 2) * 0.3;
                pShot->speed = 2.5;
                pShot->kind = img_enemyShotScale[0]; // 0:赤 (回転しているように見える)
                pShot->param_i[0] = 0; // 移動タイプ 0:直線

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
        else if (type == 3) {
            // 【回転渦巻き弾】中楕円弾(赤) - 空中で回転しながら飛ぶトマト
            for (int i = 0; i < 8; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = pEnemyShotSet->muki + (i * (DX_PI * 2.0 / 8.0));
                pShot->speed = 2.0;
                pShot->kind = img_enemyShotMediumOval[0]; // 0:赤
                pShot->param_i[0] = 2; // 移動タイプ 2:回転
                pShot->param_d[0] = 0.05 / 5; // 回転角速度
                pShot->margin = 480;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
        else if (type == 4) {
            // 【最後のトマト】大玉(赤) - 遅くてデカい
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = pEnemyShotSet->muki;
            pShot->speed = 1.5;
            pShot->kind = img_enemyShotLargeBall[0]; // 0:赤
            pShot->param_i[0] = 0; // 移動タイプ 0:直線

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 2. 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int moveType = pShot->param_i[0];

        if (moveType == 0) {
            // 直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (moveType == 1) {
            // 放物線移動
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            pShot->param_d[1] += pShot->param_d[2]; // 重力加速度を加算
        }
        else if (moveType == 2) {
            // 回転移動
            pShot->muki += pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン：トマト投げ祭り
void EnemyPat_Tomatina_Qwen()
{
    static int phase = 1;
    static int countT;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        phase = 1;
        countT = 100;

        // 祭り開始の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 敵の移動：左右往復 (120フレームごとに反転)
        int direction = (((count + 60) / 120) % 2 == 0) ? 1 : -1;
        enemy.x += 0.98 * (double)direction;

        // 画面端制限
        if (enemy.x < 40.0) enemy.x = 40.0;
        if (enemy.x > 440.0) enemy.x = 440.0;
    }

    countT = countT + 1;
    if (countT >= 900) countT = 100;

    // フェーズ1: 準備段階 (count 120 ~ 239)
    // 単発の赤い小玉がプレイヤーを狙う
    if (countT >= 120 && countT < 240 && countT % 20 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTomatoFestival;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0; // 基本直線弾

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // フェーズ2: 投げ始め (count 240 ~ 419)
    // 放物線を描いて落ちてくる中玉
    else if (countT >= 240 && countT < 420 && countT % 15 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTomatoFestival;
        pSet->x = enemy.x + (GetRand(100) - 50); // 投げる位置を少しばらつかせる
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 1; // 放物線弾

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // フェーズ3: 祭り本番 (count 420 ~ 659)
    // 拡散弾と回転渦巻き弾が混在
    else if (countT >= 420 && countT < 660 && countT % 10 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTomatoFestival;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;

        // GetRand(1) は 0 または 1 を返す
        pSet->kind = (GetRand(1) == 0) ? 2 : 3;

        if (pSet->kind == 2) {
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x); // 拡散はプレイヤー方向
        }
        else {
            pSet->muki = count * 0.1; // 渦巻きは時間経過で角度変化
        }

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // フェーズ4: 大漁投げ (count 660 ~ 779)
    // 拡散弾を高密度で連発
    else if (countT >= 660 && countT < 780 && countT % 6 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTomatoFestival;
        pSet->x = enemy.x + (GetRand(60) - 30);
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 2; // 拡散弾

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // フェーズ5: 後片付け (count == 790)
    // 画面中央からゆっくり落ちてくる巨大なトマト1発
    else if (countT == 790) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTomatoFestival;
        pSet->x = 240.0; // 画面中央上
        pSet->y = 40.0;
        pSet->muki = DX_PI / 2.0; // 真下
        pSet->kind = 4; // 最後のトマト(大玉)

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}