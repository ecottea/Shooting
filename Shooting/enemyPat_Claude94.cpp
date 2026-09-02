// enemyPat_HimawariKenran.cpp
// 向日葵絢爛(ひまわりけんらん) — ひまわりモチーフの弾幕パターン
// 専用素材は使わず、既存の弾種(小玉/中玉/中楕円/短レーザー)の色・配置・動きの組み合わせで
// 「茎→花芯(種)→花びら」の開花、自機を追う向日性、満開からの種散布を表現する。
//
// フェーズ構成(pEnemyShotSet->count / count を基準とした局所フレーム):
//   発芽・開花   :   0 〜 199  茎が下から伸び、葉が生え、花芯の種が黄金角螺旋で敷き詰められ、花びらが開く
//   向日性       : 200 〜 479  花全体が自機のx座標へ緩やかに傾き、花びら先端から自機狙い3wayを連続発射
//   満開乱舞     : 480 〜 679  花びらが分離して外側へ加速しながら回転、花芯から渦状の種弾バーストを連続発射
//   種散布       : 680 〜      赤黒点滅で予告後、花芯の種と茎葉が放射状/落下しながら飛散、自機狙い5wayで締め
// 以後ループ(周期 CYCLE フレーム)。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const double HEAD_X = 240.0;          // 花(頭部)の基準x座標
static const double HEAD_Y = 150.0;          // 花(頭部)の基準y座標
static const double STEM_BOTTOM_Y = 420.0;   // 茎の根元(下端)y座標
static const int    STEM_SEGMENTS = 9;       // 茎の節数
static const double STEM_SPACING =
(STEM_BOTTOM_Y - (HEAD_Y + 15.0)) / (double)(STEM_SEGMENTS - 1);

static const int    N_SEEDS = 180;                       // 花芯の種の数
static const double GOLDEN_ANGLE = 2.399963229728653;    // 黄金角(≒137.5077°)
static const double DISK_MAX_R = 55.0;                   // 花芯の最大半径

static const int    N_PETALS = 24;   // 花びらの枚数
static const double PETAL_R = 88.0;  // 花びら先端までの半径

static const int CYCLE = 900; // 1周期のフレーム数

static const int PHASE_SEED_START = 90;
static const int PHASE_SEED_END = PHASE_SEED_START + N_SEEDS / 2; // 180
static const int PHASE_PETAL_START = 150;
static const int PHASE_PETAL_END = PHASE_PETAL_START + N_PETALS;   // 174
static const int PHASE_B_START = 200;
static const int PHASE_C_START = 480;
static const int PHASE_D_TELEGRAPH_START = 680;
static const int PHASE_D_RELEASE = 720;

// 花全体の「太陽(自機)を追う」傾きオフセット。ファイル内の各関数で共有する。
static double g_headTiltX = 0.0;

// 弾幕パターン関数の前方参照用(sEnemyShotSet::PatternFunc と同じ型)
static void ShotHimawariHead(sEnemyShotSet* pEnemyShotSet);
static void ShotAimedFan(sEnemyShotSet* pEnemyShotSet);
static void ShotSpiralBurst(sEnemyShotSet* pEnemyShotSet);

// ============================================================
//  純粋関数:花びら/種の位置計算(生成時・毎フレーム更新時の両方から使う)
// ============================================================

// 花びらpの角度(満開乱舞フェーズで自転が加わる)
static double PetalAngle(int p, int localCount)
{
    double baseAngle = (double)p * (2.0 * DX_PI / (double)N_PETALS);
    double spin = 0.0;
    if (localCount >= PHASE_C_START) {
        spin = 0.005 * (double)(localCount - PHASE_C_START);
    }
    return baseAngle + spin;
}

// 花びらpの半径(満開乱舞フェーズで外側へ加速しながら分離)
static double PetalRadiusAt(int localCount, double growEase)
{
    double r = PETAL_R * growEase;
    if (localCount >= PHASE_C_START) {
        double tc = (double)(localCount - PHASE_C_START);
        r += 0.01 * tc * tc;
    }
    return r;
}

// 花びらpの座標・向きを計算する
static void PetalPos(int p, int localCount, double headAnchorX, double growEase,
    double* outX, double* outY, double* outAngle)
{
    double angle = PetalAngle(p, localCount);
    double r = PetalRadiusAt(localCount, growEase);
    *outX = headAnchorX + r * cos(angle);
    *outY = HEAD_Y + r * sin(angle);
    *outAngle = angle;
}

