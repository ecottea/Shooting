// enemyPat_TomatoNageMatsuri.cpp
// トマト投げ祭り(トマティーナ風)をモチーフにした無限ループ4フェーズパターン
// 専用素材がないため、既存の大玉(トマト本体)・小玉(果肉飛沫)・菱形弾(果汁の飛沫)・
// 短レーザー(放水)を組み合わせて表現する。
//
// フェーズ構成(1サイクル = LOOP_PERIODフレームで無限ループ):
//   1. 荷台積み込み : 山積みの大玉(トマト)が下から迫り上がって出現し常駐する
//   2. 号砲一斉投擲 : 山から次々とトマトがベジェ曲線状の放物線を描いて自機周辺へ飛来
//   3. 乱闘スプラッシュ : トマトの着弾点で果肉(小玉)が飛び散り、自機狙いの果汁(菱形弾)も飛ぶ
//      ※フェーズ2とフェーズ3は個々のトマトの着弾タイミングで連動発生する
//   4. 放水後片付け : 左右からホース(短レーザー)が扇状に掃射し、最後に残りのトマトが
//      放射状に大放出されてフィナーレを迎え、次サイクルの積み込みへ戻る

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ================= 共通定数 =================
static const int    LOOP_PERIOD = 700 + 20;   // 1サイクルの長さ

static const int    PILE_COLS = 6;
static const int    PILE_ROWS = 5;
static const int    PILE_COUNT = PILE_COLS * PILE_ROWS; // 30個
static const double PILE_ORIGIN_X = 165.0;
static const double PILE_ORIGIN_Y = 70.0;
static const double PILE_STEP_X = 30.0;
static const double PILE_STEP_Y = 25.0;

static const int    THROW_START = 130;
static const int    THROW_END = 480;
static const int    THROW_INTERVAL = 8;
static const int    THROW_FLIGHT_FRAME = 60;

static const int    HOSE_START = 490;
static const int    HOSE_END = 650;
static const int    HOSE_INTERVAL = 3;

static const int    FINALE_TRIGGER = 660;

// サイクル内の相対フレーム位置を返す(countは1始まりのため0始まりへ補正)
static inline int CyclePos()
{
    return (count - 30) % LOOP_PERIOD;
}

// 山積みトマトのグリッド座標を求める
static void GetPileSlot(int idx, double& x, double& y)
{
    int col = idx % PILE_COLS;
    int row = idx / PILE_COLS;
    x = PILE_ORIGIN_X + col * PILE_STEP_X;
    y = PILE_ORIGIN_Y + row * PILE_STEP_Y;
}

// ================= 弾幕: 果肉・果汁の飛沫(トマト着弾スプラッシュ) =================
static void ShotSplash(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 果肉・種の飛沫(全方位、赤7:緑3の割合で小玉)
        for (int i = 0; i < 16; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = GetRand(359) / 360.0 * 2.0 * DX_PI;
            double speed = 1.0 + GetRand(200) / 100.0; // 1.0〜3.0

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = speed;
            pEnemyShot->kind = (i % 10 < 7) ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[2];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 果汁の飛沫(自機狙い4way、橙色の菱形弾)。着弾時に捕捉した自機狙い角はmukiに保存済み
        double aimAngle = pEnemyShotSet->muki;
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double offset = (i - 1.5) * (15.0 * DX_PI / 180.0);

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = aimAngle + offset;
            pEnemyShot->speed = 3.5 - 0.5;
            pEnemyShot->kind = img_enemyShotDiamond[8]; // 橙

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾、原点(着弾点)からの直進拡散(pShot->countのみで位置決定、速度積分は行わない)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double r = pShot->speed * pShot->count;
        pShot->x = pEnemyShotSet->x + r * cos(pShot->muki);
        pShot->y = pEnemyShotSet->y + r * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ================= 弾幕: 投擲されるトマト(2次ベジェ曲線の放物線軌道) =================
static void ShotThrow(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        enemy.hp--;

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤い大玉=トマト本体
        pEnemyShot->param_i[0] = 0; // 0:飛行中 1:着弾済み(画面外へ退避中)

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 制御点(始点P0/山なり経由点P1/着弾点P2)は生成時に確定済み
    double p0x = pEnemyShotSet->x, p0y = pEnemyShotSet->y;
    double p1x = pEnemyShotSet->param_d[0], p1y = pEnemyShotSet->param_d[1];
    double p2x = pEnemyShotSet->param_d[2], p2y = pEnemyShotSet->param_d[3];

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            double frac = pShot->count / (double)THROW_FLIGHT_FRAME;
            if (frac >= 1.0) frac = 1.0;
            double t = 1.0 - frac;

            double x = t * t * p0x + 2.0 * t * frac * p1x + frac * frac * p2x;
            double y = t * t * p0y + 2.0 * t * frac * p1y + frac * frac * p2y;
            double dx = 2.0 * t * (p1x - p0x) + 2.0 * frac * (p2x - p1x);
            double dy = 2.0 * t * (p1y - p0y) + 2.0 * frac * (p2y - p1y);

            pShot->x = x;
            pShot->y = y;
            pShot->muki = atan2(dy, dx);

            if (frac >= 1.0) {
                pShot->param_i[0] = 1; // 着弾

                // 着弾地点に飛沫弾幕(ShotSplash)を発生させる
                sEnemyShotSet* pSplash = new sEnemyShotSet;
                pSplash->count = 0;
                pSplash->patternFunc = ShotSplash;
                pSplash->x = x;
                pSplash->y = y;
                pSplash->muki = atan2(player.y - y, player.x - x); // 自機狙い角を保存

                pSplash->pEnemyShotHead = new sEnemyShot;
                pSplash->pEnemyShotHead->prev = pSplash->pEnemyShotHead;
                pSplash->pEnemyShotHead->next = pSplash->pEnemyShotHead;

                pSplash->prev = enemyShotSetHead.prev;
                pSplash->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSplash;
                enemyShotSetHead.prev = pSplash;
            }
        }
        else {
            // 着弾後は画面外へ退避し、メインルーチンの画面外削除に委ねる
            pShot->x = -1000.0;
            pShot->y = -1000.0;
        }
        pShot = pShot->next;
    }
}

