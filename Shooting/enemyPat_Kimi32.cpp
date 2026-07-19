// enemyPat_tmp.cpp
// ボンバーマン風連鎖爆炎弾幕（チェーンファイア）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

/* ============================================================
   【使用素材一覧】
   ■ 弾画像
     - img_enemyShotLargeBall[7]（黒）: 予兆弾（ボンバーマンの爆弾イメージ）
     - img_enemyShotLargeBall[0]（赤）: 予兆弾の点滅色（危険を示す）
     - img_enemyShotScale[0〜8]（赤/橙）: 通常爆炎の十字ビーム（鱗弾で火炎のような見た目）
     - img_enemyShotSmallBall[0〜8]（赤/橙）: 誘爆時の小さな火花
     - img_enemyShotLargeBall[0]（赤）: 爆発中心（大）
     - img_enemyShotMediumBall[8]（橙）: 誘爆の爆発中心（中）

   ■ 効果音
     - sound_enemyShot_light: 爆弾設置時（軽い「カチッ」音を想定）
     - sound_enemyShot_heavy: 通常爆発時（大きな爆発音）
     - sound_enemyShot_medium: 誘爆（連鎖爆発）時（やや小さめの爆発音）
   ============================================================ */

   // 前方宣言
static void ShotChainFire(sEnemyShotSet* pEnemyShotSet);

