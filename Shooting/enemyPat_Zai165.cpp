#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
//  サブタレイニアンサン（東方地霊殿）
//  A: 画面外接円上から中央に引き寄せられる赤中玉（引力弾）
//  B: 中央から前後に撃たれる赤小玉（螺旋）
//  C: 中央に密集する静止した赤大玉（核）
//  ※ 自機は常に画面中央への引力を受ける
//  ※ HPが減るほど A:引力強化 / C:核成長
// ============================================================

// 画面中央
static constexpr double CENTER_X = 240.0;
static constexpr double CENTER_Y = 240.0;
// 画面(480x480)の外接円の半径 = 240 * sqrt(2)
static constexpr double CIRCUMSCRIBED_RADIUS = 240.0 * 1.41421356;

// 引力の強さ（EnemyPat_SubterraneanSun_Zai 側でHP依存に更新）
static double g_pullSpeed = 0.2;
// 引力の比例定数（spawnForceParticles および移動量に適用）
static constexpr double GRAVITY_RATIO = 3.0;


// ----- A: 引力弾 -----
// 外接円上に狭い角度間隔で発生し、自機と同じ強さの引力を受けて中央に引き寄せられる。
// 加速はさせず、常に引力の大きさと同じ速度で中央に向かい、中央に達したら消滅する。
static void ShotGravity(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // param_d[0]: 発生の基準角度
        // param_d[1]: 弾間の角度間隔（狭い）
        // param_i[0]: 同時発生数
        double baseAngle = pEnemyShotSet->param_d[0];
        double angleStep = pEnemyShotSet->param_d[1];
        int    spawnNum = pEnemyShotSet->param_i[0];

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < spawnNum; i++) {
            pEnemyShot = new sEnemyShot;
            // 基準角度の周りに狭い間隔で並べる
            double angle = baseAngle + (i - (spawnNum - 1) / 2.0) * angleStep;
            pEnemyShot->x = CENTER_X + CIRCUMSCRIBED_RADIUS * cos(angle);
            pEnemyShot->y = CENTER_Y + CIRCUMSCRIBED_RADIUS * sin(angle);
            pEnemyShot->muki = 0.0;     // 速度0で発生（移動は引力で行う）
            pEnemyShot->speed = 0.0;
            pEnemyShot->margin = 240.0;                     // 指定通り
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾を中央に向かって「引力の大きさと同じ速度」で移動
    // （自機と同じ強さの引力＝同じ速度で引き寄せられる）
    double effectiveSpeed = g_pullSpeed * GRAVITY_RATIO;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double dx = CENTER_X - pShot->x;
        double dy = CENTER_Y - pShot->y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist <= effectiveSpeed) {
            // 中央に到達したら消滅
            // （画面外に飛ばし、メインルーチンの画面外削除に委ねる）
            pShot->x = -10000.0;
            pShot->y = -10000.0;
        }
        else {
            // 加速せず、引力と同じ速度で中央へ
            pShot->x += effectiveSpeed * dx / dist;
            pShot->y += effectiveSpeed * dy / dist;
        }
        pShot = pShot->next;
    }
}


