// 二重振り子カオス弾幕パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 前方宣言（ファイル内 static 関数）
static void PatternAimed3Way(sEnemyShotSet* pSet);
static void PatternLaserPerpendicular(sEnemyShotSet* pSet);
static void PatternRadialBurst(sEnemyShotSet* pSet);
static void PatternAimedLaser(sEnemyShotSet* pSet);
static void PatternSeedFlower(sEnemyShotSet* pSet);
static void PatternChaosAimed5Way(sEnemyShotSet* pSet);

// ----------------------- 敵本体パターン -----------------------
void EnemyPat_DoublePendulum_DeepSeek()
{
    static int muki = 1;
    // 二重振り子の状態
    static double theta1, omega1, theta2, omega2;
    static const double L1 = 100.0, L2 = 80.0;
    static const double m1 = 1.0, m2 = 1.0;
    static const double g = 600.0;
    static const double dt = 0.02;
    static const double damp = 0.999;
    static int    phase = 0;
    static int    phaseStartCount = 0;
    static double energyBoost = 0.0;
    static double x1, y1, x2, y2;
    static double vx2, vy2;   // 第二錘の速度（レーザー用）

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 300;
        muki = 1;
        theta1 = 0.5;   // 約28.6°
        omega1 = 0.0;
        theta2 = 0.0;
        omega2 = 0.0;
        phase = 1;
        phaseStartCount = count;
        energyBoost = 0.0;
        x1 = 0;
        y1 = 0;
        x2 = 0;
        y2 = 0;
        vx2 = 0;
        vy2 = 0;
    }
    else {
        // ---------- 物理演算 ----------
        double dtheta = theta1 - theta2;
        double denom = 2.0 * m1 + m2 - m2 * cos(2.0 * dtheta);

        double alpha1 = (-g * (2.0 * m1 + m2) * sin(theta1)
            - m2 * g * sin(theta1 - 2.0 * theta2)
            - 2.0 * sin(dtheta) * m2 * (omega2 * omega2 * L2 + omega1 * omega1 * L1 * cos(dtheta)))
            / (L1 * denom);

        double alpha2 = (2.0 * sin(dtheta) * (omega1 * omega1 * L1 * (m1 + m2)
            + g * (m1 + m2) * cos(theta1)
            + omega2 * omega2 * L2 * m2 * cos(dtheta)))
            / (L2 * denom);

        omega1 += alpha1 * dt;
        omega2 += alpha2 * dt;

        // エネルギー注入（フェイズで強さが変わる）
        int pt = count - phaseStartCount;
        if (phase == 1) {
            energyBoost = 0.02 + pt * 0.0003;
            if (energyBoost > 0.3) energyBoost = 0.3;
        }
        else if (phase == 2) {
            energyBoost = 0.3 + (pt - 600) * 0.0006;
            if (energyBoost > 0.7) energyBoost = 0.7;
        }
        else if (phase == 3) {
            energyBoost = 0.9;
        }
        omega1 += energyBoost * 0.002;

        // 減衰
        omega1 *= damp;
        omega2 *= damp;

        // 角速度リミット（暴走防止）
        if (omega1 > 20.0) omega1 = 20.0;
        if (omega1 < -20.0) omega1 = -20.0;
        if (omega2 > 25.0) omega2 = 25.0;
        if (omega2 < -25.0) omega2 = -25.0;

        // 角度更新
        theta1 += omega1 * dt;
        theta2 += omega2 * dt;

        // 位置計算（y は下が正）
        x1 = enemy.x + L1 * sin(theta1);
        y1 = enemy.y + L1 * cos(theta1);
        x2 = x1 + L2 * sin(theta2);
        y2 = y1 + L2 * cos(theta2);

        // 第二錘の速度（弾の方向決定に使う）
        double vx1 = L1 * omega1 * cos(theta1);
        double vy1 = -L1 * omega1 * sin(theta1);
        vx2 = vx1 + L2 * omega2 * cos(theta2);
        vy2 = vy1 - L2 * omega2 * sin(theta2);

        // ボス移動（元サンプルと同様）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;

        // フェイズ遷移
        if (phase == 1 && pt >= 600) { phase = 2; phaseStartCount = count; }
        else if (phase == 2 && pt >= 480) { phase = 3; phaseStartCount = count; }
    }

    // ---------- ショットセット生成ラムダ ----------
    auto addShotSet = [&](void (*pattern)(sEnemyShotSet*), double x, double y,
        double muki = 0.0, int kind = 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;                     // メインルーチンが加算
        pSet->patternFunc = pattern;
        pSet->x = x;
        pSet->y = y;
        pSet->muki = muki;
        pSet->kind = kind;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        // 効果音（適宜）
        if (!CheckSoundMem(sound_enemyShot_medium))
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    };

    int pt = count - phaseStartCount;

    // ---------- フェイズ別弾幕発射 ----------
    if (phase == 1) {
        // 揺籃の刻：狙い3way低速弾 + 速度垂直レーザー
        if (count % 18 == 2) {
            double aim = atan2(player.y - y2, player.x - x2);
            addShotSet(PatternAimed3Way, x2, y2, aim, 0);
        }
        if (count % 3 == 2) {
            double vangle = atan2(vy2, vx2);
            addShotSet(PatternLaserPerpendicular, x2, y2, vangle, 1);
        }
    }
    else if (phase == 2) {
        // 加速する不穏：円形バースト + 第一錘狙いレーザー
        if (count % 24 == 2) {
            addShotSet(PatternRadialBurst, x2, y2, 0.0, 2);
        }
        if (count % 3 == 2) {
            double aim1 = atan2(player.y - y1, player.x - x1);
            addShotSet(PatternAimedLaser, x1, y1, aim1, 3);
        }
    }
    else if (phase == 3) {
        // 混沌の頂：遅延開花弾 + 高速5way狙い弾
        if (count % 9 == 2) {
            addShotSet(PatternSeedFlower, x2, y2, 0.0, 4);
        }
        if (count % 60 == 2) {
            double aim2 = atan2(player.y - y2, player.x - x2);
            addShotSet(PatternChaosAimed5Way, x2, y2, aim2, 5);
        }
    }
}

