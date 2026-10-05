#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：フルオート掃射（マシンガン）
//
//  敵が自機狙いのマシンガンを撃ち、銃身を左右に振りながら薬莢をばらまく。
//  専用素材は使わず、既存の敵弾の組み合わせだけで表現する。
//
//  【流れ】 1セット = 構え(40f) → 掃射(180f) → 弾倉交換(60f) = 280f
//           2セット(左→右、右→左に銃身を振る)で1サイクル(560f)、以降ループ
//
//  【使う弾】
//    銃弾(橙)      ：本命の弾丸。1回の連射(2fごと)で中心1発＋左右にずらした2発
//    短レーザー(シアン)：曳光弾。5連射に1回、中心弾の代わりに高速で飛ぶ
//    小玉(黄/白)   ：マズルフラッシュの火花
//    大玉(黄)      ：マズルフラッシュの閃光。ゆっくり出て徐々に加速する
//    中楕円弾(黄)  ：薬莢。真横に排出され、回転しながら重力で落下する
//    小玉(赤)      ：弾痕。銃弾が画面端に達した位置から内側へ飛び散る
//    短レーザー(白)：構えの予告線（掃射範囲を示す）
//    大玉(橙)・菱形弾(シアン)・中玉(黄)：弾倉交換の全方位リング
//
//  【構造】 連射1回(2fごと)につき ShotSet を1つ作り、その連射で出る弾は
//           すべてその ShotSet に入れる。弾ごとの挙動は param_i[0](役割)で分岐する。
// ============================================================

// ---- 画面 ----
static const double MG_SCREEN_SIZE = 480.0;

// ---- 弾の色番号 ----
static const int MG_COLOR_RED = 0;
static const int MG_COLOR_YELLOW = 1;
static const int MG_COLOR_CYAN = 3;
static const int MG_COLOR_WHITE = 6;
static const int MG_COLOR_ORANGE = 8;

// ---- タイムライン ----
static const int MG_WARN_TIME = 40;   // 構え
static const int MG_FIRE_TIME = 180;  // 掃射
static const int MG_RELOAD_TIME = 90;   // 弾倉交換
static const int MG_SET_TIME = MG_WARN_TIME + MG_FIRE_TIME + MG_RELOAD_TIME; // 1セット
static const int MG_SET_NUM = 2;    // 1サイクルあたりのセット数
static const int MG_CYCLE_TIME = MG_SET_TIME * MG_SET_NUM;
static const int MG_FIRE_INTERVAL = 4;    // 連射間隔(フレーム)

// ---- 銃身の動き ----
static const double MG_SWEEP_RANGE = 35.0 / 180.0 * DX_PI; // 掃射の振れ幅(片側)
static const double MG_GUN_POS_X[MG_SET_NUM] = { 140.0, 340.0 }; // セットごとの敵のx位置

// ---- 弾の物理 ----
static const double MG_CASING_GRAVITY = 0.07; // 薬莢にかかる重力

// ---- ShotSet の種類(ShotSet::kind) ----
static const int MG_SET_VOLLEY = 0; // 連射1回分
static const int MG_SET_WARNING = 1; // 構えの予告線
static const int MG_SET_RING = 2; // 弾倉交換のリング

// ---- 弾の役割(sEnemyShot::param_i[0]) ----
// param_i[1] : 画面端に達したときに出す弾痕の数(0なら出さない)
// 薬莢は param_d[0], param_d[1] = 速度成分、param_d[2] = 回転速度
// 加速弾は param_d[0] = 加速度、param_d[1] = 最高速度
static const int MG_ROLE_LINEAR = 0; // 等速直進
static const int MG_ROLE_ACCEL = 1; // 直進しつつ加速
static const int MG_ROLE_CASING = 2; // 薬莢(放物運動＋回転)

// 度(10倍値)で指定した範囲 ±range の乱数をラジアンで返す。 例: 25 → ±2.5度
// GetRand(x) は 0〜x の x+1 通りを返すので、2*range を渡して range を引く
static double MgRandRad(int rangeDegX10)
{
    return (GetRand(rangeDegX10 * 2) - rangeDegX10) / 10.0 * DX_PI / 180.0;
}

