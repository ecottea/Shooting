#include "DxLib.h"
#include "gv.h"
#include <cmath>

// ============================================================
// 禁忌「レーヴァテイン」
// ============================================================
static const int    LASER_SEGMENTS = 6;      // 短レーザーの本数
static const double SEG_LEN = 64.0;   // 短レーザー1本の長さ
static const int    PERIOD0 = 120;    // 速い平行移動の期間
static const int    PERIOD1 = 180;    // 遅い平行移動＋速い回転の期間
static const int    PERIOD = PERIOD0 + PERIOD1;
static const int    EMIT_INTERVAL = 12-10;     // 菱形弾の射出間隔

// 弾幕パターン関数
static void ShotLaevateinn(sEnemyShotSet* pEnemyShotSet)
{
    // 初回のみ効果音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    // 欠けているレーザー段を補充する
    // （画面外消去でレーザー段が消えても、長レーザーを維持するため）
    // --------------------------------------------------------
    bool laserExist[LASER_SEGMENTS] = { false };
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            int idx = pShot->param_i[1];
            if (0 <= idx && idx < LASER_SEGMENTS) laserExist[idx] = true;
        }
        pShot = pShot->next;
    }
    for (int i = 0; i < LASER_SEGMENTS; i++) {
        if (!laserExist[i]) {
            sEnemyShot* pNew = new sEnemyShot;
            pNew->kind = img_enemyShotLaser[0]; // 赤の短レーザー
            pNew->x = pEnemyShotSet->x;
            pNew->y = pEnemyShotSet->y;
            pNew->muki = 0.0;
            pNew->speed = 0.0;
            pNew->param_i[0] = 1; // 1: レーザー段
            pNew->param_i[1] = i; // 何番目の段か
            pNew->param_d[0] = 0.0;

            pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNew->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
            pEnemyShotSet->pEnemyShotHead->prev = pNew;
        }
    }

    // --------------------------------------------------------
    // 周期計算
    // --------------------------------------------------------
    int total = pEnemyShotSet->count;
    int cycle = total / PERIOD;
    int t = total % PERIOD;

    double theta;                 // レーザーの向き
    double bossX, bossY;          // ボス位置＝レーザーの一端
    double moveDirX = 0.0;        // レーザーの平行移動方向
    double moveDirY = 0.0;

    if (t < PERIOD0) {
        // 模式0：速めの平行移動（角度固定）
        int t0 = t;
        int dir = (cycle % 2 == 0) ? 1 : -1;

        double startX = 80.0;
        double endX = 400.0;
        double prog = (double)t0 / (double)PERIOD0;

        if (dir > 0) {
            bossX = startX + (endX - startX) * prog;
        }
        else {
            bossX = endX - (endX - startX) * prog;
        }
        bossY = 40.0;

        theta = DX_PI / 2.0; // 下向き
        moveDirX = (double)dir;
        moveDirY = 0.0;
    }
    else {
        // 模式1：遅めの平行移動＋速めの回転移動
        int t1 = t - PERIOD0;
        double prog = (double)t1 / (double)PERIOD1;

        bossX = 240.0 + 60.0 * sin(2.0 * DX_PI * prog);
        bossY = 60.0 + 20.0 * cos(2.0 * DX_PI * prog);

        // 平行移動方向（ボス位置の微分から）
        double ddx = 60.0 * cos(2.0 * DX_PI * prog);
        double ddy = -20.0 * sin(2.0 * DX_PI * prog);
        double len = sqrt(ddx * ddx + ddy * ddy);
        if (len > 0.0001) {
            moveDirX = ddx / len;
            moveDirY = ddy / len;
        }
        else {
            moveDirX = 0.0;
            moveDirY = 0.0;
        }

        // 速い回転（2回転）
        theta = DX_PI / 2.0 + 2.0 * DX_PI * 2.0 * prog;
    }

    // ボス位置をレーザーの一端に一致させる
    enemy.x = bossX;
    enemy.y = bossY;

    // --------------------------------------------------------
    // 既存弾の更新
    // --------------------------------------------------------
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // レーザー段：長レーザーの一部として再配置
            int idx = pShot->param_i[1];
            double cx = enemy.x + ((double)idx + 0.5) * SEG_LEN * cos(theta);
            double cy = enemy.y + ((double)idx + 0.5) * SEG_LEN * sin(theta);

            pShot->x = cx;
            pShot->y = cy;
            pShot->muki = theta;
            pShot->speed = 0.0;
        }
        else if (pShot->param_i[0] == 2) {
            // 赤菱形弾：初速0から加速し、終端速度で等速直線運動
            double vmax = pShot->param_d[0];
            double accel = pShot->param_d[1];

            if (pShot->speed < vmax) {
                pShot->speed += accel;
                if (pShot->speed > vmax) pShot->speed = vmax;
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // --------------------------------------------------------
    // レーザー移動中、一定間隔で内分点から菱形弾を射出
    // --------------------------------------------------------
    if (total % EMIT_INTERVAL == 0) {
        // レーザーに垂直な2方向のうち、移動方向との内積が正の方向を選ぶ
        double perp1 = theta + DX_PI / 2.0;
        double perp2 = theta - DX_PI / 2.0;

        double dot1 = cos(perp1) * moveDirX + sin(perp1) * moveDirY;
        double dot2 = cos(perp2) * moveDirX + sin(perp2) * moveDirY;

        double emitAngle = (dot1 >= dot2) ? perp1 : perp2;
        if (fabs(moveDirX) < 0.0001 && fabs(moveDirY) < 0.0001) {
            emitAngle = perp1;
        }

        // 内分点（端点を除く）
        for (int i = 1; i < LASER_SEGMENTS; i++) {
            double px = enemy.x + (double)i * SEG_LEN * cos(theta);
            double py = enemy.y + (double)i * SEG_LEN * sin(theta);

            sEnemyShot* pNew = new sEnemyShot;
            pNew->kind = img_enemyShotDiamond[0]; // 赤菱形弾
            pNew->x = px;
            pNew->y = py;
            pNew->muki = emitAngle;
            pNew->speed = 0.0;
            pNew->param_i[0] = 2; // 2: 菱形弾
            pNew->param_i[1] = 0;
            pNew->param_d[0] = 4.0;   // 終端速度
            pNew->param_d[1] = 0.08;  // 加速度
            pNew->margin = 480;

            pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNew->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
            pEnemyShotSet->pEnemyShotHead->prev = pNew;
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Levatain_DeepSeek()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLaevateinn;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 敵の移動は ShotLaevateinn 内でレーザー端に合わせて行う
}