// ----- B: 中央から前後に撃たれる赤小玉 -----
// 引力の影響を受けず、一定速度で直進。撃つ方向は時間経過で回転。
// 速度は一定で、角度に微小な乱数を加える。
static void ShotCoreSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // param_d[0]: 基準角度
        // param_d[1]: 速度（一定）
        double baseAngle = pEnemyShotSet->param_d[0];
        double speed = pEnemyShotSet->param_d[1];

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 前後2方向に撃つ
        for (int dir = 0; dir < 2; dir++) {
            pEnemyShot = new sEnemyShot;
            // 前後 + 微小な乱数（±5°）
            double angle = baseAngle + (dir == 1 ? DX_PI : 0.0)
                + (GetRand(10) - 5) / 180.0 * DX_PI;
            pEnemyShot->x = CENTER_X;
            pEnemyShot->y = CENTER_Y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = speed;     // 一定速度
            pEnemyShot->margin = 20.0;
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 直進（引力の影響を受けない）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// ----- C: 核（中央に密集する静止した赤大玉）-----
// 引力の影響を受けない。HP段階に応じて外側のリングを追加し、核を成長させる。
static void ShotCore(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // param_d[0]: リング半径（0なら中心のみ）
        // param_i[0]: リング内の弾数
        double ringRadius = pEnemyShotSet->param_d[0];
        int    n = pEnemyShotSet->param_i[0];

        for (int i = 0; i < n; i++) {
            double a = (n <= 1) ? 0.0 : (double)i / n * 2.0 * DX_PI;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = CENTER_X + ringRadius * cos(a);
            pEnemyShot->y = CENTER_Y + ringRadius * sin(a);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;       // 静止
            pEnemyShot->margin = 0.0;       // 静止弾なので画面外判定で消えないよう0
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤大玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 静止しているため位置更新は行わない
}


// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_SubterraneanSun_Zai()
{
    static int    muki;
    static int    shot_count;
    static double gravityBaseAngle; // Aの発生基準角度
    static double spiralAngle;      // Bの発射角度
    static int    lastCoreStage;    // 前回の核の段階

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
        gravityBaseAngle = 0.0;
        spiralAngle = 0.0;
        lastCoreStage = -1;
        g_pullSpeed = 0.2;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ----- HPに応じた引力の強さ -----
    // 残り体力が少ないほど引力は強くなる（0.2 -> 0.8）
    double hpRatio = 1.0 - (double)enemy.hp / (double)enemy.maxHp; // 0.0 -> 1.0
    g_pullSpeed = 0.2 + hpRatio * 0.6;

    // ----- プレイヤーへの引力（常に画面中央に向かう）-----
    {
        double dx = CENTER_X - player.x;
        double dy = CENTER_Y - player.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist > 0.001) {
            double pullSpeed = g_pullSpeed;
            double angle = atan2(dy, dx);
            double ratio = GRAVITY_RATIO;
            // 引力の発生を表現するパーティクル
            spawnForceParticles(player.x, player.y,
                pullSpeed * cos(angle) * ratio,
                pullSpeed * sin(angle) * ratio);
            // プレイヤー位置を引力に応じて変更
            player.x += pullSpeed * cos(angle) * ratio;
            player.y += pullSpeed * sin(angle) * ratio;
        }
    }

    // ----- 基準角度の回転 -----
    gravityBaseAngle += 0.015; // Aの発生基準角度を時間で変化
    spiralAngle += 0.060; // Bの発射方向を時間で回転

    // ----- C: 核（HP段階に応じて成長）-----
    {
        // HP 200-151: stage 0, 150-101: stage 1, 100-51: stage 2, 50-1: stage 3
        int coreStage;
        if (enemy.hp > 150) coreStage = 0;
        else if (enemy.hp > 100) coreStage = 1;
        else if (enemy.hp > 50)  coreStage = 2;
        else                     coreStage = 3;

        if (coreStage != lastCoreStage) {
            // 段階が変わるごとに新しいリングを追加（核が成長する）
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotCore;
            pEnemyShotSet->x = CENTER_X;
            pEnemyShotSet->y = CENTER_Y;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = coreStage;
            if (coreStage == 0) {
                // 最初は中心の1発のみ
                pEnemyShotSet->param_d[0] = 0.0;
                pEnemyShotSet->param_i[0] = 1;
            }
            else {
                // 段階が上がるごとに外側にリングを追加
                pEnemyShotSet->param_d[0] = 22.0 * coreStage; // リング半径
                pEnemyShotSet->param_i[0] = 6 * coreStage;     // リング内の弾数
            }
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            lastCoreStage = coreStage;
        }
    }

    // ----- A: 引力弾（中頻度で、狭い角度範囲に複数発）-----
    if (count % 30 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGravity;
        pEnemyShotSet->x = CENTER_X;
        pEnemyShotSet->y = CENTER_Y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->param_d[0] = gravityBaseAngle; // 基準角度
        pEnemyShotSet->param_d[1] = 0.04;             // 弾間角度（狭い）
        pEnemyShotSet->param_i[0] = 9;                 // 同時発生数
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ----- B: 赤小玉を前後に撃つ（高頻度）-----
    if (count % 6 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCoreSpiral;
        pEnemyShotSet->x = CENTER_X;
        pEnemyShotSet->y = CENTER_Y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->param_d[0] = spiralAngle; // 基準角度
        pEnemyShotSet->param_d[1] = 3.5;          // 速度（一定）
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
