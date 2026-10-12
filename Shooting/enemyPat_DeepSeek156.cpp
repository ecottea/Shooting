#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：ルーローの三角形「ルーロー・クローズ」
// ------------------------------------------------------------
//  ・3頂点を中弾、3本の円弧を小弾の点列で描く。
//  ・全体がゆっくり回転しながら収縮・膨張を繰り返す。
//  ・頂点は一定間隔で自機狙いの3-way弾を撃つ。
//  ・寿命が来ると全弾が外向きに飛び散って自然消滅する。
//
//  ルーローの三角形の作図:
//    中心Oの周りに頂点A,B,Cを半径R、角度90°,210°,330°に置く。
//    辺BCは頂点Aを中心とした半径S=√3Rの円弧、
//    辺CAは頂点B中心、辺ABは頂点C中心の円弧。
//    これで「どの向きで測っても幅が一定」になる。
// ============================================================
static void ShotRoulet(sEnemyShotSet* pEnemyShotSet)
{
    const double PI = DX_PI;
    const double R = 70.0*2;          // 中心から頂点までの距離
    const double S = R * sqrt(3.0); // 弧の半径（=頂点間の距離）
    const int    ARC_DIV = 24;            // 1本の弧を何分割するか
    const double OMEGA = 0.030;         // 回転速度 (rad/frame)
    const double PULSE_PERIOD = 180.0;         // 収縮の周期 (frames)
    const double PULSE_MIN = 0.35;          // 最小収縮率
    const double PULSE_MAX = 1.00;          // 最大収縮率
    const int    LIFETIME = 540;           // パターンの寿命 (frames)
    const int    FIRE_INT = 90;            // 頂点が撃つ間隔 (frames)

    const double cx = pEnemyShotSet->x;
    const double cy = pEnemyShotSet->y;

    // ----------------------------------------------------------
    //  初期化：ルーロー三角形を構成する全弾を一度に生成する
    // ----------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 3頂点の中心相対座標
        double vx[3], vy[3];
        for (int k = 0; k < 3; k++) {
            double a = (90.0 + 120.0 * k) / 180.0 * PI;
            vx[k] = R * cos(a);
            vy[k] = R * sin(a);
        }

        // 頂点：中弾（赤）
        for (int k = 0; k < 3; k++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = cx + vx[k];
            p->y = cy + vy[k];
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotMediumBall[0]; // 赤
            p->param_i[0] = 1;   // 1:頂点
            p->param_i[1] = k;   // 頂点番号
            p->param_d[0] = vx[k];
            p->param_d[1] = vy[k];
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        // 3本の弧：小弾の点列
        //   arc=0: 中心A, 端点B→C
        //   arc=1: 中心B, 端点C→A
        //   arc=2: 中心C, 端点A→B
        static const int arcColor[3] = { 3, 4, 5 }; // シアン、青、マゼンタ
        for (int arc = 0; arc < 3; arc++) {
            int cIdx = arc;
            int p1Idx = (arc + 1) % 3;
            int p2Idx = (arc + 2) % 3;

            double dir1 = atan2(vy[p1Idx] - vy[cIdx], vx[p1Idx] - vx[cIdx]);
            double dir2 = atan2(vy[p2Idx] - vy[cIdx], vx[p2Idx] - vx[cIdx]);
            double diff = dir2 - dir1;
            while (diff > PI) diff -= 2.0 * PI;
            while (diff < -PI) diff += 2.0 * PI;

            for (int j = 1; j < ARC_DIV; j++) {
                double t = (double)j / (double)ARC_DIV;
                double ang = dir1 + diff * t;
                double px = vx[cIdx] + S * cos(ang);
                double py = vy[cIdx] + S * sin(ang);

                sEnemyShot* p = new sEnemyShot;
                p->x = cx + px;
                p->y = cy + py;
                p->muki = 0.0;
                p->speed = 0.0;
                p->kind = img_enemyShotSmallBall[arcColor[arc]];
                p->param_i[0] = 0;   // 0:パターン弾（弧）
                p->param_d[0] = px;
                p->param_d[1] = py;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // ----------------------------------------------------------
    //  寿命到達：パターン弾を外向きの通常弾に変換する
    // ----------------------------------------------------------
    if (pEnemyShotSet->count == LIFETIME) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                double dx = pShot->x - cx;
                double dy = pShot->y - cy;
                double d = sqrt(dx * dx + dy * dy);
                if (d < 1.0) d = 1.0;
                pShot->muki = atan2(dy, dx);
                pShot->speed = 3.0;
                pShot->param_i[0] = -1; // 通常弾化
            }
            pShot = pNext;
        }
    }

    // ----------------------------------------------------------
    //  パターン弾：回転と収縮
    // ----------------------------------------------------------
    if (pEnemyShotSet->count < LIFETIME) {
        double theta = OMEGA * pEnemyShotSet->count;
        double phase = 2.0 * PI * pEnemyShotSet->count / PULSE_PERIOD;
        double r = PULSE_MIN + (PULSE_MAX - PULSE_MIN) * (0.5 + 0.5 * cos(phase));
        double cs = cos(theta);
        double sn = sin(theta);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                double bx = pShot->param_d[0];
                double by = pShot->param_d[1];
                pShot->x = cx + r * (bx * cs - by * sn);
                pShot->y = cy + r * (bx * sn + by * cs);
            }
            pShot = pNext;
        }
    }

    // ----------------------------------------------------------
    //  頂点からの自機狙い弾
    // ----------------------------------------------------------
    if (pEnemyShotSet->count > 0
        && pEnemyShotSet->count < LIFETIME
        && pEnemyShotSet->count % FIRE_INT == 0)
    {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            if (pShot->param_i[0] == 1) {
                double base = atan2(player.y - pShot->y, player.x - pShot->x);
                for (int k = -1; k <= 1; k++) {
                    sEnemyShot* p = new sEnemyShot;
                    p->x = pShot->x;
                    p->y = pShot->y;
                    p->muki = base + k * (15.0 / 180.0 * PI);
                    p->speed = 3.2;
                    p->kind = img_enemyShotBullet[1]; // 黄
                    p->param_i[0] = -1; // 通常弾
                    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    p->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = p;
                    pEnemyShotSet->pEnemyShotHead->prev = p;
                }
            }
            pShot = pNext;
        }
    }

    // ----------------------------------------------------------
    //  通常弾：直進
    // ----------------------------------------------------------
    {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            if (pShot->param_i[0] == -1) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pNext;
        }
    }
}

// ============================================================
//  敵本体：ルーロー三角形弾幕を定期的に発生させる
// ============================================================
void EnemyPat_ReuleauxTriangle_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 予告音（発生の 80 フレーム前）
    if (count % 600 == 520+20) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ルーロー三角形弾幕を発生（周期 600 frames = 10 秒）
    if (count % 600 <= 50 && count % 10 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRoulet;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 220.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}