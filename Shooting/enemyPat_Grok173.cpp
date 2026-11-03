#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：五線のアリア
//
//  画面中央を、右から左へ流れる五線にする。
//  その上を一小節ぶんの音符が同じ速度で流れ、
//  画面幅の 55% 地点で 0.15 秒ためてから符頭だけが自機を狙う。
//  符幹は曲がらず、休符の 1 拍がレーンを移る隙間になる。
//
//  使う弾（一覧から譜面に必要なものだけ）:
//    五線 … 小玉・白 (半径 2.5)。細く、点を並べると譜線に見える。
//    符頭 … 中楕円弾・青 (半径 10.5 x 7.0)。符頭の形に近い。発音で赤。
//    符幹 … 銃弾・青 (半径 5.0 x 2.0)。muki を縦にして細い幹にする。
//  使わない理由:
//    大玉は半径 20 で線間を埋める。
//    短レーザーは長さ方向の半径 64 で、譜線にすると通れない。
//    中玉は円なので符頭らしくない。鱗弾・菱形弾は譜面の形に合わない。
//
//  効果音: light = 五線が引かれるとき、charge = ため、medium = 発音。
//  count / pEnemyShotSet->count / pEnemyShot->count の加算と、
//  画面外の弾の消去はメインルーチン。ここではやらない。
//  旋律は固定。GetRand は使わない。
//
//  60fps 想定。ゲーム画面は 480x480。
// ============================================================

static const double kStaffSpeed = 2.0;
static const double kTelegraphSpeed = 0.5;
static const double kAimSpeed = kStaffSpeed * 4.0;
static const double kStaffSpawnX = 494.0;
static const double kNoteSpawnX = 486.0;
static const double kTriggerX = 480.0 * 0.55;
static const int kStaffInterval = 3;     // 中心間隔 = 2.0 * 3 = 6
static const int kIntroFrames = 90;      // 五線だけ 1.5 秒
static const int kBeatFrames = 42/7;       // 1 拍 0.7 秒
static const int kTelegraphFrames = 9;   // ため 0.15 秒
static const double kLineSpacing = 56.0;
static const double kStaffCenterY = 240.0;

// 添字 0 が第1線（低い音、画面下）。
static const double kLineY[5] = {
    kStaffCenterY + kLineSpacing * 2.0,
    kStaffCenterY + kLineSpacing,
    kStaffCenterY,
    kStaffCenterY - kLineSpacing,
    kStaffCenterY - kLineSpacing * 2.0,
};

// 上昇 4 拍、休符、下降 4 拍。
// line < 0 は休符。stemDown: 0 = 上向き（符頭の右）、1 = 下向き（符頭の左）。
// 第3線から上は楽譜どおり下向き。
struct ScoreNote {
    int line;
    int stemDown;
};

static const ScoreNote kMelody[] = {
    { 0, 0 },
    { 1, 0 },
    { 2, 1 },
    { 3, 1 },
    { -1, 0 },
    { 4, 1 },
    { 3, 1 },
    { 2, 1 },
    { 1, 0 },
};
static const int kMelodyLen = sizeof(kMelody) / sizeof(kMelody[0]);

// 弾の param:
//   param_i[0] 役割  0:五線  1:符頭  2:符幹
//   param_i[1] 符頭の状態  0:流れる  1:ため  2:発音後
//   param_i[3] ために入ったときの pEnemyShot->count
//   param_i[4] 音符グループ ID（五線は 0）
//   param_d[0], param_d[1]  移動量。符幹は muki を見た目の向きに使うので、
//                            サンプルのように muki では動かさない。
// ショットセットの param_i[0] は次に振る音符 ID。

static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki,
    double vx, double vy, int kind, int role, int groupId)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = fabs(vx) + fabs(vy);
    pShot->kind = kind;
    pShot->param_i[0] = role;
    pShot->param_i[1] = 0;
    pShot->param_i[3] = 0;
    pShot->param_i[4] = groupId;
    pShot->param_d[0] = vx;
    pShot->param_d[1] = vy;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

