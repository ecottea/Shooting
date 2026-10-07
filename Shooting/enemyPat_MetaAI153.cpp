#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <vector>

#define COLOR_RED 0
#define COLOR_YELLOW 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_BLUE 4
#define COLOR_MAGENTA 5
#define COLOR_WHITE 6

// ============================================================
// 三重点 -Triple Point-
// 0-179F: 固体(結晶格子) 180-419F: 液体(融解落下) 420F-: 気体(蒸発拡散)
// ============================================================
static void ShotTriplePoint(sEnemyShotSet* pSet)
{
    // 敵に追従(固体フェーズのみ)
    if (pSet->count < 180) {
        pSet->x += (enemy.x - pSet->x) * 0.05;
        pSet->y += ((enemy.y + 90.0) - pSet->y) * 0.05;
    }

    // --------------------------------------------------------
    // 0F: 結晶格子生成
    // --------------------------------------------------------
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        struct Node { double r, ang; };
        std::vector<Node> nodes;

        // 中心
        nodes.push_back({ 0.0, 0.0 });
        // 内周 6個 R=50
        for (int i = 0; i < 6; i++) {
            nodes.push_back({ 50.0, i * DX_PI / 3.0 });
        }
        // 外周 12個 R=95
        for (int i = 0; i < 12; i++) {
            nodes.push_back({ 95.0, i * DX_PI / 6.0 });
        }

        // 大玉ノード生成 param_i[0]=0:ノード
        for (auto& nd : nodes) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotLargeBall[COLOR_WHITE];
            p->param_i[0] = 0; // type node
            p->param_i[1] = 0; // 未落下
            p->param_d[0] = nd.r;
            p->param_d[1] = nd.ang;
            p->speed = 0.0;
            p->x = pSet->x + nd.r * cos(nd.ang);
            p->y = pSet->y + nd.r * sin(nd.ang);

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }

        // 結合(レーザー)生成 param_i[0]=1:ボンド
        auto addBond = [&](int idxA, int idxB) {
            if (idxA >= (int)nodes.size() || idxB >= (int)nodes.size()) return;
            double x1 = nodes[idxA].r * cos(nodes[idxA].ang);
            double y1 = nodes[idxA].r * sin(nodes[idxA].ang);
            double x2 = nodes[idxB].r * cos(nodes[idxB].ang);
            double y2 = nodes[idxB].r * sin(nodes[idxB].ang);
            double mx = (x1 + x2) * 0.5;
            double my = (y1 + y2) * 0.5;
            double rmid = sqrt(mx * mx + my * my);
            double angMid = atan2(my, mx);
            double bondAng = atan2(y2 - y1, x2 - x1);

            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotLaser[COLOR_CYAN]; // 結合線に見える
            p->param_i[0] = 1;
            p->param_d[0] = rmid;
            p->param_d[1] = angMid;
            p->param_d[2] = bondAng; // 初期向き
            p->muki = bondAng;
            p->x = pSet->x + mx;
            p->y = pSet->y + my;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        };

        // 中心-内周
        for (int i = 1; i <= 6; i++) addBond(0, i);
        // 内周リング
        for (int i = 1; i <= 6; i++) addBond(i, 1 + (i % 6));
        // 内周-外周 (各内周に外周2個対応)
        for (int i = 0; i < 6; i++) {
            int inner = 1 + i;
            int outer1 = 7 + i * 2;
            int outer2 = 7 + i * 2 + 1;
            addBond(inner, outer1);
            addBond(inner, outer2);
        }
        // 外周リング
        for (int i = 0; i < 12; i++) addBond(7 + i, 7 + ((i + 1) % 12));
    }

    // --------------------------------------------------------
    // 180F: 融解 - 結合を切断
    // --------------------------------------------------------
    if (pSet->count == 180) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 1) { // ボンドは画面外へ
                p->x = 10000.0;
            }
            p = p->next;
        }
    }

    // --------------------------------------------------------
    // 180-419F: 液体ドロップ生成
    // --------------------------------------------------------
    if (pSet->count >= 180 && pSet->count < 420 && pSet->count % 7 == 0) {
        for (int k = 0; k < 2; k++) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotMediumBall[COLOR_CYAN + GetRand(1)]; // シアン/青で水っぽく
            p->param_i[0] = 2; // 液体
            p->x = pSet->x + GetRand(120) - 60;
            p->y = pSet->y + GetRand(30) - 15;
            p->param_d[3] = (GetRand(60) - 30) / 100.0; // vx
            p->param_d[4] = 0.8 + GetRand(80) / 100.0; // vy
            p->param_d[5] = GetRand(628) / 100.0; // sin位相
            p->speed = 0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // --------------------------------------------------------
    // 420F: 蒸発 - 大玉が弾けて気体に
    // --------------------------------------------------------
    if (pSet->count == 420) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        std::vector<std::pair<double, double>> largePos;
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) {
                largePos.push_back({ p->x, p->y });
            }
            p = p->next;
        }
        // 大玉を消去
        p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) p->x = 10000.0;
            p = p->next;
        }
        // 小玉8方向に拡散
        for (auto& pos : largePos) {
            for (int i = 0; i < 8; i++) {
                sEnemyShot* pn = new sEnemyShot;
                pn->kind = img_enemyShotSmallBall[COLOR_WHITE];
                pn->param_i[0] = 3; // 気体(小玉)
                pn->param_i[2] = 0; // 反射回数
                pn->x = pos.first;
                pn->y = pos.second;
                pn->muki = i * DX_PI / 4.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
                pn->speed = 2.2 + GetRand(100) / 100.0 * 1.5;
                pn->prev = pSet->pEnemyShotHead->prev;
                pn->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pn;
                pSet->pEnemyShotHead->prev = pn;
            }
        }
    }

    // --------------------------------------------------------
    // 420F-: 気体 高速ランダム弾
    // --------------------------------------------------------
    if (pSet->count >= 420 && pSet->count % 3 == 0 && pSet->count < 600) {
        sEnemyShot* pn = new sEnemyShot;
        pn->kind = img_enemyShotBullet[COLOR_WHITE]; // 針状で分子の高速運動
        pn->param_i[0] = 4; // 気体(銃弾)
        pn->param_i[2] = 0;
        pn->x = pSet->x + GetRand(80) - 40;
        pn->y = pSet->y + GetRand(40) - 20;
        pn->muki = GetRand(360) / 180.0 * DX_PI;
        pn->speed = 3.2 + GetRand(150) / 100.0;
        pn->prev = pSet->pEnemyShotHead->prev;
        pn->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pn;
        pSet->pEnemyShotHead->prev = pn;
    }

    // --------------------------------------------------------
    // 毎フレーム移動
    // --------------------------------------------------------
    double rot = pSet->count * 0.0035; // 固体時の剛体回転
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int type = pShot->param_i[0];
        if (type == 0) { // 大玉ノード
            if (pSet->count < 180) {
                double r = pShot->param_d[0];
                double base = pShot->param_d[1];
                pShot->x = pSet->x + r * cos(base + rot);
                pShot->y = pSet->y + r * sin(base + rot);
            }
            else if (pSet->count < 420) {
                if (pShot->param_i[1] == 0) {
                    pShot->param_d[3] = (GetRand(80) - 40) / 100.0;
                    pShot->param_d[4] = 0.3 + GetRand(90) / 100.0;
                    pShot->param_d[5] = GetRand(628) / 100.0;
                    pShot->param_i[1] = 1;
                }
                pShot->x += pShot->param_d[3] + sin(pSet->count * 0.08 + pShot->param_d[5]) * 0.25;
                pShot->y += pShot->param_d[4];
            }
            else {
                pShot->x += pShot->param_d[3];
                pShot->y += pShot->param_d[4];
            }
        }
        else if (type == 1) { // 結合レーザー
            if (pSet->count < 180) {
                double r = pShot->param_d[0];
                double base = pShot->param_d[1];
                pShot->x = pSet->x + r * cos(base + rot);
                pShot->y = pSet->y + r * sin(base + rot);
                pShot->muki = pShot->param_d[2] + rot;
            }
        }
        else if (type == 2) { // 液体 中玉
            pShot->x += pShot->param_d[3] + sin(pSet->count * 0.12 + pShot->param_d[5]) * 0.6;
            pShot->y += pShot->param_d[4];
        }
        else if (type == 3 || type == 4) { // 気体
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            // 1回だけ反射で衝突表現
            if (pShot->param_i[2] == 0) {
                if (pShot->x < 6.0-6 || pShot->x > 474.0+6) {
                    pShot->muki = DX_PI - pShot->muki;
                    pShot->param_i[2] = 1;
                }
                if (pShot->y < 6.0-6 || pShot->y > 474.0+999) {
                    pShot->muki = -pShot->muki;
                    pShot->param_i[2] = 1;
                }
            }
        }
        pShot = pShot->next;
    }
}

// 敵本体
void EnemyPat_PhaseTransition_MetaAI()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 三重点の間は敵はほぼ中央で微振動
        enemy.x += 0.35 * (double)muki;
        if (count % 180 == 0) muki *= -1;
    }

    // 720F周期で再生成、HPが減るまでループする弾幕にする
    if (count % 720 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTriplePoint;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 90.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
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