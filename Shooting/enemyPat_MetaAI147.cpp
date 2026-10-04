#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ========== 角度補助 ==========
static double NormalizeRad(double a) {
    while (a > DX_PI) a -= 2.0 * DX_PI;
    while (a < -DX_PI) a += 2.0 * DX_PI;
    return a;
}
static double DiffRad(double target, double cur) {
    double d = target - cur;
    while (d > DX_PI) d -= 2.0 * DX_PI;
    while (d < -DX_PI) d += 2.0 * DX_PI;
    return d;
}
static double LerpRad(double a, double b, double t) {
    return a + DiffRad(b, a) * t;
}

// ========== 叩きつけ時の衝撃飛沫 ==========
static void ShotImpact(sEnemyShotSet* pSet)
{
    if (pSet->count == 1) {
        for (int i = 0; i < 10*20; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = (DX_TWO_PI / 10.0/20) * i + GetRand(20) / 100.0;
            pShot->speed = 1.5-1 + GetRand(150+300) / 100.0;
            pShot->kind = img_enemyShotSmallBall[0]; // 赤小玉
            pShot->margin = 120.0;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ========== 三節棍本体 ==========
static void ShotSansetsukon(sEnemyShotSet* pSet)
{
    // param_d 割り当て
    // [0] baseAngle(棍A) [1] angleB [2] angleC [3] extend(鎖の伸び) [4] targetAngle
    // param_i 割り当て
    // [0] phase 0:振り回し 1:溜め 2:叩きつけ 3:回収 [1] dir [2] timer [3] hasSpawnedImpact

    const int STICK_A_NUM = 4;
    const int CHAIN1_NUM = 6;
    const int STICK_B_NUM = 4;
    const int CHAIN2_NUM = 6;
    const int STICK_C_NUM = 4;
    const int TOTAL = 24;

    const double SPACING_STICK = 12.0;
    const double SPACING_CHAIN_BASE = 8.0;

    // 初回生成
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pSet->param_d[0] = -DX_PI * 0.5; // baseAngle
        pSet->param_d[1] = -DX_PI * 0.5; // angleB
        pSet->param_d[2] = -DX_PI * 0.5; // angleC
        pSet->param_d[3] = 1.0; // extend
        pSet->param_d[4] = 0.0;
        pSet->param_i[0] = 0;
        pSet->param_i[1] = 1;
        pSet->param_i[2] = 0;
        pSet->param_i[3] = 0;

        for (int i = 0; i < TOTAL; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_i[0] = i; // 通し番号を記憶
            pShot->speed = 0.0;
            pShot->margin = 80.0; // 振り回しで画面外に出ても消えないように大きめ
            // 棍と鎖で素材を分ける
            if (i < STICK_A_NUM) {
                pShot->kind = img_enemyShotMediumOval[0]; // 赤 : 棍A
            }
            else if (i < STICK_A_NUM + CHAIN1_NUM) {
                pShot->kind = img_enemyShotSmallBall[6]; // 白 : 鎖1
            }
            else if (i < STICK_A_NUM + CHAIN1_NUM + STICK_B_NUM) {
                pShot->kind = img_enemyShotMediumOval[3]; // シアン : 棍B
            }
            else if (i < STICK_A_NUM + CHAIN1_NUM + STICK_B_NUM + CHAIN2_NUM) {
                pShot->kind = img_enemyShotSmallBall[6]; // 白 : 鎖2
            }
            else {
                pShot->kind = img_enemyShotMediumOval[1]; // 黄 : 棍C(先端)
            }
            pShot->margin = 480;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // ボスに追従
    pSet->x = enemy.x;
    pSet->y = enemy.y;

    // ---- フェーズ更新 ----
    int& phase = pSet->param_i[0];
    int& dir = pSet->param_i[1];
    int& timer = pSet->param_i[2];
    double& baseAng = pSet->param_d[0];
    double& angB = pSet->param_d[1];
    double& angC = pSet->param_d[2];
    double& extend = pSet->param_d[3];
    double& targetAng = pSet->param_d[4];

    timer++;

    switch (phase) {
    case 0: // 振り回し
        baseAng += 0.035 * (double)dir;
        angB += DiffRad(baseAng, angB) * 0.15;
        angC += DiffRad(angB, angC) * 0.15;
        if (timer > 100) {
            phase = 1;
            timer = 0;
            targetAng = atan2(player.y - pSet->y, player.x - pSet->x);
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        break;
    case 1: // 溜め 30F
        if (timer > 30) {
            phase = 2;
            timer = 0;
            pSet->param_i[3] = 0;
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        break;
    case 2: // 叩きつけ
        extend += 0.09;
        if (extend > 2.4) extend = 2.4;
        angC += DiffRad(targetAng, angC) * 0.28;
        angB += DiffRad(targetAng, angB) * 0.18;
        baseAng += DiffRad(targetAng, baseAng) * 0.06;

        // 先端が伸び切った瞬間に飛沫を出す
        if (timer == 15 && pSet->param_i[3] == 0) {
            pSet->param_i[3] = 1;
            double totalLen = STICK_A_NUM * SPACING_STICK
                + CHAIN1_NUM * SPACING_CHAIN_BASE * extend
                + STICK_B_NUM * SPACING_STICK
                + CHAIN2_NUM * SPACING_CHAIN_BASE * extend
                + STICK_C_NUM * SPACING_STICK;
            double tipX = pSet->x + cos(angC) * totalLen;
            double tipY = pSet->y + sin(angC) * totalLen;

            sEnemyShotSet* pImpact = new sEnemyShotSet;
            pImpact->count = 0;
            pImpact->patternFunc = ShotImpact;
            pImpact->x = tipX;
            pImpact->y = tipY;
            pImpact->muki = 0;
            pImpact->kind = 0;
            pImpact->pEnemyShotHead = new sEnemyShot;
            pImpact->pEnemyShotHead->prev = pImpact->pEnemyShotHead;
            pImpact->pEnemyShotHead->next = pImpact->pEnemyShotHead;
            pImpact->prev = enemyShotSetHead.prev;
            pImpact->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pImpact;
            enemyShotSetHead.prev = pImpact;
        }
        if (timer > 40) {
            phase = 3;
            timer = 0;
        }
        break;
    case 3: // 回収
        extend -= 0.09;
        if (extend < 1.0) extend = 1.0;
        angB += DiffRad(baseAng, angB) * 0.15;
        angC += DiffRad(angB, angC) * 0.15;
        if (timer > 40) {
            phase = 0;
            timer = 0;
            dir *= -1;
        }
        break;
    }

    // ---- 座標配置 ----
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int idx = pShot->param_i[0];
        double dist = 0.0;
        double ang = baseAng;

        if (idx < STICK_A_NUM) {
            dist = idx * SPACING_STICK + SPACING_STICK * 0.5;
            ang = baseAng;
        }
        else if (idx < STICK_A_NUM + CHAIN1_NUM) {
            int c = idx - STICK_A_NUM;
            double t = (double)c / (double)(CHAIN1_NUM - 1);
            dist = STICK_A_NUM * SPACING_STICK + c * SPACING_CHAIN_BASE * extend;
            ang = LerpRad(baseAng, angB, t);
        }
        else if (idx < STICK_A_NUM + CHAIN1_NUM + STICK_B_NUM) {
            int c = idx - (STICK_A_NUM + CHAIN1_NUM);
            dist = STICK_A_NUM * SPACING_STICK + CHAIN1_NUM * SPACING_CHAIN_BASE * extend + c * SPACING_STICK + SPACING_STICK * 0.5;
            ang = angB;
        }
        else if (idx < STICK_A_NUM + CHAIN1_NUM + STICK_B_NUM + CHAIN2_NUM) {
            int c = idx - (STICK_A_NUM + CHAIN1_NUM + STICK_B_NUM);
            double t = (double)c / (double)(CHAIN2_NUM - 1);
            dist = STICK_A_NUM * SPACING_STICK + CHAIN1_NUM * SPACING_CHAIN_BASE * extend + STICK_B_NUM * SPACING_STICK + c * SPACING_CHAIN_BASE * extend;
            ang = LerpRad(angB, angC, t);
        }
        else {
            int c = idx - (STICK_A_NUM + CHAIN1_NUM + STICK_B_NUM + CHAIN2_NUM);
            dist = STICK_A_NUM * SPACING_STICK + CHAIN1_NUM * SPACING_CHAIN_BASE * extend + STICK_B_NUM * SPACING_STICK + CHAIN2_NUM * SPACING_CHAIN_BASE * extend + c * SPACING_STICK + SPACING_STICK * 0.5;
            ang = angC;
        }

        pShot->x = pSet->x + cos(ang) * dist;
        pShot->y = pSet->y + sin(ang) * dist;
        pShot->muki = ang; // 中楕円弾の向きを棍に合わせる

        pShot = pShot->next;
    }
}

// ========== 敵本体 ==========
void EnemyPat_ThreeSectionStaff_MetaAI()
{
    static int move_dir;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        move_dir = 1;

        // 三節棍セットを1つ生成(永続)
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSansetsukon;
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
    else {
        // ボスは横にゆっくり往復するだけ
        enemy.x += 0.25 * (double)move_dir;
        if (enemy.x < 180.0 || enemy.x > 300.0) move_dir *= -1;
    }
}