// 種nの角度(黄金角螺旋 + 向日性/満開フェーズでのゆるやかな自転)
static double SeedTheta(int n, int localCount)
{
    double theta = (double)n * GOLDEN_ANGLE;
    if (localCount >= PHASE_B_START) {
        int capped = (localCount < PHASE_D_RELEASE) ? localCount : PHASE_D_RELEASE;
        theta += 0.0025 * (double)(capped - PHASE_B_START);
    }
    return theta;
}

// 種nの座標を計算する(花芯フィロタキシス配置)
static void SeedPos(int n, int localCount, double headAnchorX, double growEase,
    double* outX, double* outY)
{
    double theta = SeedTheta(n, localCount);
    double targetR = DISK_MAX_R * sqrt((double)n / (double)(N_SEEDS - 1));
    double r = targetR * growEase;
    *outX = headAnchorX + r * cos(theta);
    *outY = HEAD_Y + r * sin(theta);
}

// ============================================================
//  連結リストへの弾追加(共通処理をまとめたヘルパー)
// ============================================================
static void AppendShot(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->margin = 480;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ============================================================
//  弾幕:向日葵の頭部(茎・葉・花芯の種・花びら)一式
// ============================================================
static void ShotHimawariHead(sEnemyShotSet* pEnemyShotSet)
{
    int localCount = pEnemyShotSet->count;
    double headAnchorX = HEAD_X + g_headTiltX;

    // ---- 茎:8フレーム毎に根元から1節ずつ積み上げる ----
    if (localCount < STEM_SEGMENTS * 8 && localCount % 8 == 0) {
        int idx = pEnemyShotSet->param_i[0];
        if (idx == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        if (idx < STEM_SEGMENTS) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double x = HEAD_X;
            double y = STEM_BOTTOM_Y - (double)idx * STEM_SPACING;
            pEnemyShot->x = x;
            pEnemyShot->y = y;
            pEnemyShot->muki = DX_PI / 2.0; // 縦向き
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[2]; // 緑
            pEnemyShot->param_i[0] = 0;  // role: 茎
            pEnemyShot->param_d[0] = x;  // 生成時座標(散布フェーズの落下起点)
            pEnemyShot->param_d[1] = y;
            AppendShot(pEnemyShotSet, pEnemyShot);
            pEnemyShotSet->param_i[0] = idx + 1;
        }
    }

    // ---- 葉:茎が中間まで育った時点で左右に1枚ずつ ----
    if (localCount == 32) {
        for (int side = -1; side <= 1; side += 2) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double x = HEAD_X;
            double y = STEM_BOTTOM_Y - 4.0 * STEM_SPACING;
            pEnemyShot->x = x;
            pEnemyShot->y = y;
            pEnemyShot->muki = (side < 0) ? DX_PI * 0.8 : DX_PI * 0.2;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[2]; // 緑
            pEnemyShot->param_i[0] = 1; // role: 葉
            pEnemyShot->param_d[0] = x;
            pEnemyShot->param_d[1] = y;
            AppendShot(pEnemyShotSet, pEnemyShot);
        }
    }

    // ---- 花芯の種:90〜179フレームで1フレームに2個ずつ、計180個 ----
    if (localCount >= PHASE_SEED_START && localCount < PHASE_SEED_END) {
        int idx = pEnemyShotSet->param_i[1];
        for (int k = 0; k < 2 && idx < N_SEEDS; k++, idx++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double x, y;
            SeedPos(idx, localCount, headAnchorX, 0.0, &x, &y);
            pEnemyShot->x = x;
            pEnemyShot->y = y;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotSmallBall[(idx % 5 == 0) ? 7 : 8]; // 黒/橙
            pEnemyShot->param_i[0] = 2;   // role: 種
            pEnemyShot->param_i[1] = idx; // 種番号
            pEnemyShot->param_i[2] = 0;   // 散布フェーズでの捕獲済みフラグ
            AppendShot(pEnemyShotSet, pEnemyShot);
        }
        pEnemyShotSet->param_i[1] = idx;
    }

    // ---- 花びら:150〜173フレームで1フレームに1枚、計24枚 ----
    if (localCount >= PHASE_PETAL_START && localCount < PHASE_PETAL_END) {
        int idx = pEnemyShotSet->param_i[2];
        if (idx < N_PETALS) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double x, y, angle;
            PetalPos(idx, localCount, headAnchorX, 0.0, &x, &y, &angle);
            pEnemyShot->x = x;
            pEnemyShot->y = y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumOval[1]; // 黄
            pEnemyShot->param_i[0] = 3;   // role: 花びら
            pEnemyShot->param_i[1] = idx; // 花びら番号
            AppendShot(pEnemyShotSet, pEnemyShot);
            pEnemyShotSet->param_i[2] = idx + 1;
        }
    }

    // ---- 全弾の毎フレーム位置更新(count駆動の式のみで座標を決定) ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int role = pShot->param_i[0];

        if (role == 2) {
            // 種:通常時は花芯フィロタキシス配置、種散布フェーズで放射状に加速飛散
            int n = pShot->param_i[1];
            if (localCount < PHASE_D_RELEASE) {
                double growEase = (pShot->count < 15) ? (double)pShot->count / 15.0 : 1.0;
                double x, y;
                SeedPos(n, localCount, headAnchorX, growEase, &x, &y);
                pShot->x = x;
                pShot->y = y;
                int colorIdx = (n % 5 == 0) ? 7 : 8;
                if (localCount >= PHASE_D_TELEGRAPH_START && (localCount / 6) % 2 == 0) {
                    colorIdx = 0; // 赤点滅で予告
                }
                pShot->kind = img_enemyShotSmallBall[colorIdx];
            }
            else {
                if (pShot->param_i[2] == 0) {
                    // 解放される瞬間の位置を捕獲する
                    double x, y;
                    SeedPos(n, PHASE_D_RELEASE, headAnchorX, 1.0, &x, &y);
                    pShot->param_d[0] = x;
                    pShot->param_d[1] = y;
                    pShot->param_d[2] = atan2(y - HEAD_Y, x - headAnchorX);
                    pShot->param_i[2] = 1;
                }
                double tRel = (double)(localCount - PHASE_D_RELEASE);
                double dist = 1.6 * tRel + 0.015 * tRel * tRel;
                pShot->x = pShot->param_d[0] + dist * cos(pShot->param_d[2]);
                pShot->y = pShot->param_d[1] + dist * sin(pShot->param_d[2]);
                pShot->kind = img_enemyShotSmallBall[(n % 5 == 0) ? 7 : 8];
            }
        }
        else if (role == 3) {
            // 花びら:通常時は放射配置、満開乱舞フェーズで分離・自転しながら加速
            int p = pShot->param_i[1];
            double growEase = (pShot->count < 20) ? (double)pShot->count / 20.0 : 1.0;
            double x, y, angle;
            PetalPos(p, localCount, headAnchorX, growEase, &x, &y, &angle);
            pShot->x = x;
            pShot->y = y;
            pShot->muki = angle;
            int colorIdx = 1;
            if (localCount >= PHASE_D_TELEGRAPH_START && localCount < PHASE_D_RELEASE
                && (localCount / 6) % 2 == 0) {
                colorIdx = 0; // 赤点滅で予告
            }
            pShot->kind = img_enemyShotMediumOval[colorIdx];
        }
        else {
            // 茎・葉(role 0/1):通常は静止、種散布フェーズで枯れて落下し画面外へ消える
            if (localCount >= PHASE_D_RELEASE) {
                double tRel = (double)(localCount - PHASE_D_RELEASE);
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1] + 0.5 * 0.08 * tRel * tRel;
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕:原点から扇状に発射する自機狙いway弾(花びら3way・フィニッシュ5way用)
// ============================================================
static void ShotAimedFan(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int ways = pEnemyShotSet->param_i[0];
        double spread = pEnemyShotSet->param_d[0];
        double speed = pEnemyShotSet->param_d[1];
        for (int i = 0; i < ways; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double offset = (ways == 1) ? 0.0
                : (-spread / 2.0 + spread * (double)i / (double)(ways - 1));
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + offset;
            pEnemyShot->speed = speed;
            pEnemyShot->kind = pEnemyShotSet->kind;
            AppendShot(pEnemyShotSet, pEnemyShot);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double dist = pShot->speed * (double)pShot->count;
        pShot->x = pEnemyShotSet->x + dist * cos(pShot->muki);
        pShot->y = pEnemyShotSet->y + dist * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕:花芯中心から放たれる渦状バースト(満開乱舞フェーズ用)
// ============================================================
static void ShotSpiralBurst(sEnemyShotSet* pEnemyShotSet)
{
    static const int ARMS = 6;
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < ARMS; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double baseAngle = pEnemyShotSet->muki + (double)i * (2.0 * DX_PI / (double)ARMS);
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle;
            pEnemyShot->speed = 1.4;
            pEnemyShot->param_d[0] = baseAngle;
            pEnemyShot->kind = img_enemyShotSmallBall[(i % 2 == 0) ? 8 : 7]; // 橙/黒
            AppendShot(pEnemyShotSet, pEnemyShot);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double curl = 0.005 * t; // 渦を巻きながら外側へ
        double angle = pShot->param_d[0] + curl;
        double dist = pShot->speed * t;
        pShot->x = pEnemyShotSet->x + dist * cos(angle);
        pShot->y = pEnemyShotSet->y + dist * sin(angle);
        pShot = pShot->next;
    }
}

// ============================================================
//  発射ヘルパー(新規sEnemyShotSetを生成しリストへ連結する)
// ============================================================
static sEnemyShotSet* NewLinkedShotSet()
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

static void SpawnAimedFan(double x, double y, double muki, int ways, double spread,
    double speed, int kind)
{
    sEnemyShotSet* pSet = NewLinkedShotSet();
    pSet->patternFunc = ShotAimedFan;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;
    pSet->param_i[0] = ways;
    pSet->param_d[0] = spread;
    pSet->param_d[1] = speed;
}

static void SpawnSpiralBurst(double x, double y, double armOffset)
{
    sEnemyShotSet* pSet = NewLinkedShotSet();
    pSet->patternFunc = ShotSpiralBurst;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = armOffset;
}

// 花びらp先端から自機狙い3wayを発射する(向日性フェーズ用)
static void FireAimedFromPetal(int p, int localCount)
{
    double headAnchorX = HEAD_X + g_headTiltX;
    double px, py, pAngle;
    PetalPos(p, localCount, headAnchorX, 1.0, &px, &py, &pAngle);
    double aim = atan2(player.y - py, player.x - px);
    // 中玉・黄で3way、広がり角約20度
    SpawnAimedFan(px, py, aim, 3, 20.0 / 180.0 * DX_PI, 2.2, img_enemyShotMediumBall[1]);
}

// 花芯中心から渦状バーストを発射する(満開乱舞フェーズ用)
static void FireSpiralBurst(int localCount)
{
    double headAnchorX = HEAD_X + g_headTiltX;
    double armOffset = 0.15 * (double)(localCount - PHASE_C_START);
    SpawnSpiralBurst(headAnchorX, HEAD_Y, armOffset);
}

// フィニッシュの自機狙い5way(種散布フェーズ用)
static void FireFinishing5Way()
{
    double headAnchorX = HEAD_X + g_headTiltX;
    double aim = atan2(player.y - HEAD_Y, player.x - headAnchorX);
    // 中玉・黒で5way、広がり角約31度、やや速め
    SpawnAimedFan(headAnchorX, HEAD_Y, aim, 5, 31.0 / 180.0 * DX_PI, 2.6, img_enemyShotMediumBall[7]);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Sunflower_Claude() // 敵本体の関数名
{
    if (count == 1) {
        enemy.maxHp = enemy.hp = 200; // 200で固定
        g_headTiltX = 0.0;
    }

    int localCount = (count - 10) % CYCLE;

    // 周期の先頭で新しい頭部(茎+葉+花芯+花びら)一式を生成する
    if (localCount == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        g_headTiltX = 0.0;
        sEnemyShotSet* pSet = NewLinkedShotSet();
        pSet->patternFunc = ShotHimawariHead;
        pSet->x = HEAD_X;
        pSet->y = HEAD_Y;
    }

    // フェーズ「向日性」:自機のx座標へ緩やかに傾き、花びら先端から自機狙い3wayを連続発射
    if (localCount >= PHASE_B_START && localCount < PHASE_C_START) {
        double target = player.x - HEAD_X;
        if (target > 70.0) target = 70.0;
        if (target < -70.0) target = -70.0;
        g_headTiltX += (target - g_headTiltX) * 0.02;

        if ((localCount - PHASE_B_START) % 20 == 0) {
            int p1 = ((localCount - PHASE_B_START) / 20) % N_PETALS;
            int p2 = (p1 + N_PETALS / 2) % N_PETALS;
            FireAimedFromPetal(p1, localCount);
            FireAimedFromPetal(p2, localCount);
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // フェーズ「満開乱舞」:花芯から渦状バーストを連続発射
    if (localCount >= PHASE_C_START && localCount < PHASE_D_TELEGRAPH_START) {
        if ((localCount - PHASE_C_START) % 6 == 0) {
            FireSpiralBurst(localCount);
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }

    if (localCount == PHASE_D_TELEGRAPH_START) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // フェーズ「種散布」:フィニッシュの自機狙い5way
    if (localCount == PHASE_D_RELEASE) {
        FireFinishing5Way();
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 花本体(enemy)の見た目位置も傾きに追従させる
    enemy.x = HEAD_X + g_headTiltX;
    enemy.y = HEAD_Y;
}