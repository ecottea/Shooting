// enemyPat_RyuuseiTsuiraku.cpp
// 隕石弾幕パターン：流星墜落
//
// 専用の隕石スプライトは存在しないため、大玉・中玉・小玉・鱗弾・短レーザーなど
// 既存素材を「密集させた塊」として組み合わせ、隕石本体・尾・着弾警告・衝撃波を表現する。
//
// 構成（無限ループ、1波あたり隕石5個、460フレーム周期）：
//   Phase1 予兆    : 隕石本体（大玉+小玉/中玉の塊）が画面上部から出現し、
//                    後方に短レーザーの尾を引きながら降下を開始する
//   Phase2 落下    : 二次関数的に加速しながら落下する間、周期的に鱗弾の破片が
//                    自機狙い3wayで表面から剥離していく
//   Phase3 着弾警告: 着弾点を中心に小玉のリングが出現し、赤白点滅で警告する
//   Phase4 衝突    : 着弾の瞬間、隕石本体とリングがそれぞれ元々あった位置から
//                    放射状に加速飛散（衝撃波）し、同時に着弾点から
//                    自機狙い5wayのフィニッシュ弾を発射する
//
// 敵本体の関数名は仕様に従い void EnemyPat_Meteor_Claude() とする。
// count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
// 画面外弾の消去はメインルーチン任せとし、本ファイルでは一切行わない。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の色一覧: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
namespace {
    constexpr int COL_RED = 0;
    constexpr int COL_YELLOW = 1;
    constexpr int COL_CYAN = 3;
    constexpr int COL_WHITE = 6;
    constexpr int COL_MAGENTA = 5;
    constexpr int COL_ORANGE = 8;

    // 弾の役割 (pShot->param_i[1] に格納)
    constexpr int ROLE_CORE = 0;   // 隕石本体を構成する塊
    constexpr int ROLE_RING = 1;   // 着弾警告リング → 衝撃波
    constexpr int ROLE_DEBRIS = 2; // 落下中に剥離する破片／自機狙い直進弾
}

