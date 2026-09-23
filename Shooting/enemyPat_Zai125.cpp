// ============================================================
//  enemyPat_Tmp.cpp
//  弾幕:南京玉すだれモチーフ「玉すだれ・倒れ幕」
//
//  構成(1サイクル=約6秒):
//   Phase1(設置)   : 先頭の珠(小弾)が真下に落ちて停止 → 竹の筒の代わり
//   Phase2(落下)   : 珠(小弾)が後から続いて落ち、鎖状に連なって
//                     「すだれ(珠のカーテン)」が完成する
//   Phase3(玉)     : すだれの隙間を縫うように中玉(赤)が1発
//                     プレイヤーを狙って転がり落ちる(主攻撃)
//   Phase4(巻き上げ): 珠が演者の手元へ一気に巻き上げられ、
//                     その勢いで上空へ抜けていく
//
//  使用する既存素材:
//   - img_enemyShotSmallBall[6](白) … 珠(竹の筒)
//   - img_enemyShotMediumBall[0](赤)… 玉
//   - sound_enemyShot_light / sound_enemyShot_medium / sound_enemyCharge
//
//  ※ count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//    画面外の弾の消去はメインルーチンで行う仕様を前提としています。
// ============================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---- 調整用定数 ----
static const double BEAD_GAP = 10.0;    // 珠同士の間隔(ドット)
static const double BEAD_TRAVEL = 450.0;  // 先頭の珠が落ちる距離
static const double BEAD_SPEED = 7.5;    // 珠の初速
static const double BEAD_DECEL = 0.10;   // 珠の減速度合い

// sEnemyShot->param_i[0] の役割
//  0:落下中の珠  1:静止して吊るされた珠  2:巻き上げられ上空へ抜ける珠  3:転がる「玉」
// sEnemyShotSet->param_i[0] の役割
//  0:すだれ設置中  1:巻き上げ開始

// 珠(小弾)の生成
static sEnemyShot* SpawnBead(sEnemyShotSet* pSet, double stopDist)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = pSet->x;
    pEnemyShot->y = pSet->y;
    pEnemyShot->muki = pSet->muki;          // すだれを吊るす方向(ほぼ真下)
    pEnemyShot->speed = BEAD_SPEED;
    pEnemyShot->param_i[0] = 0;             // 落下中
    pEnemyShot->param_d[0] = 0.0;           // 移動済み距離
    pEnemyShot->param_d[1] = stopDist;      // この距離でピタッと止まる
    pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
    pEnemyShot->margin = 120;

    pEnemyShot->prev = pSet->pEnemyShotHead->prev;
    pEnemyShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pEnemyShot;
    pSet->pEnemyShotHead->prev = pEnemyShot;

    return pEnemyShot;
}

// 玉すだれ1本分のセットの動作
static void ShotTamazudare(sEnemyShotSet* pEnemyShotSet)
{
    // ---------------- 発射処理 ----------------
    if (pEnemyShotSet->count == 0) {
        // Phase1: 先頭の珠(竹の筒の代わり)を1発だけ落とす
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        SpawnBead(pEnemyShotSet, BEAD_TRAVEL);
    }

    if (pEnemyShotSet->count >= 20 && pEnemyShotSet->count < 160 && pEnemyShotSet->count % 2 == 0) {
        // Phase2: 後続の珠を続々と落下させ、すだれ(カーテン)を作る
        int idx = 1 + (pEnemyShotSet->count - 20) / 2;
        // GetRand(x) は 0〜x を返すので、-2〜+2 の揺らぎは GetRand(4) - 2 で作る
        double stopDist = BEAD_TRAVEL - idx * BEAD_GAP + (GetRand(4) - 2);
        if (stopDist > 30.0) {
            SpawnBead(pEnemyShotSet, stopDist);
        }
    }

    if (pEnemyShotSet->count == 190) {
        // Phase3: 「玉」……中玉がすだれの隙間を縫ってプレイヤーへ転がる
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShot->speed = 3.0;
        pEnemyShot->param_i[0] = 3;
        pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤
        pEnemyShot->margin = 120;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    if (pEnemyShotSet->count == 240) {
        // Phase4: 巻き上げ開始(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 1;
    }

    // ---------------- 移動処理 ----------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case 0: // 落下中の珠:一定減速で滑らかに落ちて停止する
        {
            double remain = pShot->param_d[1] - pShot->param_d[0];
            if (remain <= pShot->speed) {
                // 目標距離に達した→ピタッと停止して「吊るされた珠」になる
                pShot->x += remain * cos(pShot->muki);
                pShot->y += remain * sin(pShot->muki);
                pShot->speed = 0.0;
                pShot->param_i[0] = 1;
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                pShot->param_d[0] += pShot->speed;
                // 残り距離から減速後の速度を求める(等減速で止まるように見せる)
                double nextRemain = pShot->param_d[1] - pShot->param_d[0];
                double target = sqrt(2.0 * BEAD_DECEL * nextRemain);
                if (target > BEAD_SPEED) target = BEAD_SPEED;
                pShot->speed = target;
            }
            break;
        }
        case 1: // 吊るされた珠:巻き上げ指示が出たら手元へ戻る
            if (pEnemyShotSet->param_i[0] == 1) {
                double dx = pEnemyShotSet->x - pShot->x;
                double dy = pEnemyShotSet->y - pShot->y;
                double dist = sqrt(dx * dx + dy * dy);
                if (dist <= pShot->speed) {
                    // 演者の手元に戻った→その勢いで上空へ抜けていく
                    pShot->x = pEnemyShotSet->x;
                    pShot->y = pEnemyShotSet->y;
                    pShot->param_i[0] = 2;
                    pShot->muki = pEnemyShotSet->muki + DX_PI; // 逆方向(上向き)
                    pShot->speed = 6.0;
                }
                else {
                    pShot->x += pShot->speed * dx / dist;
                    pShot->y += pShot->speed * dy / dist;
                    pShot->speed += 0.25; // 巻き上げは加速しながら一気に
                }
            }
            break;
        case 2: // 上空へ抜けていく珠:等速直線(画面外でメインルーチンが消去)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        case 3: // 転がる「玉」:等速直線でプレイヤーを追う
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン:南京玉すだれの演者
// ============================================================
void EnemyPat_NankinTamasudare_Zai()
{
    static int dirToggle;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        dirToggle = 0;
    }

    // 演者がステップを踏むように、ゆっくり左右に揺れる(6秒で1往復)
    enemy.x = 240.0 + 130.0 * sin((count % 360) / 360.0 * 2.0 * DX_PI);

    // 6秒(360フレーム)ごとに新しい玉すだれを披露する
    if (count % 360 == 30) {
        for (int i = -5; i <= 5; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotTamazudare;
            pEnemyShotSet->x = enemy.x + i * 30.0;
            pEnemyShotSet->y = enemy.y + 10.0;

            // すだれを吊るす向き:基本は真下。
            // 「もう一芸!」と演者が道具を振り直すイメージで、
            // 回ごとに左右へ僅かに傾きを変える
            double tilt = (dirToggle % 2 == 0) ? -DX_PI / 12.0 : DX_PI / 12.0;
            pEnemyShotSet->muki = DX_PI / 2.0 + tilt;
            pEnemyShotSet->param_i[0] = 0;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        dirToggle++;
    }
}