// ShotSet に弾を1つ追加する
static sEnemyShot* MgAddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki,
    double speed, int kind, int role, int impactNum)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = role;
    pEnemyShot->param_i[1] = impactNum;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// 画面外かどうか(弾痕の判定用。実際の弾の消去はメインルーチン側)
static bool MgIsOutsideScreen(const sEnemyShot* pEnemyShot)
{
    return pEnemyShot->x < 0.0 || pEnemyShot->x > MG_SCREEN_SIZE ||
        pEnemyShot->y < 0.0 || pEnemyShot->y > MG_SCREEN_SIZE;
}

// 弾痕：銃弾が画面端に達した位置から、画面の内側へ赤い小玉が飛び散る
static void MgSpawnImpact(sEnemyShotSet* pEnemyShotSet, const sEnemyShot* pHitShot)
{
    double hitX = pHitShot->x;
    double hitY = pHitShot->y;
    double normal; // 画面の内側を向く角度
    if (hitX < 0.0)                    normal = 0.0;
    else if (hitX > MG_SCREEN_SIZE)    normal = DX_PI;
    else if (hitY > MG_SCREEN_SIZE)    normal = -DX_PI / 2.0;
    else                               normal = DX_PI / 2.0;

    if (hitX < 0.0) hitX = 0.0;
    if (hitX > MG_SCREEN_SIZE) hitX = MG_SCREEN_SIZE;
    if (hitY < 0.0) hitY = 0.0;
    if (hitY > MG_SCREEN_SIZE) hitY = MG_SCREEN_SIZE;

    for (int i = 0; i < pHitShot->param_i[1]; i++) {
        const double muki = normal + MgRandRad(700); // 内側へ±70度
        const double speed = 0.8 + GetRand(130) / 100.0;
        MgAddShot(pEnemyShotSet, hitX, hitY, muki, speed,
            img_enemyShotSmallBall[MG_COLOR_RED], MG_ROLE_LINEAR, 0);
    }
}

