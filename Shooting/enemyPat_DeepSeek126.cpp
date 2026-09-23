#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：メントスコーラ・エクスプロージョン
//  ------------------------------------------------------------
//  ・白い大玉(メントス)がサイン波を描きながら落下
//  ・画面下70%付近で着弾 → 爆発音とともに噴火
//  ・橙の中玉(コーラ噴流)が上向き扇状に放物線を描く
//  ・シアンの小玉(炭酸泡)が全方向へふわふわ広がる
//  ・シアンの短レーザー(噴射の芯)が真上へ抜ける
//  ------------------------------------------------------------
//  既存素材の流用:
//    大玉(白)          : メントス
//    中玉(橙)          : コーラ噴流
//    小玉(シアン)      : 炭酸泡
//    短レーザー(シアン): 噴射の芯
//    sound_enemyCharge      : 予告音(落下開始)
//    sound_enemyShot_extreme: 着弾音
// ============================================================
static void ShotMentosCola(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->param_i[0] : フェーズ 0=落下中, 1=噴火済み
    // pEnemyShotSet->param_i[1] : 波レベル(0,1,2) 大きいほど豪華
    // pEnemyShotSet->param_d[0] : メントスの基準X
    // pEnemyShotSet->param_d[1] : 着弾Y
    const double IMPACT_Y = 340.0; // 画面下70%付近

    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // メントス(白い大玉)を1発生成
        sEnemyShot* mentos = new sEnemyShot;
        mentos->x = pEnemyShotSet->x;
        mentos->y = pEnemyShotSet->y;
        mentos->muki = DX_PI / 2.0; // 下向き
        mentos->speed = 1.8;
        mentos->kind = img_enemyShotLargeBall[6]; // 白
        mentos->param_i[0] = 1; // タグ: メントス

        mentos->prev = pEnemyShotSet->pEnemyShotHead->prev;
        mentos->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = mentos;
        pEnemyShotSet->pEnemyShotHead->prev = mentos;

        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = IMPACT_Y;
    }

    // 全弾を更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        switch (pShot->param_i[0]) {
        case 1: { // --- メントス：サイン波を描きながら落下 ---
            pShot->y += pShot->speed;
            pShot->x = pEnemyShotSet->param_d[0]
                + sin(pShot->count * 0.12) * 40.0;

            // 着弾判定
            if (pEnemyShotSet->param_i[0] == 0
                && pShot->y >= pEnemyShotSet->param_d[1]) {
                pEnemyShotSet->param_i[0] = 1; // 噴火済み

                const double ix = pShot->x;
                const double iy = pShot->y;
                // メントスを画面外へ(メインルーチンが消去)
                pShot->x = -9999.0;
                pShot->y = -9999.0;

                // 着弾音
                if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                const int waveLevel = pEnemyShotSet->param_i[1]; // 0..2

                // --- コーラ噴流(中玉/橙)：3波の扇状、重力で放物線 ---
                const int waves = 3;
                const int perWave = 12 + waveLevel * 2*5; // 12/14/16発
                for (int w = 0; w < waves; w++) {
                    for (int i = 0; i < perWave; i++) {
                        sEnemyShot* b = new sEnemyShot;
                        b->x = ix;
                        b->y = iy;
                        // 画面座標系(y下向き)で上向き扇 = -150°〜-30°
                        double deg = -150.0
                            + (120.0 * i / (perWave - 1))
                            + w * 4.0;
                        b->muki = deg / 180.0 * DX_PI;
                        b->speed = 4.5 + GetRand(250) / 100.0; // 4.5〜7.0
                        b->kind = img_enemyShotMediumBall[8];  // 橙(コーラ)
                        b->param_i[0] = 2;
                        // 速度を成分で保持(重力用)
                        b->param_d[0] = b->speed * cos(b->muki);
                        b->param_d[1] = b->speed * sin(b->muki);
                        b->param_d[2] = 0.05; // 重力
                        b->margin = 480;

                        b->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        b->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = b;
                        pEnemyShotSet->pEnemyShotHead->prev = b;
                    }
                }

                // --- 炭酸泡(小玉/シアン)：全方向、揺れながら拡散 ---
                const int bubbleCount = 30 + waveLevel * 6; // 30/36/42発
                for (int i = 0; i < bubbleCount; i++) {
                    sEnemyShot* b = new sEnemyShot;
                    b->x = ix;
                    b->y = iy;
                    b->muki = GetRand(359) / 180.0 * DX_PI;
                    b->speed = 1.5 + GetRand(200) / 100.0; // 1.5〜3.5
                    b->kind = img_enemyShotSmallBall[3];   // シアン(泡)
                    b->param_i[0] = 3;
                    b->param_d[0] = b->muki; // 基準角度
                    b->margin = 480;

                    b->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    b->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = b;
                    pEnemyShotSet->pEnemyShotHead->prev = b;
                }

                // --- 噴射の芯(短レーザー/シアン)：真上へ ---
                for (int i = 0; i < 3; i++) {
                    sEnemyShot* b = new sEnemyShot;
                    b->x = ix + (i - 1) * 18.0;
                    b->y = iy;
                    b->muki = -DX_PI / 2.0; // 上
                    b->speed = 9.0;
                    b->kind = img_enemyShotLaser[3]; // シアン
                    b->param_i[0] = 4;
                    b->margin = 480;

                    b->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    b->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = b;
                    pEnemyShotSet->pEnemyShotHead->prev = b;
                }
            }
            break;
        }

        case 2: // --- コーラ噴流：重力で放物線 ---
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            pShot->param_d[1] += pShot->param_d[2];
            break;

        case 3: { // --- 炭酸泡：ふわふわ揺れながら広がる ---
            double ang = pShot->param_d[0]
                + sin(pShot->count * 0.15) * 0.6;
            pShot->x += pShot->speed * cos(ang);
            pShot->y += pShot->speed * sin(ang);
            break;
        }

        case 4: // --- 噴射の芯：上へ直進 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }

        pShot = next;
    }
}

// ============================================================
//  敵本体：メントスコーラを定期的に落とす
// ============================================================
void EnemyPat_MentosCola_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくり左右に往復
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 100.0) muki = 1;
        if (enemy.x > 380.0) muki = -1;
    }

    // 180フレームごとにメントスコーラを1回発生
    if (count % 180 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMentosCola;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 下向き
        pEnemyShotSet->kind = shot_count;
        pEnemyShotSet->param_i[1] = shot_count; // 0/1/2 で規模が変化
        shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}