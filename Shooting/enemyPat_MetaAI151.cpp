#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 東方地霊殿 サブタレイニアンローズ
// ============================================================

// 角度正規化 [0, 2PI)
static inline double NormalizeAngle(double a) {
    a = fmod(a, 2.0 * DX_PI);
    if (a < 0) a += 2.0 * DX_PI;
    return a;
}

// セクタ判定 (start <= angle <= start+B) ラップアラウンド対応
static inline bool IsInSector(double angle, double start, double B) {
    // angle, start は共に [0,2PI) に正規化済み想定
    double end = start + B;
    if (end < 2.0 * DX_PI) {
        return angle >= start && angle <= end;
    }
    else {
        double end2 = end - 2.0 * DX_PI;
        return angle >= start || angle <= end2;
    }
}

// ------------------------------------------------------------
// 弾幕本体: 1つのShotSet = N way全方位の青 or 赤
// param_i[0] : 0=青, 1=赤
// param_i[1] : N way数
// param_d[0] : 弾速 V
// param_d[1] : 回転速度 A [rad/frame]
// param_d[2] : 大玉幅 B [rad]
// pEnemyShotSet->muki : 全方位の基準角(毎回ランダム)
// ------------------------------------------------------------
static void ShotSubRose(sEnemyShotSet* pEnemyShotSet)
{
    // 初回生成
    if (pEnemyShotSet->count == 0) {
        // 効果音: 青と赤で軽く変える
        if (pEnemyShotSet->param_i[0] == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        else {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        int N = pEnemyShotSet->param_i[1];
        double baseAngle = pEnemyShotSet->muki; // ランダム基準角
        double V = pEnemyShotSet->param_d[0];
        int isBlue = pEnemyShotSet->param_i[0] == 0;

        for (int i = 0; i < N; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            // N way全方位
            pShot->muki = baseAngle + (2.0 * DX_PI * i / N);
            pShot->speed = V;
            pShot->count = 0;
            // 生成直後は中玉で出しておく(次フレームから判定で切り替わる)
            if (isBlue) pShot->kind = img_enemyShotMediumBall[4]; // 青中玉 4:青
            else        pShot->kind = img_enemyShotMediumBall[0]; // 赤中玉 0:赤

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 毎フレーム更新: 移動 + 大玉/中玉切り替え
    // 中心から見た偏角 = muki (放射状に飛ぶため) で判定
    // 仕様: 青は [count*A , count*A+B], 赤は [-(count*A+B) , -(count*A)]
    // count はグローバルのフレームカウンタを使用し、全ShotSetで同期回転させることで薔薇模様を作る
    double A = pEnemyShotSet->param_d[1];
    double B = pEnemyShotSet->param_d[2];
    int isBlue = pEnemyShotSet->param_i[0] == 0;

    double sectorStart;
    if (isBlue) {
        sectorStart = (double)count * A;                 // count*A
    }
    else {
        sectorStart = -(double)count * A - B;            // -(count*A+B)
    }
    sectorStart = NormalizeAngle(sectorStart);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        double ang = NormalizeAngle(pShot->muki); // 中心から見た偏角

        bool inLarge = IsInSector(ang, sectorStart, B);

        if (A < 1.8 * DX_PI / 180.0) {
            if (isBlue) {
                if (inLarge) pShot->kind = img_enemyShotLargeBall[4];  // 青大玉
                else         pShot->kind = img_enemyShotMediumBall[4]; // 青中玉
            }
            else {
                if (inLarge) pShot->kind = img_enemyShotLargeBall[0];  // 赤大玉
                else         pShot->kind = img_enemyShotMediumBall[0]; // 赤中玉
            }
        }
        else {
            if (isBlue) {
                if (inLarge) pShot->kind = img_enemyShotLargeBall[4];
                else         pShot->kind = img_enemyShotSmallBall[4];
            }
            else {
                if (inLarge) pShot->kind = img_enemyShotLargeBall[0];
                else         pShot->kind = img_enemyShotSmallBall[0];
            }
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体: EnemyPat_SubterraneanRose_MetaAI
// HPが1/4削れる度に休止して T,V,A,B を変化
// ------------------------------------------------------------
void EnemyPat_SubterraneanRose_MetaAI()
{
    // フェーズ別パラメータテーブル
    // T: 発射間隔[frame], V: 弾速, A: 回転速度[deg/frame], B: 大玉幅[deg]
    static const int   T_TABLE[4] = { 30, 20, 14, 8 };
    static const double V_TABLE[4] = { 1.8, 2.4, 3.0, 3.6 };
    static const double A_DEG_TABLE[4] = { 0.8, 1.2, 1.8, 2.5 };
    static const double B_DEG_TABLE[4] = { 70.0, 60.0, 50.0, 40.0 };
    static const int   BREAK_FRAMES = 90; // 休止時間

    static int    phase;       // 0..3
    static int    breakTimer;  // >0で休止中
    static int    curT;
    static double curV;
    static double curA; // rad
    static double curB; // rad
    static int    isBlueTurn; // 次に撃つ色 0=青,1=赤
    static const int N_WAY = 60-5;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0; // 中央寄り上部に固定
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        breakTimer = 0;
        curT = T_TABLE[0];
        curV = V_TABLE[0];
        curA = A_DEG_TABLE[0] * DX_PI / 180.0;
        curB = B_DEG_TABLE[0] * DX_PI / 180.0;
        isBlueTurn = 0; // 青から開始
    }

    // 休止中処理
    if (breakTimer > 0) {
        breakTimer--;
        if (breakTimer == 0) {
            // 休止明け: 新フェーズのパラメータ適用
            curT = T_TABLE[phase];
            curV = V_TABLE[phase];
            curA = A_DEG_TABLE[phase] * DX_PI / 180.0;
            curB = B_DEG_TABLE[phase] * DX_PI / 180.0;
        }
        // 敵は中央に留まる
        enemy.x = 240.0;
        enemy.y = 50.0;
        return;
    }

    // HP閾値でフェーズ移行判定
    // 400 -> 300 -> 200 -> 100
    if (phase == 0 && enemy.hp <= 150) {
        phase = 1;
        breakTimer = BREAK_FRAMES;
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        return;
    }
    if (phase == 1 && enemy.hp <= 100) {
        phase = 2;
        breakTimer = BREAK_FRAMES;
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        return;
    }
    if (phase == 2 && enemy.hp <= 50) {
        phase = 3;
        breakTimer = BREAK_FRAMES;
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        return;
    }

    // 通常時: 中央に固定 (少し揺らしても良いが薔薇を崩さないため固定)
    enemy.x = 240.0;
    enemy.y = 50.0;

    // Tフレーム間隔で交互に発射
    if (count % curT == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSubRose;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        // 基準角は毎回ランダム: GetRand(x)は0..x inclusiveなので359で0-359度
        double randDeg = (double)GetRand(359);
        if (phase == 3) randDeg = 0;
        pSet->muki = randDeg * DX_PI / 180.0;
        pSet->kind = 0;

        pSet->param_i[0] = isBlueTurn; // 色
        pSet->param_i[1] = N_WAY;
        pSet->param_d[0] = curV;
        pSet->param_d[1] = curA;
        pSet->param_d[2] = curB;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        isBlueTurn = 1 - isBlueTurn; // 交互
    }
}