// 連射1回分：銃弾・曳光弾・マズルフラッシュ・薬莢
static void MgSpawnVolley(sEnemyShotSet* pEnemyShotSet)
{
    const int volleyIdx = pEnemyShotSet->param_i[0]; // 掃射中の何回目の連射か
    const double aim = pEnemyShotSet->muki;
    const double dirX = cos(aim);
    const double dirY = sin(aim);

    // 効果音：頭だけ重め、以降は2回に1回軽く鳴らす
    if (volleyIdx == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    else if (volleyIdx % 2 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // (1) 本命の弾丸：中心＋左右にずらした計3発。5連射に1回は中心が曳光弾になる
    static const double spreadDeg[3] = { 0.0, -4.5, 4.5 };
    for (int i = 0; i < 3; i++) {
        const double muki = aim + spreadDeg[i] / 180.0 * DX_PI + MgRandRad(25); // 散布±2.5度
        const bool isTracer = (i == 0 && volleyIdx % 5 == 4);
        if (isTracer) {
            // 短レーザーは長い(64px)ので、銃口から少し前にずらして出す
            MgAddShot(pEnemyShotSet, pEnemyShotSet->x + dirX * 36.0, pEnemyShotSet->y + dirY * 36.0,
                muki, 7.2, img_enemyShotLaser[MG_COLOR_CYAN], MG_ROLE_LINEAR, 3);
        }
        else {
            const double speed = (i == 0) ? 6.0 : 5.6;
            MgAddShot(pEnemyShotSet, pEnemyShotSet->x + dirX * 8.0, pEnemyShotSet->y + dirY * 8.0,
                muki, speed, img_enemyShotBullet[MG_COLOR_ORANGE], MG_ROLE_LINEAR, (i == 0) ? 2 : 0);
        }
    }

    // (2) マズルフラッシュの火花：銃口前方の扇状に小玉(黄1・白2)
    for (int i = 0; i < 3; i++) {
        const double muki = aim + MgRandRad(350); // ±35度
        const double speed = 1.8 + GetRand(160) / 100.0;
        const int color = (i == 0) ? MG_COLOR_YELLOW : MG_COLOR_WHITE;
        MgAddShot(pEnemyShotSet, pEnemyShotSet->x + dirX * 6.0, pEnemyShotSet->y + dirY * 6.0,
            muki, speed, img_enemyShotSmallBall[color], MG_ROLE_LINEAR, 0);
    }

    // (3) マズルフラッシュの閃光：6連射に1回、大玉がゆっくり出て徐々に加速する
    if (volleyIdx % 6 == 0) {
        sEnemyShot* pFlash = MgAddShot(pEnemyShotSet, pEnemyShotSet->x + dirX * 4.0, pEnemyShotSet->y + dirY * 4.0,
            aim, 0.3, img_enemyShotLargeBall[MG_COLOR_YELLOW], MG_ROLE_ACCEL, 0);
        pFlash->param_d[0] = 0.05; // 加速度
        pFlash->param_d[1] = 3.2;  // 最高速度
    }

    // (4) 薬莢：3連射に1回、発射方向の真横(右手側)へ排出。回転しながら落下する
    if (volleyIdx % 3 == 0) {
        const double ejectAngle = aim + DX_PI / 2.0 + MgRandRad(250); // 真横±25度
        const double ejectSpeed = 2.0 + GetRand(150) / 100.0;
        const double spin = ((GetRand(17) + 8) / 100.0) * (GetRand(1) ? 1.0 : -1.0); // 向きは回転で変化させる
        sEnemyShot* pCasing = MgAddShot(pEnemyShotSet, pEnemyShotSet->x + dirX * 2.0, pEnemyShotSet->y + dirY * 2.0,
            GetRand(628) / 100.0, ejectSpeed,
            img_enemyShotMediumOval[MG_COLOR_YELLOW], MG_ROLE_CASING, 0);
        pCasing->param_d[0] = ejectSpeed * cos(ejectAngle);
        pCasing->param_d[1] = ejectSpeed * sin(ejectAngle);
        pCasing->param_d[2] = spin;
    }
}

// 構えの予告線：掃射範囲の両端と中間を白い短レーザーで示す
static void MgSpawnWarning(sEnemyShotSet* pEnemyShotSet)
{
    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

    for (int i = 0; i < 7; i++) {
        const double muki = pEnemyShotSet->muki + (-MG_SWEEP_RANGE + 2.0 * MG_SWEEP_RANGE * i / 6.0);
        MgAddShot(pEnemyShotSet, pEnemyShotSet->x + cos(muki) * 36.0, pEnemyShotSet->y + sin(muki) * 36.0,
            muki, 2.5, img_enemyShotLaser[MG_COLOR_WHITE], MG_ROLE_LINEAR, 0);
    }
}

// 弾倉交換のリング：大玉(橙)20・菱形弾(シアン)40・中玉(黄)20 の全方位
static void MgSpawnRing(sEnemyShotSet* pEnemyShotSet)
{
    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

    const double base = pEnemyShotSet->muki;
    for (int i = 0; i < 20; i++) {
        const double muki = base + i * 2.0 * DX_PI / 20.0;
        MgAddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, muki, 1.6,
            img_enemyShotLargeBall[MG_COLOR_ORANGE], MG_ROLE_LINEAR, 0);
    }
    for (int i = 0; i < 40; i++) {
        const double muki = base + (i + 0.5) * 2.0 * DX_PI / 40.0;
        MgAddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, muki, 2.4,
            img_enemyShotDiamond[MG_COLOR_CYAN], MG_ROLE_LINEAR, 0);
    }
    for (int i = 0; i < 20; i++) {
        const double muki = base + (i + 0.5) * 2.0 * DX_PI / 20.0;
        MgAddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, muki, 3.4,
            img_enemyShotMediumBall[MG_COLOR_YELLOW], MG_ROLE_LINEAR, 0);
    }
}

