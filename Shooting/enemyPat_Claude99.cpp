// enemyPat_DopplerCrescent.cpp
// 音速跳躍波 - ドップラー・クレセント
//
// 移動する波源（ボス）が等間隔でリング弾（波面）を放つと、
// 進行方向側では波面の中心同士が近づくため弾が密集し、
// 後方では波面が離れるため疎になる――ドップラー効果の視覚化。
// 移動速度が波の拡散速度を超えると、波面同士が重なり合って
// 円錐状の衝撃波（マッハコーン）を形成する。
//
// フェーズ構成:
//   1. 静止波源フェーズ … ボス静止、波は同心円状に等間隔で広がる（基準状態）
//   2. 移動波源フェーズ … ボスが加速横移動、進行方向側で波が圧縮される
//   3. 超音速フェーズ   … ボスが高速ダッシュを2回行い、マッハコーンが発生
//   4. 波源停止フェーズ … ボス中央で停止、最後の大波を解放して締める

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static constexpr double RING_SPEED = 1.3;                    // 波面（リング）の拡散速度
static constexpr double MACH_EXPAND_SPEED = 2.0;                    // 衝撃波セグメントの拡張速度
static constexpr double MACH_CONE_HALF_ANGLE = 35.0 * DX_PI / 180.0;  // 円錐半頂角（視認性重視で誇張した値）
static constexpr int    RING_BULLET_NUM = 28;                     // 1つの波面を構成する弾数