// リストへ1発追加して繋ぎ直すだけの共通処理
static sEnemyShot* SpawnShot(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->margin = 480;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 指定フレーム時点での隕石本体の中心座標（落下軌道、二次イーズインで加速）
static void ClusterCenterAt(sEnemyShotSet* pSet, int atCount, double* outX, double* outY)
{
    int impactFrame = pSet->param_i[0];
    double progress = (double)atCount / (double)impactFrame;
    if (progress < 0.0) progress = 0.0;
    if (progress > 1.0) progress = 1.0;
    double eased = progress * progress;
    *outX = pSet->x + (pSet->param_d[0] - pSet->x) * eased;
    *outY = pSet->y + (pSet->param_d[1] - pSet->y) * eased;
}

// 隕石本体(ROLE_CORE)／警告リング(ROLE_RING)1発の初期化
// 塊中心からのオフセットで形を作り、生成時点を基準にした「着弾までの残りフレーム」を持たせる
static void InitCoreOrRingShot(sEnemyShot* pShot, sEnemyShotSet* pSet, double offsetX, double offsetY, int role, int kind)
{
    pShot->kind = kind;
    pShot->param_d[0] = offsetX;
    pShot->param_d[1] = offsetY;
    pShot->param_i[0] = pSet->param_i[0] - pSet->count; // このシステム視点での着弾までの残りフレーム
    pShot->param_i[1] = role;
    // x, y は下の更新ループで同じフレーム内に計算されるため、ここでは仮値のままでよい
    pShot->x = pSet->x;
    pShot->y = pSet->y;
}

// 弾幕：隕石本体＋尾＋警告リング＋破片＋衝撃波
static void ShotMeteorCluster(sEnemyShotSet* pSet)
{
    int impactFrame = pSet->param_i[0];
    int ringSpawnFrame = pSet->param_i[1];

    // --- 初回：隕石本体（塊）と尾を生成 ---
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double travelDir = atan2(pSet->param_d[1] - pSet->y, pSet->param_d[0] - pSet->x);

        // 中心の核（大玉）
        {
            double ox = (GetRand(60) - 30) / 10.0; // -3.0〜3.0の微小ジッタ
            double oy = (GetRand(60) - 30) / 10.0;
            sEnemyShot* pShot = SpawnShot(pSet);
            InitCoreOrRingShot(pShot, pSet, ox, oy, ROLE_CORE, img_enemyShotLargeBall[COL_ORANGE]);
        }

        // 表面の岩塊（小玉/中玉、いびつな円周状に配置してゴツゴツした塊に見せる）
        for (int i = 0; i < 14; i++) {
            double angle = i * (2.0 * DX_PI / 14.0) + (GetRand(20) - 10) / 100.0;
            double radius = 6.0 + GetRand(10);
            double ox = radius * cos(angle);
            double oy = radius * sin(angle);
            int color = (i % 3 == 0) ? COL_ORANGE : (i % 3 == 1 ? COL_RED : COL_YELLOW);
            int kind = (i % 4 == 0) ? img_enemyShotMediumBall[color] : img_enemyShotSmallBall[color];
            sEnemyShot* pShot = SpawnShot(pSet);
            InitCoreOrRingShot(pShot, pSet, ox, oy, ROLE_CORE, kind);
        }

        // 後方に流れる尾（短レーザーを進行方向と逆向きに配置して燃え尽きる筋を表現）
        for (int i = 0; i < 2; i++) {
            double dist = 26.0 + i * 16.0 + GetRand(8);
            double ox = -cos(travelDir) * dist + (GetRand(10) - 5);
            double oy = -sin(travelDir) * dist + (GetRand(10) - 5);
            int color = (i == 0) ? COL_YELLOW : COL_ORANGE;
            sEnemyShot* pShot = SpawnShot(pSet);
            InitCoreOrRingShot(pShot, pSet, ox, oy, ROLE_CORE, img_enemyShotLaser[color]);
        }
    }

    // --- 着弾警告リング生成 ---
    if (pSet->count == ringSpawnFrame) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 10; i++) {
            double angle = i * (2.0 * DX_PI / 10.0);
            double ox = 42.0 * cos(angle);
            double oy = 42.0 * sin(angle);
            sEnemyShot* pShot = SpawnShot(pSet);
            InitCoreOrRingShot(pShot, pSet, ox, oy, ROLE_RING, img_enemyShotSmallBall[COL_RED]);
        }
    }

    // --- 落下中の破片剥離（自機狙い3way） ---
    if (pSet->count > 20 && pSet->count < ringSpawnFrame - 20 && pSet->count % 24 == 10) {
        double cx, cy;
        ClusterCenterAt(pSet, pSet->count, &cx, &cy);
        double baseAngle = atan2(player.y - cy, player.x - cx);
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pShot = SpawnShot(pSet);
            pShot->kind = img_enemyShotScale[COL_ORANGE];
            pShot->param_i[1] = ROLE_DEBRIS;
            pShot->param_d[2] = cx;
            pShot->param_d[3] = cy;
            pShot->param_d[4] = baseAngle + i * 0.28;
            pShot->param_d[5] = 2.6 + GetRand(5) / 10.0;
            pShot->x = cx;
            pShot->y = cy;
        }
    }

    // --- 着弾の瞬間：効果音とフィニッシュ5way ---
    if (pSet->count == impactFrame) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pSet->param_d[1], player.x - pSet->param_d[0]);
        for (int i = -2; i <= 2; i++) {
            sEnemyShot* pShot = SpawnShot(pSet);
            pShot->kind = img_enemyShotBullet[COL_MAGENTA];
            pShot->param_i[1] = ROLE_DEBRIS;
            pShot->param_d[2] = pSet->param_d[0];
            pShot->param_d[3] = pSet->param_d[1];
            pShot->param_d[4] = baseAngle + i * 0.18;
            pShot->param_d[5] = 3.2;
            pShot->x = pSet->param_d[0];
            pShot->y = pSet->param_d[1];
        }
    }

    // --- 全弾の位置更新（役割ごとに数式で直接計算し、加算積分は行わない） ---
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[1] == ROLE_DEBRIS) {
            // 破片／フィニッシュ弾：自身の発生位置から直進する単純な自機狙い弾
            double sx = pShot->param_d[2];
            double sy = pShot->param_d[3];
            double mk = pShot->param_d[4];
            double sp = pShot->param_d[5];
            pShot->x = sx + sp * cos(mk) * pShot->count;
            pShot->y = sy + sp * sin(mk) * pShot->count;
            pShot->muki = mk; // 進行方向を向かせる
        }
        else {
            // 隕石本体／警告リング：塊中心からのオフセット位置 → 着弾後は放射状に飛散
            int framesUntilImpact = pShot->param_i[0];
            double offsetX = pShot->param_d[0];
            double offsetY = pShot->param_d[1];

            if (pShot->count < framesUntilImpact) {
                double progress = (double)pShot->count / (double)framesUntilImpact;
                double eased = progress * progress;
                double cx = pSet->x + (pSet->param_d[0] - pSet->x) * eased;
                double cy = pSet->y + (pSet->param_d[1] - pSet->y) * eased;

                pShot->x = cx + offsetX;
                pShot->y = cy + offsetY;

                if (pShot->param_i[1] == ROLE_RING) {
                    pShot->muki = atan2(offsetY, offsetX); // リングは中心から外向きを向く

                    // 赤白点滅で着弾を警告
                    bool flashOn = ((pShot->count / 8) % 2 == 0);
                    pShot->kind = flashOn ? img_enemyShotSmallBall[COL_RED] : img_enemyShotSmallBall[COL_WHITE];
                }
                else {
                    // 隕石本体は塊全体の落下方向を向く
                    pShot->muki = atan2(pSet->param_d[1] - pSet->y, pSet->param_d[0] - pSet->x);
                }
            }
            else {
                double et = pShot->count - framesUntilImpact;
                double angle = atan2(offsetY, offsetX);
                double dist = (pShot->param_i[1] == ROLE_RING)
                    ? (2.2 * et + 0.02 * et * et)   // 警告リング → 衝撃波は少し速く広がる
                    : (1.7 * et + 0.014 * et * et); // 隕石本体の破片飛散

                pShot->x = pSet->param_d[0] + offsetX + dist * cos(angle);
                pShot->y = pSet->param_d[1] + offsetY + dist * sin(angle);
                pShot->muki = angle; // 爆散後は飛散方向を向く

                if (pShot->param_i[1] == ROLE_RING && et < 1.0) {
                    pShot->kind = img_enemyShotMediumBall[COL_CYAN]; // 衝撃波として一度だけ再着色
                }
            }
        }

        pShot = pShot->next;
    }
}

