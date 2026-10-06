// enemyPat_Tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 定数
// ============================================================
static const double WALL_SPACING = 12.0;
static const int    WALL_COUNT = 20*2;
static const double START_Y = -150.0-100;
static const double GATE_WIDTH = 80.0;
static const double LEFT_GATE_CENTER = 140.0;
static const double RIGHT_GATE_CENTER = 340.0;
static const double BOTTOM_Y = START_Y + (WALL_COUNT - 1) * WALL_SPACING;

// ============================================================
// 左右反転用ヘルパ
// ============================================================
static inline double ApplyFlipX(double x, int flip) {
    return flip ? (480.0 - x) : x;
}

// ============================================================
// 弾を1つ追加するヘルパ
// ============================================================
static sEnemyShot* AddShot(
    sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed,
    int kind,
    int pi0, int pi1, int pi2, int pi3,
    double pd0, double pd1)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->margin = 480.0;
    p->param_i[0] = pi0;
    p->param_i[1] = pi1;
    p->param_i[2] = pi2;
    p->param_i[3] = pi3;
    p->param_d[0] = pd0;
    p->param_d[1] = pd1;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// ============================================================
// ミュラーリヤー・ゲート本体
// ============================================================
static void ShotMLGate(sEnemyShotSet* pSet)
{
    int flip = pSet->param_i[2];

    // --------------------------------------------------------
    // 初回：壁と矢羽根を生成（座標は反転前の原位置で保持）
    // --------------------------------------------------------
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double leftGateLeftWallX = LEFT_GATE_CENTER - GATE_WIDTH / 2.0;
        double leftGateRightWallX = LEFT_GATE_CENTER + GATE_WIDTH / 2.0;
        double rightGateLeftWallX = RIGHT_GATE_CENTER - GATE_WIDTH / 2.0;
        double rightGateRightWallX = RIGHT_GATE_CENTER + GATE_WIDTH / 2.0;

        // --- 左ゲート：内向き矢羽根（安全） ---
        for (int i = 0; i < WALL_COUNT; i++) {
            double y = START_Y + i * WALL_SPACING;
            AddShot(pSet, leftGateLeftWallX, y, 0, 0, img_enemyShotSmallBall[4],
                0, 0, 0, 0, leftGateLeftWallX, y);
            AddShot(pSet, leftGateRightWallX, y, 0, 0, img_enemyShotSmallBall[4],
                0, 0, 1, 0, leftGateRightWallX, y);
        }
        AddShot(pSet, leftGateLeftWallX + 10, BOTTOM_Y, 0, 0, img_enemyShotMediumBall[5],
            0, 0, 0, 0, leftGateLeftWallX + 10, BOTTOM_Y);
        AddShot(pSet, leftGateLeftWallX, BOTTOM_Y - 6, 0, 0, img_enemyShotSmallBall[5],
            0, 0, 0, 1, leftGateLeftWallX, BOTTOM_Y - 6);
        AddShot(pSet, leftGateLeftWallX, BOTTOM_Y + 6, 0, 0, img_enemyShotSmallBall[5],
            0, 0, 0, 2, leftGateLeftWallX, BOTTOM_Y + 6);
        AddShot(pSet, leftGateRightWallX - 10, BOTTOM_Y, 0, 0, img_enemyShotMediumBall[5],
            0, 0, 1, 0, leftGateRightWallX - 10, BOTTOM_Y);
        AddShot(pSet, leftGateRightWallX, BOTTOM_Y - 6, 0, 0, img_enemyShotSmallBall[5],
            0, 0, 1, 1, leftGateRightWallX, BOTTOM_Y - 6);
        AddShot(pSet, leftGateRightWallX, BOTTOM_Y + 6, 0, 0, img_enemyShotSmallBall[5],
            0, 0, 1, 2, leftGateRightWallX, BOTTOM_Y + 6);

        // --- 右ゲート：外向き矢羽根（罠） ---
        for (int i = 0; i < WALL_COUNT; i++) {
            double y = START_Y + i * WALL_SPACING;
            AddShot(pSet, rightGateLeftWallX, y, 0, 0, img_enemyShotSmallBall[4],
                0, 1, 0, 0, rightGateLeftWallX, y);
            AddShot(pSet, rightGateRightWallX, y, 0, 0, img_enemyShotSmallBall[4],
                0, 1, 1, 0, rightGateRightWallX, y);
        }
        AddShot(pSet, rightGateLeftWallX - 10, BOTTOM_Y, 0, 0, img_enemyShotMediumBall[5],
            0, 1, 0, 0, rightGateLeftWallX - 10, BOTTOM_Y);
        AddShot(pSet, rightGateLeftWallX, BOTTOM_Y - 6, 0, 0, img_enemyShotSmallBall[5],
            0, 1, 0, 1, rightGateLeftWallX, BOTTOM_Y - 6);
        AddShot(pSet, rightGateLeftWallX, BOTTOM_Y + 6, 0, 0, img_enemyShotSmallBall[5],
            0, 1, 0, 2, rightGateLeftWallX, BOTTOM_Y + 6);
        AddShot(pSet, rightGateRightWallX + 10, BOTTOM_Y, 0, 0, img_enemyShotMediumBall[5],
            0, 1, 1, 0, rightGateRightWallX + 10, BOTTOM_Y);
        AddShot(pSet, rightGateRightWallX, BOTTOM_Y - 6, 0, 0, img_enemyShotSmallBall[5],
            0, 1, 1, 1, rightGateRightWallX, BOTTOM_Y - 6);
        AddShot(pSet, rightGateRightWallX, BOTTOM_Y + 6, 0, 0, img_enemyShotSmallBall[5],
            0, 1, 1, 2, rightGateRightWallX, BOTTOM_Y + 6);
    }

    // --------------------------------------------------------
    // 毎フレームの更新
    // --------------------------------------------------------
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            double yOffset;
            if (pSet->count < 60) {
                yOffset = pSet->count * 2.0;
            }
            else if (pSet->count < 120) {
                yOffset = 120.0;
            }
            else {
                yOffset = 120.0 + (pSet->count - 120) * 2.0;
            }

            double xOffset = 0.0;
            if (pSet->count >= 120) {
                double delta = (pSet->count - 120) * 0.5;
                if (pShot->param_i[1] == 0) {
                    // 左ゲート：外へ開く
                    if (pShot->param_i[2] == 0) xOffset = -delta;
                    else                         xOffset = delta;
                }
                else {
                    // 右ゲート：内へ閉じる
                    if (pShot->param_i[2] == 0) xOffset = delta;
                    else                         xOffset = -delta;
                }
            }
            // 反転を適用
            double baseX = ApplyFlipX(pShot->param_d[0], flip);
            double baseY = pShot->param_d[1];
            double mirroredOffset = flip ? -xOffset : xOffset;
            pShot->x = baseX + mirroredOffset;
            pShot->y = baseY + yOffset;
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // --------------------------------------------------------
    // 右ゲート（罠）が閉じ始めたら自機狙い3way
    // --------------------------------------------------------
    if (pSet->count == 120 && pSet->param_i[0] == 0) {
        pSet->param_i[0] = 1;
        double sx = ApplyFlipX(RIGHT_GATE_CENTER, flip);
        double sy = BOTTOM_Y + 120.0;
        double baseMuki = atan2(player.y - sy, player.x - sx);
        for (int i = -1-2; i <= 1+2; i++) {
            double muki = baseMuki + i * 15.0 / 180.0 * DX_PI;
            AddShot(pSet, sx, sy, muki, 3.0, img_enemyShotSmallBall[0],
                2, 0, 0, 0, 0, 0);
        }
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    // 締めの扇弾
    // --------------------------------------------------------
    if (pSet->count == 200 && pSet->param_i[1] == 0) {
        pSet->param_i[1] = 1;
        double sx = 240.0;
        double sy = 200.0;
        double baseMuki = GetRand(360) / 360.0 * DX_PI * 2.0;
        const int N = 12 * 5;
        for (int i = 0; i < N; i++) {
            double muki = baseMuki + i * DX_PI * 2.0 / N;
            AddShot(pSet, sx, sy, muki, 2.5, img_enemyShotSmallBall[8],
                3, 0, 0, 0, 0, 0);
        }
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_MullerLyer_DeepSeek()
{
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }

    // 360フレームごとにミュラーリヤー・ゲートを出現させる
    if (count % 360 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotMLGate;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;
        pSet->param_i[0] = 0;             // 自機狙い弾生成済みフラグ
        pSet->param_i[1] = 0;             // 扇弾生成済みフラグ
        pSet->param_i[2] = GetRand(1);    // 0: 通常、1: 左右反転

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}