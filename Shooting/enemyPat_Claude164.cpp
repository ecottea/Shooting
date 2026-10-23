// ============================================================
//  enemyPat_Hokuto.cpp
//  弾幕「北斗七星 ― 天枢より北辰を指す」
//
//  ボス＝北極星(ポラリス)。画面上部中央に固定。
//  ① 七星が柄杓の形に1つずつ灯る（中玉＋瞬く小玉のリング。開陽には輔星）
//  ② 星座線を書き順に引く（シアンの小玉を2列で敷き詰める）
//  ③ 星座全体が北極星を中心に反時計回りにゆっくり回転しながら、各星が砲撃する
//       升の4星  ：黄の中玉リング（奇数回は橙の小玉リングを重ねる。わずかに曲がる）
//       柄の3星  ：緑の鱗弾＋橙の菱形弾のスパイラル（先端ほど速い）
//       輔星     ：青の銃弾を自機狙いで3連射
//  ④ 天枢・天璇から北極星へ向けて短レーザー → 北極星が炸裂（白の全方位弾）
//       同時に星座は外側へ散っていく
//  1周期 HK_CYCLE フレームでループする。
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数（調整はここで）
// ============================================================
static const double HK_PI = DX_PI;
static const double HK_TWO_PI = DX_PI * 2.0;

// 弾の色（img_enemyShotXxx[色] のインデックス）
enum {
    HK_COL_RED, HK_COL_YELLOW, HK_COL_GREEN, HK_COL_CYAN, HK_COL_BLUE,
    HK_COL_MAGENTA, HK_COL_WHITE, HK_COL_BLACK, HK_COL_ORANGE
};

// 星座弾の役割（param_i[0]）
enum { HK_ROLE_CORE = 0, HK_ROLE_HALO = 1, HK_ROLE_ALCOR = 2 };

// ---------- 時間配置（1周期内のフレーム） ----------
static const int HK_CYCLE = 1240-100; // 1周期の長さ
static const int HK_T_CHARGE = 10;   // 予告音
static const int HK_T_STAR0 = 30;   // 最初の星が灯る
static const int HK_T_STAR_STEP = 24/2;   // 星が灯る間隔
static const int HK_T_LINE0 = 200-50;  // 最初の星座線を引き始める
static const int HK_T_LINE_STEP = 26/2;   // 星座線を引き始める間隔
static const int HK_T_ROT_START = 390-100;  // 星座の回転開始
static const int HK_T_FIRE0 = 400-100;  // 七星の砲撃開始
static const int HK_T_FIRE1 = 1000-100; // 七星の砲撃終了（同時に予告音）
static const int HK_T_LASER0 = 1010-100; // 北極星へのレーザー開始
static const int HK_T_LASER_STEP = 4;    // レーザーを撃つ間隔
static const int HK_T_LASER_NUM = 15;   // レーザーを撃つ回数
static const int HK_T_BURST0 = 1040-100; // 北極星の炸裂＝星座が散り始める
static const int HK_T_BURST_STEP = 12;   // 炸裂の波の間隔
static const int HK_BURST_WAVE = 3;    // 炸裂の波の数

// ---------- 星座の形（北極星を原点、回転前のローカル座標、y下向き） ----------
// 0:天枢(Dubhe) 1:天璇(Merak) 2:天璣(Phecda) 3:天権(Megrez)
// 4:玉衡(Alioth) 5:開陽(Mizar) 6:揺光(Alkaid)
// 天枢・天璇・北極星は x=0 の一直線上に並ぶ（指極星）
static const int HK_STAR_NUM = 7;
static const int HK_MIZAR = 5;
static const double HK_STAR_X[HK_STAR_NUM] = { 0.0,   0.0,  -90.0, -108.0, -168.0, -226.0, -278.0 };
static const double HK_STAR_Y[HK_STAR_NUM] = { 115.0, 177.0,  180.0,  128.0,  112.0,   88.0,   52.0 };

