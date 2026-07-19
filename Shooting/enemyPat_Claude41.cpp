// enemyPat_ReflectBrick.cpp
//
// 弾幕名: 「反射煉瓦陣」
// 画面上部に配置した「レンガ」弾群に向かって「ボール」弾が反射しながら飛び回り、
// 命中したレンガは崩壊して破片弾をばら撒く。レンガの列が全滅すると
// 残りのレンガが1段分プレイヤー側へ降りてくる、ブロック崩しモチーフの弾幕。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const int    BRICK_ROWS = 7;
static const int    BRICK_COLS = 8;
static const double BRICK_START_X = 72.0;
static const double BRICK_START_Y = 70.0;
static const double BRICK_SPACING_X = 48.0;
static const double BRICK_SPACING_Y = 34.0;
static const double BRICK_RADIUS = 10.0;  // 大玉(20x20)相当の当たり半径
static const double BALL_RADIUS = 3.5;   // 中玉(7x7)相当の当たり半径
static const double DROP_STEP_Y = 26.0;  // 列全滅時にレンガ群が降りてくる量
static const double BALL_MAX_SPEED = 6.0;
static const double WALL_LEFT = 20.0;
static const double WALL_RIGHT = 460.0;
static const double WALL_TOP = 20.0;

// レンガの色 = 青(4)、ボールの色 = 白(6)
static const int COLOR_BRICK = 4;
static const int COLOR_BALL = 6;

// ============================================================
//  レンガ管理用のファイルスコープ状態
// ============================================================
static bool        g_brickAlive[BRICK_ROWS][BRICK_COLS];
static sEnemyShot* g_brickShotPtr[BRICK_ROWS][BRICK_COLS];
static int         g_brickAliveCountRow[BRICK_ROWS];
static int         g_totalBricksAlive;
static double      g_dropOffsetCurrent; // 実際に描画へ反映するオフセット（滑らかに追従）
static double      g_dropOffsetTarget;  // 目標オフセット
static bool         g_finaleTriggered;

// ============================================================
//  前方宣言
// ============================================================
static void SpawnDebrisBurst(double x, double y, int fragCount, double speedMin, double speedMax, int colorIdx);
static void BreakBrick(int row, int col);
static void ShotBrickWall(sEnemyShotSet* pEnemyShotSet);
static void ShotBall(sEnemyShotSet* pEnemyShotSet);
static void ShotDebris(sEnemyShotSet* pEnemyShotSet);

// ============================================================
//  破片弾の一斉発生（レンガ崩壊 / フィナーレ共用）
// ============================================================
static void SpawnDebrisBurst(double x, double y, int fragCount, double speedMin, double speedMax, int colorIdx)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotDebris;
    pSet->x = x;
    pSet->y = y;
    pSet->param_i[0] = fragCount;
    pSet->param_i[1] = colorIdx;
    pSet->param_d[2] = speedMin;
    pSet->param_d[3] = speedMax;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ============================================================
//  レンガ1個を破壊する
// ============================================================
static void BreakBrick(int row, int col)
{
    sEnemyShot* p = g_brickShotPtr[row][col];
    if (p == nullptr) return;

    double bx = p->x;
    double by = p->y;

    // リストから外してプールへ返却
    p->prev->next = p->next;
    p->next->prev = p->prev;
    delete p;
    g_brickShotPtr[row][col] = nullptr;

    g_brickAlive[row][col] = false;
    g_brickAliveCountRow[row]--;
    g_totalBricksAlive--;

    // 破片弾をばら撒く（橙と赤を交互に）
    SpawnDebrisBurst(bx, by, 5, 0.8, 1.6, (col % 2 == 0) ? 8 : 0);

    // その列が全滅したら、残っているレンガ全体を1段降ろす
    if (g_brickAliveCountRow[row] == 0) {
        g_dropOffsetTarget += DROP_STEP_Y;
    }
}

