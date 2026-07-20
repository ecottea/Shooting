// enemyPat_Tmp.cpp
// 『スイミー』モチーフ弾幕パターン実装

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：スイミーモチーフ『巨大魚の形成とスイミーの瞳』
// ============================================================
static void ShotSwimmy(sEnemyShotSet* pEnemyShotSet)
{
    // フェーズ管理: 
    // 0~119 : 赤い魚の放出と遊泳
    // 120~239 : 巨大魚の輪郭への収束
    // 240~299 : スイミー（黒）が目として配置・待機
    // 300~419 : スイミーの瞳からの狙い撃ち

    if (pEnemyShotSet->count == 0) {
        // フェーズ1開始：放出音
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 赤い魚（鱗弾）を30発ばら撒く
        for (int i = 0; i < 30; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = (DX_PI * 2.0 / 30.0) * i;
            pShot->speed = 2.0 + GetRand(100) / 100.0 + 1.0; // 2.0 ~ 3.0 の速度
            pShot->kind = img_enemyShotScale[0]; // 0:赤 の鱗弾

            // param_d[0]: 初期角度（収束時の目標角度として使用）
            pShot->param_d[0] = pShot->muki;
            // param_d[1]: 遊泳時の揺らぎ位相
            pShot->param_d[1] = GetRand(360) / 180.0 * DX_PI;
            pShot->margin = 999;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
    else if (pEnemyShotSet->count == 120) {
        // フェーズ2開始：収束予告音
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else if (pEnemyShotSet->count == 240) {
        // フェーズ3開始：スイミー（黒い中玉）を目の位置に配置
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pSwimmy = new sEnemyShot;
        // 魚の左側を目と仮定 (中心 240,240, 角度 PI の位置)
        // radius = 120 + 40 * cos(PI) = 80
        // x = 240 + 80 * cos(PI) = 160, y = 240 + 80 * sin(PI) = 240
        pSwimmy->x = 160.0;
        pSwimmy->y = 240.0;
        pSwimmy->kind = img_enemyShotMediumBall[7]; // 7:黒 の中玉
        pSwimmy->speed = 0.0;
        pSwimmy->muki = 0.0;
        pSwimmy->param_i[0] = 1; // スイミー識別フラグ
        pSwimmy->margin = 999;

        pSwimmy->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pSwimmy->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pSwimmy;
        pEnemyShotSet->pEnemyShotHead->prev = pSwimmy;
    }
    else if (pEnemyShotSet->count == 300) {
        // フェーズ4開始：狙い撃ち予告音
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 各弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count < 120) {
            // フェーズ1：群れのようにうねりながら進む
            double wave = sin(pEnemyShotSet->count * 0.1 + pShot->param_d[1]) * 2.0;
            pShot->x += pShot->speed * cos(pShot->muki) + wave * cos(pShot->muki + DX_PI / 2.0);
            pShot->y += pShot->speed * sin(pShot->muki) + wave * sin(pShot->muki + DX_PI / 2.0);
        }
        else if (pEnemyShotSet->count < 240) {
            // フェーズ2：巨大魚の輪郭へ収束 (線形補間)
            double targetAngle = pShot->param_d[0];
            // 右側(角度0)を尖らせた魚っぽい楕円形を形成
            double radius = 120.0 + 40.0 * cos(targetAngle);
            double targetX = 240.0 + radius * cos(targetAngle);
            double targetY = 240.0 + radius * sin(targetAngle);

            pShot->x += (targetX - pShot->x) * 0.08;
            pShot->y += (targetY - pShot->y) * 0.08;
        }
        else if (pEnemyShotSet->count < 300) {
            // フェーズ3：スイミーは静止、他の弾も配置維持（何もしない）
        }
        else {
            // フェーズ4：スイミーの瞳からの狙い撃ち
            if (pShot->param_i[0] == 1) {
                // 15フレームに1回、プレイヤーに向けて黒い短レーザーを発射
                if (pEnemyShotSet->count % 15 == 0) {
                    sEnemyShot* pLaser = new sEnemyShot;
                    pLaser->x = pShot->x;
                    pLaser->y = pShot->y;
                    pLaser->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pLaser->speed = 5.0;
                    pLaser->kind = img_enemyShotLaser[7]; // 7:黒 の短レーザー

                    pLaser->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pLaser->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pLaser;
                    pEnemyShotSet->pEnemyShotHead->prev = pLaser;
                }
            }
        }
        pShot = pShot->next;
    }

    {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->kind == img_enemyShotLaser[7]) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }

            pShot = pShot->next;
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Swimmy_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // パターン用にHPを少し多めに設定
    }
    else {
        // 敵は画面中央上部 (240, 120) 付近へゆっくり移動し、そこから動かない
        double targetX = 240.0;
        double targetY = 120.0;
        enemy.x += (targetX - enemy.x) * 0.02;
        enemy.y += (targetY - enemy.y) * 0.02;
    }

    // count 60 で弾幕パターン開始
    if (count % 470 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSwimmy;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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

    // パターン終了後のリセット（例：480フレームでループ）
    // 必要に応じてループさせたり、次のパターンへ遷移させてください
    //if (count > 480) {
    //    count = 60;
    //}
}