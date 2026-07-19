// enemyPat_sierpinski.cpp
//
// シェルピンスキーのギャスケットをモチーフにした弾幕パターン
//
// ■ 弾幕 A  ShotChaosStream
//     カオスゲーム法を用いて、シェルピンスキー三角形のアトラクタ上に
//     弾を連続生成し、プレイヤーへ向けて発射する青い弾流。
//     60 フレーム間だけ弾を生成（毎フレーム 2 発）し、以降は移動のみ。
//     ※ sEnemyShotSet に param_d フィールドがないため、カオスゲームの
//       現在点はファイルスコープの static 変数で管理する。
//
// ■ 弾幕 B  ShotSierpinskiBurst
//     深度 3（2^3 = 8 行 → 27 点）のシェルピンスキー格子点に弾を
//     一斉配置し、真下へ降下させる「フラクタル壁」弾幕。
//     3 つのサブ三角形をマゼンタ・黄・シアンで色分けし、
//     プレイヤーは除去された「穴」（安全地帯）を通り抜けなければならない。
//
// ■ 敵関数  EnemyPat_Sierpinski_Claude
//     画面上部を左右往復しながら A と B を交互（50 フレームずつ）に展開。
//       count % 100 == 20 : 弾幕 A（カオスゲーム連射）
//       count % 100 == 70 : 弾幕 B（シェルピンスキー降下壁）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ─────────────────────────────────────────────────────────────────────────────
// 内部ヘルパー：弾を ShotSet の循環双方向リスト末尾に挿入
// ─────────────────────────────────────────────────────────────────────────────
static void AppendShot(sEnemyShotSet* pSet, sEnemyShot* pShot) {
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ─────────────────────────────────────────────────────────────────────────────
// 弾幕 A：カオスゲーム連射
// ─────────────────────────────────────────────────────────────────────────────

// カオスゲームの現在点（全インスタンス共有・永続）
// 重心 = ((240+80+400)/3, (40+200+200)/3) = (240, 146.7) 付近で初期化
static double s_chaos_x = 240.0;
static double s_chaos_y = 120.0;

// シェルピンスキーアトラクタとなる固定三角形の頂点（スクリーン座標）
//   上頂点 (240, 40)、左下 (80, 200)、右下 (400, 200)
static const double CHAOS_VX[3] = { 240.0,  80.0, 400.0 };
static const double CHAOS_VY[3] = { 40.0, 200.0, 200.0 };

static void ShotChaosStream(sEnemyShotSet* pEnemyShotSet) {
    // 60 フレーム間、毎フレーム 2 発をカオスゲーム位置から生成
    if (pEnemyShotSet->count < 60) {
        if (pEnemyShotSet->count % 5 == 0) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < 5; i++) {
            // 0, 1, 2 のいずれかをランダム選択（GetRand(2) は 0〜2 の 3 値）
            int v = GetRand(2);

            // カオスゲーム：選んだ頂点との中点へ移動
            s_chaos_x = (s_chaos_x + CHAOS_VX[v]) * 0.5;
            s_chaos_y = (s_chaos_y + CHAOS_VY[v]) * 0.5;

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = s_chaos_x;
            pEnemyShot->y = s_chaos_y;
            pEnemyShot->muki = atan2(player.y - s_chaos_y,
                player.x - s_chaos_x);
            pEnemyShot->speed = 0;
            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青

            AppendShot(pEnemyShotSet, pEnemyShot);
        }
    }

    // 全弾移動
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->count == 60) p->speed = 2.2;

        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 弾幕 B：シェルピンスキー深度 3 一斉展開（降下壁）
// ─────────────────────────────────────────────────────────────────────────────
//
// 【シェルピンスキー格子の構成】
//   深度 3 → 2^3 = 8 行（行インデックス r = 0〜7）
//   行 r において列インデックス c (0 ≤ c ≤ r) が
//   「c & r == c」（c が r のビットサブセット）を満たす点が格子点。
//   → 合計 3^3 = 27 点。
//
// 【3 色のサブ三角形分類（深度 1 レベル）】
//   行 r ≤ 3         → 上サブ三角形     （9 点、マゼンタ）
//   行 r ≥ 4 かつ c ≤ r−4 → 左下サブ三角形 （9 点、黄）
//   行 r ≥ 4 かつ c ≥ 4   → 右下サブ三角形 （9 点、シアン）
//
// 【座標変換】
//   重心座標 (alpha, beta, gamma) を用いて三角形頂点を内挿：
//     alpha = 1 − r/7  （上頂点の重み）
//     beta  = (r−c)/7  （左下頂点の重み）
//     gamma = c/7      （右下頂点の重み）

static void ShotSierpinskiBurst(sEnemyShotSet* pEnemyShotSet) {
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 三角形頂点（ShotSet 生成座標を原点とする相対座標）
        // 上頂点が画面上端付近、底辺が敵より 90px 下に来るよう設定。
        // 深度 3 → 分母は 7（行 0〜7 を 7 等分）
        const double Tx = 0.0, Ty = -60.0; // 上頂点
        const double Lx = -120.0, Ly = 90.0; // 左下頂点
        const double Rx = 120.0, Ry = 90.0; // 右下頂点

        for (int r = 0; r < 8; r++) {
            for (int c = 0; c <= r; c++) {
                // シェルピンスキー条件：c が r のビットサブセット
                if ((c & r) != c) continue;

                // 重心座標 → 三角形上の相対座標
                const double alpha = 1.0 - r / 7.0;
                const double beta = (r - c) / 7.0;
                const double gamma = c / 7.0;

                const double fx = alpha * Tx + beta * Lx + gamma * Rx;
                const double fy = alpha * Ty + beta * Ly + gamma * Ry;

                // サブ三角形による色分け
                int color;
                if (r <= 3) {
                    color = 5; // マゼンタ：上サブ三角形
                }
                else if (c <= r - 4) {
                    color = 1; // 黄：左下サブ三角形
                }
                else {
                    color = 3; // シアン：右下サブ三角形
                }

                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + fx;
                pEnemyShot->y = pEnemyShotSet->y + fy;
                pEnemyShot->muki = DX_PI * 0.5; // 真下
                pEnemyShot->speed = 2.0;
                pEnemyShot->kind = img_enemyShotSmallBall[color];

                AppendShot(pEnemyShotSet, pEnemyShot);
            }
        }
    }

    // 全弾移動
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 敵本体：EnemyPat_Sierpinski_Claude
// ─────────────────────────────────────────────────────────────────────────────
void EnemyPat_Sierpinski_Claude() {
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.2 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 弾幕 A：カオスゲーム連射（100 フレームに 1 回）
    if (count % 100 == 20) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotChaosStream;
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

    // 弾幕 B：シェルピンスキー降下壁（弾幕 A から 50 フレーム後）
    if (count % 100 == 70) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSierpinskiBurst;
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
}