// 十字爆炎の生成処理（内部用）
static void CreateCrossExplosion(sEnemyShotSet* pEnemyShotSet, int chainLevel)
{
    sEnemyShot* pEnemyShot;
    double cx = pEnemyShotSet->param_d[0];
    double cy = pEnemyShotSet->param_d[1];

    // 4方向：右、下、左、上
    double dirs[4] = { 0.0, DX_PI / 2.0, DX_PI, DX_PI * 1.5 };
    int baseColor = (chainLevel == 0) ? 0 : 8; // 通常:赤(0)、連鎖:橙(8)

    // --- 中心に爆炎 ---
    pEnemyShot = new sEnemyShot;
    pEnemyShot->x = cx;
    pEnemyShot->y = cy;
    pEnemyShot->muki = 0.0;
    pEnemyShot->speed = 0.0;
    pEnemyShot->param_i[0] = 1;               // 爆炎弾フラグ
    pEnemyShot->param_i[1] = chainLevel;
    pEnemyShot->param_i[2] = (chainLevel == 0) ? 50 : 35; // 寿命（フレーム）
    pEnemyShot->param_i[3] = pEnemyShotSet->count; // 生成時のcount
    pEnemyShot->kind = (chainLevel == 0) ? img_enemyShotLargeBall[0]   // 赤 大玉
        : img_enemyShotMediumBall[8]; // 橙 中玉
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

    // --- 4方向に伸びる爆炎 ---
    // 通常:3〜4マス、連鎖:2マス（GetRandは0以上x以下なので注意）
    int length = (chainLevel == 0) ? (3 + GetRand(1)) : 2;
    double spacing = 22.0; // 1マスあたりのピクセル数

    for (int d = 0; d < 4; d++) {
        for (int i = 1; i <= length; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + spacing * i * cos(dirs[d]);
            pEnemyShot->y = cy + spacing * i * sin(dirs[d]);
            pEnemyShot->muki = dirs[d];
            pEnemyShot->speed = 0.0;
            pEnemyShot->param_i[0] = 1; // 爆炎弾
            pEnemyShot->param_i[1] = chainLevel;
            pEnemyShot->param_i[2] = 50 + i * 3; // 外側ほど少し長く残る
            pEnemyShot->param_i[3] = pEnemyShotSet->count;

            int colorIdx = (baseColor + i) % 9;
            if (chainLevel == 0) {
                pEnemyShot->kind = img_enemyShotScale[colorIdx]; // 鱗弾で火炎表現
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[colorIdx]; // 小玉で火花表現
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 連鎖：子セットを生成（最大2段階まで） ---
    if (chainLevel < 2) {
        sEnemyShotSet* pChild = new sEnemyShotSet;
        pChild->count = 0;
        pChild->patternFunc = ShotChainFire;

        // ランダム方向に30〜50ピクセルずれた位置に誘爆
        double childAngle = GetRand(360) / 180.0 * DX_PI;
        double childDist = 30.0 + GetRand(20);
        pChild->x = cx + childDist * cos(childAngle);
        pChild->y = cy + childDist * sin(childAngle);

        // 画面内クランプ（480x480、margin考慮）
        if (pChild->x < 20.0) pChild->x = 20.0;
        if (pChild->x > 460.0) pChild->x = 460.0;
        if (pChild->y < 20.0) pChild->y = 20.0;
        if (pChild->y > 460.0) pChild->y = 460.0;

        pChild->muki = 0.0;
        pChild->kind = 999;            // 識別用
        pChild->param_i[0] = 2;        // 即爆発モード
        pChild->param_i[1] = chainLevel + 1; // 連鎖段階+1

        pChild->pEnemyShotHead = new sEnemyShot;
        pChild->pEnemyShotHead->prev = pChild->pEnemyShotHead;
        pChild->pEnemyShotHead->next = pChild->pEnemyShotHead;

        pChild->prev = enemyShotSetHead.prev;
        pChild->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pChild;
        enemyShotSetHead.prev = pChild;
    }
}

// ============================================================
//  弾幕パターン：ボンバーマン風連鎖爆炎
// ============================================================
static void ShotChainFire(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int state = pEnemyShotSet->param_i[0];
    int chainLevel = pEnemyShotSet->param_i[1];

    // ===== 初期化（count == 0） =====
    if (pEnemyShotSet->count == 0) {
        if (state == 2) {
            // --- 即爆発モード（誘爆）：予兆をスキップ ---
            pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
            pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
            pEnemyShotSet->param_i[0] = 1; // 爆発状態へ

            // 誘爆音
            if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            CreateCrossExplosion(pEnemyShotSet, chainLevel);
        }
        else {
            // --- 通常：予兆弾（黒い爆弾）を生成 ---
            if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLargeBall[7]; // 黒色（爆弾）
            pEnemyShot->param_i[0] = 0; // 予兆弾フラグ
            pEnemyShot->param_i[1] = chainLevel;
            pEnemyShot->param_i[2] = 0;
            pEnemyShot->param_i[3] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
            pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
            pEnemyShotSet->param_i[0] = 0; // 予兆中
        }
    }

    // ===== 予兆中の点滅処理（危険を示す赤黒点滅） =====
    if (pEnemyShotSet->param_i[0] == 0) {
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 0) { // 予兆弾のみ
                // 8フレーム周期で赤⇔黒を点滅
                if (pEnemyShotSet->count % 8 < 4) {
                    pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤
                }
                else {
                    pEnemyShot->kind = img_enemyShotLargeBall[7]; // 黒
                }
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // ===== 初爆（count == 60、予兆から爆発へ遷移） =====
    if (pEnemyShotSet->count == 60 && pEnemyShotSet->param_i[0] == 0) {
        pEnemyShotSet->param_i[0] = 1; // 爆発状態

        // 予兆弾を画面外に追い出し、メインルーチンの消去処理に委譲
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 0) {
                pEnemyShot->x = -1000.0;
            }
            pEnemyShot = pEnemyShot->next;
        }

        // 大爆発音
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        CreateCrossExplosion(pEnemyShotSet, chainLevel);
    }

    // ===== 爆発中の寿命管理 =====
    if (pEnemyShotSet->param_i[0] == 1) {
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 1) { // 爆炎弾
                // 生成時からの経過フレームが個別寿命を超えたら画面外へ
                if (pEnemyShotSet->count - pEnemyShot->param_i[3] >= pEnemyShot->param_i[2]) {
                    pEnemyShot->x = -1000.0;
                }
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // ===== セット終了処理（150フレームで全弾確実に消去） =====
    if (pEnemyShotSet->count >= 150) {
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            pEnemyShot->x = -1000.0;
            pEnemyShot = pEnemyShot->next;
        }
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Bomberman_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 45フレーム毎に爆弾を設置
    if (count % 45 == 0) {
        // 同時存在する有効セット数をカウント
        int activeSets = 0;
        //sEnemyShotSet* pCheck = enemyShotSetHead.next;
        //while (pCheck != &enemyShotSetHead) {
        //    if (pCheck->patternFunc == ShotChainFire && pCheck->count < 150) {
        //        activeSets++;
        //    }
        //    pCheck = pCheck->next;
        //}

        if (activeSets < 5) { // 最大5セットまで
            for(int i = 0; i < 5; i++) {
                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotChainFire;

                // ランダム位置（画面内。GetRand(400)は0〜400）
                while (true) {
                    pEnemyShotSet->x = 40.0 + GetRand(400);
                    pEnemyShotSet->y = 80.0 + GetRand(380);
                    double dx = pEnemyShotSet->x - player.x;
                    double dy = pEnemyShotSet->y - player.y;
                    if (hypot(dx, dy) > 30) break;
                }
                pEnemyShotSet->muki = 0.0;
                pEnemyShotSet->kind = shot_count++;
                pEnemyShotSet->param_i[0] = 0; // 通常予兆モード
                pEnemyShotSet->param_i[1] = 0; // 連鎖段階0

                pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

                pEnemyShotSet->prev = enemyShotSetHead.prev;
                pEnemyShotSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pEnemyShotSet;
                enemyShotSetHead.prev = pEnemyShotSet;
            }
        }
    }
}