// ============================================================
//  弾セット生成ヘルパー
// ============================================================
static sEnemyShotSet* SpawnShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind = 0)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// ============================================================
//  弾幕：波源リング
//  発射時のボス座標を中心に固定し、そこから同心円状に等速拡散する。
//  ボスが移動しながら一定間隔で発射することで、
//  中心同士の距離差＝ドップラー効果の疎密がそのまま可視化される。
//
//  param_d[0], param_d[1] : 発射時に固定された中心座標 (x, y)
//  param_d[2]             : このリング上での弾の角度（固定・位相オフセット込み）
//  param_d[3]             : 位相オフセット（複数リングを同時に重ねて撃つ際、
//                            弾の隙間をずらして密度を上げるために使用。既定値0）
//  kind(セット)           : 基本カラー（0〜8）
// ============================================================
static void ShotRingWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double centerX = pEnemyShotSet->x;
        double centerY = pEnemyShotSet->y;
        double phaseOffset = pEnemyShotSet->param_d[3];
        int    baseColor = pEnemyShotSet->kind % 9;

        for (int i = 0; i < RING_BULLET_NUM; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = DX_PI * 2.0 * i / RING_BULLET_NUM + phaseOffset;

            pEnemyShot->x = centerX;
            pEnemyShot->y = centerY;
            pEnemyShot->muki = angle; // 波面が広がっていく向き
            pEnemyShot->speed = 0.0;  // 位置は公式駆動のため未使用

            pEnemyShot->param_d[0] = centerX;
            pEnemyShot->param_d[1] = centerY;
            pEnemyShot->param_d[2] = angle;

            int color = (baseColor + (i % 2)) % 9; // 波面内に2色のニュアンスを持たせる
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count;
        pShot->x = pShot->param_d[0] + RING_SPEED * t * cos(pShot->param_d[2]);
        pShot->y = pShot->param_d[1] + RING_SPEED * t * sin(pShot->param_d[2]);
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：自機狙い扇状弾
//  発射時のボス座標を起点に固定し、発射時の自機座標へ向けた
//  基準角(muki)を中心にway数ぶん扇状に広げて直進する。
//
//  param_i[0] : way数
//  param_d[0] : 扇の開き角（ラジアン）
//  param_d[1] : 弾速
//  param_i[1] : 色（0〜8）
// ============================================================
static void ShotAimedFan(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int    way = pEnemyShotSet->param_i[0];
        double spread = pEnemyShotSet->param_d[0];
        double baseAngle = pEnemyShotSet->muki;
        double shotSpeed = pEnemyShotSet->param_d[1];
        int    color = pEnemyShotSet->param_i[1] % 9;

        for (int i = 0; i < way; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = (way == 1) ? baseAngle
                : baseAngle + spread * ((double)i / (way - 1) - 0.5);

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0;

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = angle;
            pEnemyShot->param_d[3] = shotSpeed;

            pEnemyShot->kind = img_enemyShotBullet[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count;
        pShot->x = pShot->param_d[0] + pShot->param_d[3] * t * cos(pShot->param_d[2]);
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * t * sin(pShot->param_d[2]);
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：マッハコーン（衝撃波面）
//  発射時のボス座標を頂点に固定し、進行方向の後方へ
//  円錐状に開く2本のレーザー列を伸ばす。
//  各アームは複数セグメントで構成し、頂点からの初期オフセットを
//  変えることで、拡張中も一定の長さを持つ光条として見せる。
//
//  muki(セット)           : 円錐が伸びていく基準方向（ボスの後方）
//  param_d[0], param_d[1] : 頂点座標（固定）
//  param_d[2]             : このセグメントの角度（固定）
//  param_d[3]             : このセグメントの初期オフセット（頂点からの距離）
// ============================================================
static void ShotMachCone(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double baseAngle = pEnemyShotSet->muki;
        const int    segPerArm = 8;
        const double segGap = 18.0;

        for (int arm = 0; arm < 2; arm++) {
            double angle = baseAngle + (arm == 0 ? -MACH_CONE_HALF_ANGLE : MACH_CONE_HALF_ANGLE);

            for (int s = 0; s < segPerArm; s++) {
                pEnemyShot = new sEnemyShot;

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 0.0;

                pEnemyShot->param_d[0] = pEnemyShotSet->x;
                pEnemyShot->param_d[1] = pEnemyShotSet->y;
                pEnemyShot->param_d[2] = angle;
                pEnemyShot->param_d[3] = s * segGap;

                pEnemyShot->kind = img_enemyShotLaser[3]; // シアン系で衝撃波を光らせる

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count;
        double r = pShot->param_d[3] + MACH_EXPAND_SPEED * t;
        pShot->x = pShot->param_d[0] + r * cos(pShot->param_d[2]);
        pShot->y = pShot->param_d[1] + r * sin(pShot->param_d[2]);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Doppler_Claude()
{
    // ---- フェーズ境界 ----
    constexpr int PHASE1_END = 180; // 静止波源フェーズ
    constexpr int PHASE2_END = 480; // 移動波源フェーズ
    constexpr int PHASE3A_END = 570; // 超音速フェーズ ダッシュA（右→左）
    constexpr int PHASE3_END = 660; // 超音速フェーズ ダッシュB（左→右）終了
    constexpr int PHASE4_MID = 761; // 波源停止フェーズ 追撃波

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    const int T = 800;
    int countT = count % T;

    // ---- ボスの移動 ----
    if (countT <= PHASE1_END) {
        // フェーズ1：静止 ― 波はどこまでも同心円状に等間隔で広がる（基準状態）
        enemy.x = 240.0;
        enemy.y = 80.0;
    }
    else if (countT <= PHASE2_END) {
        // フェーズ2：加速横移動 ― 進行方向側で波の中心が近づき、密な壁になる
        double t = (countT - (PHASE1_END + 1)) / (double)(PHASE2_END - PHASE1_END);
        enemy.x = 60.0 + 360.0 * t * t; // イーズイン（時間とともに加速）
        enemy.y = 80.0;
    }
    else if (countT <= PHASE3A_END) {
        // フェーズ3前半：超音速ダッシュA（右→左）
        double t = (countT - (PHASE2_END + 1)) / (double)(PHASE3A_END - PHASE2_END);
        enemy.x = 420.0 - 360.0 * t;
        enemy.y = 80.0;
    }
    else if (countT <= PHASE3_END) {
        // フェーズ3後半：超音速ダッシュB（左→右）
        double t = (countT - (PHASE3A_END + 1)) / (double)(PHASE3_END - PHASE3A_END);
        enemy.x = 60.0 + 360.0 * t;
        enemy.y = 80.0;
    }
    else {
        // フェーズ4：波源停止 ― 中央で静止し、最後の大波を解放する
        enemy.x = 240.0;
        enemy.y = 80.0;
    }

    double shotY = enemy.y + 10.0;

    // ---- フェーズ1：静止波源（30フレーム毎に同心円リング）----
    if (countT >= 1 && countT <= PHASE1_END && (countT - 1) % 30 == 0) {
        SpawnShotSet(ShotRingWave, enemy.x, shotY, 0.0, 3); // シアン系
    }

    // ---- フェーズ2：移動波源（22フレーム毎にリング＋自機狙い3way）----
    if (countT > PHASE1_END && countT <= PHASE2_END && (countT - PHASE1_END - 1) % 22 == 0) {
        SpawnShotSet(ShotRingWave, enemy.x, shotY, 0.0, 4); // 青系

        double aimAngle = atan2(player.y - shotY, player.x - enemy.x);
        sEnemyShotSet* pFan = SpawnShotSet(ShotAimedFan, enemy.x, shotY, aimAngle);
        pFan->param_i[0] = 3;                    // 3way
        pFan->param_d[0] = 25.0 * DX_PI / 180.0;  // 開き角
        pFan->param_d[1] = 2.6;                   // 弾速
        pFan->param_i[1] = 4;                     // 青系
    }

    // ---- フェーズ3：超音速（12フレーム毎にリング、15フレーム毎にマッハコーン）----
    if (countT > PHASE2_END && countT <= PHASE3_END) {
        int localCount = countT - PHASE2_END - 1;

        if (localCount % 12 == 0) {
            SpawnShotSet(ShotRingWave, enemy.x, shotY, 0.0, 0); // 赤系（圧縮された熱を表現）
        }

        if (localCount % 15 == 0) {
            // 現在どちらのダッシュ区間かで進行方向を判定し、その反対（後方）へ円錐を伸ばす
            double moveAngle = (countT <= PHASE3A_END) ? DX_PI : 0.0; // 左移動=π、右移動=0
            double baseAngle = moveAngle + DX_PI; // 進行方向の逆＝後方

            SpawnShotSet(ShotMachCone, enemy.x, shotY, baseAngle);
        }
    }

    // ダッシュ切り替わり・終了地点で自機狙い5wayのインパクト
    if (countT == PHASE3A_END || countT == PHASE3_END) {
        double aimAngle = atan2(player.y - shotY, player.x - enemy.x);
        sEnemyShotSet* pFan = SpawnShotSet(ShotAimedFan, enemy.x, shotY, aimAngle);
        pFan->param_i[0] = 5;
        pFan->param_d[0] = 40.0 * DX_PI / 180.0;
        pFan->param_d[1] = 3.0;
        pFan->param_i[1] = 0; // 赤系
    }

    // フィナーレ予告音
    if (countT == PHASE3_END - 20) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- フェーズ4：波源停止・全波面解放 ----
    if (countT == PHASE3_END + 1) {
        // 静止した瞬間、それまで圧縮されていた波が一気に押し寄せる（3リングを位相ずらしで重ねて超高密度に）
        for (int k = 0; k < 3; k++) {
            sEnemyShotSet* pRing = SpawnShotSet(ShotRingWave, enemy.x, shotY, 0.0, 6); // 白
            pRing->param_d[3] = k * (2.0 * DX_PI / (3.0 * RING_BULLET_NUM));
        }

        double aimAngle = atan2(player.y - shotY, player.x - enemy.x);
        sEnemyShotSet* pFan = SpawnShotSet(ShotAimedFan, enemy.x, shotY, aimAngle);
        pFan->param_i[0] = 7;
        pFan->param_d[0] = 60.0 * DX_PI / 180.0;
        pFan->param_d[1] = 2.4;
        pFan->param_i[1] = 6; // 白
    }

    if (countT == PHASE4_MID) {
        for (int k = 0; k < 2; k++) {
            sEnemyShotSet* pRing = SpawnShotSet(ShotRingWave, enemy.x, shotY, 0.0, 8); // 橙
            pRing->param_d[3] = k * (2.0 * DX_PI / (2.0 * RING_BULLET_NUM));
        }

        double aimAngle = atan2(player.y - shotY, player.x - enemy.x);
        sEnemyShotSet* pFan = SpawnShotSet(ShotAimedFan, enemy.x, shotY, aimAngle);
        pFan->param_i[0] = 9;
        pFan->param_d[0] = 70.0 * DX_PI / 180.0;
        pFan->param_d[1] = 2.6;
        pFan->param_i[1] = 5; // マゼンタ
    }
}