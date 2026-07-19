// EnemyPat_Tackle_DeepSeek.cpp
// 巨軀激突『デストロイ・ラッシュ』

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕パターン関数（静的）
// ------------------------------------------------------------

// 側面針弾（左右共通）
static void PatternRushSide(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double sign = (double)pSet->param_i[0];   // -1:左, +1:右
        const double dx = pSet->param_d[0];             // 突進方向x
        const double dy = pSet->param_d[1];             // 突進方向y

        // 側面方向（左に -dy, 右に +dy を垂直方向として扱う）
        double perp_x, perp_y;
        if (sign < 0) {         // 左側面
            perp_x = -dy;
            perp_y = dx;
        }
        else {                // 右側面
            perp_x = dy;
            perp_y = -dx;
        }

        const double offset = 30.0;
        const double ox = pSet->x + perp_x * offset;
        const double oy = pSet->y + perp_y * offset;

        const double aim = atan2(player.y - oy, player.x - ox);
        const double step = 7.5 * DX_PI / 180.0;   // 5way用の角度刻み

        for (int i = -10; i <= 10; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = ox;
            p->y = oy;
            p->muki = aim + i * step;
            p->speed = 3.0;
            // 左：青(4)の銃弾、右：赤(0)の銃弾（針弾）
            p->kind = (sign < 0) ? img_enemyShotBullet[4] : img_enemyShotBullet[0];

            // リスト接続
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // 毎フレームの移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 後方破裂弾
static void PatternRushRear(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double dx = pSet->param_d[0];
        const double dy = pSet->param_d[1];
        const double offset = 28.0;                    // 後方オフセット
        const double ox = pSet->x - dx * offset;
        const double oy = pSet->y - dy * offset;
        const double base_angle = atan2(dy, dx) + DX_PI; // 突進と逆方向

        const int num = 12;                              // 6個ほど低速で漂う
        const double spread = 60.0 * DX_PI / 180.0;    // ±30度の範囲

        for (int i = 0; i < num; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = ox;
            p->y = oy;
            p->muki = base_angle + (GetRand(200) - 100) / 100.0 * spread;
            p->speed = 1.2 + GetRand(80) / 100.0;      // 1.2〜2.0
            p->kind = img_enemyShotScale[5];           // マゼンタの鱗弾（紫の破裂弾）

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 衝撃波＋破片（激突時）
static void PatternImpact(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // --- 衝撃波リング ---
        const int ring_num = 50;
        for (int i = 0; i < ring_num; ++i) {
            sEnemyShot* p = new sEnemyShot;
            double ang = 2.0 * DX_PI * i / ring_num;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = ang;
            p->speed = 1.5;
            p->kind = img_enemyShotMediumBall[6];      // 白の中玉

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }

        // --- 破片（ランダム小弾） ---
        const int debris_num = 36;
        for (int i = 0; i < debris_num; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x + GetRand(10) - 5;
            p->y = pSet->y + GetRand(10) - 5;
            p->muki = 2.0 * DX_PI * GetRand(1000) / 1000.0;
            p->speed = 1.0 + GetRand(200) / 100.0;     // 1.0〜3.0
            p->kind = img_enemyShotSmallBall[GetRand(7)]; // ランダム色小玉

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 後退中の3way弾
static void PatternRetreat3Way(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const double aim = atan2(player.y - pSet->y, player.x - pSet->x);
        const double step = 5.0 * DX_PI / 180.0;   // 5度刻み

        for (int i = -2; i <= 2; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = aim + i * step;
            p->speed = 3.5;
            p->kind = img_enemyShotSmallBall[1];   // 黄色の小玉

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Tackle_DeepSeek()
{
    // 状態
    enum State { CHARGE, RUSH, STUN, RETREAT };
    static State phase;
    static int phase_timer;

    // 突進パラメータ
    static double rush_target_x, rush_target_y;
    static double rush_dir_x, rush_dir_y;
    static double rush_speed = 6.0;

    // 後退目標
    static const double retreat_x = 240.0;
    static const double retreat_y = 40.0;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        phase = CHARGE;
        phase_timer = 0;
        return;
    }

    // フェーズ進行
    phase_timer++;

    switch (phase) {
    case CHARGE:
        // 画面上部でエネルギー充填（1.5秒）
        // 実際の照準レーザーなどはビジュアル面なので省略
        if (phase_timer == 30) PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        if (phase_timer >= 90) {
            // 突進開始：現在の自機位置を狙う
            rush_target_x = player.x;
            rush_target_y = player.y;

            double dx = rush_target_x - enemy.x;
            double dy = rush_target_y - enemy.y;
            double len = sqrt(dx * dx + dy * dy);
            if (len > 0.0) {
                rush_dir_x = dx / len;
                rush_dir_y = dy / len;
            }
            else {
                rush_dir_x = 0.0;
                rush_dir_y = 1.0;   // 真下フォールバック
            }
            phase = RUSH;
            phase_timer = 0;
        }
        break;

    case RUSH:
        // 高速直進
        enemy.x += rush_dir_x * rush_speed;
        enemy.y += rush_dir_y * rush_speed;

        // 0.2秒(12フレーム)ごとに側面/後方弾を生成
        if (phase_timer % 12 == 0) {
            // 左側面
            {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = PatternRushSide;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = 0.0;
                pSet->kind = 0;
                pSet->param_d[0] = rush_dir_x;
                pSet->param_d[1] = rush_dir_y;
                pSet->param_i[0] = -1;                     // 左

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
            // 右側面
            {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = PatternRushSide;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = 0.0;
                pSet->kind = 0;
                pSet->param_d[0] = rush_dir_x;
                pSet->param_d[1] = rush_dir_y;
                pSet->param_i[0] = 1;                      // 右

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
            // 後方
            {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = PatternRushRear;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = 0.0;
                pSet->kind = 0;
                pSet->param_d[0] = rush_dir_x;
                pSet->param_d[1] = rush_dir_y;

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }

        // 画面下部到達で激突
        if (enemy.y >= 440.0) {
            enemy.y = 440.0;   // 下端に固定
            // 衝撃波＋破片
            {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = PatternImpact;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
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
            phase = STUN;
            phase_timer = 0;
        }
        break;

    case STUN:
        // 硬直（1.5秒）
        if (phase_timer >= 90) {
            phase = RETREAT;
            phase_timer = 0;
        }
        break;

    case RETREAT:
    {
        // ゆっくり上部へ後退
        double dx = retreat_x - enemy.x;
        double dy = retreat_y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < 1.5) {
            // 到達 → 再度チャージへ
            enemy.x = retreat_x;
            enemy.y = retreat_y;
            phase = CHARGE;
            phase_timer = 0;
        }
        else {
            // y方向の移動サイン（向き：上なら-1、下なら1、動かないなら0）
            double signY = (dy > 0) ? 1.0 : ((dy < 0) ? -1.0 : 0.0);

            // y方向の速度を1.2に固定（向きを考慮）
            enemy.y += 1.2 * signY;

            // x方向は、元の比率（dx / dy）を保ったまま変化させる
            if (dy != 0) {
                enemy.x += (dx / abs(dy)) * 1.2;
            }
            else {
                // もし完全に真横（dyが0）の場合の処理（必要に応じて）
                enemy.x += (dx > 0) ? 1.2 : -1.2;
            }

            // 0.33秒(20フレーム)ごとに3way
            if (phase_timer % 20 == 0) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = PatternRetreat3Way;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
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
        }
    }
    break;
    }
}