// 星座線（書き順）
static const int HK_SEG_NUM = 7;
static const int HK_SEG_A[HK_SEG_NUM] = { 0, 1, 2, 3, 3, 4, 5 };
static const int HK_SEG_B[HK_SEG_NUM] = { 1, 2, 3, 0, 4, 5, 6 };

static const double HK_POLARIS_X = 240.0;
static const double HK_POLARIS_Y = 36.0;

// 回転（反時計回りが正）
static const double HK_ANG0 = 0.48;  // 開始時の傾き[rad]
static const double HK_ANG_TOTAL = 0.50;  // 回転開始から炸裂までに回る量[rad]

// 星の見た目
static const int    HK_HALO_NUM = 6;     // 瞬きの小玉の数
static const double HK_HALO_R = 12.0;  // 瞬きの半径
static const double HK_HALO_SPIN = 0.035; // 瞬きの公転速度[rad/frame]
static const double HK_ALCOR_R = 26.0;  // 輔星の公転半径
static const double HK_ALCOR_W = 0.045; // 輔星の公転速度[rad/frame]

// 星座線
static const double HK_LINE_MARGIN = 18.0; // 星の中心から線を始めるまでの距離
static const double HK_LINE_STEP = 8.0;  // 線上の弾の間隔
static const double HK_LINE_HALF_WIDTH = 4.0;  // 2列の線の片側の幅

// 升の星（リング）
static const int    HK_RING_NUM = 16;
static const double HK_RING_SPEED = 1.5;
static const double HK_RING_TURN = 0.002; // 1フレームあたりの曲がり[rad]
static const int    HK_RING_INTERVAL = 48+10;
static const int    HK_RING_SHIFT = 12;    // 星ごとの発射ずらし

// 柄の星（スパイラル）
static const int    HK_HANDLE_INTERVAL = 10+5;
static const double HK_HANDLE_SPEED[3] = { 2.0, 2.5, 3.0 }; // 玉衡・開陽・揺光

// 輔星の自機狙い
static const int HK_ALCOR_INTERVAL = 30;

// 北極星への指極レーザー
static const double HK_LASER_SPEED = 6.5;

// 北極星の炸裂
static const int HK_POLARIS_NUM = 56;

// ============================================================
//  状態
// ============================================================
static int s_hkBase = 1; // 現在の周期が始まった count

// 周期内の経過フレーム
static int HkNow()
{
    return count - s_hkBase;
}

