#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const double PHI = 1.6180339887;
static const double GOLDEN_ANGLE = 2.3999632297; // 137.5deg in rad
static double s_base[12][3] = {
    {0,1,PHI},{0,-1,PHI},{0,1,-PHI},{0,-1,-PHI},
    {1,PHI,0},{-1,PHI,0},{1,-PHI,0},{-1,-PHI,0},
    {PHI,0,1},{-PHI,0,1},{PHI,0,-1},{-PHI,0,-1}
};
static double s_norm[12][3];
static bool s_init = false;
static void InitIcosa() {
    if (s_init) return;
    for (int i = 0; i < 12; i++) {
        double x = s_base[i][0], y = s_base[i][1], z = s_base[i][2];
        double l = sqrt(x * x + y * y + z * z);
        s_norm[i][0] = x / l; s_norm[i][1] = y / l; s_norm[i][2] = z / l;
    }
    s_init = true;
}
static void Proj(int idx, double rotY, double rad, double& ox, double& oy, double& ang) {
    InitIcosa();
    double x = s_norm[idx][0], y = s_norm[idx][1], z = s_norm[idx][2];
    double c = cos(rotY), s = sin(rotY);
    double xr = x * c - z * s;
    double yr = y;
    ox = xr * rad;
    oy = yr * rad * 0.85;
    ang = atan2(oy, ox);
}
static int LoopPhase() { return (count - 1) % 480; } // 8秒ループ
static double TargetR_TAS(int ph) {
    if (ph < 30) return 60.0 + ph * 4.0; // 60->180
    if (ph < 180) return 180.0;
    if (ph < 360) return 180.0 - (ph - 180) * (102.0 / 180.0); // 180->78 TAS最終檻
    return 78.0; // 1px隙間
}

// A: TAS結界 大玉で1px隙間
static void ShotBarrier_TAS(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 12; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotLargeBall[4]; // 青大玉 20x20
            p->param_i[0] = i;
            p->x = pSet->x; p->y = pSet->y;
            p->muki = 0; p->speed = 0;
            p->margin = 240;
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    pSet->x = enemy.x; pSet->y = enemy.y + 10.0;
    int ph = LoopPhase();
    double R = TargetR_TAS(ph);
    double rot = pSet->count * 0.025; // 高速回転 TASは毎フレーム円運動必須
    pSet->param_d[0] = R; pSet->param_d[1] = rot;
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        int idx = p->param_i[0];
        double px, py, ang;
        Proj(idx, rot, R, px, py, ang);
        p->x = pSet->x + px;
        p->y = pSet->y + py;
        p = p->next;
    }
}

// B: TAS螺旋 逆回転2層 重ねて潰す
static void ShotSpiral_TAS(sEnemyShotSet* pSet) {
    pSet->x = enemy.x; pSet->y = enemy.y + 10.0;
    int ph = LoopPhase();
    if (ph >= 390) return; // 崩壊中は停止
    double R = TargetR_TAS(ph);
    double rot = pSet->count * 0.025;
    double dir = (ph < 180) ? 1.0 : -2.3; // 後半逆転高速

    if (pSet->count % 3 == 0) {
        for (int i = 0; i < 12; i++) {
            double vx, vy, va;
            Proj(i, rot, R, vx, vy, va);
            double sx = pSet->x + vx;
            double sy = pSet->y + vy;
            // 1頂点から逆回転2発
            for (int k = 0; k < 2; k++) {
                sEnemyShot* p = new sEnemyShot;
                double golden = (pSet->param_i[0] % 7) * GOLDEN_ANGLE;
                double base = va + golden + pSet->count * 0.018 * dir;
                if (k == 1) base = va - golden - pSet->count * 0.018 * dir;
                p->x = sx; p->y = sy;
                p->muki = base;
                p->speed = 3.8 + sin(pSet->count * 0.07 + i) * 1.8 + k * 0.4; // 自機速度に張り付く
                p->kind = (k == 0) ? img_enemyShotSmallBall[6] : img_enemyShotMediumOval[6];
                p->margin = 240;
                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
        pSet->param_i[0]++;
        if (pSet->count % 45 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        // 微加速で逃げ道を潰す
        p->speed += 0.008;
        p = p->next;
    }
}

// C: TAS刈り取り 自機狙い+1ドットずらし
static void ShotNeedle_TAS(sEnemyShotSet* pSet) {
    pSet->x = enemy.x; pSet->y = enemy.y + 10.0;
    int ph = LoopPhase();
    if (ph < 180) return;
    double rot = pSet->count * 0.025;

    if (pSet->count % 5 == 0) {
        // プレイヤー未来位置予測狙い TASは1F先読み回避必須
        double aim = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int i = 0; i < 12; i++) {
            double vx, vy, va;
            Proj(i, rot, 1.0, vx, vy, va);
            double gap = va + DX_PI / 12.0; // 隙間角度
            // 隙間と自機狙いの中間を高速で塞ぐ
            double muki = aim * 0.7 + gap * 0.3;
            muki += (GetRand(20) - 10) / 1000.0; // 再現性ある極小乱数 TASは暗記必須
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x; p->y = pSet->y;
            p->muki = muki + (i * DX_PI / 6.0) * 0.05;
            p->speed = 13.5 + GetRand(20) / 10.0; // 13.5-15.5
            p->kind = img_enemyShotBullet[0]; // 赤銃弾
            p->margin = 240;
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// D: TAS崩壊 レーザー壁で1px回転通路
static void ShotCollapse_TAS(sEnemyShotSet* pSet) {
    pSet->x = enemy.x; pSet->y = enemy.y + 10.0;
    int ph = LoopPhase();
    if (ph < 360) return;
    double rot = pSet->count * 0.025;

    if (pSet->count % 6 == 0) {
        for (int i = 0; i < 12; i++) {
            double ang = rot + i * DX_PI / 6.0;
            double sx = pSet->x + cos(ang) * 340.0;
            double sy = pSet->y + sin(ang) * 340.0;
            double toC = atan2(pSet->y - sy, pSet->x - sx);
            // 双対変換の1F隙間: 偶奇で0.08radずらす
            double muki = toC + (i % 2 == 0 ? 0.08 : -0.08);
            sEnemyShot* p = new sEnemyShot;
            p->x = sx; p->y = sy;
            p->muki = muki;
            p->speed = 5.8;
            p->kind = img_enemyShotLaser[6]; // 白レーザー 64x4 壁
            p->margin = 240;
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
        if (pSet->count % 24 == 0) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

void EnemyPat_GeometryTAS_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0; enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        auto NewSet = [](auto func) {
            sEnemyShotSet* s = new sEnemyShotSet;
            s->count = 0; s->patternFunc = func;
            s->x = enemy.x; s->y = enemy.y + 10.0; s->muki = 0; s->kind = 0;
            s->alive = 9999;
            s->pEnemyShotHead = new sEnemyShot;
            s->pEnemyShotHead->prev = s->pEnemyShotHead;
            s->pEnemyShotHead->next = s->pEnemyShotHead;
            s->prev = enemyShotSetHead.prev;
            s->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = s;
            enemyShotSetHead.prev = s;
            return s;
        };
        NewSet(ShotBarrier_TAS);
        NewSet(ShotSpiral_TAS);
        NewSet(ShotNeedle_TAS);
        NewSet(ShotCollapse_TAS);
    }
    else {
        // TASはこの微揺れすら計算に入れる必要がある
        enemy.x = 240.0 + sin(count * 0.015) * 35.0;
        enemy.y = 80.0 + cos(count * 0.011) * 12.0;
    }
}