// 弾幕：フルオート掃射（ShotSet のパターン関数）
static void ShotMachineGun(sEnemyShotSet* pEnemyShotSet)
{
    // 弾の生成は count == 0 のときだけ(カウントの加算はメインルーチン側)
    if (pEnemyShotSet->count == 0) {
        switch (pEnemyShotSet->kind) {
        case MG_SET_VOLLEY:  MgSpawnVolley(pEnemyShotSet);  break;
        case MG_SET_WARNING: MgSpawnWarning(pEnemyShotSet); break;
        case MG_SET_RING:    MgSpawnRing(pEnemyShotSet);    break;
        }
    }

    // 弾の移動(画面外の弾の消去はメインルーチン側)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == MG_ROLE_CASING) {
            // 薬莢：重力で加速しながら落下し、muki は回転させる
            pShot->param_d[1] += MG_CASING_GRAVITY;
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            pShot->muki += pShot->param_d[2];
            pShot->speed = sqrt(pShot->param_d[0] * pShot->param_d[0] + pShot->param_d[1] * pShot->param_d[1]);
        }
        else {
            if (pShot->param_i[0] == MG_ROLE_ACCEL) {
                pShot->speed += pShot->param_d[0];
                if (pShot->speed > pShot->param_d[1]) pShot->speed = pShot->param_d[1];
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 画面端に初めて達したら弾痕を出す(1回だけ)
            if (pShot->param_i[1] > 0 && MgIsOutsideScreen(pShot)) {
                MgSpawnImpact(pEnemyShotSet, pShot);
                pShot->param_i[1] = 0;
            }
        }
        pShot = pShot->next;
    }
}

// ShotSet を1つ作って登録する
static void MgCreateShotSet(int setKind, double x, double y, double muki, int volleyIdx)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotMachineGun;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = setKind;
    pEnemyShotSet->param_i[0] = volleyIdx;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_MachineGun_Claude()
{
    static double aimBase;  // セット開始時に固定した自機狙いの角度(掃射の中心)
    static int    sweepDir; // 銃身を振る向き(+1:左→右、-1:右→左)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = MG_GUN_POS_X[0];
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 100; // 200から変更しないこと
        aimBase = DX_PI / 2.0;
        sweepDir = 1;
    }

    const int phase = (count - 31) % MG_CYCLE_TIME; // サイクル内の経過フレーム
    const int setIdx = phase / MG_SET_TIME;         // 何セット目か
    const int t = phase % MG_SET_TIME;         // セット内の経過フレーム

    // 敵の移動：掃射中はその場で撃ち、弾倉交換中に次のセットの位置へ移る
    const int targetIdx = (t < MG_WARN_TIME + MG_FIRE_TIME) ? setIdx : (setIdx + 1) % MG_SET_NUM;
    enemy.x += (MG_GUN_POS_X[targetIdx] - enemy.x) * 0.1;

    const double muzzleY = enemy.y + 10.0;

    // 構え：自機狙いの角度を固定し、予告線を出す
    if (t == 0) {
        sweepDir = (setIdx % 2 == 0) ? 1 : -1;
        aimBase = atan2(player.y - muzzleY, player.x - enemy.x);
        MgCreateShotSet(MG_SET_WARNING, enemy.x, muzzleY, aimBase, 0);
    }

    // 掃射：2fごとに連射1回。狙い角を sin 波(cos の半周期)で端から端まで振る
    const int fireTime = t - MG_WARN_TIME;
    if (fireTime >= 0 && fireTime < MG_FIRE_TIME && fireTime % MG_FIRE_INTERVAL == 0) {
        const double u = (double)fireTime / (double)(MG_FIRE_TIME - MG_FIRE_INTERVAL); // 0.0〜1.0
        const double offset = -MG_SWEEP_RANGE * cos(DX_PI * u) * sweepDir;
        MgCreateShotSet(MG_SET_VOLLEY, enemy.x, muzzleY, aimBase + offset, fireTime / MG_FIRE_INTERVAL);
    }

    // 弾倉交換：撃ち終わりに全方位リングを出して区切る
    if (t == MG_WARN_TIME + MG_FIRE_TIME) {
        MgCreateShotSet(MG_SET_RING, enemy.x, muzzleY, aimBase, 0);
    }
}