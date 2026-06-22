// enemyPat_spiderWeb.cpp
//
// クモの巣弾幕パターン
//
// 【構成】
//   ① ShotSpoke    ─ 8 方向放射状弾列（縦糸）
//                     6 フレームごとに 1 発ずつ計 9 発を等間隔に発射
//   ② ShotWebRing  ─ 全方位同時弾幕（横糸）
//                     16 発を一斉射出してリング状に拡張
//   ③ ShotSpider   ─ プレイヤー照準 5WAY 弾（クモ本体の動き）
//
// 【弾幕の見え方】
//   白い縦糸（小玉）がスポーク状に広がり、
//   シアンの横糸（菱形弾）のリングが交差して網目を形成する。
//   マゼンタのクモ弾（中玉）が定期的にプレイヤーを狙う。
//
// 【発射タイミング】
//   ① スポーク  : count == 60, 240, 420, ...（180 フレームごと）
//   ② リング    : count == 10,  55, 100, ...（ 45 フレームごと）
//   ③ スパイダー: count == 30, 150, 270, ...（120 フレームごと）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------------
// ① スポーク（縦糸）
//   pEnemyShotSet->muki : 発射方向
//   pEnemyShotSet->kind : 1 = このセットが効果音を担当（代表セットのみ）
//
//   6 フレームごとに 1 発ずつ計 9 発を発射（count 0, 6, 12, ..., 48）。
//   同じ方向・速度で順に撃ち出すことで、12px 間隔の弾列（縦糸）を形成する。
// ---------------------------------------------------------------
static void ShotSpoke(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count <= 48 && pEnemyShotSet->count % 6 == 0) {
        // 代表セット（kind == 1）のみ発射音を鳴らす（8 セット同時生成の重複再生を防ぐ）
        if (pEnemyShotSet->count == 0 && pEnemyShotSet->kind == 1) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 2.0;
        pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白・小玉

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 全弾前進
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------------------------------------------
// ② リング（横糸）
//   pEnemyShotSet->muki : リングの基準角度（発射ごとに 7.5° ずつ回転）
//
//   count == 0 に 16 発を等角度で一斉射出。
//   スポークより遅い速度で拡張し、縦糸と交差して網目を形成する。
//   基準角度を毎回少しずつずらすことで、複数リングが重なって見える
//   ときに微妙なスパイラル感が生まれる。
// ---------------------------------------------------------------
static void ShotWebRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        const int N = 100;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (2.0 * DX_PI / N) * i;
            pEnemyShot->speed = 1.5;
            pEnemyShot->kind = img_enemyShotDiamond[3]; // シアン・菱形弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾前進
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------------------------------------------
// ③ スパイダー（クモ本体の動き表現・照準弾）
//   pEnemyShotSet->muki : プレイヤー方向（発射時に EnemyPat_SpiderWeb_Claude で計算）
//
//   プレイヤー方向を中心に ±30°（間隔 15°）の 5WAY 弾。
//   速度が高く、クモが糸を伝って素早く迫る演出をする。
// ---------------------------------------------------------------
static void ShotSpider(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 2) * (DX_PI / 12.0); // -30° 〜 +30°
            pEnemyShot->speed = 3.5;
            pEnemyShot->kind = img_enemyShotLargeBall[5]; // マゼンタ・中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾前進
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ---------------------------------------------------------------
// 敵本体
// ---------------------------------------------------------------
void EnemyPat_SpiderWeb_Claude()
{
    // サイクル間で引き継ぐ状態
    static double spokeMuki; // スポーク基準角（発射サイクルごとに 22.5° 回転）
    static double ringMuki;  // リング基準角（発射ごとに 7.5° 回転）
    static int    moveMuki;  // 移動方向（+1: 右、-1: 左）

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        spokeMuki = 0.0;
        ringMuki = 0.0;
        moveMuki = 1;
    }
    else {
        // ── 移動（ゆっくり左右往復、150 フレームごとに反転）──────
        enemy.x += 0.7 * moveMuki;
        if (count % 150 == 0) moveMuki *= -1;

        // ── ① スポーク（縦糸）: 180 フレームごと ────────────────
        //    count == 60, 240, 420, ... に 8 セットを同時生成
        if (count % 90 == 60) {
            const int SPOKE_N = 50;
            for (int i = 0; i < SPOKE_N; i++) {
                sEnemyShotSet* pSS = new sEnemyShotSet;
                pSS->count = 0;
                pSS->kind = (i == 0) ? 1 : 0; // 代表（i==0）のみ音を担当
                pSS->patternFunc = ShotSpoke;
                pSS->x = enemy.x;
                pSS->y = enemy.y;
                pSS->muki = spokeMuki + (2.0 * DX_PI / SPOKE_N) * i;

                pSS->pEnemyShotHead = new sEnemyShot;
                pSS->pEnemyShotHead->prev = pSS->pEnemyShotHead;
                pSS->pEnemyShotHead->next = pSS->pEnemyShotHead;

                pSS->prev = enemyShotSetHead.prev;
                pSS->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSS;
                enemyShotSetHead.prev = pSS;
            }
            spokeMuki += DX_PI / 8.0; // 次サイクルで 22.5° 回転（網目がずれる）
        }

        // ── ② リング（横糸）: 45 フレームごと ───────────────────
        //    count == 10, 55, 100, 145, 190, ... に発射
        if (count % 45 == 10) {
            sEnemyShotSet* pSS = new sEnemyShotSet;
            pSS->count = 0;
            pSS->patternFunc = ShotWebRing;
            pSS->x = enemy.x;
            pSS->y = enemy.y;
            pSS->muki = ringMuki;

            pSS->pEnemyShotHead = new sEnemyShot;
            pSS->pEnemyShotHead->prev = pSS->pEnemyShotHead;
            pSS->pEnemyShotHead->next = pSS->pEnemyShotHead;

            pSS->prev = enemyShotSetHead.prev;
            pSS->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSS;
            enemyShotSetHead.prev = pSS;

            ringMuki += DX_PI / 24.0; // 7.5° ずつ回転（微妙なスパイラル感）
        }

        // ── ③ スパイダー（照準弾）: 120 フレームごと ──────────
        //    count == 30, 150, 270, ... に発射
        if (count % 120 == 30) {
            sEnemyShotSet* pSS = new sEnemyShotSet;
            pSS->count = 0;
            pSS->patternFunc = ShotSpider;
            pSS->x = enemy.x;
            pSS->y = enemy.y;
            pSS->muki = atan2(player.y - enemy.y, player.x - enemy.x);

            pSS->pEnemyShotHead = new sEnemyShot;
            pSS->pEnemyShotHead->prev = pSS->pEnemyShotHead;
            pSS->pEnemyShotHead->next = pSS->pEnemyShotHead;

            pSS->prev = enemyShotSetHead.prev;
            pSS->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSS;
            enemyShotSetHead.prev = pSS;
        }
    }
}