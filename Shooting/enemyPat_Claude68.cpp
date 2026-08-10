// enemyPat_hanabi_shakudama.cpp
//
// 「尺玉二段咲き」パターン
// 画面下部から玉弾がゆっくり上昇 → 頂点で一段目開花 → 少し遅れて二段目がより派手に開花
// 複数箇所から時間差で打ち上げることで、花火大会のような画面を演出する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ----------------------------------------------------------------------
// 弾の役割（sEnemyShot::param_i[0] に格納）
// ----------------------------------------------------------------------
enum {
    ROLE_SHELL = 0,   // 打ち上げ玉本体
    ROLE_TRAIL = 1,   // 上昇トレイル火花
    ROLE_PETAL1 = 2,  // 開花弾（一段目）
    ROLE_PETAL2 = 3,  // 開花弾（二段目）
};

// 花火に使う色パレット（黒(7)は見えづらいため除外）
// 色一覧: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 8:橙
static const int    hanabiColorPalette[] = { 0, 1, 2, 3, 4, 5, 8 };
static const int    hanabiColorCount = 7;

static const double GRAVITY = 0.012; // 開花後・落下火花のたれ下がり具合

// ----------------------------------------------------------------------
// 弾をショットセットのリストに追加する共通処理
// ----------------------------------------------------------------------
static sEnemyShot* AddShot(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ----------------------------------------------------------------------
// 指定した役割の弾をリストから取り除く（玉弾を開花の瞬間に消すために使用）
// ----------------------------------------------------------------------
static void RemoveShotByRole(sEnemyShotSet* pSet, int role)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;
        if (p->param_i[0] == role) {
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
        }
        p = next;
    }
}

// ----------------------------------------------------------------------
// 開花弾を中心 (cx, cy) から放射状に生成する
// ----------------------------------------------------------------------
static void SpawnPetals(sEnemyShotSet* pSet, double cx, double cy, int petalNum,
    double v0, double decel, double angleOffset, int role, int color)
{
    for (int i = 0; i < petalNum; i++) {
        double angle = angleOffset + (2.0 * DX_PI * i) / petalNum;

        sEnemyShot* pShot = AddShot(pSet);
        pShot->x = cx;
        pShot->y = cy;
        pShot->muki = angle; // 外向きの方向にスプライトを向ける
        pShot->speed = v0;
        pShot->kind = img_enemyShotBullet[color]; // 銃弾：外向きに飛ぶ火花として採用
        pShot->param_i[0] = role;
        pShot->param_d[0] = cx;
        pShot->param_d[1] = cy;
        pShot->param_d[2] = angle;
        pShot->param_d[3] = v0;
        pShot->param_d[4] = decel;
    }
}

