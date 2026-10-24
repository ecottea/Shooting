#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
// サブタレイニアンサン 弾幕パターン関数
// ============================================================

// 弾幕A: 引力を受ける赤中玉の移動処理
static void PatternA(sEnemyShotSet* pSet) {
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;

        double dx = 240.0 - pShot->x;
        double dy = 240.0 - pShot->y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < 10.0) {
            // 画面中央に達したら消滅（メインルーチンで画面外消去されるように座標を飛ばす）
            pShot->x = -1000.0;
            pShot->y = -1000.0;
        }
        else {
            // 自機と同じ強さの引力を受けて移動（加速させず、常に引力の大きさと同じ速度で移動）
            double hp_ratio = (double)enemy.hp / enemy.maxHp;
            double pullSpeed = 1.5 + (1.0 - hp_ratio) * 2.5; // 残り体力に応じて 1.5 〜 4.0 に変化

            double angle = atan2(dy, dx);
            pShot->x += pullSpeed * cos(angle);
            pShot->y += pullSpeed * sin(angle);
        }
        pShot = nextShot;
    }
}

// 弾幕B: 回転しながら前後に撃つ赤小玉の移動処理
static void PatternB(sEnemyShotSet* pSet) {
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;
        // 引力の影響を受けない、一定速度での直進移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = nextShot;
    }
}

// 弾幕C: 静止した赤大玉（核）の移動処理
static void PatternC(sEnemyShotSet* pSet) {
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;
        // 引力の影響を受けない、静止した弾なので位置更新は行わない
        // (核の大きさ増加は生成時の margin と配置半径で表現済み)
        pShot = nextShot;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_SubterraneanSun_Qwen()
{
    static int shot_count;
    static double base_angle_a;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        shot_count = 0;
        base_angle_a = 0.0;
    }
    else {
        // 敵の移動（例: 中央付近を左右にゆっくり移動）
        enemy.x = 240.0 + 100.0 * sin(count * 0.02);
        enemy.y = 40.0;
    }

    // --------------------------------------------------------
    // 自機への引力処理
    // --------------------------------------------------------
    double hp_ratio = (double)enemy.hp / enemy.maxHp;
    double pullSpeed = 1.5 + (1.0 - hp_ratio) * 2.5; // 残り体力に応じて 1.5 〜 4.0 に変化

    double dx = 240.0 - player.x;
    double dy = 240.0 - player.y;
    double dist = sqrt(dx * dx + dy * dy);

    if (dist > 0.0) {
        double angle = atan2(dy, dx);
        double ratio = 3.0; // 比例定数

        // 指定された引力発生表現のコード
        spawnForceParticles(player.x, player.y, pullSpeed * cos(angle) * ratio, pullSpeed * sin(angle) * ratio);

        // 引力による自機座標の変更
        player.x += pullSpeed * cos(angle);
        player.y += pullSpeed * sin(angle);
    }

    // --------------------------------------------------------
    // 弾幕A: 中頻度で外接円上に赤中玉を発生
    // --------------------------------------------------------
    if (count % 45 == 1) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = PatternA;
        pSet->x = 240.0;
        pSet->y = 240.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 発生の基準となる角度は時間経過で変化させる
        base_angle_a += 0.15;
        double R = 340.0; // 480x480 画面の外接円半径 (240 * √2 ≒ 339.4)

        // 狭い間隔 (5度ごと = 72発) で発生
        for (int i = 0; i < 72/3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double theta = base_angle_a + i * 2.0 * DX_PI / 72.0*3;
            pShot->x = 240.0 + R * cos(theta);
            pShot->y = 240.0 + R * sin(theta);
            pShot->kind = img_enemyShotMediumBall[0]; // 赤中玉
            pShot->margin = 240.0; // 仕様: 弾のmarginは240とする
            pShot->speed = 0.0;    // 速度0で発生
            pShot->param_i[0] = 1; // PatternA 識別子

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --------------------------------------------------------
    // 弾幕B: 高頻度で画面中央から赤小玉を前後に撃つ
    // --------------------------------------------------------
    if (count % 10 == 1) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = PatternB;
        pSet->x = 240.0;
        pSet->y = 240.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 撃つ方向は時間経過で回転させる
        double angle_b = count * 0.05/2;

        // 前方へ発射
        sEnemyShot* pShot1 = new sEnemyShot;
        pShot1->x = 240.0;
        pShot1->y = 240.0;
        pShot1->kind = img_enemyShotSmallBall[0]; // 赤小玉
        pShot1->muki = angle_b + (GetRand(20) - 10) / 100.0; // 角度に微小な乱数を加える
        pShot1->speed = 3.0; // 速度は一定
        pShot1->margin = 2.5;
        pShot1->param_i[0] = 2; // PatternB 識別子

        pShot1->prev = pSet->pEnemyShotHead->prev;
        pShot1->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot1;
        pSet->pEnemyShotHead->prev = pShot1;

        // 後方へ発射 (前後に撃つ)
        sEnemyShot* pShot2 = new sEnemyShot;
        pShot2->x = 240.0;
        pShot2->y = 240.0;
        pShot2->kind = img_enemyShotSmallBall[0]; // 赤小玉
        pShot2->muki = angle_b + DX_PI + (GetRand(20) - 10) / 100.0; // 角度に微小な乱数を加える
        pShot2->speed = 3.0; // 速度は一定
        pShot2->margin = 2.5;
        pShot2->param_i[0] = 2; // PatternB 識別子

        pShot2->prev = pSet->pEnemyShotHead->prev;
        pShot2->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot2;
        pSet->pEnemyShotHead->prev = pShot2;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --------------------------------------------------------
    // 弾幕C: 静止した赤大玉を画面中央に密集させて核を表現
    // --------------------------------------------------------
    if (count % 10 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = PatternC;
        pSet->x = 240.0;
        pSet->y = 240.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // ボスの残り体力に応じて核の大きさを増加させる
        double core_radius = 20.0 + (1.0 - hp_ratio) * 60.0; // 配置半径: 20.0 〜 80.0
        double core_margin = 20.0 + (1.0 - hp_ratio) * 30.0; // margin: 20.0 〜 50.0

        // 1フレームに複数発生成して密集させる
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double r = (double)GetRand(100) / 100.0 * core_radius;
            double theta = (double)GetRand(360) / 360.0 * 2.0 * DX_PI;

            pShot->x = 240.0 + r * cos(theta);
            pShot->y = 240.0 + r * sin(theta);
            pShot->kind = img_enemyShotLargeBall[0]; // 赤大玉
            pShot->margin = core_margin;             // 核の大きさ増加を反映
            pShot->speed = 0.0;                      // 静止
            pShot->param_i[0] = 3;                   // PatternC 識別子

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}