// 隕石1個を生成して弾幕セットとして登録
static void SpawnMeteor(double targetX, double targetY, int impactFrame)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotMeteorCluster;
    pSet->x = 50.0 + GetRand(380);       // 出現位置（画面上部の外側、ランダム）
    pSet->y = -20.0;
    pSet->param_d[0] = targetX;          // 着弾点X
    pSet->param_d[1] = targetY;          // 着弾点Y
    pSet->param_i[0] = impactFrame;      // 着弾までのフレーム数
    pSet->param_i[1] = impactFrame - 60; // 警告リング生成フレーム（着弾60F前）

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// 敵本体：隕石雨を周期的に降らせるボス
void EnemyPat_Meteor_Claude()
{
    constexpr int WAVE_PERIOD = 460;
    constexpr int METEOR_IMPACT_FRAME = 200;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 数式で直接計算する緩やかな左右・上下の揺れ（加算積分はしない）
        enemy.x = 240.0 + 60.0 * sin(count * 0.01);
        enemy.y = 40.0 + 8.0 * sin(count * 0.017);
    }

    // 460フレーム周期で、70フレームおきに5個の隕石を降らせる（無限ループ）
    int wc = count % WAVE_PERIOD;
    if (wc <= 280 && wc % 70 == 0) {
        double targetX = 60.0 + GetRand(360);
        double targetY = 380.0 + GetRand(50);
        SpawnMeteor(targetX, targetY, METEOR_IMPACT_FRAME);
    }
}