// ================= 弾幕: 山積みトマト(荷台) =================
static void ShotPile(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < PILE_COUNT; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤い大玉
            pEnemyShot->param_i[0] = i; // 出現/退避の遅延に使うインデックス

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    int cyclePos = CyclePos();

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int idx = pShot->param_i[0];
        double targetX, targetY;
        GetPileSlot(idx, targetX, targetY);

        if (cyclePos < HOSE_START) {
            // 積み込みフェーズ：下から迫り上がる形で出現し、以後は静止
            double delay = idx * 3.0;
            double t = pShot->count - delay;
            if (t < 0.0) t = 0.0;
            double frac = t / 30.0;
            if (frac > 1.0) frac = 1.0;
            double eased = 1.0 - (1.0 - frac) * (1.0 - frac); // イーズアウト

            pShot->x = targetX;
            pShot->y = (targetY + 300.0) * (1.0 - eased) + targetY * eased;
        }
        else {
            // 放水フェーズ：残ったトマトを上方へ退避させる(片付け演出)
            double clearDelay = idx * 2.0;
            double t = (cyclePos - HOSE_START) - clearDelay;
            if (t < 0.0) t = 0.0;
            pShot->x = targetX;
            pShot->y = targetY - t * 5.0;
        }

        pShot = pShot->next;
    }
}

