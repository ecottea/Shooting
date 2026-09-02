// EnemyPat_Violate_Grok_Foul.cpp
// 反則テーマ弾幕：「反則検知・即失格」
// 敵本体関数名: void EnemyPat_Violate_Grok()
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  監視視線弾（審判の視線を表現する周回弾）
//  小玉を敵周囲でゆっくり回転させる
// ============================================================
static void ShotMonitorGaze(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 予告音は使わず軽め
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 12方向の周回弾を生成
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = (DX_PI * 2.0 / 12.0) * i + pEnemyShotSet->muki;
            pEnemyShot->x = enemy.x + 70.0 * cos(ang);
            pEnemyShot->y = enemy.y + 70.0 * sin(ang);
            pEnemyShot->muki = ang;               // 初期角度
            pEnemyShot->speed = 0.0;              // 位置は手動更新
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン小玉（視線っぽく）
            // param_d[0] = 現在角度
            // param_d[1] = 角速度
            // param_d[2] = 半径
            pEnemyShot->param_d[0] = ang;
            pEnemyShot->param_d[1] = 0.025 * ((i % 2 == 0) ? 1.0 : -1.0); // 交互に逆回転
            pEnemyShot->param_d[2] = 70.0 + (i % 3) * 8.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：敵の位置を中心に回転させ、一定時間後に半径を広げて画面外へ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_d[0] += pShot->param_d[1];
        // セットの寿命に合わせて半径を徐々に拡大（約2秒後に画面外へ）
        if (pEnemyShotSet->count > 60) {
            pShot->param_d[2] += 1.8;
        }
        pShot->x = enemy.x + pShot->param_d[2] * cos(pShot->param_d[0]);
        pShot->y = enemy.y + pShot->param_d[2] * sin(pShot->param_d[0]);
        pShot = pShot->next;
    }
}

// ============================================================
//  笛吹き演出（放射状の短レーザー）
// ============================================================
static void ShotWhistle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 36本の放射レーザー（笛の音を視覚化）
        for (int i = 0; i < 36; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = (DX_PI * 2.0 / 36.0) * i;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 6.5 + (i % 3) * 0.4;
            pEnemyShot->kind = img_enemyShotLaser[6]; // 白レーザー
            // 少しだけ寿命っぽく見せるため param に使用フレームを入れておく（消去はメイン）
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->param_i[0]++;
        pShot = pShot->next;
    }
}

// ============================================================
//  失格弾幕（画面埋め尽くし）
//  ・四隅からの収束直線弾
//  ・自機位置中心の急速拡大円
//  ・高密度ばら撒き
// ============================================================
static void ShotDisqualify(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // --- 1. 四隅から中心方向への直線弾（壁） ---
        const double corners[4][2] = {
            {  20.0,  20.0 },
            { 460.0,  20.0 },
            {  20.0, 460.0 },
            { 460.0, 460.0 }
        };
        for (int c = 0; c < 4; c++) {
            double cx = corners[c][0];
            double cy = corners[c][1];
            double baseAng = atan2(240.0 - cy, 240.0 - cx);
            for (int i = 0; i < 14; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx + (GetRand(30) - 15);
                pEnemyShot->y = cy + (GetRand(30) - 15);
                pEnemyShot->muki = baseAng + (GetRand(40) - 20) / 180.0 * DX_PI;
                pEnemyShot->speed = 2.8 + GetRand(20) / 10.0;
                // 赤と黒を交互
                pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotMediumBall[0] : img_enemyShotMediumBall[7];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // --- 2. 自機位置を中心に急速拡大する円形弾 ---
        for (int ring = 0; ring < 3; ring++) {
            int num = 18 + ring * 6;
            for (int i = 0; i < num; i++) {
                pEnemyShot = new sEnemyShot;
                double ang = (DX_PI * 2.0 / num) * i + ring * 0.15;
                pEnemyShot->x = player.x;
                pEnemyShot->y = player.y;
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 1.2 + ring * 0.9;
                pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤小玉
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // --- 3. 高密度ばら撒き（画面中央寄り） ---
        for (int i = 0; i < 48; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 120.0 + GetRand(240);
            pEnemyShot->y = 80.0 + GetRand(160);
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.5 + GetRand(25) / 10.0;
            // 鱗弾や菱形でバリエーション
            if (i % 3 == 0) {
                pEnemyShot->kind = img_enemyShotScale[0];
            }
            else if (i % 3 == 1) {
                pEnemyShot->kind = img_enemyShotDiamond[7];
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumOval[0];
            }
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 通常移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体パターン
//  関数名は必ず void EnemyPat_Violate_Grok()
// ============================================================
void EnemyPat_Violate_Grok()
{
    // フェーズ管理
    // 0: 監視中
    // 1: 笛吹き演出中
    // 2: 失格弾幕展開中
    // 3: クールダウン（再監視準備）
    static int phase = 0;
    static int phaseTimer = 0;
    static int monitorShotTimer = 0;
    static int mukiDir = 1;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseTimer = 0;
        monitorShotTimer = 0;
        mukiDir = 1;
    }
    else {
        // 敵の軽い左右移動（監視感を出す）
        enemy.x += 0.6 * (double)mukiDir;
        if (enemy.x < 160.0) mukiDir = 1;
        if (enemy.x > 320.0) mukiDir = -1;
    }

    // ---------- フェーズ遷移判定 ----------
    if (phase == 0) {
        // 反則検知条件
        // 1. 自機が左右端に寄りすぎ
        // 2. 敵HPが半分以下になった（追い詰められた審判が厳しくなる）
        bool foul = false;
        if (player.x < 55.0 || player.x > 425.0) {
            foul = true;
        }
        if (enemy.hp <= 100) {
            foul = true;
        }

        if (foul) {
            phase = 1;
            phaseTimer = 0;

            // 笛吹きセットを即座に生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotWhistle;
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
    else if (phase == 1) {
        // 笛吹き演出 約40フレーム
        phaseTimer++;
        if (phaseTimer >= 40) {
            phase = 2;
            phaseTimer = 0;

            // 失格弾幕セット生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotDisqualify;
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
    else if (phase == 2) {
        // 失格弾幕展開中 約120フレーム
        phaseTimer++;
        if (phaseTimer >= 120) {
            phase = 3;
            phaseTimer = 0;
        }
    }
    else if (phase == 3) {
        // クールダウン 約90フレーム（再検知可能になるまで）
        phaseTimer++;
        if (phaseTimer >= 90) {
            phase = 0;
            phaseTimer = 0;
        }
    }

    // ---------- 監視視線弾の定期生成（監視フェーズのみ） ----------
    if (phase == 0) {
        monitorShotTimer++;
        if (monitorShotTimer >= 45) { // 約0.75秒ごと
            monitorShotTimer = 0;

            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotMonitorGaze;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = (double)count * 0.02; // 時間で少しずらす
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
}