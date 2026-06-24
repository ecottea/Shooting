// enemyPat_volcano.cpp
//
// 火山をモチーフにした弾幕パターン
//
// ■ 弾幕一覧
//   ShotLavaFountain    : 重力付き溶岩弾を扇状に打ち上げる（放物線軌道）
//   ShotEmberBurst      : 爆発のように全方位へ火の粉を散らす（弱重力＋空気抵抗）
//   ShotPyroclasticRing : 衝撃波状の噴石を2重リングで全周へ発射（回転変化付き）
//
// ■ 敵本体
//   EnemyPat_Volcano_Claude : 画面下部中央に鎮座する火山ボス
//                  HPが減るにつれて3段階で噴火パターンが激化する

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ================================================================
// 弾幕①: 溶岩噴泉 (ShotLavaFountain)
//
// 噴火口から溶岩弾 12 発を扇状（±60°）に打ち上げる。
// 重力加速により自然な放物線を描きながらプレイヤー側へ降り注ぐ。
//
// param_d[0] : vx（水平速度成分）
// param_d[1] : vy（鉛直速度成分、毎フレーム +0.07 の重力が加算される）
// muki       : 扇の中心方向（呼び出し元で真上方向 -DX_PI/2 ± 傾きを設定）
// ================================================================
static void ShotLavaFountain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int    N = 12;
        const double spreadTotal = DX_PI * 2.0 / 3.0; // 扇の開き幅：120°
        const double step = spreadTotal / (N - 1);
        // 橙(8)・赤(0)・黄(1) の3色で溶岩を表現
        const int lavaCols[3] = { 8, 0, 1 };

        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20); // 噴火口を少しばらけさせる
            pEnemyShot->y = pEnemyShotSet->y;

            // 扇の各方向に小さなランダムゆらぎを加えて自然な噴火に見せる
            double angle = pEnemyShotSet->muki
                + (i - (N - 1) / 2.0) * step
                + (GetRand(8) - 4) / 180.0 * DX_PI;
            double spd = 3.0 + GetRand(20) / 10.0 + 4; // 3.0 〜 5.0

            pEnemyShot->muki = angle;
            pEnemyShot->speed = spd;
            pEnemyShot->param_d[0] = spd * cos(angle); // vx
            pEnemyShot->param_d[1] = spd * sin(angle); // vy（上向きは負値）

            int col = lavaCols[GetRand(2)];
            pEnemyShot->kind = (GetRand(1) == 0)
                ? img_enemyShotLargeBall[col]
                : img_enemyShotMediumBall[col];

            // 放物線を描いて画面外下方に落ちるまで消さない
            pEnemyShot->margin = 120.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：重力加算 → 速度で位置更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->param_d[1] += 0.07;            // 重力
        pEnemyShot->x += pEnemyShot->param_d[0];
        pEnemyShot->y += pEnemyShot->param_d[1];
        pEnemyShot = pEnemyShot->next;
    }
}

// ================================================================
// 弾幕②: 火の粉散布 (ShotEmberBurst)
//
// 噴火口の小爆発で 24 発の小弾を全方位に散らす。
// 弱い重力と空気抵抗により、煙に乗って漂う火の粉の雰囲気を演出する。
//
// param_d[0] : vx（毎フレーム 0.992 倍に減衰）
// param_d[1] : vy（毎フレーム +0.035 の重力加算後、0.992 倍に減衰）
// muki       : 全体の回転位相オフセット（呼び出し元でランダムに設定）
// ================================================================
static void ShotEmberBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int N = 24;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(20) - 10);
            pEnemyShot->y = pEnemyShotSet->y;

            // 全周均等配置 + 位相オフセット + 小ランダムゆらぎ
            double angle = (2.0 * DX_PI / N) * i
                + pEnemyShotSet->muki
                + (GetRand(14) - 7) / 180.0 * DX_PI;
            double spd = 1.8 + GetRand(20) / 10.0 + 4; // 1.8 〜 3.8

            pEnemyShot->muki = angle;
            pEnemyShot->speed = spd;
            pEnemyShot->param_d[0] = spd * cos(angle);
            pEnemyShot->param_d[1] = spd * sin(angle);

            int col = (GetRand(1) == 0) ? 0 : 8; // 赤 or 橙
            pEnemyShot->kind = img_enemyShotSmallBall[col];
            pEnemyShot->margin = 40.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：弱重力加算 → 空気抵抗減衰 → 位置更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->param_d[1] += 0.035;        // 弱い重力
        pEnemyShot->param_d[0] *= 0.992;        // 水平空気抵抗
        pEnemyShot->param_d[1] *= 0.992;        // 垂直空気抵抗
        pEnemyShot->x += pEnemyShot->param_d[0];
        pEnemyShot->y += pEnemyShot->param_d[1];
        pEnemyShot = pEnemyShot->next;
    }
}

