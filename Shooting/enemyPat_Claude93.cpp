// enemyPat_SantouJujiro.cpp
// 「三灯十字路」 信号機モチーフの弾幕パターン
//
// 構成：
//   ・起動時(count==1)に信号機ハウジング(外枠+赤黄緑3灯)を1度だけ組み上げ、以後は常駐させたまま
//     フェーズに応じて灯りの色だけを切り替える。
//   ・INTRO_LEN フレームでハウジングが展開したのち、以下の4フェーズを CYCLE_LEN フレーム周期で
//     無限ループする(自機がハウジング本体=enemyを撃破するまで継続)。
//       フェーズ1 青信号  : 縦レーンの弾幕が流れ落ちる。空きレーン(安全地帯)が緩やかに横移動。
//       フェーズ2 黄信号  : 黄灯が点滅し、警告リングと自機狙い3wayを交互に発射。
//       フェーズ3 赤信号  : 全幅の弾の壁が段々と降下。横断歩道に見立てた抜け道がジグザグに移動。
//       フェーズ4 全赤点滅: 3灯が同時に赤で点滅し、ハウジング中心から放射状の弾が一斉に解放される。
//
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  信号フェーズの定義
// ============================================================
enum class SignalPhase { Intro, Green, Yellow, Red, Finale };

static const int INTRO_LEN = 90;   // ハウジング組み立て時間

static const int GREEN_LEN = 200;  // 青信号の長さ
static const int YELLOW_LEN = 100;  // 黄信号の長さ
static const int RED_LEN = 220;  // 赤信号の長さ
static const int FINALE_LEN = 80;   // 全赤点滅の長さ

static const int YELLOW_START = GREEN_LEN;                 // 200
static const int RED_START = YELLOW_START + YELLOW_LEN;    // 300
static const int FINALE_START = RED_START + RED_LEN;       // 520
static const int CYCLE_LEN = FINALE_START + FINALE_LEN;    // 600

static const double HOUSE_CX = 240.0; // ハウジング中心(画面は480x480)
static const double HOUSE_CY = 100.0;

// count(グローバルフレーム数)から現在のフェーズと、そのフェーズ内での相対フレーム数を求める
static SignalPhase GetSignalPhase(int c, int* outLocalCyc)
{
    if (c < INTRO_LEN) {
        if (outLocalCyc) *outLocalCyc = c;
        return SignalPhase::Intro;
    }
    int cyc = (c - INTRO_LEN) % CYCLE_LEN;
    if (cyc < YELLOW_START) {
        if (outLocalCyc) *outLocalCyc = cyc;
        return SignalPhase::Green;
    }
    if (cyc < RED_START) {
        if (outLocalCyc) *outLocalCyc = cyc - YELLOW_START;
        return SignalPhase::Yellow;
    }
    if (cyc < FINALE_START) {
        if (outLocalCyc) *outLocalCyc = cyc - RED_START;
        return SignalPhase::Red;
    }
    if (outLocalCyc) *outLocalCyc = cyc - FINALE_START;
    return SignalPhase::Finale;
}

// ============================================================
//  共通ヘルパー：弾セット/弾の生成
// ============================================================
static sEnemyShotSet* SpawnShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki = 0.0, int kind = 0)
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

static sEnemyShot* AddShot(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ============================================================
//  ハウジング(外枠+赤黄緑3灯)：常駐する信号機本体
// ============================================================
static void ShotHousing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 位置：中心から目標座標(param_d)へイーズアウトで展開し、組み上がったら静止し続ける
        double t = pShot->count / (double)INTRO_LEN;
        if (t > 1.0) t = 1.0;
        double ease = 1.0 - pow(1.0 - t, 3.0);
        pShot->x = HOUSE_CX + pShot->param_d[0] * ease;
        pShot->y = HOUSE_CY + pShot->param_d[1] * ease;

        // 色：役割(param_i[0])と現在の信号フェーズから決定
        int role = pShot->param_i[0]; // 0:外枠 1:赤灯 2:黄灯 3:緑灯
        if (role == 0) {
            pShot->kind = img_enemyShotSmallBall[6]; // 外枠は白
        }
        else {
            int localCyc = 0;
            SignalPhase phase = GetSignalPhase(count, &localCyc);
            int litColor = 7; // 既定は消灯(黒)
            switch (phase) {
            case SignalPhase::Green:
                if (role == 3) litColor = 2; // 緑灯点灯
                break;
            case SignalPhase::Yellow:
                if (role == 2) litColor = ((localCyc / 10) % 2 == 0) ? 1 : 7; // 黄灯点滅
                break;
            case SignalPhase::Red:
                if (role == 1) litColor = 0; // 赤灯点灯
                break;
            case SignalPhase::Finale:
                litColor = ((localCyc / 6) % 2 == 0) ? 0 : 7; // 3灯同時に赤で点滅(全赤点滅)
                break;
            default:
                break; // 組み立て中(Intro)は全消灯のまま
            }
            pShot->kind = img_enemyShotSmallBall[litColor];
        }

        pShot = pShot->next;
    }
}

