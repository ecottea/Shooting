// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：『メテオ・ストライク・ライン』の残像・覚醒制御
// ============================================================
static void ShotAfterimage(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int born_cycle = pEnemyShotSet->param_i[0];
    int activation_frame = 270 - born_cycle; // 全体のcycleが270になるタイミング

    // 1. 覚醒フレーム（一斉解放のタイミング）に達しているか
    if (pEnemyShotSet->count >= activation_frame) {
        int active_count = pEnemyShotSet->count - activation_frame;

        // 【第一形態：覚醒】
        // 覚醒した最初のフレームで、見た目を「警告色（赤の中玉）」に変更して壁化
        if (active_count == 0) {
            pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
                pEnemyShot->kind = img_enemyShotMediumBall[0]; // 0:赤の中玉 (7.0 x 7.0)
                pEnemyShot = pEnemyShot->next;
            }
        }

        // 【第二形態：残貌】
        // ボスが次の周期の攻撃に移ったら（新サイクル0F ≒ active_count 210）、
        // 古い障害物であることをプレイヤーに伝えるため、壁の色を「白（消えかけのエネルギー）」に変色させて残す
        if (active_count == 210) {
            pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
                pEnemyShot->kind = img_enemyShotMediumBall[6]; // 6:白の中玉
                pEnemyShot = pEnemyShot->next;
            }
        }

        // 【最終フェーズ：消滅】
        // 次の周期のボスが帰還し、"新たな壁"が覚醒する直前（新サイクル260F ≒ active_count 470）で安全に一斉消去
        // これにより最大でも2周期分の弾しか同時に存在しないため、メモリプール枯渇のクラッシュを完全に防げます
        if (active_count >= 470) {
            pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
                pEnemyShot->x = -999.0;
                pEnemyShot->y = -999.0;
                pEnemyShot = pEnemyShot->next;
            }
            pEnemyShotSet->x = -999.0;
            pEnemyShotSet->y = -999.0;
            return;
        }

        // 15フレームに1回、残像が配置されたその場から、プレイヤーに向けて狭角の「削り弾」を放つ
        // （削り弾の連射は、最初の覚醒期間である120フレームのみに限定）
        if (active_count < 120 && active_count % 15 == 0) {
            sEnemyShot* pNewShot = new sEnemyShot;
            pNewShot->x = pEnemyShotSet->x;
            pNewShot->y = pEnemyShotSet->y;

            double to_player = atan2(player.y - pNewShot->y, player.x - pNewShot->x);
            pNewShot->muki = to_player + (GetRand(40) - 20) / 180.0 * DX_PI;
            pNewShot->speed = 1.2 + (GetRand(100) / 100.0);
            pNewShot->count = 0;
            pNewShot->kind = img_enemyShotSmallBall[3];     // 3:シアンの小玉 (2.5 x 2.5)

            pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNewShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
            pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
        }
    }

    // 2. 全ての所属弾の移動処理（壁は移動速度0、削り弾のみが飛んでいく）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->x > -900.0) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体のパターン（メインルーチンから毎フレーム呼ばれる）
// ============================================================
void EnemyPat_Tackle_Gemini()
{
    static int start_count = 0;
    static double move_dx = 0.0;
    static double move_dy = 0.0;

    if (count == 1 || start_count == 0) {
        start_count = count;
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }

    int boss_timer = count - start_count;
    int cycle = boss_timer % 480;

    if (cycle == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 突進直前にプレイヤー位置をロックオン
    if (cycle == 39 || cycle == 99 || cycle == 159) {
        double angle = atan2(player.y - enemy.y, player.x - enemy.x);
        double target_x = enemy.x + cos(angle) * 400.0;
        double target_y = enemy.y + sin(angle) * 400.0;

        if (target_x < 30.0)  target_x = 30.0;
        if (target_x > 450.0) target_x = 450.0;
        if (target_y < 30.0)  target_y = 30.0;
        if (target_y > 450.0) target_y = 450.0;

        move_dx = (target_x - enemy.x) / 20.0;
        move_dy = (target_y - enemy.y) / 20.0;

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 突進音
    if (cycle == 40 || cycle == 100 || cycle == 160) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 中央上部への帰還準備
    if (cycle == 219) {
        move_dx = (240.0 - enemy.x) / 30.0;
        move_dy = (60.0 - enemy.y) / 30.0;
    }

    // 覚醒音
    if (cycle == 270) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 覚醒中の環境SE
    if (cycle >= 270 && cycle < 390 && cycle % 15 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // ボス本体の移動
    bool is_dashing = false;
    if ((cycle >= 40 && cycle < 60) || (cycle >= 100 && cycle < 120) || (cycle >= 160 && cycle < 180)) {
        enemy.x += move_dx;
        enemy.y += move_dy;
        is_dashing = true;
    }
    else if (cycle >= 220 && cycle < 250) {
        enemy.x += move_dx;
        enemy.y += move_dy;
    }

    // 突進中の残像（弾セット）配置
    if (is_dashing && (cycle % 2 == 0)) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAfterimage;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = cycle;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = enemy.x;
        pEnemyShot->y = enemy.y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->count = 0;
        pEnemyShot->kind = img_enemyShotDiamond[1]; // 1:黄色の菱形弾

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}