// ================================================================
// 弾幕③: 噴石リング (ShotPyroclasticRing)
//
// 大噴火の衝撃波として、噴石を 2 重リング状に全周へ発射する。
//   内輪：低速・中弾 16 発（赤 or 橙）  ← 避けにくい近距離脅威
//   外輪：高速・小弾 16 発（黄 or 白）  ← 外まで届く遠距離脅威
// 外輪は内輪から半ピッチずらして配置することで弾幕密度を高める。
//
// muki : リング全体の初期回転位相（呼び出し元で毎回 +PI/8 ずつ加算）
// ================================================================
static void ShotPyroclasticRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme) == 1)
            StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 内輪・外輪それぞれのパラメータ
        const int    ringN[2] = { 16,           16 };
        const double ringSpd[2] = { 2.0,          3.2 };
        const double ringPhase[2] = { 0.0,          DX_PI / 16.0 }; // 外輪を半ピッチずらす

        for (int r = 0; r < 2; r++) {
            int    N = ringN[r];
            double spd = ringSpd[r];
            double phase = ringPhase[r] + pEnemyShotSet->muki;

            for (int i = 0; i < N; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                double angle = (2.0 * DX_PI / N) * i + phase;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = spd;

                if (r == 0) { // 内輪：赤 or 橙の中弾
                    int col = (GetRand(1) == 0) ? 0 : 8;
                    pEnemyShot->kind = img_enemyShotMediumBall[col];
                }
                else {      // 外輪：黄 or 白の小弾
                    int col = (GetRand(1) == 0) ? 1 : 6;
                    pEnemyShot->kind = img_enemyShotSmallBall[col];
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレーム：直進
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ================================================================
// 敵本体: 火山ボス (EnemyPat_Volcano_Claude)
//
// 画面下部中央（x=240, y=450）に鎮座する火山が噴火を繰り返す。
// 火山は動かず、HPが減るにつれて噴火パターンが3段階で激化する。
//
// ─────────────────────────────────────────────────────────────
// フェーズ1（HP > 333）: 溶岩噴泉のみ  60 フレームごと
// フェーズ2（HP 167〜333）: 噴泉 45f ＋ 火の粉 80f（独立周期で重なり変化）
// フェーズ3（HP ≦ 166）: 噴泉 30f ＋ 火の粉 50f ＋ 噴石リング 150f
// ─────────────────────────────────────────────────────────────
//
// 溶岩噴泉の傾き：プレイヤーの水平位置に応じて ±15° 傾けることで
// 「追いかけてくる溶岩」のような自然な誘導感を出す。
// ================================================================
void EnemyPat_Volcano_Claude()
{
    // 噴石リングは毎発射ごとに 22.5° 回転させて変化を出す
    static double ringRotation;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 450.0; // 画面下部：噴火口がちょうど見える高さ
        enemy.maxHp = enemy.hp = 1800;
        ringRotation = 0.0;
    }
    enemy.hp--;

    // ── HPに応じたフェーズ判定 ─────────────────────────────────
    int phase;
    if (enemy.hp > enemy.maxHp * 2 / 3) phase = 1;
    else if (enemy.hp > enemy.maxHp / 3)     phase = 2;
    else                                      phase = 3;

    // ── 各弾幕の発射フラグを立てる ────────────────────────────
    bool doFountain = false;
    bool doEmber = false;
    bool doRing = false;

    switch (phase) {
    case 1:
        doFountain = (count % 60 == 0);
        break;
    case 2:
        doFountain = (count % 45 == 0);
        doEmber = (count % 80 == 0); // 噴泉と異なる周期で重なりが変化し続ける
        break;
    default: // phase == 3
        doFountain = (count % 30 == 0);
        doEmber = (count % 50 == 0);
        doRing = (count % 150 == 0);
        break;
    }

    // ── 溶岩噴泉の生成 ──────────────────────────────────────────
    if (doFountain) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLavaFountain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        // 真上（-PI/2）を基準に、プレイヤーの水平位置に応じて最大 ±15° 傾ける
        double lean = (player.x - enemy.x) / 480.0 * (DX_PI / 12.0);
        pEnemyShotSet->muki = -DX_PI / 2.0 + lean;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ── 火の粉散布の生成 ─────────────────────────────────────────
    if (doEmber) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotEmberBurst;
        pEnemyShotSet->x = enemy.x + (GetRand(60) - 30); // 噴火口付近でランダムにずらす
        pEnemyShotSet->y = enemy.y - 10.0;
        pEnemyShotSet->muki = GetRand(360) / 180.0 * DX_PI; // 毎回ランダムに回転

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ── 噴石リングの生成 ─────────────────────────────────────────
    if (doRing) {
        ringRotation += DX_PI / 8.0; // 発射のたびに 22.5° 回転して変化をつける

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPyroclasticRing;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = ringRotation;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}