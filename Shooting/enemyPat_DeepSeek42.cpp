// enemyPat_Pyramid.cpp
// ピラミッドをモチーフにした弾幕パターン
// 関数名: void EnemyPat_Pyramid_DeepSeek()

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// バースト（崩壊）パターン
// ============================================================
static void BurstExplosion(sEnemyShotSet* pB)
{
    if (pB->count == 1) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        const int num = 36;
        for (int i = 0; i < num; ++i) {
            sEnemyShot* p = new sEnemyShot;
            double angle = (2.0 * DX_PI) * i / num + (GetRand(30) - 15) * DX_PI / 180.0;
            p->x = pB->x + 50 * cos(angle);
            p->y = pB->y + 50 * sin(angle);
            p->muki = angle;
            p->speed = (200.0 + GetRand(100)) / 100.0; // 2.0～3.0
            p->kind = img_enemyShotDiamond[1];         // 黄色菱形（ピラミッド破片）
            p->param_i[0] = 0;
            p->prev = pB->pEnemyShotHead->prev;
            p->next = pB->pEnemyShotHead;
            pB->pEnemyShotHead->prev->next = p;
            pB->pEnemyShotHead->prev = p;
        }
    }

    // バースト弾の移動
    sEnemyShot* p = pB->pEnemyShotHead->next;
    while (p != pB->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
// ピラミッド全体を管理するマスターパターン
// ============================================================
static void PyramidMaster(sEnemyShotSet* pSet)
{
    const double APEX_X = 240.0;
    const double APEX_Y = 20.0;
    const double LEFT_BASE_X = 20.0;
    const double RIGHT_BASE_X = 460.0;
    const double BASE_Y = 465.0;
    const double CENTER_X = 240.0;
    const double CENTER_Y = (APEX_Y + BASE_Y + BASE_Y) / 3.0; // 300.0
    const double WALL_SPACING = 10.0;
    const double BOTTOM_RISE_SPEED = 1.5;
    const double EYE_SPEED = 1.2;
    const double SIDE_SHOT_SPEED = 2.5;

    sEnemyShot* pShot;
    sEnemyShot* next;

    // 初期化（count == 0）
    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 左側面の弾
        double dx = APEX_X - LEFT_BASE_X;
        double dy = APEX_Y - BASE_Y;
        double len = sqrt(dx * dx + dy * dy);
        int nLeft = (int)(len / WALL_SPACING) + 1;
        for (int i = 0; i < nLeft; ++i) {
            double t = (double)i / (nLeft - 1);
            double x = APEX_X + t * (LEFT_BASE_X - APEX_X);
            double y = APEX_Y + t * (BASE_Y - APEX_Y);
            pShot = new sEnemyShot;
            pShot->x = x;
            pShot->y = y;
            pShot->muki = 0.0;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotSmallBall[1];   // 黄色小玉
            pShot->param_i[0] = 0;                     // 壁弾
            pShot->param_d[0] = x;                     // 元の座標（回転の中心計算用）
            pShot->param_d[1] = y;
            pShot->margin = 999;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // 右側面の弾
        dx = RIGHT_BASE_X - APEX_X;
        dy = BASE_Y - APEX_Y;
        len = sqrt(dx * dx + dy * dy);
        int nRight = (int)(len / WALL_SPACING) + 1;
        for (int i = 0; i < nRight; ++i) {
            double t = (double)i / (nRight - 1);
            double x = APEX_X + t * (RIGHT_BASE_X - APEX_X);
            double y = APEX_Y + t * (BASE_Y - APEX_Y);
            pShot = new sEnemyShot;
            pShot->x = x;
            pShot->y = y;
            pShot->muki = 0.0;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotSmallBall[1];
            pShot->param_i[0] = 0;
            pShot->param_d[0] = x;
            pShot->param_d[1] = y;
            pShot->margin = 999;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        pSet->param_i[0] = 0;   // phase 0
        pSet->param_d[0] = 0.0; // rotation
        pSet->param_d[1] = 1.0; // scale
        pSet->param_d[2] = 0.015;   // delta rotate
        pSet->param_d[3] = -0.0012; // delta scale
    }

    int& phase = pSet->param_i[0];
    double& rot = pSet->param_d[0];
    double& scl = pSet->param_d[1];
    const double drot = pSet->param_d[2];
    const double dscl = pSet->param_d[3];

    // フェーズ移行判定
    if (phase == 0 && pSet->count >= 60) {
        // 底辺生成
        phase = 1;
        int nBottom = (int)((RIGHT_BASE_X - LEFT_BASE_X) / WALL_SPACING) + 1;
        for (int i = 0; i < nBottom; ++i) {
            double x = LEFT_BASE_X + (RIGHT_BASE_X - LEFT_BASE_X) * (double)i / (nBottom - 1);
            pShot = new sEnemyShot;
            pShot->x = x;
            pShot->y = 500.0;
            pShot->muki = -DX_PI / 2.0;
            pShot->speed = BOTTOM_RISE_SPEED;
            pShot->kind = img_enemyShotSmallBall[1];
            pShot->param_i[0] = 1;         // 上昇中の底辺
            pShot->param_d[0] = x;         // 目標X
            pShot->param_d[1] = BASE_Y;    // 目標Y
            pShot->margin = 999;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    if (phase == 1 && pSet->count >= 180) {
        phase = 2;
        rot = 0.0;
        scl = 1.0;

        // ホルスの目を生成
        pShot = new sEnemyShot;
        pShot->x = APEX_X;
        pShot->y = APEX_Y;
        pShot->muki = atan2(player.y - APEX_Y, player.x - APEX_X);
        pShot->speed = EYE_SPEED;
        pShot->kind = img_enemyShotMediumOval[1]; // 黄色楕円弾
        pShot->param_i[0] = 2; // 目弾
        pShot->margin = 100;
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 崩壊条件
    if (phase == 2 && scl <= 0.3) {
        phase = 3;
    }

    // ---- 毎フレームの弾処理 ----
    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        next = pShot->next;
        int type = pShot->param_i[0];

        if (type == 0) {
            // 壁弾：回転・収縮中は座標を更新
            if (phase >= 2) {
                double ox = pShot->param_d[0];
                double oy = pShot->param_d[1];
                double dx = ox - CENTER_X;
                double dy = oy - CENTER_Y;
                double ca = cos(rot);
                double sa = sin(rot);
                pShot->x = CENTER_X + (dx * ca - dy * sa) * scl;
                pShot->y = CENTER_Y + (dx * sa + dy * ca) * scl;
            }
        }
        else if (type == 1) {
            // 上昇中の底辺
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->y <= BASE_Y) {
                pShot->y = BASE_Y;
                pShot->speed = 0.0;
                pShot->muki = 0.0;
                pShot->param_i[0] = 0; // 壁扱いに
                pShot->param_d[0] = pShot->x;
                pShot->param_d[1] = BASE_Y;
            }
        }
        else if (type == 2) {
            // ホルスの目：追尾
            double targetAngle = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = targetAngle - pShot->muki;
            while (diff > DX_PI) diff -= 2 * DX_PI;
            while (diff < -DX_PI) diff += 2 * DX_PI;
            pShot->muki += diff * 0.02;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (type == 3) {
            // 内側への剣弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = next;
    }

    // 回転・収縮の更新
    if (phase >= 2) {
        rot += drot;
        scl += dscl;
        if (scl < 0.3) scl = 0.3;
    }

    // 内部攻撃（側面からの弾）
    if (phase == 2 && pSet->count % 40 == 0) {
        auto transform = [&](double ox, double oy) -> std::pair<double, double> {
            double dx = ox - CENTER_X;
            double dy = oy - CENTER_Y;
            double ca = cos(rot);
            double sa = sin(rot);
            return {
                CENTER_X + (dx * ca - dy * sa) * scl,
                CENTER_Y + (dx * sa + dy * ca) * scl
            };
        };
        auto [ax, ay] = transform(APEX_X, APEX_Y);
        auto [lx, ly] = transform(LEFT_BASE_X, BASE_Y);
        auto [rx, ry] = transform(RIGHT_BASE_X, BASE_Y);

        double pts[3][2] = { {ax, ay}, {lx, ly}, {rx, ry} };
        for (int i = 0; i < 3; ++i) {
            pShot = new sEnemyShot;
            pShot->x = pts[i][0];
            pShot->y = pts[i][1];
            pShot->muki = atan2(CENTER_Y - pShot->y, CENTER_X - pShot->x);
            pShot->speed = SIDE_SHOT_SPEED;
            pShot->kind = img_enemyShotSmallBall[1];
            pShot->param_i[0] = 3;
            pShot->margin = 100;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // バースト処理（phase 3 に入った最初のフレームだけ実行）
    if (phase == 3) {
        phase = 4; // 二重実行防止

        // 壁弾をすべて削除
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            next = pShot->next;
            delete pShot;
            pShot = next;
        }
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 崩壊エフェクト用の弾幕セットを生成
        sEnemyShotSet* pBurst = new sEnemyShotSet;
        pBurst->count = 0;
        pBurst->patternFunc = BurstExplosion; // ★静的関数を直接設定
        pBurst->x = CENTER_X;
        pBurst->y = CENTER_Y;
        pBurst->muki = 0.0;
        pBurst->kind = 0;

        pBurst->pEnemyShotHead = new sEnemyShot;
        pBurst->pEnemyShotHead->prev = pBurst->pEnemyShotHead;
        pBurst->pEnemyShotHead->next = pBurst->pEnemyShotHead;

        // リストに追加
        pBurst->prev = enemyShotSetHead.prev;
        pBurst->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBurst;
        enemyShotSetHead.prev = pBurst;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_Pyramid_DeepSeek()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.5 * muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (count % 700 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = PyramidMaster;
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