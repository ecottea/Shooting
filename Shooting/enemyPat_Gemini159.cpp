#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：刀の回転（刀身）および斬撃後の放射状弾幕
static void ShotSwordSlash(sEnemyShotSet* pEnemyShotSet)
{
    // param_i[0]: 0 = 刀身モード（回転する刀）、1 = 飛散モード（衝撃波）
    if (pEnemyShotSet->count == 0) {
        if (pEnemyShotSet->param_i[0] == 0) {
            // 刀のチャージ・構え音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        else {
            // 斬撃・衝撃波の拡散音
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }

    if (pEnemyShotSet->param_i[0] == 0) {
        // === 刀身（短レーザーを直列に並べた刃）の回転アニメーション ===
        // param_d[0] を回転角として毎フレーム加算
        pEnemyShotSet->param_d[0] += 0.12;

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        int index = 0;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // ボスの中心から外側へ向かって一定間隔で直線状に配置
            double dist = 12.0 + index * 14.0;
            double baseAngle = pEnemyShotSet->muki + pEnemyShotSet->param_d[0];

            pShot->x = pEnemyShotSet->x + dist * cos(baseAngle);
            pShot->y = pEnemyShotSet->y + dist * sin(baseAngle);
            pShot->muki = baseAngle; // 弾の向きも刀身に合わせる

            pShot = pShot->next;
            index++;
        }
    }
    else {
        // === 斬撃後の放射状バラマキ弾の移動 ===
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Sword_Gemini()
{
    static int muki;
    static int state;          // 0: 移動＆構え, 1: 踏み込み＆一閃
    static int state_count;
    static sEnemyShotSet* pSwordSet = nullptr;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        state = 0;
        state_count = 0;
        pSwordSet = nullptr;
    }

    state_count++;

    if (state == 0) {
        // 左右にゆったり移動しながら攻撃の機会を伺う
        enemy.x += 1.0 * (double)muki;
        if (enemy.x < 120.0 || enemy.x > 360.0) {
            muki *= -1;
        }

        // 状態開始時に刀を生成して構える
        if (state_count == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSwordSlash;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = -DX_PI / 2.0; // 上向き基準
            pEnemyShotSet->kind = 6;
            pEnemyShotSet->param_i[0] = 0; // 刀身モード
            pEnemyShotSet->param_d[0] = 0.0;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            // 白の短レーザー(img_enemyShotLaser[6])を6個直列に並べて「刀身」を表現
            for (int i = 0; i < 6; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = enemy.x;
                pEnemyShot->y = enemy.y;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotLaser[6]; // 白の短レーザー

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            pSwordSet = pEnemyShotSet;
        }

        // 刀身のセット位置をボスの位置に常時追従させる
        if (pSwordSet != nullptr) {
            pSwordSet->x = enemy.x;
            pSwordSet->y = enemy.y;
        }

        // 100フレーム経過したら斬撃フェーズへ移行
        if (state_count >= 100) {
            state = 1;
            state_count = 0;
        }
    }
    else if (state == 1) {
        // 斬撃フェーズ：プレイヤーの近くへ踏み込んで猛スピードで薙ぎ払う
        if (state_count == 1) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // プレイヤーの少し上へ瞬間移動（踏み込み）
            enemy.x = player.x + (GetRand(60) - 30);
            enemy.y = player.y - 100.0;
            if (enemy.x < 50.0) enemy.x = 50.0;
            if (enemy.x > 430.0) enemy.x = 430.0;
            if (enemy.y < 40.0) enemy.y = 40.0;
        }

        // 刀身をボスに追従させつつ、回転速度を加速させて「一閃」を表現
        if (pSwordSet != nullptr) {
            pSwordSet->x = enemy.x;
            pSwordSet->y = enemy.y;
            pSwordSet->param_d[0] += 0.35; // 高速スイング
        }

        // 40フレーム経過したら刀身を役目終了として消去し、周囲へ衝撃波（バラマキ弾）を放つ
        if (state_count >= 40) {
            if (pSwordSet != nullptr) {
                pSwordSet->prev->next = pSwordSet->next;
                pSwordSet->next->prev = pSwordSet->prev;
                pSwordSet = nullptr;
            }

            // 衝撃波（シアンの銃弾：img_enemyShotBullet[3]）の放射状展開
            sEnemyShotSet* pScatterSet = new sEnemyShotSet;
            pScatterSet->count = 0;
            pScatterSet->patternFunc = ShotSwordSlash;
            pScatterSet->x = enemy.x;
            pScatterSet->y = enemy.y;
            pScatterSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
            pScatterSet->kind = 3;
            pScatterSet->param_i[0] = 1; // 飛散モード

            pScatterSet->pEnemyShotHead = new sEnemyShot;
            pScatterSet->pEnemyShotHead->prev = pScatterSet->pEnemyShotHead;
            pScatterSet->pEnemyShotHead->next = pScatterSet->pEnemyShotHead;

            // 16方向の放射状弾
            for (int i = 0; i < 16; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = enemy.x;
                pEnemyShot->y = enemy.y;
                pEnemyShot->muki = (2.0 * DX_PI / 16.0) * i + GetRand(20) / 100.0;
                pEnemyShot->speed = 3.0 + GetRand(100) / 100.0;
                pEnemyShot->kind = img_enemyShotBullet[3]; // シアンの銃弾

                pEnemyShot->prev = pScatterSet->pEnemyShotHead->prev;
                pEnemyShot->next = pScatterSet->pEnemyShotHead;
                pScatterSet->pEnemyShotHead->prev->next = pEnemyShot;
                pScatterSet->pEnemyShotHead->prev = pEnemyShot;
            }

            pScatterSet->prev = enemyShotSetHead.prev;
            pScatterSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pScatterSet;
            enemyShotSetHead.prev = pScatterSet;

            // 通常の移動＆構えフェーズへループ
            state = 0;
            state_count = 0;
            pSwordSet = nullptr;
        }
    }
}