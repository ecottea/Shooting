#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾種別フラグ (sEnemyShot.param_i[0]に保存)
#define TYPE_CORE  0
#define TYPE_LINE  1
#define TYPE_ORBIT 2
#define TYPE_ATTACK 3

static void ShotHokuto(sEnemyShotSet* pSet)
{
    // 北斗七星のローカル座標 - 画面中央を0,0とした時の形
    static const double baseX[7] = { -100.0, -90.0, -20.0, -10.0,  50.0, 110.0, 170.0 };
    static const double baseY[7] = { -40.0,  30.0,  40.0, -20.0, -10.0,  20.0,  60.0 };
    // 星座線の接続 0-1-2-3-0が器、3-4-5-6が柄
    static const int edgeFrom[7] = { 0, 1, 2, 3, 3, 4, 5 };
    static const int edgeTo[7] = { 1, 2, 3, 0, 4, 5, 6 };

    auto NewShot = [&](int imgKind) -> sEnemyShot* {
        sEnemyShot* p = new sEnemyShot;
        p->count = 0;
        p->margin = 20.0;
        p->kind = imgKind;
        p->speed = 0.0;
        p->muki = 0.0;
        for (int i = 0; i < 16; i++) { p->param_i[i] = 0; p->param_d[i] = 0.0; }
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
        return p;
    };

    // --- 初回生成 ---
    if (pSet->count == 0) {
        pSet->param_i[0] = 0; // state 0:顕現 1:点呼 2:破砕
        pSet->param_i[1] = 6; // 次に攻撃する星 (6から0へ)
        pSet->param_i[2] = 0; // state内タイマー
        pSet->param_d[0] = -0.3; // 回転角
        pSet->x = enemy.x;
        pSet->y = enemy.y + 60.0;

        if (CheckSoundMem(sound_enemyCharge) == 0) PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 7つの核
        for (int i = 0; i < 7; i++) {
            sEnemyShot* p = NewShot(img_enemyShotLargeBall[6]); // 白大玉
            p->param_i[0] = TYPE_CORE;
            p->param_i[1] = i;
            p->x = pSet->x + baseX[i];
            p->y = pSet->y + baseY[i];
        }
        // 星座線
        const int lineDiv = 12*2;
        for (int e = 0; e < 7; e++) {
            for (int k = 0; k < lineDiv; k++) {
                double t = (double)(k + 1) / (double)(lineDiv + 1); // 0-1
                sEnemyShot* p = NewShot(img_enemyShotSmallBall[4]); // 青小玉
                p->param_i[0] = TYPE_LINE;
                p->param_i[1] = edgeFrom[e];
                p->param_i[2] = edgeTo[e];
                p->param_d[0] = t;
                // 初期位置
                double bx = baseX[edgeFrom[e]] * (1.0 - t) + baseX[edgeTo[e]] * t;
                double by = baseY[edgeFrom[e]] * (1.0 - t) + baseY[edgeTo[e]] * t;
                p->x = pSet->x + bx;
                p->y = pSet->y + by;
            }
        }
        // 煌めき (各星に3個)
        for (int i = 0; i < 7; i++) {
            for (int j = 0; j < 3; j++) {
                sEnemyShot* p = NewShot(img_enemyShotSmallBall[6]); // 白小玉
                p->param_i[0] = TYPE_ORBIT;
                p->param_i[1] = i; // 親星
                p->param_d[0] = (DX_TWO_PI / 3.0) * j; // 初期角度
                p->param_d[1] = 18.0; // 半径
                p->param_d[2] = 0.12 + j * 0.03; // 角速度
            }
        }
    }

    // 敵に追従
    pSet->x += (enemy.x - pSet->x) * 0.06;
    pSet->y += ((enemy.y + 70.0) - pSet->y) * 0.06;

    int state = pSet->param_i[0];
    int nextStar = pSet->param_i[1];
    int tState = pSet->param_i[2];

    // --- ステートマシン ---
    if (state == 0) { // 顕現 120F
        pSet->param_d[0] += 0.010;
        tState++;
        if (tState > 120) { state = 1; tState = 0; nextStar = 6; }
    }
    else if (state == 1) { // 点呼攻撃
        pSet->param_d[0] += 0.004;
        tState++;
        if (tState % 50 == 1 && nextStar >= 0) {
            double rot = pSet->param_d[0];
            double cr = cos(rot), sr = sin(rot);
            double wx = pSet->x + baseX[nextStar] * cr - baseY[nextStar] * sr;
            double wy = pSet->y + baseX[nextStar] * sr + baseY[nextStar] * cr;
            double aim = atan2(player.y - wy, player.x - wx);

            if (nextStar >= 4) { // ⑤⑥⑦ 揺らぎ 3WAY中玉
                if (CheckSoundMem(sound_enemyShot_medium) == 0) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                for (int d = -1-1; d <= 1+1; d++) {
                    sEnemyShot* p = NewShot(img_enemyShotMediumBall[1]); // 黄中玉
                    p->param_i[0] = TYPE_ATTACK;
                    p->x = wx; p->y = wy;
                    p->muki = aim + d * 0.35;
                    p->speed = 2.2 + GetRand(40) / 100.0;
                }
            }
            else if (nextStar >= 2) { // ③④ 溜め リング
                if (CheckSoundMem(sound_enemyShot_light) == 0) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                for (int k = 0; k < 18*2; k++) {
                    double baseAng = DX_TWO_PI * k / 18.0/2;
                    for (int layer = 0; layer < 2; layer++) {
                        sEnemyShot* p = NewShot(layer == 0 ? img_enemyShotScale[4] : img_enemyShotScale[3]);
                        p->param_i[0] = TYPE_ATTACK;
                        p->x = wx; p->y = wy;
                        p->muki = baseAng + (GetRand(10) - 5) / 180.0 * DX_PI;
                        p->speed = layer == 0 ? 1.7 : 3.1;
                    }
                }
            }
            else { // ①② 破軍 高速中楕円
                if (CheckSoundMem(sound_enemyShot_heavy) == 0) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                for (int k = 0; k < 5*2; k++) {
                    sEnemyShot* p = NewShot(img_enemyShotMediumOval[6]); // 白中楕円
                    p->param_i[0] = TYPE_ATTACK;
                    p->x = wx; p->y = wy;
                    p->muki = aim + (GetRand(20) - 10) / 360.0 * DX_PI*2;
                    p->speed = 4.0 + k * 0.5;
                }
            }
            nextStar--;
        }
        if (nextStar < 0 && tState > 400) { state = 2; tState = 0; }
    }
    else if (state == 2) { // 北辰落ち
        tState++;
        if (tState == 1) {
            if (CheckSoundMem(sound_enemyShot_extreme) == 0) PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
            for (int k = 0; k < 32; k++) {
                sEnemyShot* p = NewShot(img_enemyShotMediumBall[6]);
                p->param_i[0] = TYPE_ATTACK;
                p->x = pSet->x; p->y = pSet->y;
                p->muki = DX_TWO_PI * k / 32.0;
                p->speed = 2.8 + GetRand(80) / 100.0;
            }
        }
        if (tState == 35) {
            // 残った星座構成パーツを飛散させる
            sEnemyShot* p = pSet->pEnemyShotHead->next;
            while (p != pSet->pEnemyShotHead) {
                if (p->param_i[0] != TYPE_ATTACK) {
                    p->param_i[0] = TYPE_ATTACK;
                    p->muki = atan2(p->y - pSet->y, p->x - pSet->x) + (GetRand(20) - 10) / 180.0 * DX_PI * 0.3;
                    p->speed = 1.2 + GetRand(150) / 100.0;
                }
                p = p->next;
            }
        }
    }

    pSet->param_i[0] = state;
    pSet->param_i[1] = nextStar;
    pSet->param_i[2] = tState;

    // --- 位置更新 ---
    double coreWX[7], coreWY[7];
    double rot = pSet->param_d[0];
    double cr = cos(rot), sr = sin(rot);
    for (int i = 0; i < 7; i++) {
        coreWX[i] = pSet->x + baseX[i] * cr - baseY[i] * sr;
        coreWY[i] = pSet->y + baseX[i] * sr + baseY[i] * cr;
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        int tp = pShot->param_i[0];
        if (tp == TYPE_CORE) {
            int idx = pShot->param_i[1];
            if (idx >= 0 && idx < 7 && !(state == 2 && tState >= 35)) {
                pShot->x = coreWX[idx];
                pShot->y = coreWY[idx];
            }
            else {
                // 飛散後は通常移動
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (tp == TYPE_LINE) {
            if (state == 2 && tState >= 35) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                int fr = pShot->param_i[1];
                int to = pShot->param_i[2];
                double t = pShot->param_d[0];
                double bx = baseX[fr] * (1.0 - t) + baseX[to] * t;
                double by = baseY[fr] * (1.0 - t) + baseY[to] * t;
                pShot->x = pSet->x + bx * cr - by * sr;
                pShot->y = pSet->y + bx * sr + by * cr;
            }
        }
        else if (tp == TYPE_ORBIT) {
            if (state == 2 && tState >= 35) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                int parent = pShot->param_i[1];
                double ang = pShot->param_d[0] + pSet->count * pShot->param_d[2];
                double rad = pShot->param_d[1];
                if (parent >= 0 && parent < 7) {
                    pShot->x = coreWX[parent] + cos(ang) * rad;
                    pShot->y = coreWY[parent] + sin(ang) * rad;
                }
            }
        }
        else { // TYPE_ATTACK
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pNext;
    }
}

void EnemyPat_BigDipper_MetaAI()
{
    static int dir;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        dir = 1;
    }
    else {
        enemy.x += 0.35 * (double)dir;
        if (count % 360 == 180) dir *= -1;
    }

    // 既存の北斗が生きているかチェック
    bool hasHokuto = false;
    for (sEnemyShotSet* p = enemyShotSetHead.next; p != &enemyShotSetHead; p = p->next) {
        if (p->patternFunc == ShotHokuto && p->count < 550) { hasHokuto = true; break; }
    }

    if (!hasHokuto && count % 550 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHokuto;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 60.0;
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