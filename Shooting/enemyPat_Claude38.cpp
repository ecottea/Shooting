// enemyPat_hataoriLaser.cpp
//
// 「機織り(はたおり)レーザー」
// 経糸(たていと)＝画面を縦に貫く固定レーザーの束
// 緯糸(よこいと)＝一定間隔で下へ織り込まれていく横レーザーの段
// 緯糸の各段には、経糸と経糸の間(＝もともと安全な隙間)のどこか1箇所だけ
// 「安全な織り目の隙間」を開けておき、段を追うごとにその隙間の位置を
// 1レーンずつずらしていく。これにより、経糸の隙間で待っているだけでは
// いずれ塞がれてしまい、緯糸が織り込まれるたびにジグザグに隙間を
// 縫って移動することを強いられる。
//
// 素材選定:
//   ・経糸/緯糸ともに img_enemyShotLaser (とても長細い楕円弾) を
//     一定間隔で並べて「静止した長いレーザー線」を表現する。
//     色は経糸=シアン(寒色/index3)、緯糸=橙(暖色/index8)で役割を分離。
//   ・フィナーレの拡散弾は img_enemyShotDiamond (菱形弾・糸くず) を使用。
//   ・画面外への消去はメインルーチン任せの仕様なので、
//     「保持→ほどけて画面外へ飛び去る」の形で自然消滅させている。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ================= パターン定数 =================
static const int    WARP_COUNT = 5;                                          // 経糸の本数
static const double WARP_X[WARP_COUNT] = { 60.0, 150.0, 240.0, 330.0, 420.0 };       // 経糸のX座標(画面480x480想定)

// 経糸と経糸の「間」＝もともと安全な隙間の中心X座標(両端含めて経糸本数+1箇所)
static const int    GAP_COUNT = WARP_COUNT + 1;
static const double GAP_X[GAP_COUNT] = { 15.0, 105.0, 195.0, 285.0, 375.0, 465.0 };

static const double WARP_WOBBLE_AMP = 6.0;    // 経糸のたわみ振幅(布のような揺らぎ)
static const double WARP_WOBBLE_SPEED = 0.02;   // 経糸のたわみ速度
static const double SEGMENT_PITCH = 16.0;   // レーザー弾を並べる間隔(縦・横共通)
static const double GAP_HALF_WIDTH = 34.0;   // 緯糸に開ける「安全な隙間」の半幅

static const int    WEFT_INTERVAL = 70;     // 緯糸を織り込む間隔(フレーム)
static const int    WEFT_HOLD = 50;     // 緯糸が静止して危険な状態を保つフレーム数
static const int    WEFT_ROW_COUNT = 6;      // 織り込む段数
static const double ROW_SPACING = 70.0;   // 段ごとのY方向間隔
static const double ROW_START_Y = 40.0;   // 1段目のY座標

