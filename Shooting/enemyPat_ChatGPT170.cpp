#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 転世「一条戻り橋」
//
// ・青 / 赤の針弾を4方向へ同時発射し、16フレーム間隔で連射する。
// ・青と赤は発射ごとの基準角度の変化量が異なる。
// ・各針弾は飛行中も少しずつ旋回し、発射位置付近へ戻る円弧軌道を描く。
// ・攻撃開始から一定時間後、青い大玉の自機狙いを追加する。
//
// 弾・弾セットの count はメインルーチン側でインクリメントされるため、
// このファイルでは count を変更しない。
// 画面外弾の削除もメインルーチン側に任せる。
// ============================================================

namespace {
    // 攻略資料にある「発射周期(16フレーム)ごとの角度変化量」。
    const double ICH_BLUE_SHOT_DELTA = -2.0 * DX_PI / 15.0;
    const double ICH_RED_SHOT_DELTA = 4.0 * DX_PI / 35.0;

    // 針弾の旋回量。1周して発射位置付近へ戻る軌道にする。
    // 画面座標系（Y下向き）では、青は反時計回り、赤は時計回り。
    const double ICH_BLUE_TURN = -2.0 * DX_PI / 240.0;
    const double ICH_RED_TURN = 2.0 * DX_PI / 280.0;

    const int ICH_SHOT_INTERVAL = 16+8;
    const int ICH_BIG_SHOT_START = 600-500; // 60秒制限を想定し、残り50秒相当から開始
    const int ICH_BIG_SHOT_INTERVAL = 120;

    // 弾を弾セットのリスト末尾へ追加する。
    static sEnemyShot* AddIchijoShot(
        sEnemyShotSet* pSet,
        double x, double y,
        double muki, double speed,
        int kind,
        double turnRate,
        int shotType)
    {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = x;
        pShot->y = y;
        pShot->muki = muki;
        pShot->speed = speed/2;
        pShot->kind = kind;
        pShot->param_d[0] = turnRate/2;
        pShot->param_i[0] = shotType;
        pShot->margin = 480;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
        return pShot;
    }

    // 戻り弾と自機狙い大玉を管理する弾セット関数。
    static void ShotIchijoModoribashi(sEnemyShotSet* pSet)
    {
        // param_d[0] : 青粒弾の現在の発射基準角度
        // param_d[1] : 赤粒弾の現在の発射基準角度
        // param_i[0] : 角度初期化済みフラグ
        if (pSet->count == 0 && pSet->param_i[0] == 0) {
            // 開幕角度はランダム。ただし、以降は固定パターン。
            const double startAngle = (double)GetRand(3599) / 3600.0 * 2.0 * DX_PI;
            pSet->param_d[0] = startAngle;

            // 青・赤の初期相対角度は固定値。
            // 公開攻略資料では相対角度の具体値までは確認できないため、
            // ここでは弾群が重なりすぎない位相差を設定している。
            pSet->param_d[1] = startAngle + DX_PI / 16.0;
            pSet->param_i[0] = 1;

            if (CheckSoundMem(sound_enemyShot_light)) {
                StopSoundMem(sound_enemyShot_light);
            }
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 16フレームごとに青・赤を同時に4方向へ発射。
        // 8連射を繰り返す構成。発射角度は青・赤で別々にずらす。
        if (pSet->count % ICH_SHOT_INTERVAL == 0) {
            for (int i = 0; i < 4; i++) {
                const double offset = (double)i * DX_PI / 2.0;

                AddIchijoShot(
                    pSet,
                    pSet->x, pSet->y,
                    pSet->param_d[0] + offset,
                    3.6,
                    img_enemyShotBullet[4], // 青
                    ICH_BLUE_TURN,
                    0);

                AddIchijoShot(
                    pSet,
                    pSet->x, pSet->y,
                    pSet->param_d[1] + offset,
                    3.6,
                    img_enemyShotBullet[0], // 赤
                    ICH_RED_TURN,
                    0);
            }

            pSet->param_d[0] += ICH_BLUE_SHOT_DELTA;
            pSet->param_d[1] += ICH_RED_SHOT_DELTA;

            // 8連射のまとまりごとに軽い発射音を鳴らす。
            if ((pSet->count / ICH_SHOT_INTERVAL) % 8 == 0 && pSet->count != 0) {
                if (CheckSoundMem(sound_enemyShot_light)) {
                    StopSoundMem(sound_enemyShot_light);
                }
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }

        // 残り時間50秒相当から、青い大玉を一定間隔で自機狙い発射。
        if (pSet->count >= ICH_BIG_SHOT_START &&
            (pSet->count - ICH_BIG_SHOT_START) % ICH_BIG_SHOT_INTERVAL == 0) {
            const double aim = atan2(player.y - pSet->y, player.x - pSet->x);
            AddIchijoShot(
                pSet,
                pSet->x, pSet->y,
                aim,
                2.8,
                img_enemyShotLargeBall[4], // 青大玉
                0.0,
                1);

            if (CheckSoundMem(sound_enemyShot_medium)) {
                StopSoundMem(sound_enemyShot_medium);
            }
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 弾の移動。
        // shotType == 0 : 青 / 赤の旋回する戻り針弾
        // shotType == 1 : 直進する自機狙い大玉
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                const double turnRate = pShot->param_d[0];
                const int life = (turnRate < 0.0) ? 240 : 280;

                // 一周して発射位置付近へ戻った弾は、メインルーチンの
                // 画面外弾削除処理に渡す。
                if (pShot->count >= life*2) {
                    pShot->x = -1000.0;
                    pShot->y = -1000.0;
                }
                else {
                    pShot->muki += turnRate;
                    pShot->x += pShot->speed * cos(pShot->muki);
                    pShot->y += pShot->speed * sin(pShot->muki);
                }
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Modoribashi_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 62.0+100;
        enemy.maxHp = enemy.hp = 200;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotIchijoModoribashi;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 本スペルは角度パターンを安定させるため、敵本体は中央に留める。
    enemy.x = 240.0;
    enemy.y = 62.0+100;
}