// ============================================================
//  弾幕：レンガ壁
// ============================================================
static void ShotBrickWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    if (pEnemyShotSet->count == 0) {
        for (int row = 0; row < BRICK_ROWS; row++) {
            for (int col = 0; col < BRICK_COLS; col++) {
                pShot = new sEnemyShot;
                pShot->kind = img_enemyShotLargeBall[COLOR_BRICK];
                pShot->x = BRICK_START_X + col * BRICK_SPACING_X;
                pShot->y = BRICK_START_Y + row * BRICK_SPACING_Y;
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->param_i[0] = row;
                pShot->param_i[1] = col;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;

                g_brickShotPtr[row][col] = pShot;
            }
        }
    }

    // 列全滅による降下オフセットを滑らかに追従させる
    g_dropOffsetCurrent += (g_dropOffsetTarget - g_dropOffsetCurrent) * 0.06;

    // 生きているレンガの位置を更新（微振動 + 降下オフセット）
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int row = pShot->param_i[0];
        int col = pShot->param_i[1];
        double baseX = BRICK_START_X + col * BRICK_SPACING_X;
        double jitter = sin((count + row * 17 + col * 7) * 0.05) * 2.0;

        pShot->x = baseX + jitter;
        pShot->y = BRICK_START_Y + row * BRICK_SPACING_Y + g_dropOffsetCurrent;

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：反射ボール
// ============================================================
static void ShotBall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    if (pEnemyShotSet->count == 0) {
        pShot = new sEnemyShot;
        pShot->kind = img_enemyShotMediumBall[COLOR_BALL];
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki;
        pShot->speed = pEnemyShotSet->param_d[0];

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double vx = pShot->speed * cos(pShot->muki);
        double vy = pShot->speed * sin(pShot->muki);
        bool collided = false;

        // レンガとの衝突判定（円と円）
        for (int row = 0; row < BRICK_ROWS && !collided; row++) {
            for (int col = 0; col < BRICK_COLS; col++) {
                if (!g_brickAlive[row][col]) continue;
                sEnemyShot* b = g_brickShotPtr[row][col];
                if (b == nullptr) continue;

                double dx = pShot->x - b->x;
                double dy = pShot->y - b->y;
                double dist2 = dx * dx + dy * dy;
                double rr = BALL_RADIUS + BRICK_RADIUS;

                if (dist2 <= rr * rr) {
                    double dist = sqrt(dist2);
                    double nx, ny;
                    if (dist > 0.0001) {
                        nx = dx / dist;
                        ny = dy / dist;
                    }
                    else {
                        nx = 0.0;
                        ny = -1.0;
                    }

                    // 法線に対して反射
                    double dot = vx * nx + vy * ny;
                    vx -= 2.0 * dot * nx;
                    vy -= 2.0 * dot * ny;

                    // レンガ表面まで押し出す（めり込み防止）
                    pShot->x = b->x + nx * rr;
                    pShot->y = b->y + ny * rr;

                    // 命中のたびに少しだけ加速（上限あり）
                    double spd = sqrt(vx * vx + vy * vy) * 1.04;
                    if (spd > BALL_MAX_SPEED) spd = BALL_MAX_SPEED;
                    double ang = atan2(vy, vx);
                    vx = spd * cos(ang);
                    vy = spd * sin(ang);

                    BreakBrick(row, col);
                    collided = true;
                    break;
                }
            }
        }

        if (!collided) {
            double nx = pShot->x + vx;
            double ny = pShot->y + vy;

            if (nx < WALL_LEFT && vx < 0.0) vx = -vx;
            if (nx > WALL_RIGHT && vx > 0.0) vx = -vx;
            if (ny < WALL_TOP && vy < 0.0) vy = -vy;

            pShot->x += vx;
            pShot->y += vy;
        }

        pShot->speed = sqrt(vx * vx + vy * vy);
        pShot->muki = atan2(vy, vx);

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：崩壊時の破片
// ============================================================
static void ShotDebris(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    if (pEnemyShotSet->count == 0) {
        int fragCount = pEnemyShotSet->param_i[0];
        int colorIdx = pEnemyShotSet->param_i[1];
        double speedMin = pEnemyShotSet->param_d[2];
        double speedMax = pEnemyShotSet->param_d[3];

        for (int i = 0; i < fragCount; i++) {
            pShot = new sEnemyShot;

            // GetRand(x) は 0～x の整数を返す
            double angle = GetRand(359) / 180.0 * DX_PI;
            double spd = speedMin + (speedMax - speedMin) * GetRand(100) / 100.0;

            pShot->kind = (i % 2 == 0) ? img_enemyShotDiamond[colorIdx] : img_enemyShotBullet[colorIdx];
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = 0.0;
            pShot->speed = 0.0;
            // 速度は param_d に直接持たせて重力落下させる
            pShot->param_d[0] = spd * cos(angle);        // vx
            pShot->param_d[1] = spd * sin(angle) - 1.2;  // vy（上向きに弾けてから落下）

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_d[1] += 0.05; // 重力
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体：反射煉瓦陣
// ============================================================
void EnemyPat_BlockBreak_Claude()
{
    static int nextBallSpawnCount;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        // レンガ状態のリセット
        for (int row = 0; row < BRICK_ROWS; row++) {
            g_brickAliveCountRow[row] = BRICK_COLS;
            for (int col = 0; col < BRICK_COLS; col++) {
                g_brickAlive[row][col] = true;
                g_brickShotPtr[row][col] = nullptr;
            }
        }
        g_totalBricksAlive = BRICK_ROWS * BRICK_COLS;
        g_dropOffsetCurrent = 0.0;
        g_dropOffsetTarget = 0.0;
        g_finaleTriggered = false;

        // レンガ壁を1回だけ生成
        sEnemyShotSet* pBrickSet = new sEnemyShotSet;
        pBrickSet->count = 0;
        pBrickSet->patternFunc = ShotBrickWall;
        pBrickSet->x = 0.0;
        pBrickSet->y = 0.0;

        pBrickSet->pEnemyShotHead = new sEnemyShot;
        pBrickSet->pEnemyShotHead->prev = pBrickSet->pEnemyShotHead;
        pBrickSet->pEnemyShotHead->next = pBrickSet->pEnemyShotHead;

        pBrickSet->prev = enemyShotSetHead.prev;
        pBrickSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBrickSet;
        enemyShotSetHead.prev = pBrickSet;

        nextBallSpawnCount = 50;
    }
    else {
        // 敵本体はゆったり左右に揺れる
        enemy.x = 240.0 + sin(count * 0.015) * 80.0;
    }

    // ボールを一定間隔で発射
    if (count == nextBallSpawnCount && g_totalBricksAlive > 0) {
        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pBallSet = new sEnemyShotSet;
        pBallSet->count = 0;
        pBallSet->patternFunc = ShotBall;
        pBallSet->x = enemy.x + GetRand(40) - 20;
        pBallSet->y = enemy.y + 20.0;
        // 真下を中心に±60度のばらつきで発射
        pBallSet->muki = DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI;
        pBallSet->param_d[0] = 2.4 + GetRand(60) / 100.0; // 初速 2.4～3.0

        pBallSet->pEnemyShotHead = new sEnemyShot;
        pBallSet->pEnemyShotHead->prev = pBallSet->pEnemyShotHead;
        pBallSet->pEnemyShotHead->next = pBallSet->pEnemyShotHead;

        pBallSet->prev = enemyShotSetHead.prev;
        pBallSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBallSet;
        enemyShotSetHead.prev = pBallSet;

        nextBallSpawnCount = count + 60 + GetRand(40);
    }

    // 全レンガ崩壊でフィナーレの大放出
    if (g_totalBricksAlive <= 0 && !g_finaleTriggered) {
        SpawnDebrisBurst(enemy.x, enemy.y + 60.0, 28, 1.5, 3.2, 8);
        g_finaleTriggered = true;
    }
}