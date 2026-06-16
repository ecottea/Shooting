// enemyPat_tatsujinOu_Brutal.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：達人王風・真（ワインダー ＋ 超高速スナイパー ＋ 遅延超加速弾）
static void ShotTatsujinOu_Brutal(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    // ==========================================
    // 弾の生成フェーズ
    // ==========================================

    // 【凶悪ギミックA：ワインダー】（0〜180フレームの間、4フレーム毎）
    // 画面全体を薙ぎ払い、プレイヤーの移動方向を強制する螺旋鞭
    if (t <= 180 && t % 4 == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 時間経過で撃ち出し角度が左右にスイングする
            double baseAngle = sin(t * 0.05) * 1.5;
            pEnemyShot->muki = baseAngle + i * (DX_PI * 2.0 / 5.0);
            pEnemyShot->speed = 6.0;
            pEnemyShot->kind = img_enemyShotScale[5]; // マゼンタの鱗弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 【凶悪ギミックB：超高速スナイパー】（60〜200フレームの間、20フレーム毎）
    // ワインダーで逃げ道を塞いだ上で、回避限界速度の自機狙い7WAYを叩き込む
    if (t >= 60 && t <= 200 && t % 20 == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        double aimAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        for (int i = -3; i <= 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = aimAngle + i * 0.08; // 非常に狭い角度差で微細な回避を強要

            // 弾速 10.0〜12.0 の「見てからでは避けられない」不可避級の速度
            pEnemyShot->speed = 10.0 + GetRand(20) / 10.0;
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤の銃弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 【凶悪ギミックC：遅延・再照準・超加速】（100フレーム目のみ発動）
    // 一見遅い弾をばら撒き、忘れた頃にプレイヤー目掛けて超加速させる初見殺しの罠
    if (t == 100) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        double randOffset = (GetRand(314) / 100.0);
        for (int i = 0; i < 32; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = randOffset + i * (DX_PI * 2.0 / 32.0);

            pEnemyShot->speed = 0.5; // 初速は極端に遅く、画面内に滞留させる
            pEnemyShot->kind = img_enemyShotLargeBall[1]; // 黄の大玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ==========================================
    // 弾の移動・軌道変化フェーズ
    // ==========================================
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 【軌道変化の処理】
        // 黄色の大玉（ギミックC）のみ、発射から60フレーム経過で自機を再ロックオンし超加速
        if (pEnemyShot->kind == img_enemyShotLargeBall[1]) {
            if (pEnemyShot->count == 60) {
                // 自機へ向けて角度を急激に書き換える
                pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            }
            if (pEnemyShot->count >= 60 && pEnemyShot->speed < 15.0) {
                // 毎フレーム加速し、最終的にスピード15.0（即死級）へ到達
                pEnemyShot->speed += 0.4;
            }
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Tatsujinou_Gemini()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // ボスとしての高耐久
    }
    else {
        // 8の字（リサージュ図形）にヌルヌルと動き続ける
        // これにより、ワインダーと狙撃弾の「発射原点」が狂い、回避難易度が跳ね上がる
        enemy.x = 240.0 + 120.0 * sin(count * 0.015);
        enemy.y = 100.0 + 30.0 * sin(count * 0.030);
    }

    // 240フレーム周期で凶悪弾幕セットを生成
    if (count % 240 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTatsujinOu_Brutal;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}