static void SpawnStaffLine(sEnemyShotSet* pSet)
{
    for (int i = 0; i < 5; i++) {
        AddShot(pSet, kStaffSpawnX, kLineY[i], DX_PI,
            -kStaffSpeed, 0.0, img_enemyShotSmallBall[6], 0, 0);
    }
}

static void SpawnScoreNote(sEnemyShotSet* pSet, int line, int stemDown)
{
    int groupId = pSet->param_i[0] + 1;
    pSet->param_i[0] = groupId;

    double x = kNoteSpawnX;
    double y = kLineY[line];
    AddShot(pSet, x, y, DX_PI, -kStaffSpeed, 0.0,
        img_enemyShotMediumOval[4], 1, groupId);

    // 縦向きの銃弾。中心を符頭から 10 と 18 に置き、先端は線から 23 まで。
    // 線間隔 56 なので、隣の譜線とのあいだに約 30 残る。
    double side = stemDown ? -1.0 : 1.0;
    double dir = stemDown ? 1.0 : -1.0;
    double stemMuki = stemDown ? (DX_PI * 0.5) : (-DX_PI * 0.5);
    double stemX = x + side * 8.5;
    int stemKind = img_enemyShotBullet[4];
    AddShot(pSet, stemX, y + dir * 10.0, stemMuki, -kStaffSpeed, 0.0, stemKind, 2, groupId);
    AddShot(pSet, stemX, y + dir * 18.0, stemMuki, -kStaffSpeed, 0.0, stemKind, 2, groupId);
}

static int IsGroupSlow(const int* ids, int n, int id)
{
    for (int i = 0; i < n; i++) {
        if (ids[i] == id) return 1;
    }
    return 0;
}

static void ShotGosenAria(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count % kStaffInterval == 0) {
        SpawnStaffLine(pEnemyShotSet);
    }

    if (pEnemyShotSet->count >= kIntroFrames) {
        int t = pEnemyShotSet->count - kIntroFrames;
        if (t % kBeatFrames == 0) {
            int step = (t / kBeatFrames) % kMelodyLen;
            if (kMelody[step].line >= 0) {
                SpawnScoreNote(pEnemyShotSet, kMelody[step].line, kMelody[step].stemDown);
            }
        }
    }

    int slowGroup[8];
    int slowNum = 0;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            if (pShot->param_i[1] == 0 && pShot->x <= kTriggerX) {
                pShot->param_i[1] = 1;
                pShot->param_i[3] = pShot->count;
                //if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                //PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            }
            if (pShot->param_i[1] == 1
                && pShot->count - pShot->param_i[3] >= kTelegraphFrames) {
                double ang = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->param_i[1] = 2;
                pShot->muki = ang;
                pShot->speed = kAimSpeed;
                pShot->param_d[0] = kAimSpeed * cos(ang);
                pShot->param_d[1] = kAimSpeed * sin(ang);
                pShot->kind = img_enemyShotMediumOval[0];
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
            if (pShot->param_i[1] == 1 && slowNum < 8) {
                slowGroup[slowNum] = pShot->param_i[4];
                slowNum++;
            }
        }
        pShot = pShot->next;
    }

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int role = pShot->param_i[0];
        if (role == 1 && pShot->param_i[1] == 1) {
            pShot->param_d[0] = -kTelegraphSpeed;
            pShot->param_d[1] = 0.0;
            pShot->speed = kTelegraphSpeed;
        }
        else if (role == 2) {
            if (IsGroupSlow(slowGroup, slowNum, pShot->param_i[4])) {
                pShot->param_d[0] = -kTelegraphSpeed;
                pShot->speed = kTelegraphSpeed;
            }
            else {
                pShot->param_d[0] = -kStaffSpeed;
                pShot->speed = kStaffSpeed;
            }
            pShot->param_d[1] = 0.0;
        }
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];
        pShot = pShot->next;
    }
}

// 敵本体。名前は指定どおり EnemyPat_StaffNotation_Grok。
void EnemyPat_StaffNotation_Grok()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480。五線の最上段は y=128 なので、その上に置く。
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGosenAria;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }
}