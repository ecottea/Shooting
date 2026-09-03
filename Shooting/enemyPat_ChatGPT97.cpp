// enemyPat_PerfectFreeze.cpp
// 凍符「パーフェクトフリーズ」風弾幕
//
// 構成：
//  1. 虹色の小玉をランダム方向へ大量発射
//  2. 一定時間後、画面上に残る小玉を白く凍結して停止
//  3. 凍結中に青い6-way自機狙い弾を発射
//  4. 凍結弾を個別のランダム方向へ再始動
//  5. これを繰り返す
//
// 弾数の増減・画面外削除・countのインクリメントはメインルーチン側に任せる。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {

    // ------------------------------------------------------------
    //  1発追加
    // ------------------------------------------------------------
    static void AddShot(sEnemyShotSet* pSet, double x, double y,
        double muki, double speed, int kind)
    {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = x;
        pShot->y = y;
        pShot->muki = muki;
        pShot->speed = speed;
        pShot->kind = kind;
        pShot->margin = 7;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // ------------------------------------------------------------
    //  虹色ばら撒き＋凍結＋解凍
    // ------------------------------------------------------------
    static void ShotPerfectFreeze(sEnemyShotSet* pSet)
    {
        // 1周期の流れ
        //  0 ～ FREEZE_AT-1 : 虹色弾を追加しながら飛ばす
        //  FREEZE_AT ～ RELEASE_AT-1 : 全弾凍結＋青弾
        //  RELEASE_AT ～ CYCLE-1 : 凍結弾をランダム方向へ再始動
        //  CYCLE ごとに繰り返す
        const int CYCLE = 150;
        const int FREEZE_AT = 78;
        const int RELEASE_AT = 112;

        int phase = pSet->count % CYCLE;

        // --------------------------------------------------------
        // 虹色弾を追加
        // --------------------------------------------------------
        if (phase < FREEZE_AT && phase % 3 == 0) {
            // 1回につき複数発。少し広がった位置から出して密度を上げる。
            int n = 6 + GetRand(3) - 5; // 6～9発
            double base = (double)(GetRand(359)) * DX_PI / 180.0;

            for (int i = 0; i < n; i++) {
                double a = base + 2.0 * DX_PI * (double)i / (double)n;
                a += ((double)GetRand(20) - 10.0) * DX_PI / 180.0;

                // 赤・黄・緑・シアン・青・マゼンタを順に回す。
                int color = (pSet->kind + i + phase / 3) % 6;
                AddShot(pSet,
                    pSet->x + 4.0 * cos(a),
                    pSet->y + 4.0 * sin(a),
                    a,
                    1.65 + 0.10 * GetRand(7),
                    img_enemyShotSmallBall[color]);
            }
        }

        // --------------------------------------------------------
        // 凍結開始：現在画面に残っている弾を白くして停止
        // --------------------------------------------------------
        if (phase == FREEZE_AT) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                // 未凍結の弾だけを凍結する。
                if (pShot->param_i[1] == 0) {
                    pShot->param_i[0] = pShot->kind;
                    pShot->param_i[1] = 1;
                    pShot->param_d[0] = pShot->muki;
                    pShot->param_d[1] = pShot->speed;
                    pShot->kind = img_enemyShotSmallBall[6]; // 白
                    pShot->speed = 0.0;
                }
                pShot = pShot->next;
            }
        }

        // --------------------------------------------------------
        // 凍結中の青い6-way自機狙い弾
        // --------------------------------------------------------
        if (phase >= FREEZE_AT && phase < RELEASE_AT &&
            (phase - FREEZE_AT) % 9 == 0) {

            double target = atan2(player.y - pSet->y, player.x - pSet->x);
            const double step = 0.34; // 6-wayの広がり

            for (int i = -2; i <= 3; i++) {
                AddShot(pSet,
                    pSet->x,
                    pSet->y,
                    target + step * (double)i,
                    2.55,
                    img_enemyShotMediumBall[4]); // 青中玉
            }
        }

        // --------------------------------------------------------
        // 解凍：各弾が別々のランダム方向へ再始動
        // --------------------------------------------------------
        if (phase == RELEASE_AT) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                if (pShot->param_i[1] == 1) {
                    // 以前の色へ戻す
                    pShot->kind = pShot->param_i[0];

                    // 元の向きは参照情報として残しつつ、完全に別の方向へ散らす。
                    pShot->muki = ((double)GetRand(359)) * DX_PI / 180.0;
                    pShot->speed = pShot->param_d[1] * (1.00 - 0.15 + 0.30 * GetRand(4) / 4.0);
                    pShot->param_i[1] = 0;
                }
                pShot = pShot->next;
            }
        }

        // --------------------------------------------------------
        // 弾の通常移動
        // 凍結中は speed=0 なので自然に停止する。
        // --------------------------------------------------------
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }

} // namespace

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_PerfectFreeze_ChatGPT()
{
    static int muki;
    static sEnemyShotSet* pPerfectFreeze = nullptr;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        pPerfectFreeze = nullptr;
    }
    else {
        // チルノ風に左右へ往復移動
        enemy.x += 1.05 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 1つのショットセットで全周期を管理する。
    if (count == 1) {
        pPerfectFreeze = new sEnemyShotSet;
        pPerfectFreeze->count = 0;
        pPerfectFreeze->patternFunc = ShotPerfectFreeze;
        pPerfectFreeze->x = enemy.x;
        pPerfectFreeze->y = enemy.y + 10.0;
        pPerfectFreeze->muki = 0.0;
        pPerfectFreeze->kind = 0;

        pPerfectFreeze->pEnemyShotHead = new sEnemyShot;
        pPerfectFreeze->pEnemyShotHead->prev = pPerfectFreeze->pEnemyShotHead;
        pPerfectFreeze->pEnemyShotHead->next = pPerfectFreeze->pEnemyShotHead;

        pPerfectFreeze->prev = enemyShotSetHead.prev;
        pPerfectFreeze->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pPerfectFreeze;
        enemyShotSetHead.prev = pPerfectFreeze;
    }

    // 発射元は常にボス位置へ追従。
    if (pPerfectFreeze != nullptr) {
        pPerfectFreeze->x = enemy.x;
        pPerfectFreeze->y = enemy.y + 10.0;
    }
}
