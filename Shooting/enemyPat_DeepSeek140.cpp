#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：機密裁断シュレッダー
//   紙(小弾グリッド)が降下 → 刃(中弾列)で裁断 →
//   短冊(銃弾)になって舞い落ちる → 紙粉(小弾)が散る
//
//   param_i[0] 役割マーカー
//     0: ローラー(大弾)
//     1: 刃(中弾)
//     2: 紙(小弾) → 裁断後は短冊(銃弾)
//     3: 紙粉(小弾)
// ============================================================
static void ShotShredder(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    sEnemyShot* pShot;

    const double bladeY = pEnemyShotSet->y + 40.0;  // 裁断ライン
    const double paperStartY = pEnemyShotSet->y - 80.0;  // 紙の初期位置
    const double cutY = bladeY;

    if (pEnemyShotSet->count == 0) {
        // 使える効果音: light / medium / heavy / extreme / charge(予告)
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // ---- ローラー(大弾2個) ----
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i == 0 ? -30.0 : 30.0);
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            // 弾の種類: 小玉/中玉/大玉/銃弾/鱗弾/菱形弾/中楕円弾/短レーザー
            // 弾の色:   0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
            pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青
            pEnemyShot->param_i[0] = 0;
            pEnemyShot->param_i[1] = i;
            pEnemyShot->param_d[0] = pEnemyShot->x;       // 基準X
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ---- 刃(中弾 横一列) ----
        for (int i = -3; i <= 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + i * 20.0;
            pEnemyShot->y = bladeY;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_d[0] = pEnemyShot->x;
            pEnemyShot->param_d[1] = (double)i;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ---- 紙の格子(小弾・白) ----
        // 列 -3..3 (7列) × 行 0..3 (4行)
        for (int row = 0; row < 4; row++) {
            for (int col = -3; col <= 3; col++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + col * 20.0;
                pEnemyShot->y = paperStartY - row * 15.0;
                pEnemyShot->muki = DX_PI / 2;
                pEnemyShot->speed = 1.2;
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
                pEnemyShot->param_i[0] = 2;   // 紙
                pEnemyShot->param_i[1] = 0;   // 未裁断
                pEnemyShot->param_i[2] = col; // 列
                pEnemyShot->param_i[3] = 0;   // カーブ方向
                pEnemyShot->param_d[0] = cutY;
                pEnemyShot->param_d[1] = (double)row;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ---- 既存弾の更新 ----
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;

        switch (pShot->param_i[0]) {
        case 0: // ローラー: 微振動
            pShot->x = pShot->param_d[0]
                + sin(pEnemyShotSet->count * 0.12 + pShot->param_i[1] * DX_PI) * 2.0;
            break;

        case 1: // 刃: 左右往復
            pShot->x = pShot->param_d[0]
                + sin(pEnemyShotSet->count * 0.18 + pShot->param_d[1] * 0.7) * 4.0;
            break;

        case 2: // 紙 / 裁断後の短冊
            if (pShot->param_i[1] == 0) {
                // 降下中
                pShot->y += pShot->speed;
                if (pShot->y >= pShot->param_d[0]) {
                    // 裁断！ 紙→短冊(銃弾)
                    pShot->param_i[1] = 1;
                    pShot->kind = img_enemyShotBullet[6]; // 白い銃弾＝短冊
                    pShot->muki = DX_PI / 2;              // 下向き
                    pShot->speed = 1.6;
                    pShot->param_d[2] = pShot->x;         // 基準X
                    pShot->param_d[3] = 0.0;              // 位相
                    pShot->param_i[3] = (GetRand(1) == 0) ? -1 : 1; // カーブ方向
                }
            }
            else {
                // 舞い落ちる短冊
                pShot->param_d[3] += 0.12;
                pShot->x = pShot->param_d[2]
                    + sin(pShot->param_d[3]) * 9.0 * pShot->param_i[3];
                pShot->y += pShot->speed;
            }
            break;

        case 3: // 紙粉
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }

        pShot = nextShot;
    }

    // ---- 紙粉の放出(裁断エフェクト) ----
    if (pEnemyShotSet->count >= 20 && pEnemyShotSet->count % 8 == 0) {
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(120) - 60;
            pEnemyShot->y = bladeY + GetRand(10);
            pEnemyShot->muki = DX_PI / 2 + (GetRand(60) - 30) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
            pEnemyShot->param_i[0] = 3;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Shredder_DeepSeek()
{
    static int muki;
    static int shot_count;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔でシュレッダー弾幕を発動
    if (count % 360 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShredder;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;
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