// ================= 弾幕: 放水(片付け) =================
static void ShotHose(sEnemyShotSet* pEnemyShotSet)
{
    // HOSE_END到達までのみ新規噴射。既存の弾は画面外まで直進し自動的に消える
    if (pEnemyShotSet->count < (HOSE_END - HOSE_START) &&
        pEnemyShotSet->count % HOSE_INTERVAL == 0) {

        double baseAngle = pEnemyShotSet->param_d[0];
        double amplitude = pEnemyShotSet->param_d[1];
        double period = pEnemyShotSet->param_d[2];
        double phaseShift = pEnemyShotSet->param_d[3];

        double sweep = baseAngle + amplitude * sin(2.0 * DX_PI * (count + phaseShift) / period);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = sweep;
        pEnemyShot->speed = 4.0;
        pEnemyShot->kind = img_enemyShotLaser[3]; // シアン(水しぶき)
        pEnemyShot->margin = 70;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double r = pShot->speed * pShot->count;
        pShot->x = pEnemyShotSet->x + r * cos(pShot->muki);
        pShot->y = pEnemyShotSet->y + r * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ================= 弾幕: フィナーレの大放出 =================
static void ShotFinaleBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 放射状に均等配置(赤6:緑4の小玉)
        for (int i = 0; i < 40; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = i * (2.0 * DX_PI / 40.0);

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = (i % 5 < 3) ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[2];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 自機狙い5way(橙の菱形弾)
        double aimAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double offset = (i - 2) * (12.0 * DX_PI / 180.0);

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = aimAngle + offset;
            pEnemyShot->speed = 4.0;
            pEnemyShot->kind = img_enemyShotDiamond[8];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double r = pShot->speed * pShot->count;
        pShot->x = pEnemyShotSet->x + r * cos(pShot->muki);
        pShot->y = pEnemyShotSet->y + r * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ================= 敵本体のパターン =================
void EnemyPat_Tomatina_Claude()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }

    // 祭りの賑わいを表す左右への緩やかな揺れ(count純関数、速度積分なし)
    enemy.x = 240.0 + 60.0 * sin(2.0 * DX_PI * count / 240.0);
    enemy.y = 40.0;

    int cyclePos = CyclePos();

    // フェーズ1: 山積みトマトの生成(サイクル開始時に1回だけ)
    if (cyclePos == 0) {
        sEnemyShotSet* pPile = new sEnemyShotSet;
        pPile->count = 0;
        pPile->patternFunc = ShotPile;
        pPile->x = 0.0;
        pPile->y = 0.0;

        pPile->pEnemyShotHead = new sEnemyShot;
        pPile->pEnemyShotHead->prev = pPile->pEnemyShotHead;
        pPile->pEnemyShotHead->next = pPile->pEnemyShotHead;

        pPile->prev = enemyShotSetHead.prev;
        pPile->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pPile;
        enemyShotSetHead.prev = pPile;
    }

    // フェーズ2: 投擲(群衆が山からトマトを取っては自機周辺へ投げ続ける)
    if (cyclePos >= THROW_START && cyclePos < THROW_END &&
        (cyclePos - THROW_START) % THROW_INTERVAL == 0) {

        int col = GetRand(PILE_COLS - 1);
        int row = GetRand(PILE_ROWS - 1);
        double originX = PILE_ORIGIN_X + col * PILE_STEP_X;
        double originY = PILE_ORIGIN_Y + row * PILE_STEP_Y;

        double destX = player.x + (GetRand(120) - 60);
        double destY = player.y + (GetRand(60) - 30);

        double ctrlX = (originX + destX) / 2.0 + (GetRand(160) - 80);
        double lowerY = (originY < destY) ? originY : destY;
        double ctrlY = lowerY - 100.0 - GetRand(80);

        sEnemyShotSet* pThrow = new sEnemyShotSet;
        pThrow->count = 0;
        pThrow->patternFunc = ShotThrow;
        pThrow->x = originX;
        pThrow->y = originY;
        pThrow->param_d[0] = ctrlX;
        pThrow->param_d[1] = ctrlY;
        pThrow->param_d[2] = destX;
        pThrow->param_d[3] = destY;

        pThrow->pEnemyShotHead = new sEnemyShot;
        pThrow->pEnemyShotHead->prev = pThrow->pEnemyShotHead;
        pThrow->pEnemyShotHead->next = pThrow->pEnemyShotHead;

        pThrow->prev = enemyShotSetHead.prev;
        pThrow->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pThrow;
        enemyShotSetHead.prev = pThrow;
    }

    // フェーズ4: 放水清掃(左右のホースが同時に開始)
    if (cyclePos == HOSE_START) {
        const double baseAngles[2] = { -60.0 * DX_PI / 180.0, -120.0 * DX_PI / 180.0 };
        const double originsX[2] = { 0.0, 480.0 };
        const double phaseShifts[2] = { 0.0, 45.0 };

        for (int i = 0; i < 2; i++) {
            sEnemyShotSet* pHose = new sEnemyShotSet;
            pHose->count = 0;
            pHose->patternFunc = ShotHose;
            pHose->x = originsX[i];
            pHose->y = 470.0;
            pHose->param_d[0] = baseAngles[i];
            pHose->param_d[1] = 25.0 * DX_PI / 180.0; // 振幅
            pHose->param_d[2] = 90.0;                 // 周期
            pHose->param_d[3] = phaseShifts[i];

            pHose->pEnemyShotHead = new sEnemyShot;
            pHose->pEnemyShotHead->prev = pHose->pEnemyShotHead;
            pHose->pEnemyShotHead->next = pHose->pEnemyShotHead;

            pHose->prev = enemyShotSetHead.prev;
            pHose->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pHose;
            enemyShotSetHead.prev = pHose;
        }
    }

    // フェーズ4締め: 残りのトマトが放射状に大放出されるフィナーレ
    if (cyclePos == FINALE_TRIGGER) {
        sEnemyShotSet* pFinale = new sEnemyShotSet;
        pFinale->count = 0;
        pFinale->patternFunc = ShotFinaleBurst;
        pFinale->x = 240.0;
        pFinale->y = 100.0;

        pFinale->pEnemyShotHead = new sEnemyShot;
        pFinale->pEnemyShotHead->prev = pFinale->pEnemyShotHead;
        pFinale->pEnemyShotHead->next = pFinale->pEnemyShotHead;

        pFinale->prev = enemyShotSetHead.prev;
        pFinale->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pFinale;
        enemyShotSetHead.prev = pFinale;
    }
}