#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// -----------------------------------------------------------
// 弾幕：三相変化「フェーズ・シフト」（高難易度版）
// 気体(ランダム) → 液体(波動) → 固体(停止) → 昇華(爆発)
// -----------------------------------------------------------
static void ShotPhaseShift(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // セット生成時の初期化（気体の発生）
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 15; i++) { // 1回あたりの弾数を大幅増加
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20; // 発生範囲を拡大
            pEnemyShot->y = pEnemyShotSet->y + GetRand(40) - 20;

            pEnemyShot->muki = (GetRand(359) / 180.0) * DX_PI;
            pEnemyShot->speed = 1.5 + GetRand(200) / 100.0; // 初速をアップ
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 0:赤

            pEnemyShot->param_i[0] = 0; // フェーズ管理
            pEnemyShot->param_d[0] = 0.0; // 液体化時の基準角度用
            pEnemyShot->param_d[1] = (GetRand(359) / 180.0) * DX_PI; // 波の初期位相用

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 各弾の挙動更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 相転移の周期を短縮 (240フレーム周期) して難易度と密度をアップ
        int loop_count = count % 240;

        if (loop_count < 60) {
            // 【Phase 1: 気体】ブラウン運動と拡散
            if (pShot->param_i[0] != 0) {
                pShot->param_i[0] = 0;
                pShot->kind = img_enemyShotSmallBall[0];
                pShot->muki = (GetRand(359) / 180.0) * DX_PI;
                pShot->speed = 1.5 + GetRand(200) / 100.0;
            }
            // ランダムな揺れを激しくする
            pShot->muki += (GetRand(20) - 10) / 180.0 * DX_PI;
        }
        else if (loop_count < 120) {
            // 【Phase 2: 液体】自機方向への流動と波動
            if (pShot->param_i[0] != 1) {
                pShot->param_i[0] = 1;
                pShot->kind = img_enemyShotMediumBall[4]; // 青中玉
                // 液体化した瞬間に、各弾が自機を正確に狙う
                pShot->param_d[0] = atan2(player.y - pShot->y, player.x - pShot->x);
            }
            // 自機狙いの軌道を軸に、振幅の大きい波を描いて殺到する
            double wave = sin(loop_count * 0.2 + pShot->param_d[1]);
            pShot->muki = pShot->param_d[0] + wave * 0.5;
            pShot->speed = 3.0;
        }
        else if (loop_count < 180) {
            // 【Phase 3: 固体】瞬間凍結・停止
            if (pShot->param_i[0] != 2) {
                pShot->param_i[0] = 2;
                pShot->kind = img_enemyShotDiamond[6]; // 白菱形弾
                pShot->speed = 0.0; // 停止
                // 停止と同時に「次の標的」として自機の方を向く（昇華時のベクトルになる）
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            }
        }
        else {
            // 【Phase 4: 昇華】自機狙いの爆発的拡散
            if (pShot->param_i[0] != 3) {
                pShot->param_i[0] = 3;
                pShot->kind = img_enemyShotSmallBall[0];
                // 完全にランダムではなく、自機方向ベースで扇状に拡散させる（殺意高め）
                pShot->muki += (GetRand(60) - 30) / 180.0 * DX_PI;
                pShot->speed = 4.0 + GetRand(250) / 100.0; // さらに高速
            }
        }

        // 座標更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// -----------------------------------------------------------
// 敵本体のパターン
// -----------------------------------------------------------
void EnemyPat_PhaseTransition_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 気体・液体フェーズ中(0-119)は激しく動き回り、弾の発生源を散らす
        if (count % 240 < 120) {
            enemy.x += 2.0 * (double)muki;
            if (count % 80 == 40) muki *= -1; // 切り返しを早く
        }
    }

    int loop_count = count % 240;

    // ボスからの弾発射
    if (loop_count >= 5 && loop_count < 60) {
        // 4フレームごと(高密度)に発射
        if (loop_count % 4 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotPhaseShift;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    // フェーズ切り替えのSE演出
    if (loop_count == 5) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    else if (loop_count == 60) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    else if (loop_count == 120) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else if (loop_count == 180) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
}