// enemyPat_tmp.cpp
// 残像分身突進（アフタ―イメージラッシュ）- 修正版

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 残像の ShotSet ポインタ（散開時に正確な位置を参照するため）
static sEnemyShotSet* g_afterimage[3] = { nullptr, nullptr, nullptr };

// ------------------------------------------------------------
// 照準線：敵位置＋オフセットで線状に見える
// ------------------------------------------------------------
static void ShotAimLaser(sEnemyShotSet* pSet)
{
    double ox = pSet->param_d[0] * cos(pSet->muki);
    double oy = pSet->param_d[0] * sin(pSet->muki);
    pSet->x = enemy.x + ox;
    pSet->y = enemy.y + oy;
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x = pSet->x;
        p->y = pSet->y;
        p = p->next;
    }
}

// ------------------------------------------------------------
// 残像：敵と同じ速度・同じ角度で直線移動
// ------------------------------------------------------------
static void ShotAfterimage(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
    // sEnemyShotSet の位置も更新（散開時に参照するため）
    if (pSet->pEnemyShotHead->next != pSet->pEnemyShotHead) {
        pSet->x = pSet->pEnemyShotHead->next->x;
        pSet->y = pSet->pEnemyShotHead->next->y;
    }
}

// ------------------------------------------------------------
// 散開弾：直進
// ------------------------------------------------------------
static void ShotSpreadBullet(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン：残像分身突進
// ------------------------------------------------------------
void EnemyPat_Tackle_Kimi()
{
    static int    phase;       // 0:予兆, 1:突進, 2:散開, 3:瞬間移動
    static int    phaseCnt;
    static double rushAngle;
    static double rushSpeed;
    static int    nextSide;
    static int    rushFrame;

    // ---- 初期化 ----
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 20.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseCnt = -60;
        nextSide = 0;
        rushFrame = 0;
        for (int i = 0; i < 3; i++) g_afterimage[i] = nullptr;
        return;
    }

    switch (phase) {

        // ========================================================
        // 0: 予兆・照準（60フレーム）
        // ========================================================
    case 0:
    {
        if (phaseCnt == 0) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            rushAngle = atan2(player.y - enemy.y, player.x - enemy.x);
            rushSpeed = 12.0; // 高速突進

            // レーザー照準線（赤）：敵からプレイヤー方向に16px間隔で20個
            // ボスからプレイヤーまで一直線に見える予兆線を形成
            for (int i = 0; i < 17; i++) {
                sEnemyShotSet* ps = new sEnemyShotSet;
                ps->count = 0;
                ps->patternFunc = ShotAimLaser;
                ps->x = enemy.x;
                ps->y = enemy.y;
                ps->muki = rushAngle;
                ps->kind = 0; // 赤
                ps->param_d[0] = i * 16.0; // 敵からの距離オフセット

                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = enemy.x;
                pShot->y = enemy.y;
                pShot->muki = rushAngle;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotLaser[0]; // 赤レーザー
                pShot->count = 0;
                pShot->margin = 999;

                ps->pEnemyShotHead = new sEnemyShot;
                ps->pEnemyShotHead->prev = ps->pEnemyShotHead;
                ps->pEnemyShotHead->next = ps->pEnemyShotHead;

                pShot->prev = ps->pEnemyShotHead->prev;
                pShot->next = ps->pEnemyShotHead;
                ps->pEnemyShotHead->prev->next = pShot;
                ps->pEnemyShotHead->prev = pShot;

                ps->prev = enemyShotSetHead.prev;
                ps->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = ps;
                enemyShotSetHead.prev = ps;
            }
        }

        phaseCnt++;
        if (phaseCnt >= 60) {
            phase = 1;
            phaseCnt = 0;
            rushFrame = 0;
            for (int i = 0; i < 3; i++) g_afterimage[i] = nullptr;

            // 突進開始音
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        break;
    }

    // ========================================================
    // 1: 超高速体当たり突進（画面端まで貫通）
    // ========================================================
    case 1:
    {
        enemy.x += rushSpeed * cos(rushAngle);
        enemy.y += rushSpeed * sin(rushAngle);
        rushFrame++;

        // 18フレーム間隔で残像（大玉・シアン）を3体生成
        // 敵と同じ速度・同じ角度で直線移動するため、確実に軌道を追う
        for (int i = 0; i < 3; i++) {
            int delay = (i + 1) * 18;
            if (rushFrame == delay && g_afterimage[i] == nullptr) {
                sEnemyShotSet* ps = new sEnemyShotSet;
                ps->count = 0;
                ps->patternFunc = ShotAfterimage;
                ps->x = enemy.x;
                ps->y = enemy.y;
                ps->muki = rushAngle;
                ps->kind = 3; // シアン
                
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = enemy.x;
                pShot->y = enemy.y;
                pShot->muki = rushAngle;
                pShot->speed = rushSpeed; // 敵と同じ速度で直進
                pShot->kind = img_enemyShotLargeBall[3]; // 大玉・シアン
                pShot->count = 0;
                pShot->margin = 480;

                ps->pEnemyShotHead = new sEnemyShot;
                ps->pEnemyShotHead->prev = ps->pEnemyShotHead;
                ps->pEnemyShotHead->next = ps->pEnemyShotHead;

                pShot->prev = ps->pEnemyShotHead->prev;
                pShot->next = ps->pEnemyShotHead;
                ps->pEnemyShotHead->prev->next = pShot;
                ps->pEnemyShotHead->prev = pShot;

                ps->prev = enemyShotSetHead.prev;
                ps->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = ps;
                enemyShotSetHead.prev = ps;

                g_afterimage[i] = ps;
            }
        }

        // 画面外に出るまで突進し続ける（接近判定なし）
        if (enemy.x < 20.0 || enemy.x > 460.0 ||
            enemy.y < 20.0 || enemy.y > 460.0) {
            phase = 2;
            phaseCnt = 0;
        }
        break;
    }

    // ========================================================
    // 2: 90度散開・扇状弾発射
    // ========================================================
    case 2:
    {
        if (phaseCnt % 2 == 0 && phaseCnt < 20) {
            if (CheckSoundMem(sound_enemyShot_noize)) StopSoundMem(sound_enemyShot_noize);
            PlaySoundMem(sound_enemyShot_noize, DX_PLAYTYPE_BACK);

            enemy.hp--;
        }

        if (phaseCnt == 0) {
            // 散開効果音
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 発射点：敵本体 + 残像3体の現在位置
            double srcX[4], srcY[4];
            srcX[0] = enemy.x; srcY[0] = enemy.y;
            for (int i = 0; i < 3; i++) {
                if (g_afterimage[i]) {
                    srcX[i + 1] = g_afterimage[i]->x;
                    srcY[i + 1] = g_afterimage[i]->y;
                }
                else {
                    srcX[i + 1] = enemy.x;
                    srcY[i + 1] = enemy.y;
                }
            }

            // 各発射点から扇状（-60°～+60°）に1発ずつ、合計4発
            for (int j = 0 - 3; j < 4 + 3; j++) for (int k = 0; k < 2; k++) {
                double ang = rushAngle - DX_PI / 3.0 + (DX_PI / 4.5) * j;

                sEnemyShotSet* ps = new sEnemyShotSet;
                ps->count = 0;
                ps->patternFunc = ShotSpreadBullet;
                ps->x = srcX[0];
                ps->y = srcY[0];
                ps->muki = ang;
                ps->kind = 0; // 赤

                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = srcX[0];
                pShot->y = srcY[0];
                pShot->muki = ang;
                pShot->speed = 5.0 - 2 * k; // 高速
                pShot->kind = img_enemyShotMediumBall[0]; // 中玉・赤
                pShot->count = 0;
                pShot->margin = 480;

                ps->pEnemyShotHead = new sEnemyShot;
                ps->pEnemyShotHead->prev = ps->pEnemyShotHead;
                ps->pEnemyShotHead->next = ps->pEnemyShotHead;

                pShot->prev = ps->pEnemyShotHead->prev;
                pShot->next = ps->pEnemyShotHead;
                ps->pEnemyShotHead->prev->next = pShot;
                ps->pEnemyShotHead->prev = pShot;

                ps->prev = enemyShotSetHead.prev;
                ps->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = ps;
                enemyShotSetHead.prev = ps;
            }
        }

        phaseCnt++;
        if (phaseCnt >= 40) {
            phase = 3;
            phaseCnt = 0;
        }
        break;
    }

    // ========================================================
    // 3: 次の画面端へ瞬間移動
    // ========================================================
    case 3:
    {
        if (phaseCnt == 0) {
            // 4辺を順番に移動
            switch (nextSide) {
            case 0: enemy.x = 20.0;  enemy.y = 240.0; break; // 左
            case 2: enemy.x = 460.0; enemy.y = 240.0; break; // 右
            case 3: enemy.x = 240.0; enemy.y = 20.0;  break; // 上
            case 1: enemy.x = 240.0; enemy.y = 460.0; break; // 下
            }
            nextSide = (nextSide + 1) % 4;
            for (int i = 0; i < 3; i++) g_afterimage[i] = nullptr;
        }

        phaseCnt++;
        if (phaseCnt >= 30) {
            phase = 0;
            phaseCnt = 0;
        }
        break;
    }

    } // switch
}