static void SpawnHousing()
{
    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

    sEnemyShotSet* pSet = SpawnShotSet(ShotHousing, HOUSE_CX, HOUSE_CY);

    // 外枠(幅100x高さ180の矩形を10px間隔の小玉で形成)
    for (int i = -5; i <= 5; i++) {
        sEnemyShot* pTop = AddShot(pSet);
        pTop->param_d[0] = i * 10.0; pTop->param_d[1] = -90.0; pTop->param_i[0] = 0;

        sEnemyShot* pBottom = AddShot(pSet);
        pBottom->param_d[0] = i * 10.0; pBottom->param_d[1] = 90.0; pBottom->param_i[0] = 0;
    }
    for (int i = -8; i <= 8; i++) {
        sEnemyShot* pLeft = AddShot(pSet);
        pLeft->param_d[0] = -50.0; pLeft->param_d[1] = i * 10.0; pLeft->param_i[0] = 0;

        sEnemyShot* pRight = AddShot(pSet);
        pRight->param_d[0] = 50.0; pRight->param_d[1] = i * 10.0; pRight->param_i[0] = 0;
    }

    // 3灯(各12発のリングで円を表現)。上から赤・黄・緑の順に配置
    const double dyList[3] = { -60.0, 0.0, 60.0 };
    for (int s = 0; s < 3; s++) {
        for (int i = 0; i < 12; i++) {
            double ang = DX_PI * 2.0 * i / 12.0;
            sEnemyShot* p = AddShot(pSet);
            p->param_d[0] = cos(ang) * 22.0;
            p->param_d[1] = dyList[s] + sin(ang) * 22.0;
            p->param_i[0] = s + 1; // 1:赤 2:黄 3:緑
        }
    }
}

// ============================================================
//  フェーズ1 青信号：縦レーンの流れ弾(空きレーン=安全地帯が横移動)
// ============================================================
static void ShotLaneStream(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0];
        pShot->y = pShot->param_d[1] + pShot->param_d[2] * pShot->count;
        pShot = pShot->next;
    }
}

static void SpawnLaneWave(int waveIndex)
{
    const int laneCount = 12;
    const double laneSpacing = 38.0;
    const double laneX0 = 30.0;
    int safeLaneA = (waveIndex / 5) % laneCount;
    int safeLaneB = (safeLaneA + 6) % laneCount;

    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

    sEnemyShotSet* pSet = SpawnShotSet(ShotLaneStream, HOUSE_CX, HOUSE_CY);
    for (int lane = 0; lane < laneCount; lane++) {
        if (lane == safeLaneA || lane == safeLaneB) continue;
        sEnemyShot* p = AddShot(pSet);
        p->kind = img_enemyShotBullet[3]; // シアン
        p->muki = DX_PI / 2.0;            // 見た目を下向きに
        p->param_d[0] = laneX0 + lane * laneSpacing;
        p->param_d[1] = -20.0;
        p->param_d[2] = 3.0;
    }
}

// ============================================================
//  フェーズ2 黄信号：警告リング + 自機狙い3way
// ============================================================
static void ShotWarningRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double r = pShot->param_d[0] + pShot->param_d[1] * pShot->count;
        double ang = pShot->param_d[2];
        pShot->x = pEnemyShotSet->x + cos(ang) * r;
        pShot->y = pEnemyShotSet->y + sin(ang) * r;
        pShot = pShot->next;
    }
}