// ============================================================
//  ユーティリティ
// ============================================================
static void HkPlaySE(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 弾を1つ生成してShotSetに追加
static sEnemyShot* HkAddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ShotSetを生成して登録
static sEnemyShotSet* HkAddShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// 星座の回転角（反時計回りが正）
static double HkAngle(int t)
{
    int tt = t - HK_T_ROT_START;
    const int ttMax = HK_T_BURST0 - HK_T_ROT_START;
    if (tt < 0) tt = 0;
    if (tt > ttMax) tt = ttMax;
    return HK_ANG0 + HK_ANG_TOTAL * (double)tt / (double)ttMax;
}

// ローカル座標（北極星基準）→ 画面座標。angが正で反時計回りに回る
static void HkLocalToScreen(double lx, double ly, double ang, double* px, double* py)
{
    const double c = cos(ang);
    const double s = sin(ang);
    *px = HK_POLARIS_X + lx * c + ly * s;
    *py = HK_POLARIS_Y - lx * s + ly * c;
}

// 星iの現在の画面座標
static void HkStarPos(int i, int t, double* px, double* py)
{
    HkLocalToScreen(HK_STAR_X[i], HK_STAR_Y[i], HkAngle(t), px, py);
}

static int HkStarAppearT(int i)
{
    return HK_T_STAR0 + HK_T_STAR_STEP * i;
}

static int HkLineT(int seg)
{
    return HK_T_LINE0 + HK_T_LINE_STEP * seg;
}

// 輔星（開陽の周りを公転する）
static double HkAlcorAngle(int t)
{
    return 0.7 + HK_ALCOR_W * (double)(t - HkStarAppearT(HK_MIZAR));
}

static void HkAlcorPos(int t, double* px, double* py)
{
    double sx, sy;
    HkStarPos(HK_MIZAR, t, &sx, &sy);
    const double a = HkAlcorAngle(t);
    *px = sx + HK_ALCOR_R * cos(a);
    *py = sy + HK_ALCOR_R * sin(a);
}

// 星座の弾を北極星から外側へ散らし始める（param_i[1]=1 で自由移動）
static void HkStartDisperse(sEnemyShot* pShot)
{
    pShot->param_i[1] = 1;
    pShot->muki = atan2(pShot->y - HK_POLARIS_Y, pShot->x - HK_POLARIS_X);
    pShot->speed = 0.6 + GetRand(70) / 100.0;
}

static void HkMoveFree(sEnemyShot* pShot)
{
    pShot->x += pShot->speed * cos(pShot->muki);
    pShot->y += pShot->speed * sin(pShot->muki);
}

// 汎用の弾移動
//   param_d[0]: 毎フレームの曲がり[rad]
//   param_d[1]: 毎フレームの加速度（0なら等速）
//   param_d[2]: 加速・減速の到達速度
static void HkMoveGeneric(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->muki += pShot->param_d[0];
        if (pShot->param_d[1] != 0.0) {
            pShot->speed += pShot->param_d[1];
            if (pShot->param_d[1] > 0.0 && pShot->speed > pShot->param_d[2]) pShot->speed = pShot->param_d[2];
            if (pShot->param_d[1] < 0.0 && pShot->speed < pShot->param_d[2]) pShot->speed = pShot->param_d[2];
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  ① 星（中玉＋瞬きの小玉。開陽には輔星）
//     param_i[0]: 星の番号
//     弾ごと param_i[0]: 役割 / param_i[1]: 0=固定中 1=散開中
// ============================================================
static void HkShotStar(sEnemyShotSet* pSet)
{
    const int t = HkNow();
    const int star = pSet->param_i[0];
    double sx, sy;
    HkStarPos(star, t, &sx, &sy);

    if (pSet->count == 0) {
        HkPlaySE(sound_enemyShot_light);
        const int col = (star == HK_MIZAR) ? HK_COL_WHITE : HK_COL_YELLOW;
        sEnemyShot* pNew = HkAddShot(pSet, sx, sy, 0.0, 0.0, img_enemyShotMediumBall[col]);
        pNew->param_i[0] = HK_ROLE_CORE;
        for (int k = 0; k < HK_HALO_NUM; k++) {
            pNew = HkAddShot(pSet, sx, sy, 0.0, 0.0, img_enemyShotSmallBall[col]);
            pNew->param_i[0] = HK_ROLE_HALO;
            pNew->param_d[0] = HK_TWO_PI * (double)k / (double)HK_HALO_NUM;
        }
        if (star == HK_MIZAR) {
            pNew = HkAddShot(pSet, sx, sy, 0.0, 0.0, img_enemyShotSmallBall[HK_COL_BLUE]);
            pNew->param_i[0] = HK_ROLE_ALCOR;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 0 && t >= HK_T_BURST0) HkStartDisperse(pShot);
        if (pShot->param_i[1] != 0) {
            HkMoveFree(pShot);
            pShot = pShot->next;
            continue;
        }
        switch (pShot->param_i[0]) {
        case HK_ROLE_CORE:
            pShot->x = sx;
            pShot->y = sy;
            pShot->muki = 0.04 * (double)pSet->count; // ゆっくり自転
            break;
        case HK_ROLE_HALO: {
            const double a = pShot->param_d[0] + HK_HALO_SPIN * (double)pSet->count;
            const double r = HK_HALO_R + 2.5 * sin(0.08 * (double)pSet->count + pShot->param_d[0]); // 瞬き
            pShot->x = sx + r * cos(a);
            pShot->y = sy + r * sin(a);
            pShot->muki = a + HK_PI / 2.0; // 公転の接線方向
            break;
        }
        case HK_ROLE_ALCOR: {
            double ax, ay;
            HkAlcorPos(t, &ax, &ay);
            pShot->x = ax;
            pShot->y = ay;
            pShot->muki = HkAlcorAngle(t) + HK_PI / 2.0; // 公転の接線方向
            break;
        }
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  ② 星座線（星aから星bへ向けて2フレームに1組ずつ描いていく）
//     param_i[0]: 線の番号
//     弾ごと param_d[0], param_d[1]: 回転前のローカル座標
// ============================================================
static void HkShotLine(sEnemyShotSet* pSet)
{
    const int t = HkNow();
    const int seg = pSet->param_i[0];
    const int a = HK_SEG_A[seg];
    const int b = HK_SEG_B[seg];
    const double ax = HK_STAR_X[a];
    const double ay = HK_STAR_Y[a];
    const double dx = HK_STAR_X[b] - ax;
    const double dy = HK_STAR_Y[b] - ay;
    const double len = sqrt(dx * dx + dy * dy);
    const double ux = dx / len;
    const double uy = dy / len;
    const int n = (int)((len - 2.0 * HK_LINE_MARGIN) / HK_LINE_STEP) + 1; // 1列あたりの弾数

    if (pSet->count % 2 == 0 && pSet->count / 2 < n) {
        if (pSet->count == 0) HkPlaySE(sound_enemyShot_light);
        const double d = HK_LINE_MARGIN + HK_LINE_STEP * (double)(pSet->count / 2);
        for (int row = 0; row < 2; row++) {
            const double off = (row == 0) ? -HK_LINE_HALF_WIDTH : HK_LINE_HALF_WIDTH;
            sEnemyShot* pNew = HkAddShot(pSet, 0.0, 0.0, 0.0, 0.0, img_enemyShotSmallBall[HK_COL_CYAN]);
            pNew->param_d[0] = ax + ux * d - uy * off;
            pNew->param_d[1] = ay + uy * d + ux * off;
        }
    }

    // 線の向き（回転後）
    const double ang = HkAngle(t);
    const double c = cos(ang);
    const double s = sin(ang);
    const double lineMuki = atan2(-ux * s + uy * c, ux * c + uy * s);

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 0 && t >= HK_T_BURST0) HkStartDisperse(pShot);
        if (pShot->param_i[1] != 0) {
            HkMoveFree(pShot);
        }
        else {
            double px, py;
            HkLocalToScreen(pShot->param_d[0], pShot->param_d[1], ang, &px, &py);
            pShot->x = px;
            pShot->y = py;
            pShot->muki = lineMuki;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  ③-a 升の星のリング
//     set->muki: 基準角 / param_i[0]: 発射回数 / param_i[1]: 星の番号
// ============================================================
static void HkShotRing(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int n = pSet->param_i[0];
        const int star = pSet->param_i[1];
        const double turn = ((star % 2 == 0) ? 1.0 : -1.0) * HK_RING_TURN; // 星ごとに逆向きに曲げる
        if (star == 0 && n % 2 == 0) HkPlaySE(sound_enemyShot_medium);

        // 黄の中玉リング
        for (int k = 0; k < HK_RING_NUM; k++) {
            const double a = pSet->muki + HK_TWO_PI * (double)k / (double)HK_RING_NUM;
            sEnemyShot* pNew = HkAddShot(pSet, pSet->x, pSet->y, a, HK_RING_SPEED, img_enemyShotMediumBall[HK_COL_YELLOW]);
            pNew->param_d[0] = turn;
        }
        // 奇数回は橙の小玉リングを半ステップずらして重ねる（少し速い）
        if (n % 2 == 1) {
            for (int k = 0; k < HK_RING_NUM; k++) {
                const double a = pSet->muki + HK_TWO_PI * ((double)k + 0.5) / (double)HK_RING_NUM;
                sEnemyShot* pNew = HkAddShot(pSet, pSet->x, pSet->y, a, HK_RING_SPEED * 1.3, img_enemyShotSmallBall[HK_COL_ORANGE]);
                pNew->param_d[0] = turn;
            }
        }
    }
    HkMoveGeneric(pSet);
}

// ============================================================
//  ③-b 柄の星のスパイラル
//     set->muki: 基準角 / param_i[0]: 0=玉衡 1=開陽 2=揺光 / param_i[1]: 発射回数
// ============================================================
static void HkShotHandle(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int h = pSet->param_i[0];
        const double vmax = HK_HANDLE_SPEED[h];
        const double dir = (h == 1) ? -1.0 : 1.0; // 開陽だけ逆向きに曲げる
        if (h == 0 && pSet->param_i[1] % 2 == 0) HkPlaySE(sound_enemyShot_light);

        for (int j = -1; j <= 1; j++) {
            const double a = pSet->muki + (double)j * 0.34;
            // 緑の鱗弾：直進しながら加速
            sEnemyShot* pNew = HkAddShot(pSet, pSet->x, pSet->y, a, 0.8, img_enemyShotScale[HK_COL_GREEN]);
            pNew->param_d[0] = 0.0;
            pNew->param_d[1] = 0.03;
            pNew->param_d[2] = vmax;
            // 橙の菱形弾：半歩ずらして、少し曲がりながら加速
            pNew = HkAddShot(pSet, pSet->x, pSet->y, a + 0.17, 0.8, img_enemyShotDiamond[HK_COL_ORANGE]);
            pNew->param_d[0] = dir * 0.003;
            pNew->param_d[1] = 0.03;
            pNew->param_d[2] = vmax * 0.85;
        }
    }
    HkMoveGeneric(pSet);
}

// ============================================================
//  ③-c 輔星の自機狙い3連射（set->muki: 自機への角度）
// ============================================================
static void HkShotAimed(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        HkPlaySE(sound_enemyShot_light);
        for (int k = 0; k < 3; k++) {
            HkAddShot(pSet, pSet->x, pSet->y, pSet->muki, 3.0 + 0.4 * (double)k, img_enemyShotBullet[HK_COL_BLUE]);
        }
    }
    HkMoveGeneric(pSet);
}

// ============================================================
//  ④-a 北極星を指す短レーザー
//     param_d[0..1]: 天枢の座標 / param_d[2..3]: 天璇の座標 / param_i[0]: 発射回数
// ============================================================
static void HkShotLaser(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (pSet->param_i[0] % 2 == 0) HkPlaySE(sound_enemyShot_light);
        for (int k = 0; k < 2; k++) {
            const double x = pSet->param_d[k * 2];
            const double y = pSet->param_d[k * 2 + 1];
            const double a = atan2(HK_POLARIS_Y - y, HK_POLARIS_X - x);
            HkAddShot(pSet, x, y, a, HK_LASER_SPEED, img_enemyShotLaser[HK_COL_WHITE]);
        }
    }
    HkMoveGeneric(pSet);
}

// ============================================================
//  ④-b 北極星の炸裂（param_i[0]: 波の番号）
// ============================================================
static void HkShotPolaris(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int w = pSet->param_i[0];
        HkPlaySE(w == 0 ? sound_enemyShot_extreme : sound_enemyShot_heavy);

        // 白の小玉の全方位リング（波ごとに速く、少しずつ角度をずらす）
        const double v = 1.8 + 0.7 * (double)w;
        const double base = (double)w * (HK_TWO_PI / (double)HK_POLARIS_NUM) / 3.0;
        for (int k = 0; k < HK_POLARIS_NUM; k++) {
            const double a = base + HK_TWO_PI * (double)k / (double)HK_POLARIS_NUM;
            HkAddShot(pSet, pSet->x, pSet->y, a, v, img_enemyShotSmallBall[HK_COL_WHITE]);
        }
        // 白の銃弾の自機狙い5way
        const double aim = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int j = -2; j <= 2; j++) {
            HkAddShot(pSet, pSet->x, pSet->y, aim + (double)j * 0.14, 3.2 + 0.3 * (double)w, img_enemyShotBullet[HK_COL_WHITE]);
        }
    }
    HkMoveGeneric(pSet);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_BigDipper_Claude()
{
    if (count == 1) {
        // ボス＝北極星は動かない
        enemy.x = HK_POLARIS_X;
        enemy.y = HK_POLARIS_Y;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        s_hkBase = 1;
    }

    // 周期の管理
    int t = count - s_hkBase;
    if (t >= HK_CYCLE) {
        s_hkBase += HK_CYCLE;
        t -= HK_CYCLE;
    }

    // ---------- ① 七星が灯る ----------
    if (t == HK_T_CHARGE) HkPlaySE(sound_enemyCharge);
    for (int i = 0; i < HK_STAR_NUM; i++) {
        if (t == HkStarAppearT(i)) {
            sEnemyShotSet* pSet = HkAddShotSet(HkShotStar, 0.0, 0.0, 0.0);
            pSet->param_i[0] = i;
        }
    }

    // ---------- ② 星座線を引く ----------
    for (int seg = 0; seg < HK_SEG_NUM; seg++) {
        if (t == HkLineT(seg)) {
            sEnemyShotSet* pSet = HkAddShotSet(HkShotLine, 0.0, 0.0, 0.0);
            pSet->param_i[0] = seg;
        }
    }

    // ---------- ③ 七星の砲撃 ----------
    if (t >= HK_T_FIRE0 && t < HK_T_FIRE1) {
        const int ft = t - HK_T_FIRE0;

        // 升の4星：リング
        for (int i = 0; i < 4; i++) {
            const int st = ft - i * HK_RING_SHIFT;
            if (st >= 0 && st % HK_RING_INTERVAL == 0) {
                const int n = st / HK_RING_INTERVAL;
                double sx, sy;
                HkStarPos(i, t, &sx, &sy);
                sEnemyShotSet* pSet = HkAddShotSet(HkShotRing, sx, sy, (double)n * 0.19 + (double)i * 0.7);
                pSet->param_i[0] = n;
                pSet->param_i[1] = i;
            }
        }

        // 柄の3星：スパイラル
        for (int h = 0; h < 3; h++) {
            const int st = ft - h * 3;
            if (st >= 0 && st % HK_HANDLE_INTERVAL == 0) {
                const int n = st / HK_HANDLE_INTERVAL;
                const double dir = (h == 1) ? -1.0 : 1.0;
                double sx, sy;
                HkStarPos(4 + h, t, &sx, &sy);
                sEnemyShotSet* pSet = HkAddShotSet(HkShotHandle, sx, sy, dir * (double)n * 0.31 + (double)h * 1.1);
                pSet->param_i[0] = h;
                pSet->param_i[1] = n;
            }
        }

        // 輔星：自機狙い3連射
        if (ft >= 20 && (ft - 20) % HK_ALCOR_INTERVAL == 0) {
            double ax, ay;
            HkAlcorPos(t, &ax, &ay);
            HkAddShotSet(HkShotAimed, ax, ay, atan2(player.y - ay, player.x - ax));
        }
    }

    // ---------- ④ 北辰を指す ----------
    if (t == HK_T_FIRE1) HkPlaySE(sound_enemyCharge);

    if (t >= HK_T_LASER0 && (t - HK_T_LASER0) % HK_T_LASER_STEP == 0) {
        const int n = (t - HK_T_LASER0) / HK_T_LASER_STEP;
        if (n < HK_T_LASER_NUM) {
            double dx, dy, mx, my;
            HkStarPos(0, t, &dx, &dy); // 天枢
            HkStarPos(1, t, &mx, &my); // 天璇
            sEnemyShotSet* pSet = HkAddShotSet(HkShotLaser, dx, dy, 0.0);
            pSet->param_d[0] = dx;
            pSet->param_d[1] = dy;
            pSet->param_d[2] = mx;
            pSet->param_d[3] = my;
            pSet->param_i[0] = n;
        }
    }

    if (t >= HK_T_BURST0 && (t - HK_T_BURST0) % HK_T_BURST_STEP == 0) {
        const int w = (t - HK_T_BURST0) / HK_T_BURST_STEP;
        if (w < HK_BURST_WAVE) {
            sEnemyShotSet* pSet = HkAddShotSet(HkShotPolaris, enemy.x, enemy.y, 0.0);
            pSet->param_i[0] = w;
        }
    }
}