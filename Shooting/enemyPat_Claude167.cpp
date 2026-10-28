#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <cmath>

// ============================================================
//  弾幕：へのへのもへじ
//
//  ボスの前に、敵弾が一画ずつ「へのへのもへじ」の顔を描く。
//  顔が完成したら黒目が自機を見つめ、その後パーツごとに別々の動きで崩れて弾幕になる。
//  顔は2種類（kind=0:通常顔 / kind=1:怒り顔）を交互に繰り返す。
//
//  使う弾（専用素材は無いので既存の敵弾の組み合わせで表現）
//    じ(輪郭)   : 中玉
//    へ(眉)×2   : 銃弾
//    の(目)×2   : 小玉の渦巻き + 大玉(黒目)
//    も(鼻)     : 菱形弾
//    へ(口)     : 中楕円弾（崩壊時に小玉の追従弾が付く）
// ============================================================

static const double kPi = DX_PI;

// ------------------------------------------------------------
//  顔の配置（中心からの相対座標。yは下向き）
// ------------------------------------------------------------
static const double kFaceCX = 240.0;   // 顔の中心（ゲーム画面は480x480）
static const double kFaceCY = 190.0;
static const double kFaceR = 130.0;    // 輪郭の半径
static const double kEyeX = 46.0;      // 目の中心X（左右対称）
static const double kEyeY = -10.0;     // 目の中心Y
static const double kBrowY = -56.0;    // 眉の基準Y
static const double kMouthY = 90.0;    // 口の基準Y

// ------------------------------------------------------------
//  パーツごとの弾数
// ------------------------------------------------------------
static const int kNumOutline = 120;
static const int kNumBrow = 30;    // 片方あたり
static const int kNumEye = 72;     // 片方あたり（渦巻き部分。黒目は別に1発）
static const int kNumNoseV = 16;   // 縦線
static const int kNumNoseH = 12;   // 横線（2本とも）
static const int kNumMouth = 48;

// ------------------------------------------------------------
//  タイムライン（フレーム）
// ------------------------------------------------------------
// 描画フェーズ：一画ずつ弾が出現して顔を描く
static const int kTOutline = 0, kDOutline = 50;
static const int kTBrow = 56, kDBrow = 24;
static const int kTEye = 86, kDEye = 44;
static const int kTPupil = 130;
static const int kTNoseV = 138, kDNoseV = 14;
static const int kTNoseH0 = 152, kDNoseH = 8;
static const int kTNoseH1 = 160;
static const int kTMouth = 170, kDMouth = 28;
// 視線フェーズ：黒目が自機を見つめる
static const int kGazeStart = 200;
// 崩壊フェーズ：以降、パーツごとに動き出す
static const int kRelease = 260;

// 視線で動く量（ピクセル）
static const double kGazePupil = 6.0;
static const double kGazeEye = 1.5;

// 崩壊フェーズ内の時刻（kReleaseからの経過フレーム）
static const int kOutlineExpandT = 50;  // 輪郭が広がる時間
static const int kOutlineWaves = 8;     // 輪郭の自機狙いの波の数
static const int kPupilFireT = 80;      // 黒目の発射時刻

// 顔を描き直す間隔
static const int kFaceInterval = 540;

// ------------------------------------------------------------
//  色（0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙）
//  添字は表情（0:通常顔 / 1:怒り顔）
// ------------------------------------------------------------
static const int kColOutline[2] = { 6, 8 };
static const int kColBrow[2] = { 1, 0 };
static const int kColEye[2] = { 3, 8 };
static const int kColPupil[2] = { 0, 6 };
static const int kColNose[2] = { 8, 0 };
static const int kColMouth[2] = { 5, 0 };
static const int kColMouthChild[2] = { 6, 1 };

// ------------------------------------------------------------
//  弾のパーツ種別と、param_i[] の使い道
// ------------------------------------------------------------
enum {
    PART_OUTLINE,
    PART_BROW,
    PART_EYE,
    PART_PUPIL,
    PART_NOSE_V,
    PART_NOSE_H,
    PART_MOUTH,
    PART_FREE      // 崩壊時に追加で生まれる追従弾
};