// ================= 緯糸(横糸)パターン =================
// 1段分の横糸レーザー。安全レーン以外を隙間なく埋めて生成し、
// 一定時間静止(WEFT_HOLD)して危険な状態を保った後、
// 画面の上半分/下半分それぞれ近い方の端へほどけるように飛び去って自然消滅する。
static void ShotWeftRow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double rowY = pEnemyShotSet->param_d[0]; // この段のY座標
        int safeLane = pEnemyShotSet->param_i[0]; // 安全レーン(GAP_Xのインデックス)
        double safeX = GAP_X[safeLane];

        for (double x = -20.0; x <= 500.0; x += SEGMENT_PITCH) {
            // 安全レーンの周囲だけは隙間として空けておく
            if (fabs(x - safeX) < GAP_HALF_WIDTH) continue;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x;
            pEnemyShot->y = rowY;
            pEnemyShot->muki = 0.0;                     // 水平方向(=緯糸)の向き
            pEnemyShot->speed = 0.0;                     // 静止(織り込み中)
            pEnemyShot->kind = img_enemyShotLaser[8];    // 橙色(緯糸=暖色)
            pEnemyShot->param_i[0] = 0;                  // 0:静止中 1:ほどけ中
            pEnemyShot->margin = 100;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 保持時間が過ぎたら、糸がほどけるように上下へ飛び去らせて自然消滅させる
    bool release = (pEnemyShotSet->count == WEFT_HOLD);

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (release && pEnemyShot->param_i[0] == 0) {
            pEnemyShot->param_i[0] = 1;
            double vy = (pEnemyShot->y < 240.0) ? -3.0 : 3.0;   // 近い方の端(上/下)へ
            double vx = (GetRand(200) - 100) / 100.0;           // 左右に少しだけ散らす
            pEnemyShot->speed = sqrt(vx * vx + vy * vy);
            pEnemyShot->muki = atan2(vy, vx);
        }
        if (pEnemyShot->param_i[0] == 1) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// ================= 経糸(縦糸)パターン =================
// 画面全体を貫く縦レーザーの束。サイン波でごくわずかにたわませ、
// 布のような有機的な揺らぎを出す。フィナーレの合図(param_i[0]=1)を
// 受け取ると、左右近い方の端へほどけるように飛び去って自然消滅する。
static void ShotWarpBundle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_heavy)) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int lane = 0; lane < WARP_COUNT; lane++) {
            for (double y = -20.0; y <= 500.0; y += SEGMENT_PITCH) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = WARP_X[lane];
                pEnemyShot->y = y;
                pEnemyShot->muki = DX_PI / 2.0;              // 垂直方向(=経糸)の向き
                pEnemyShot->speed = 0.0;                      // 静止
                pEnemyShot->kind = img_enemyShotLaser[3];     // シアン(経糸=寒色)
                pEnemyShot->param_i[0] = 0;                   // 0:静止中 1:ほどけ中
                pEnemyShot->param_d[0] = WARP_X[lane];        // たわみの中心X
                pEnemyShot->param_d[1] = (double)GetRand(628) / 100.0; // たわみの位相をレーンごとにずらす
                pEnemyShot->margin = 100;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 外部(EnemyPat_Laser_Claude)からの「ほどけ開始」合図をチェック(二重発動防止つき)
    bool release = (pEnemyShotSet->param_i[0] == 1 && pEnemyShotSet->param_i[1] == 0);
    if (release) pEnemyShotSet->param_i[1] = 1;

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (release && pEnemyShot->param_i[0] == 0) {
            pEnemyShot->param_i[0] = 1;
            double vx = (pEnemyShot->x < 240.0) ? -3.0 : 3.0; // 近い方の端(左/右)へ
            pEnemyShot->speed = fabs(vx);
            pEnemyShot->muki = (vx < 0) ? DX_PI : 0.0;
        }

        if (pEnemyShot->param_i[0] == 1) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        else {
            // 静止中は布のようにごくわずかにたわむ
            pEnemyShot->x = pEnemyShot->param_d[0]
                + WARP_WOBBLE_AMP * sin(pEnemyShot->y * 0.05 + pEnemyShot->param_d[1] + count * WARP_WOBBLE_SPEED);
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// ================= フィナーレ：糸くず拡散弾 =================
// 布が織り上がりきった合図として、自機方向を中心に円状の糸くずをばら撒く。
static void ShotThreadScraps(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int SCRAP_NUM = 16;

    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_extreme)) PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < SCRAP_NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * (2.0 * DX_PI / SCRAP_NUM);
            pEnemyShot->speed = 1.6 + GetRand(80) / 100.0;
            pEnemyShot->kind = img_enemyShotDiamond[6]; // 白の菱形弾(糸くず)
            pEnemyShot->margin = 100;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ================= ヘルパー：ShotSetを生成してリストに繋ぐ =================
static sEnemyShotSet* SpawnShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = x;
    p->y = y;
    p->muki = muki;

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;

    return p;
}

// ================= 敵本体：機織りレーザー =================
void EnemyPat_Laser_Claude()
{
    static int weftRowIndex;
    static int safeLaneCursor;
    static sEnemyShotSet* pWarpSet;

    const int WARP_START_FRAME = 60;                                        // 経糸が確定するタイミング
    const int WEFT_START_FRAME = WARP_START_FRAME + 10;                     // 緯糸を織り始めるタイミング
    const int WEFT_END_FRAME = WEFT_START_FRAME + WEFT_INTERVAL * WEFT_ROW_COUNT;
    const int FINALE_FRAME = WEFT_END_FRAME + 40;                       // フィナーレのタイミング

    static int cnt;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        weftRowIndex = 0;
        safeLaneCursor = GetRand(GAP_COUNT - 1); // 最初の安全レーンをランダムに決定
        pWarpSet = nullptr;
        cnt = 1;
    }
    else {
        // 織機らしく、ゆっくり左右に揺れる
        enemy.x = 240.0 + 60.0 * sin(count * 0.01);
    }

    // 経糸(縦糸)を確定させる
    if (cnt == WARP_START_FRAME) {
        pWarpSet = SpawnShotSet(ShotWarpBundle, enemy.x, enemy.y, 0.0);
    }

    // 緯糸(横糸)を段ごとに織り込んでいく。安全レーンは1段ごとに1つずつずらす。
    if (cnt >= WEFT_START_FRAME && cnt < WEFT_END_FRAME &&
        (cnt - WEFT_START_FRAME) % WEFT_INTERVAL == 0) {

        sEnemyShotSet* pRow = SpawnShotSet(ShotWeftRow, enemy.x, enemy.y, 0.0);
        pRow->param_d[0] = ROW_START_Y + weftRowIndex * ROW_SPACING; // この段のY座標
        pRow->param_i[0] = safeLaneCursor;                            // この段の安全レーン

        safeLaneCursor = (safeLaneCursor + 1) % GAP_COUNT; // 綾織りのように安全レーンの位相をずらす
        weftRowIndex++;
    }

    // フィナーレ：経糸をほどいて消し、自機狙いの糸くず弾を放って締める
    if (cnt == FINALE_FRAME) {
        if (pWarpSet != nullptr) {
            pWarpSet->param_i[0] = 1; // ほどけ開始の合図
        }
        double muki = atan2(player.y - enemy.y, player.x - enemy.x);
        SpawnShotSet(ShotThreadScraps, enemy.x, enemy.y, muki);
    }

    if (cnt == FINALE_FRAME + 60) {
        cnt = 1;
        weftRowIndex = 0;
    }

    cnt++;
}