// ----------------------- ショットパターン群 -----------------------
static void PatternAimed3Way(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        double base = pSet->muki;
        for (int i = -1; i <= 1; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = base + i * (10.0 / 180.0 * DX_PI);
            p->speed = 1.5;
            p->kind = img_enemyShotSmallBall[0]; // 赤
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

static void PatternLaserPerpendicular(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        double ang = pSet->muki; // 速度の向き
        for (int sign = -1; sign <= 1; sign += 2) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = ang + sign * DX_PI / 2.0;
            p->speed = 0.0;
            p->kind = img_enemyShotLaser[4]; // 青
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next;
        if (p->count > 30) { // 寿命
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
        }
        p = pNext;
    }
}

static void PatternRadialBurst(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        for (int i = 0; i < 12; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = i * 2.0 * DX_PI / 12.0;
            p->speed = 2.0;
            p->kind = img_enemyShotMediumBall[1]; // 黄
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

static void PatternAimedLaser(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        sEnemyShot* p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = pSet->muki; // 自機方向
        p->speed = 0.0;
        p->kind = img_enemyShotLaser[0]; // 赤
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next;
        if (p->count > 30) {
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
        }
        p = pNext;
    }
}

static void PatternSeedFlower(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        sEnemyShot* pSeed = new sEnemyShot;
        pSeed->x = pSet->x;
        pSeed->y = pSet->y;
        pSeed->muki = 0.0;
        pSeed->speed = 0.0;
        pSeed->kind = img_enemyShotDiamond[6]; // 白
        pSeed->param_i[0] = 0; // 開花フラグ
        pSeed->prev = pSet->pEnemyShotHead->prev;
        pSeed->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pSeed;
        pSet->pEnemyShotHead->prev = pSeed;
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next;
        // 種判定（菱形弾なら種とみなす）
        if (p->speed == 0.0 && p->kind >= img_enemyShotDiamond[0] && p->kind <= img_enemyShotDiamond[7]) {
            if (p->count == 36 && p->param_i[0] == 0) {
                // 8方向開花
                for (int i = 0; i < 8; ++i) {
                    sEnemyShot* pBloom = new sEnemyShot;
                    pBloom->x = p->x;
                    pBloom->y = p->y;
                    pBloom->muki = i * 2.0 * DX_PI / 8.0;
                    pBloom->speed = 1.5;
                    pBloom->kind = img_enemyShotScale[2]; // 緑の鱗弾
                    pBloom->prev = pSet->pEnemyShotHead->prev;
                    pBloom->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pBloom;
                    pSet->pEnemyShotHead->prev = pBloom;
                }
                // 種を削除
                p->prev->next = p->next;
                p->next->prev = p->prev;
                delete p;
            }
        }
        else {
            // 通常弾は移動
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = pNext;
    }
}

static void PatternChaosAimed5Way(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        double base = pSet->muki;
        double halfSpread = 10.0 / 180.0 * DX_PI; // 全体で ±20°
        for (int i = -2; i <= 2; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = base + i * halfSpread;
            p->speed = 3.0;
            p->kind = img_enemyShotBullet[0]; // 赤い銃弾
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}