enum {
    SP_PART = 0,   // param_i[0]: パーツ種別
    SP_INDEX = 1,  // param_i[1]: パーツ内の通し番号
    SP_SIDE = 2,   // param_i[2]: 左右(0:左 1:右) / 鼻の横線の上下(0:上 1:下)
    SP_BOUNCE = 3  // param_i[3]: 鼻の横線が画面端で折り返した回数
};

// sEnemyShotSet::param_d[] の使い道
static const int kSetParamGaze = 0;       // [0..3]: 視線の向き(左目x,y / 右目x,y)
static const int kSetParamMouthAim = 4;   // [4]: 口の扇の中心方向

// ============================================================
//  共通ヘルパー
// ============================================================
static double MinD(double a, double b) { return (a < b) ? a : b; }

static void PlayShotSound(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

static double AimAngle(double x, double y)
{
    return atan2(player.y - y, player.x - x);
}

// 弾を1発、ShotSetの末尾に追加する
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;

    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
    pEnemyShotSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 「へ」の形（0〜1の位置tに対して、0〜1の高さを返す折れ線）
static double Tent(double t, double peak)
{
    return (t < peak) ? (t / peak) : ((1.0 - t) / (1.0 - peak));
}

// ============================================================
//  顔のパーツ上の点（中心からの相対座標）
//    part : パーツ種別
//    side : 左右(0:左 1:右) / 鼻の横線の上下
//    i, n : n発中のi番目
//    expr : 表情(0:通常顔 / 1:怒り顔)
// ============================================================
static void FacePoint(int part, int side, int i, int n, int expr, double& x, double& y)
{
    const double t = (n > 1) ? (double)i / (double)(n - 1) : 0.0;

    switch (part) {
    case PART_OUTLINE: {
        // じ：顎を少し尖らせた円
        const double a = -kPi / 2.0 + 2.0 * kPi * (double)i / (double)n;
        x = kFaceR * cos(a);
        y = kFaceR * sin(a);
        if (y > 0.0) y += 18.0 * pow(y / kFaceR, 6.0);
        break;
    }
    case PART_BROW: {
        // へ：左右とも同じ向きの「へ」。怒り顔では逆ハの字（内側が下がる）
        const double cx = (side == 0) ? -kEyeX : kEyeX;
        x = cx + (t - 0.5) * 60.0;
        if (expr == 0) {
            y = kBrowY - 14.0 * Tent(t, 0.4);
        }
        else {
            y = kBrowY + ((side == 0) ? (-14.0 + 28.0 * t) : (14.0 - 28.0 * t));
        }
        break;
    }
    case PART_EYE: {
        // の：外へ向かう渦巻き（黒目の周りを1.75周）
        const double cx = (side == 0) ? -kEyeX : kEyeX;
        const double r = 11.0 + 13.0 * t;
        const double a = -kPi / 2.0 + 2.0 * kPi * 1.75 * t;
        x = cx + r * cos(a);
        y = kEyeY + r * sin(a);
        break;
    }
    case PART_NOSE_V: {
        // も：縦線（下端が右に少しはね上がる）
        y = 14.0 + 40.0 * t;
        x = (t > 0.8) ? 9.0 * pow((t - 0.8) / 0.2, 2.0) : 0.0;
        break;
    }
    case PART_NOSE_H: {
        // も：横線2本（side 0:上 1:下）
        if (side == 0) {
            y = 26.0;
            x = -11.0 + 22.0 * t;
        }
        else {
            y = 38.0;
            x = -14.0 + 28.0 * t;
        }
        break;
    }
    case PART_MOUTH: {
        // へ：口。怒り顔ではより深いへの字
        x = (t - 0.5) * 88.0;
        if (expr == 0) {
            y = kMouthY - 14.0 * Tent(t, 0.4);
        }
        else {
            y = kMouthY + 6.0 - 30.0 * Tent(t, 0.45);
        }
        break;
    }
    default:
        x = 0.0;
        y = 0.0;
        break;
    }
}

// 顔のパーツの弾を1発、描画位置に置く（速度0、向きは線の進行方向）
static void AddFacePart(sEnemyShotSet* pEnemyShotSet, int part, int side, int i, int n, int kind)
{
    const int expr = pEnemyShotSet->kind;
    double x, y, nx, ny;

    FacePoint(part, side, i, n, expr, x, y);
    if (i < n - 1) {
        FacePoint(part, side, i + 1, n, expr, nx, ny);
    }
    else {
        // 最後の弾は、ひとつ前の弾からの向きを引き継ぐ
        double px, py;
        FacePoint(part, side, i - 1, n, expr, px, py);
        nx = x + (x - px);
        ny = y + (y - py);
    }

    sEnemyShot* pShot = AddShot(pEnemyShotSet, pEnemyShotSet->x + x, pEnemyShotSet->y + y, atan2(ny - y, nx - x), 0.0, kind);
    pShot->param_i[SP_PART] = part;
    pShot->param_i[SP_INDEX] = i;
    pShot->param_i[SP_SIDE] = side;
    if (part == PART_MOUTH) pShot->param_d[1] = (double)i / (double)(n - 1);  // 口の中での位置(0〜1)
}

// 黒目（大玉）を置く
static void AddPupil(sEnemyShotSet* pEnemyShotSet, int side)
{
    const int expr = pEnemyShotSet->kind;
    const double cx = pEnemyShotSet->x + ((side == 0) ? -kEyeX : kEyeX);
    const double cy = pEnemyShotSet->y + kEyeY;

    sEnemyShot* pShot = AddShot(pEnemyShotSet, cx, cy, kPi / 2.0, 0.0, img_enemyShotLargeBall[kColPupil[expr]]);
    pShot->param_i[SP_PART] = PART_PUPIL;
    pShot->param_i[SP_INDEX] = 0;
    pShot->param_i[SP_SIDE] = side;
}

// i番目の弾の出現フレームが今かどうか（durフレームかけてn発を等間隔に出す）
static bool IsSpawnFrame(int fc, int start, int dur, int n, int i)
{
    return start + i * dur / n == fc;
}

// ============================================================
//  描画フェーズ：一画ずつ顔を描く
// ============================================================
static void SpawnFace(sEnemyShotSet* pEnemyShotSet)
{
    const int fc = pEnemyShotSet->count;
    const int e = pEnemyShotSet->kind;

    // 各パーツを描き始める合図の予告音
    if (fc == kTOutline || fc == kTBrow || fc == kTEye || fc == kTNoseV || fc == kTMouth) {
        PlayShotSound(sound_enemyCharge);
    }

    // じ（輪郭）
    for (int i = 0; i < kNumOutline; i++) {
        if (IsSpawnFrame(fc, kTOutline, kDOutline, kNumOutline, i)) {
            AddFacePart(pEnemyShotSet, PART_OUTLINE, 0, i, kNumOutline, img_enemyShotMediumBall[kColOutline[e]]);
        }
    }
    // へ（眉）左右同時
    for (int side = 0; side < 2; side++) {
        for (int i = 0; i < kNumBrow; i++) {
            if (IsSpawnFrame(fc, kTBrow, kDBrow, kNumBrow, i)) {
                AddFacePart(pEnemyShotSet, PART_BROW, side, i, kNumBrow, img_enemyShotBullet[kColBrow[e]]);
            }
        }
    }
    // の（目）左右同時
    for (int side = 0; side < 2; side++) {
        for (int i = 0; i < kNumEye; i++) {
            if (IsSpawnFrame(fc, kTEye, kDEye, kNumEye, i)) {
                AddFacePart(pEnemyShotSet, PART_EYE, side, i, kNumEye, img_enemyShotSmallBall[kColEye[e]]);
            }
        }
    }
    // 黒目
    if (fc == kTPupil) {
        AddPupil(pEnemyShotSet, 0);
        AddPupil(pEnemyShotSet, 1);
    }
    // も（鼻）縦線 → 上の横線 → 下の横線
    for (int i = 0; i < kNumNoseV; i++) {
        if (IsSpawnFrame(fc, kTNoseV, kDNoseV, kNumNoseV, i)) {
            AddFacePart(pEnemyShotSet, PART_NOSE_V, 0, i, kNumNoseV, img_enemyShotDiamond[kColNose[e]]);
        }
    }
    for (int i = 0; i < kNumNoseH; i++) {
        if (IsSpawnFrame(fc, kTNoseH0, kDNoseH, kNumNoseH, i)) {
            AddFacePart(pEnemyShotSet, PART_NOSE_H, 0, i, kNumNoseH, img_enemyShotDiamond[kColNose[e]]);
        }
        if (IsSpawnFrame(fc, kTNoseH1, kDNoseH, kNumNoseH, i)) {
            AddFacePart(pEnemyShotSet, PART_NOSE_H, 1, i, kNumNoseH, img_enemyShotDiamond[kColNose[e]]);
        }
    }
    // へ（口）
    for (int i = 0; i < kNumMouth; i++) {
        if (IsSpawnFrame(fc, kTMouth, kDMouth, kNumMouth, i)) {
            AddFacePart(pEnemyShotSet, PART_MOUTH, 0, i, kNumMouth, img_enemyShotMediumOval[kColMouth[e]]);
        }
    }
}

// ============================================================
//  弾1発の更新
// ============================================================
static void UpdateShot(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pShot)
{
    const int fc = pEnemyShotSet->count;
    const int e = pEnemyShotSet->kind;
    const double k = (e == 0) ? 1.0 : 1.25;   // 怒り顔は速く

    // ---- 描画・視線フェーズ ----
    if (fc < kRelease) {
        // 視線フェーズ：目の弾を自機の方向へ少しだけずらす（黒目は大きくずれる）
        const int part = pShot->param_i[SP_PART];
        if (fc >= kGazeStart && (part == PART_EYE || part == PART_PUPIL)) {
            const double amp = (part == PART_PUPIL) ? kGazePupil : kGazeEye;
            const int side = pShot->param_i[SP_SIDE];
            const double step = amp / (double)(kRelease - kGazeStart);
            pShot->x += pEnemyShotSet->param_d[kSetParamGaze + side * 2] * step;
            pShot->y += pEnemyShotSet->param_d[kSetParamGaze + side * 2 + 1] * step;
        }
        return;
    }

    // ---- 崩壊フェーズ ----
    const int t = fc - kRelease;

    switch (pShot->param_i[SP_PART]) {

    case PART_OUTLINE: {
        // 輪郭：ゆっくり外へ広がり、その後8つの組が順に自機狙いで降ってくる
        const int g = pShot->param_i[SP_INDEX] % kOutlineWaves;
        const int tFire = kOutlineExpandT + g * ((e == 0) ? 14 : 10);

        if (t == 0) pShot->muki = atan2(pShot->y - pEnemyShotSet->y, pShot->x - pEnemyShotSet->x);

        if (t < kOutlineExpandT) {
            pShot->speed = 1.4 * (1.0 - (double)t / (double)kOutlineExpandT);
        }
        else if (t < tFire) {
            pShot->speed = 0.0;
        }
        else if (t == tFire) {
            pShot->muki = AimAngle(pShot->x, pShot->y);
            pShot->speed = 1.4 * k;
        }
        else {
            pShot->speed = MinD(pShot->speed + 0.04 * k, 3.6 * k);
        }
        break;
    }

    case PART_BROW: {
        // 眉：雨のように真下へ。順番はばらばら、左右で加速が違う
        const int i = pShot->param_i[SP_INDEX];
        const int side = pShot->param_i[SP_SIDE];
        const int delay = ((i * 7) % kNumBrow) * 2;

        if (t == delay) {
            // 左の眉は少し左へ、右の眉は少し右へ開く
            pShot->muki = kPi / 2.0 + ((side == 0) ? 1.0 : -1.0) * (0.02 + 0.002 * i);
            pShot->speed = 0.3;
            pShot->param_d[0] = ((side == 0) ? 0.035 : 0.05) * k * (0.9 + GetRand(20) / 100.0);
        }
        else if (t > delay) {
            pShot->speed = MinD(pShot->speed + pShot->param_d[0], 4.2 * k);
        }
        break;
    }

    case PART_EYE: {
        // 目：渦巻きの外側の弾から順に、回転しながら放射状に飛び出す
        const int side = pShot->param_i[SP_SIDE];
        const int delay = kNumEye - 1 - pShot->param_i[SP_INDEX];

        if (t == delay) {
            const double ex = pEnemyShotSet->x + ((side == 0) ? -kEyeX : kEyeX);
            const double ey = pEnemyShotSet->y + kEyeY;
            pShot->muki = atan2(pShot->y - ey, pShot->x - ex);
            pShot->speed = 1.0;
        }
        else if (t > delay) {
            pShot->speed = MinD(pShot->speed + 0.025 * k, 2.8 * k);
            // 左右で逆回転。怒り顔ではさらに逆向きになる
            const double omega = ((side == 0) ? 0.018 : -0.018) * ((e == 0) ? 1.0 : -1.0);
            pShot->muki += omega*0.75;
        }
        break;
    }

    case PART_PUPIL: {
        // 黒目：最後に自機狙いで高速発射
        if (t == kPupilFireT) {
            PlayShotSound(sound_enemyShot_extreme);
            pShot->muki = AimAngle(pShot->x, pShot->y);
            pShot->speed = 2.0;
        }
        else if (t > kPupilFireT) {
            pShot->speed = MinD(pShot->speed + 0.1 * k, 6.5 * k);
        }
        break;
    }

    case PART_NOSE_V: {
        // 鼻の縦線：上から順に、発射時点の自機を狙う高速直線弾
        const int delay = pShot->param_i[SP_INDEX] * 5;
        if (t == delay) {
            pShot->muki = AimAngle(pShot->x, pShot->y);
            pShot->speed = 5.0 * k;
        }
        break;
    }

    case PART_NOSE_H: {
        // 鼻の横線：上の線は右へ、下の線は左へ加速しながらスライド。画面端で最大2回折り返す
        const int stroke = pShot->param_i[SP_SIDE];
        const int delay = pShot->param_i[SP_INDEX] * 3;

        if (t == delay) {
            pShot->muki = (stroke == 0) ? 0.12 : kPi - 0.12;
            pShot->speed = 0.2;
        }
        else if (t > delay) {
            pShot->speed = MinD(pShot->speed + 0.05 * k, 4.5 * k);
            const double vx = cos(pShot->muki);
            if (pShot->param_i[SP_BOUNCE] < 2 && ((pShot->x < 4.0 && vx < 0.0) || (pShot->x > 476.0 && vx > 0.0))) {
                pShot->muki = kPi - pShot->muki;
                pShot->param_i[SP_BOUNCE]++;
            }
        }
        break;
    }

    case PART_MOUTH: {
        // 口：中央の弾から先に出て、自機方向を中心とした扇状に広がる。1発ごとに追従弾が2発付く
        if (t == 0) {
            // 全弾共通の扇の中心方向（口の中心から見た自機の向き）
            pEnemyShotSet->param_d[kSetParamMouthAim] = atan2(player.y - (pEnemyShotSet->y + kMouthY), player.x - pEnemyShotSet->x);
        }

        const double pos = pShot->param_d[1];                 // 口の中での位置(0〜1)
        const double edge = fabs(pos - 0.5) * 2.0;            // 中央0〜端1
        const int delay = (int)(edge * 20.0);
        const double fan = (e == 0) ? 1.5 : 2.0;              // 扇の広がり(ラジアン)

        if (t == delay) {
            const double ang = pEnemyShotSet->param_d[kSetParamMouthAim] + (pos - 0.5) * fan;
            pShot->muki = ang;
            pShot->speed = (2.0 + (1.0 - edge) * 0.8) * k;    // 中央ほど速い

            for (int j = 1; j <= 2; j++) {
                sEnemyShot* pChild = AddShot(pEnemyShotSet, pShot->x, pShot->y, ang, pShot->speed - 0.5 * j, img_enemyShotSmallBall[kColMouthChild[e]]);
                pChild->param_i[SP_PART] = PART_FREE;
                pChild->param_d[0] = 3.2 * k;                 // 最高速度
                pChild->param_d[1] = 0.02 * k;                // 加速度
            }
        }
        else if (t > delay) {
            pShot->speed = MinD(pShot->speed + 0.02 * k, 3.6 * k);
        }
        break;
    }

    case PART_FREE: {
        // 追従弾：まっすぐ加速
        pShot->speed = MinD(pShot->speed + pShot->param_d[1], pShot->param_d[0]);
        break;
    }

    default:
        break;
    }

    pShot->x += pShot->speed * cos(pShot->muki);
    pShot->y += pShot->speed * sin(pShot->muki);
}

// ============================================================
//  弾幕：へのへのもへじ（ShotSet1つが顔1つ）
// ============================================================
static void ShotFace(sEnemyShotSet* pEnemyShotSet)
{
    const int fc = pEnemyShotSet->count;
    const int e = pEnemyShotSet->kind;

    // 描画フェーズ：弾を出現させる
    if (fc < kGazeStart) {
        SpawnFace(pEnemyShotSet);
    }

    // 視線フェーズ開始：各目から自機への向きを記録して、予告音を鳴らす
    if (fc == kGazeStart) {
        PlayShotSound(sound_enemyCharge);
        for (int side = 0; side < 2; side++) {
            const double ex = pEnemyShotSet->x + ((side == 0) ? -kEyeX : kEyeX);
            const double ey = pEnemyShotSet->y + kEyeY;
            double dx = player.x - ex;
            double dy = player.y - ey;
            const double d = sqrt(dx * dx + dy * dy);
            if (d > 0.001) {
                dx /= d;
                dy /= d;
            }
            else {
                dx = 0.0;
                dy = 1.0;
            }
            pEnemyShotSet->param_d[kSetParamGaze + side * 2] = dx;
            pEnemyShotSet->param_d[kSetParamGaze + side * 2 + 1] = dy;
        }
    }

    // 崩壊フェーズの効果音
    if (fc == kRelease) {
        PlayShotSound(sound_enemyShot_heavy);
    }
    if (fc >= kRelease) {
        const int t = fc - kRelease;
        const int interval = (e == 0) ? 14 : 10;
        const int tw = t - kOutlineExpandT;
        if (tw >= 0 && tw < kOutlineWaves * interval && tw % interval == 0) {
            PlayShotSound(sound_enemyShot_light);   // 輪郭の自機狙いの波
        }
    }

    // 全弾の更新（崩壊中に追加された弾も同じループで処理される）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        UpdateShot(pEnemyShotSet, pShot);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Henohenomoheji_Claude()
{
    static int dir;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        dir = 1;
    }
    else {
        // ボスは顔の上で左右にゆっくり揺れるだけ
        enemy.x += 0.5 * (double)dir;
        if (count % 120 == 60) dir *= -1;
    }

    // 一定間隔で顔を1つ描く（通常顔と怒り顔を交互に）
    if (count % kFaceInterval == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFace;
        pEnemyShotSet->x = kFaceCX;
        pEnemyShotSet->y = kFaceCY;
        pEnemyShotSet->muki = kPi / 2.0;
        pEnemyShotSet->kind = (count / kFaceInterval) % 2;   // 表情 0:通常顔 1:怒り顔

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}