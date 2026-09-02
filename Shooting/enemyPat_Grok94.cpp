// enemyPat_Tmp_Sunflower.cpp
// ひまわりをモチーフにした弾幕パターン「向日葵の開花」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：向日葵の開花
// 使用素材：
//   - 中心円盤・種弾 : 中玉 (img_enemyShotMediumBall) / 色 黄(1)・橙(8)
//   - 花びらレーザー : 短レーザー (img_enemyShotLaser) / 色 黄(1)
//   - 飛び散る種    : 小玉 (img_enemyShotSmallBall) / 色 橙(8)・黄(1)
//   - 補助弾        : 菱形弾 (img_enemyShotDiamond) / 色 黄(1)
// ============================================================
static void ShotSunflower(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // フェーズ管理（param_i[0] をフェーズカウンタとして使用）
    // 0: 中心形成＋種発射準備
    // 1: 花びらレーザー展開
    // 2: 回転＋追加種弾
    // 3: 終了（弾は画面外消去に任せる）

    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 0; // フェーズ
        pEnemyShotSet->param_d[0] = 0.0; // 回転角度蓄積用
        pEnemyShotSet->param_i[1] = 0; // 花びら本数カウンタ
    }

    // ---- フェーズ0：中心の円盤形成（固定中玉）＋最初の種弾 ----
    if (pEnemyShotSet->count == 30) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 中心に密集した中玉（花の中心）
        for (int i = 0; i < 12; i++) {
            double ang = i * (2.0 * DX_PI / 12.0);
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + cos(ang) * 8.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(ang) * 8.0;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 0.0; // 固定
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄
            pEnemyShot->param_i[0] = 1; // 中心弾フラグ（後で動かす用）
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 最初の種弾（放射状の小玉）
        for (int i = 0; i < 16; i++) {
            double ang = i * (2.0 * DX_PI / 16.0) + (GetRand(20) - 10) / 180.0 * DX_PI * 0.3;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 2.2 + GetRand(8) / 10.0;
            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        pEnemyShotSet->param_i[0] = 1;
    }

    // ---- フェーズ1：花びらレーザー展開 ----
    if (pEnemyShotSet->count == 60) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int petalNum = 10;
        for (int i = 0; i < petalNum; i++) {
            double ang = i * (2.0 * DX_PI / petalNum);
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + cos(ang) * 20.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(ang) * 20.0;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 0.0; // 最初は固定、後で回転
            pEnemyShot->kind = img_enemyShotLaser[1]; // 黄レーザー
            pEnemyShot->param_i[0] = 2; // 花びらフラグ
            pEnemyShot->param_d[0] = ang; // 初期角度保存
            pEnemyShot->param_d[1] = 20.0; // 中心からの距離
            pEnemyShot->margin = 70;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        pEnemyShotSet->param_i[0] = 2;
        pEnemyShotSet->param_i[1] = petalNum;
    }

    // ---- フェーズ2：花びら回転＋追加種弾 ----
    if (pEnemyShotSet->count >= 90 && pEnemyShotSet->count <= 240) {
        // 回転速度を徐々に上げる
        pEnemyShotSet->param_d[0] += 0.018;

        // 一定間隔で追加の種弾を発射
        if (pEnemyShotSet->count % 18 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            int seedNum = 8 + GetRand(4) + 50;
            for (int i = 0; i < seedNum; i++) {
                double baseAng = pEnemyShotSet->param_d[0] + i * (2.0 * DX_PI / seedNum);
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAng + (GetRand(30) - 15) / 180.0 * DX_PI * 0.4;
                pEnemyShot->speed = 1.8 + GetRand(12) / 10.0;
                // 黄と橙をランダムに
                pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[1] : img_enemyShotSmallBall[8];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // たまに菱形弾を追加（装飾）
            if (pEnemyShotSet->count % 36 == 0) {
                for (int i = 0; i < 6 * 8; i++) {
                    double ang = pEnemyShotSet->param_d[0] * 1.5 + i * (2.0 * DX_PI / 6.0 / 8);
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pEnemyShotSet->x;
                    pEnemyShot->y = pEnemyShotSet->y;
                    pEnemyShot->muki = ang;
                    pEnemyShot->speed = 2.5;
                    pEnemyShot->kind = img_enemyShotDiamond[1]; // 黄菱形
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }

    // ---- 全弾の移動処理 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 花びらレーザーの回転処理
        if (pShot->param_i[0] == 2) {
            if (pEnemyShotSet->count < 180) {
                double rot = pEnemyShotSet->param_d[0];
                double baseAng = pShot->param_d[0] + rot;
                double dist = pShot->param_d[1];
                // 距離を少し広げながら回転
                if (pEnemyShotSet->count > 90 && pEnemyShotSet->count < 180) {
                    dist += 0.15;
                    pShot->param_d[1] = dist;
                }
                pShot->x = pEnemyShotSet->x + cos(baseAng) * dist;
                pShot->y = pEnemyShotSet->y + sin(baseAng) * dist;
                pShot->muki = baseAng; // レーザーの向きも回転に合わせる
            }
            else {
                pShot->speed = 0.5;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        // 中心弾はゆっくり外側へ（開花後）
        else if (pShot->param_i[0] == 1 && pEnemyShotSet->count > 200) {
            pShot->speed = 1.2;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        // 通常弾（種弾など）
        else if (pShot->param_i[0] != 2) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_Sunflower_Grok()
{
    static int moveDir;
    static int shotPhase;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
        shotPhase = 0;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += 0.6 * (double)moveDir;
        if (enemy.x < 120.0) moveDir = 1;
        if (enemy.x > 360.0) moveDir = -1;
    }

    // 一定間隔で「向日葵の開花」パターンを発生
    // 最初は少し待ってから、その後は約4秒おき
    if (count % 240 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSunflower;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 5.0;
        pEnemyShotSet->muki = 0.0; // このパターンでは自機狙いではなく固定中心
        pEnemyShotSet->kind = shotPhase++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}