static void SpawnWarningRing()
{
    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

    sEnemyShotSet* pSet = SpawnShotSet(ShotWarningRing, HOUSE_CX, HOUSE_CY);
    const int n = 28;
    for (int i = 0; i < n; i++) {
        sEnemyShot* p = AddShot(pSet);
        p->kind = img_enemyShotMediumBall[1]; // 黄
        double ang = DX_PI * 2.0 * i / n;
        p->param_d[0] = 5.0;  // 開始半径
        p->param_d[1] = 2.5;  // 拡大速度
        p->param_d[2] = ang;
    }
}

static void ShotAimedBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double speed = pShot->param_d[0];
        pShot->x = pEnemyShotSet->x + speed * cos(pShot->muki) * pShot->count;
        pShot->y = pEnemyShotSet->y + speed * sin(pShot->muki) * pShot->count;
        pShot = pShot->next;
    }
}

static void SpawnAimedBurst()
{
    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

    double baseAng = atan2(player.y - HOUSE_CY, player.x - HOUSE_CX);
    sEnemyShotSet* pSet = SpawnShotSet(ShotAimedBurst, HOUSE_CX, HOUSE_CY);
    const double spreadList[3] = { -0.18, 0.0, 0.18 };
    for (int i = 0; i < 3; i++) {
        sEnemyShot* p = AddShot(pSet);
        p->kind = img_enemyShotBullet[1]; // 黄
        p->muki = baseAng + spreadList[i];
        p->param_d[0] = 3.5;
    }
}

// ============================================================
//  フェーズ3 赤信号：全幅の弾の壁(横断歩道の抜け道がジグザグに移動)
// ============================================================
static void ShotWallRow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0];
        pShot->y = pEnemyShotSet->y + pShot->param_d[1] * pShot->count;
        pShot = pShot->next;
    }
}

static void SpawnWallRow(int rowIndex)
{
    double gapCenterX = HOUSE_CX + 150.0 * sin(DX_PI * 2.0 * rowIndex / 8.0);
    const double xStart = 10.0, xEnd = 470.0, spacing = 8.0;
    const double gapHalf = 30.0;

    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

    sEnemyShotSet* pSet = SpawnShotSet(ShotWallRow, HOUSE_CX, -10.0);
    for (double x = xStart; x <= xEnd; x += spacing) {
        if (fabs(x - gapCenterX) < gapHalf) continue; // 横断歩道(抜け道)
        sEnemyShot* p = AddShot(pSet);
        p->kind = img_enemyShotSmallBall[0]; // 赤
        p->param_d[0] = x;
        p->param_d[1] = 2.2;
    }
}

// ============================================================
//  フェーズ4 全赤点滅：ハウジング中心からの放射状バースト
// ============================================================
static void ShotRadialBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double speed = pShot->param_d[0];
        double ang = pShot->param_d[1];
        pShot->x = pEnemyShotSet->x + speed * pShot->count * cos(ang);
        pShot->y = pEnemyShotSet->y + speed * pShot->count * sin(ang);
        pShot = pShot->next;
    }
}

static void SpawnRadialBurst()
{
    if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

    sEnemyShotSet* pSet = SpawnShotSet(ShotRadialBurst, HOUSE_CX, HOUSE_CY);
    const int n = 48;
    const int colorList[3] = { 0, 1, 2 }; // 赤黄緑
    for (int i = 0; i < n; i++) {
        sEnemyShot* p = AddShot(pSet);
        p->kind = img_enemyShotMediumBall[colorList[i % 3]];
        double ang = DX_PI * 2.0 * i / n;
        p->param_d[0] = 3.0 + (i % 3) * 0.3;
        p->param_d[1] = ang;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_TrafficLight_Claude()
{
    if (count == 1) {
        enemy.x = HOUSE_CX;
        enemy.y = HOUSE_CY;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        SpawnHousing();
    }

    int localCyc = 0;
    SignalPhase phase = GetSignalPhase(count, &localCyc);

    switch (phase) {
    case SignalPhase::Green:
        if (localCyc % 8 == 0) SpawnLaneWave(localCyc / 8);
        break;
    case SignalPhase::Yellow:
        if (localCyc % 30 == 0) SpawnWarningRing();
        if (localCyc % 20 == 0) SpawnAimedBurst();
        break;
    case SignalPhase::Red:
        if (localCyc % 20 == 0) SpawnWallRow(localCyc / 20);
        break;
    case SignalPhase::Finale:
        if (localCyc == 0) SpawnRadialBurst();
        break;
    default:
        break; // Introは組み立てのみ
    }
}