// ----------------------------------------------------------------------
// 弾幕：尺玉一発分の挙動（打ち上げ → 一段目開花 → 二段目開花）
// ----------------------------------------------------------------------
static void ShotHanabiShakudama(sEnemyShotSet* pSet)
{
    // pSet->param_d 使用一覧
    //   [0] 発射X座標
    //   [1] 発射Y座標（画面下端よりやや下）
    //   [2] 開花Y座標（頂点高度）
    //   [3] 上昇にかけるフレーム数
    // pSet->param_i 使用一覧
    //   [0] 二段目開花までの追加遅延フレーム
    //   [1] 一段目の弾数
    //   [2] 二段目の弾数
    //   [3] 使用する色（hanabiColorPalette のインデックス）

    const double startX = pSet->param_d[0];
    const double startY = pSet->param_d[1];
    const double bloomY = pSet->param_d[2];
    const int    riseDuration = (int)pSet->param_d[3];
    const int    secondDelay = pSet->param_i[0];
    const int    petalNum1 = pSet->param_i[1];
    const int    petalNum2 = pSet->param_i[2];
    const int    color = hanabiColorPalette[pSet->param_i[3]];

    const int t = pSet->count; // このショットセットが生成されてからの経過フレーム

    // ---- 玉弾（打ち上げ本体）生成 ----
    if (t == 0) {
        sEnemyShot* pShell = AddShot(pSet);
        pShell->x = startX;
        pShell->y = startY;
        pShell->muki = -DX_PI / 2.0; // 真上
        pShell->kind = img_enemyShotSmallBall[color];
        pShell->param_i[0] = ROLE_SHELL;
    }

    // ---- 上昇中：玉弾の位置更新 ＋ トレイル火花生成 ----
    if (t < riseDuration) {
        double ratio = (double)t / riseDuration;
        double eased = 1.0 - (1.0 - ratio) * (1.0 - ratio); // イーズアウト：頂点付近で失速

        double curX = startX;
        double curY = startY + (bloomY - startY) * eased;

        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == ROLE_SHELL) {
                p->x = curX;
                p->y = curY;
            }
            p = p->next;
        }

        // 3フレームごとにトレイル火花を1発生成
        if (t % 3 == 0) {
            sEnemyShot* pSpark = AddShot(pSet);
            pSpark->x = curX;
            pSpark->y = curY;
            double driftVX = (GetRand(40) - 20) / 100.0; // わずかな横ぶれ
            double driftVY = (GetRand(30) + 10) / 100.0; // ゆっくり降下
            pSpark->muki = atan2(driftVY, driftVX);
            pSpark->kind = img_enemyShotSmallBall[color];
            pSpark->param_i[0] = ROLE_TRAIL;
            pSpark->param_d[0] = curX;
            pSpark->param_d[1] = curY;
            pSpark->param_d[2] = driftVX;
            pSpark->param_d[3] = driftVY;
        }
    }

    // ---- 頂点到達：玉弾を消し、一段目を開花 ----
    if (t == riseDuration) {
        RemoveShotByRole(pSet, ROLE_SHELL);
        SpawnPetals(pSet, startX, bloomY, petalNum1, 2.6, 0.024, 0.0, ROLE_PETAL1, color);

        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ---- 二段目開花（少し遅れて、より派手に。一段目の半ピッチずらし） ----
    if (t == riseDuration + secondDelay) {
        double halfStep = DX_PI / petalNum1;
        SpawnPetals(pSet, startX, bloomY, petalNum2, 3.4, 0.020, halfStep, ROLE_PETAL2, color);

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // ---- トレイル火花・開花弾（一段目/二段目）の位置更新 ----
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int role = pShot->param_i[0];

        if (role == ROLE_TRAIL) {
            double tt = pShot->count; // この弾自身が生成されてからの経過フレーム
            double bx = pShot->param_d[0];
            double by = pShot->param_d[1];
            double vx = pShot->param_d[2];
            double vy = pShot->param_d[3];
            pShot->x = bx + vx * tt;
            pShot->y = by + vy * tt + 0.5 * GRAVITY * tt * tt;
        }
        else if (role == ROLE_PETAL1 || role == ROLE_PETAL2) {
            double tt = pShot->count;
            double cx = pShot->param_d[0];
            double cy = pShot->param_d[1];
            double angle = pShot->param_d[2];
            double v0 = pShot->param_d[3];
            double decel = pShot->param_d[4];

            double tCap = v0 / decel; // 半径が伸び切るまでの時間（これ以上は速度0扱い）
            double te = (tt < tCap) ? tt : tCap;
            double r = v0 * te - 0.5 * decel * te * te;

            pShot->x = cx + r * cos(angle);
            pShot->y = cy + r * sin(angle) + 0.5 * GRAVITY * tt * tt; // 重力でたれ下がる余韻
            pShot->speed = (v0 - decel * te > 0.0) ? (v0 - decel * te) : 0.0;
        }

        pShot = pShot->next;
    }
}

// ----------------------------------------------------------------------
// 敵本体のパターン：「尺玉二段咲き」
// ----------------------------------------------------------------------
void EnemyPat_Firework_Claude()
{
    static int nextLaunchCount;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        nextLaunchCount = 30; // 開始直後は少し間を置いてから最初の打ち上げ
    }
    else {
        // 花火大会を見守るような、ゆったりとした横揺れ
        enemy.x = 240.0 + 60.0 * sin(count / 90.0);
    }

    // 打ち上げタイミング
    if (count == nextLaunchCount) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHanabiShakudama;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        // GetRand(x) は 0 から x までの x+1 種類の整数をランダムに返す関数なので注意！
        double launchX;  // 画面下部のランダムなX座標（60〜420）
        while (true) {
            launchX = 60.0 + GetRand(360);
            if (abs(launchX - player.x) > 20) break;
        }
        double bloomY = 90.0 + GetRand(90);    // 開花高度（80〜180あたり、画面上部寄り）
        int    riseDuration = 55 + GetRand(15); // 上昇フレーム数（55〜70）

        pSet->param_d[0] = launchX;
        pSet->param_d[1] = 500.0; // 画面下端(480)よりやや下から発射
        pSet->param_d[2] = bloomY;
        pSet->param_d[3] = (double)riseDuration;

        pSet->param_i[0] = 30 + GetRand(15);              // 二段目までの追加遅延（30〜45フレーム）
        pSet->param_i[1] = 14 + GetRand(6);               // 一段目弾数（14〜20）
        pSet->param_i[2] = 20 + GetRand(8);               // 二段目弾数（20〜28）
        pSet->param_i[3] = GetRand(hanabiColorCount - 1); // 色をランダム選択

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        // 次の打ち上げまでの間隔（時間経過とともに詰めていき、最短35フレームまで）
        int interval = 70 - (count / 20);
        if (interval < 35) interval = 35;
        nextLaunchCount = count + interval;
    }
}