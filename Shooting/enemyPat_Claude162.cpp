// enemyPat_milkmaid.cpp
// 弾幕「牛乳を注ぐ女」(フェルメール)
//   第1段階: 注ぐ     ... 牛乳の細い糸(白の小玉)が器に落ち、飛沫になって跳ね返る
//   第2段階: 窓の光   ... 左上から差す黄色い短レーザーの帯と、光の中を舞うパン屑(橙の小玉)
//   第3段階: あふれる ... 糸が白の大玉+二重螺旋に変わり、頭巾(白)・胴着(黄)・エプロン(青)の弾が広がる
//   終盤の糸の途切れが器に届く頃、器から全方位リングが溢れて1サイクル(1800フレーム)が終わり、最初に戻る。
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
// 弾の色番号 (0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙)
static constexpr int kMilkColYellow = 1;
static constexpr int kMilkColBlue = 4;
static constexpr int kMilkColWhite = 6;
static constexpr int kMilkColOrange = 8;

// 進行 (1サイクル = 3段階 x 600フレーム)
static constexpr int kMilkPhaseLen = 600;
static constexpr int kMilkCycleLen = 1800;
static constexpr int kMilkStreamInterval = 8;     // 糸のShotSetを作る間隔(フレーム)。ShotSetは作成後この期間だけ糸を出す
static constexpr int kMilkGapStart = 1696;  // 糸が途切れる期間(kMilkStreamIntervalの倍数に揃えること)
static constexpr int kMilkGapEnd = 1760;

// 敵(注ぎ手)の動き
static constexpr double kMilkEnemyY = 40.0;
static constexpr double kMilkEnemyBaseX = 240.0;
static constexpr double kMilkSwayAmp = 70.0;   // 左右の揺れ幅
static constexpr double kMilkSwayPeriod = 360.0;  // 揺れの周期(フレーム)。サイクル長の約数にしておくとループが滑らか
static constexpr double kMilkNozzleDY = 14.0;   // 敵の中心から注ぎ口までの距離

// 牛乳の糸と飛沫
static constexpr double kMilkStreamV0 = 1.0;   // 糸の初速
static constexpr double kMilkStreamGravity = 0.06;  // 糸の重力加速度
static constexpr double kMilkSplashGravity = 0.07;  // 飛沫の重力加速度
static constexpr double kMilkHelixRadius = 24.0;  // 第3段階の二重螺旋の半径
static constexpr double kMilkHelixOmega = 0.22;  // 二重螺旋の角速度(rad/フレーム)

// 器
static constexpr double kMilkBowlY = 372.0;  // 器の縁(水面)のY座標
static constexpr double kMilkBowlHalfW = 46.0;
static constexpr double kMilkBowlDepth = 24.0;
static constexpr int    kMilkBowlShellNum = 11;     // 器の殻(橙の中楕円弾)の数
static constexpr int    kMilkBowlSurfaceNum = 13;     // 水面(白の小玉)の数
static constexpr double kMilkBowlSurfaceStep = 7.0;

// 窓の光(短レーザー)
static constexpr double kMilkRayAngle = DX_PI / 3.0;  // 光の向き(右下へ60度)
static constexpr double kMilkRaySpeed = 2.4;
static constexpr int    kMilkLightRowInterval = 40;  // 同じ光線に次のレーザーを出す間隔。速さ x 間隔 = レーザー同士の間隔(px)
static constexpr int    kMilkLightRowNum = 3;

// ============================================================
//  共通ヘルパー
// ============================================================
static void MilkPlaySound(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ShotSetを作ってリストに登録する
static sEnemyShotSet* MilkCreateSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->kind = 0;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// ShotSetに弾を1発追加する
static sEnemyShot* MilkAddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
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

// 等速直線運動。param_d[0]が0でなければ加速度、param_d[1]を速さの上限として加速する
static void MilkMoveLinear(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_d[0] != 0.0 && pShot->speed < pShot->param_d[1]) {
            pShot->speed += pShot->param_d[0];
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 時刻tでの敵のX座標 (全ての弾の位置計算の基準)
static double MilkEnemyX(int t)
{
    return kMilkEnemyBaseX + kMilkSwayAmp * sin(2.0 * DX_PI * (double)t / kMilkSwayPeriod);
}

// 注ぎ口から出た糸が器の高さに達するまでのフレーム数
static int MilkFallFrames()
{
    const double h = kMilkBowlY - (kMilkEnemyY + kMilkNozzleDY);
    const double t = (-kMilkStreamV0 + sqrt(kMilkStreamV0 * kMilkStreamV0 + 2.0 * kMilkStreamGravity * h)) / kMilkStreamGravity;
    return (int)(t + 0.5);
}

// 器のX座標: 糸の落下地点に常に重なるよう、落下時間ぶん遅れて敵を追う
static double MilkBowlX(int t)
{
    return MilkEnemyX(t - MilkFallFrames());
}

// 飛沫の初速を与える(真上を中心に左右約75度の扇状)
static void MilkSetSplashVelocity(sEnemyShot* pShot, double power)
{
    const double angle = -DX_PI / 2.0 + (GetRand(100) - 50) / 50.0 * (DX_PI * 0.42);
    const double spd = (1.6 + GetRand(240) / 100.0) * power;
    pShot->param_d[0] = spd * cos(angle);
    pShot->param_d[1] = spd * sin(angle);
    pShot->muki = angle;
    pShot->speed = spd;
}

// 糸の弾を飛沫に変え、さらにextraNum発の飛沫を追加する
static void MilkConvertToSplash(sEnemyShotSet* pSet, sEnemyShot* pShot, int extraNum, double power)
{
    if (pShot->param_i[1] != 0) {
        // 大玉は中玉の飛沫になる
        pShot->kind = img_enemyShotMediumBall[kMilkColWhite];
        pShot->param_i[1] = 0;
    }
    pShot->y = kMilkBowlY - 1.0;
    pShot->param_i[0] = 1;
    MilkSetSplashVelocity(pShot, power);
    for (int i = 0; i < extraNum; i++) {
        sEnemyShot* p = MilkAddShot(pSet, pShot->x + (GetRand(16) - 8), kMilkBowlY - 1.0, -DX_PI / 2.0, 0.0,
            img_enemyShotSmallBall[kMilkColWhite]);
        p->param_i[0] = 1;
        MilkSetSplashVelocity(p, power);
    }
}

// ============================================================
//  弾幕: 牛乳の糸と飛沫
//  param_i[0]  : 0=直線落下の糸 / 1=飛沫 / 2=二重螺旋の糸
//  param_i[1]  : 1なら大玉(着弾時に中玉+小玉の飛沫になる)
//  param_d[0,1]: 飛沫のvx,vy / 糸のvyはparam_d[1]
//  param_d[2,3]: 二重螺旋の中心X、位相
//  ShotSetのparam_i[0]が0なら細い糸、1なら太い糸(大玉+二重螺旋)
// ============================================================
static void ShotMilkStream(sEnemyShotSet* pSet)
{
    const bool thick = (pSet->param_i[0] != 0);

    // 作成後kMilkStreamIntervalフレームの間、注ぎ口から糸を出す
    if (pSet->count < kMilkStreamInterval) {
        const double nozzleX = MilkEnemyX(::count);
        const double nozzleY = kMilkEnemyY + kMilkNozzleDY;
        if (!thick) {
            // 細い糸: 1フレームに2発(半フレームずらして間隔を均等にする)
            for (int i = 0; i < 2; i++) {
                const double half = 0.5 * i;
                sEnemyShot* p = MilkAddShot(pSet, nozzleX, nozzleY + kMilkStreamV0 * half, DX_PI / 2.0,
                    kMilkStreamV0 + kMilkStreamGravity * half,
                    img_enemyShotSmallBall[kMilkColWhite]);
                p->param_i[0] = 0;
                p->param_d[1] = kMilkStreamV0 + kMilkStreamGravity * half;
            }
        }
        else {
            // 二重螺旋(小玉): 位相をπずらした2本
            for (int s = 0; s < 2; s++) {
                sEnemyShot* p = MilkAddShot(pSet, nozzleX, nozzleY, DX_PI / 2.0, kMilkStreamV0,
                    img_enemyShotSmallBall[kMilkColWhite]);
                p->param_i[0] = 2;
                p->param_d[1] = kMilkStreamV0;
                p->param_d[2] = nozzleX;
                p->param_d[3] = s * DX_PI;
            }
            // 鎖の芯になる大玉
            if (pSet->count == 0) {
                sEnemyShot* p = MilkAddShot(pSet, nozzleX, nozzleY, DX_PI / 2.0, kMilkStreamV0,
                    img_enemyShotLargeBall[kMilkColWhite]);
                p->param_i[0] = 0;
                p->param_i[1] = 1;
                p->param_d[1] = kMilkStreamV0;
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 飛沫: 放物線
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            pShot->param_d[1] += kMilkSplashGravity;
            pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
            pShot->speed = sqrt(pShot->param_d[0] * pShot->param_d[0] + pShot->param_d[1] * pShot->param_d[1]);
        }
        else {
            // 糸: 重力で加速しながら落下
            pShot->y += pShot->param_d[1];
            pShot->param_d[1] += kMilkStreamGravity;
            if (pShot->param_i[0] == 2) {
                const double rate = (pShot->count < 15) ? pShot->count / 15.0 : 1.0;
                pShot->x = pShot->param_d[2] + kMilkHelixRadius * rate * sin(kMilkHelixOmega * pShot->count + pShot->param_d[3]);
            }
            pShot->muki = DX_PI / 2.0;
            pShot->speed = pShot->param_d[1];
            if (pShot->y >= kMilkBowlY) {
                if (pShot->param_i[1] != 0) MilkConvertToSplash(pSet, pShot, 7, 1.3);
                else                        MilkConvertToSplash(pSet, pShot, 1, 1.0);
            }
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕: 器 (橙の中楕円弾の殻 + 白の小玉の水面)
//  param_i[0]  : 0=殻 / 1=水面
//  param_d[0,1]: 器の中心からの相対座標
//  param_d[2]  : 水面のさざ波の位相
// ============================================================
static void ShotMilkBowl(sEnemyShotSet* pSet)
{
    const double bowlX = MilkBowlX(::count);

    if (pSet->count == 0) {
        // 殻: 下半分の楕円に沿って並べ、各弾の向きは曲線の接線方向にする
        for (int i = 0; i < kMilkBowlShellNum; i++) {
            const double th = DX_PI * (double)i / (double)(kMilkBowlShellNum - 1);
            const double relX = kMilkBowlHalfW * cos(th);
            const double relY = kMilkBowlDepth * sin(th);
            const double tangent = atan2(kMilkBowlDepth * cos(th), -kMilkBowlHalfW * sin(th));
            sEnemyShot* p = MilkAddShot(pSet, bowlX + relX, kMilkBowlY + relY, tangent, 0.0,
                img_enemyShotMediumOval[kMilkColOrange]);
            p->param_i[0] = 0;
            p->param_d[0] = relX;
            p->param_d[1] = relY;
        }
        // 水面
        for (int i = 0; i < kMilkBowlSurfaceNum; i++) {
            const double relX = ((double)i - (kMilkBowlSurfaceNum - 1) / 2.0) * kMilkBowlSurfaceStep;
            sEnemyShot* p = MilkAddShot(pSet, bowlX + relX, kMilkBowlY, 0.0, 0.0,
                img_enemyShotSmallBall[kMilkColWhite]);
            p->param_i[0] = 1;
            p->param_d[0] = relX;
            p->param_d[1] = 0.0;
            p->param_d[2] = i * 0.9;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        const double ripple = (pShot->param_i[0] == 1) ? 1.5 * sin(0.2 * pShot->count + pShot->param_d[2]) : 0.0;
        pShot->x = bowlX + pShot->param_d[0];
        pShot->y = kMilkBowlY + pShot->param_d[1] + ripple;
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕: 窓の光 (黄の短レーザー)
//  左上(上辺と左辺)から右下へ向かう平行な光線を、40フレーム間隔で3列出す
//  ShotSetのparam_i[0]: 何番目の波か(光線の出る位置を波ごとにずらす)
// ============================================================
static void ShotMilkLight(sEnemyShotSet* pSet)
{
    if (pSet->count % kMilkLightRowInterval == 0 && pSet->count / kMilkLightRowInterval < kMilkLightRowNum) {
        const int shift = pSet->param_i[0] % 3;
        const int kind = img_enemyShotLaser[kMilkColYellow];
        // 上辺から (画面外のmargin内から出す)
        for (int k = 0; k < 7; k++) {
            MilkAddShot(pSet, 30.0 + 66.0 * k - 22.0 * shift, -8.0, kMilkRayAngle, kMilkRaySpeed, kind);
        }
        // 左辺から (上辺の光線と平行な間隔になるよう計算)
        for (int j = 0; j < 4; j++) {
            MilkAddShot(pSet, -8.0, 60.0 + 114.0 * j + 38.0 * shift, kMilkRayAngle, kMilkRaySpeed, kind);
        }
    }
    MilkMoveLinear(pSet);
}

// ============================================================
//  弾幕: 光の中を舞うパン屑 (橙の小玉)
//  param_d[0]: 基準X / [1]: 横揺れの振幅 / [2]: 位相 / [3]: 落下速度
// ============================================================
static void ShotMilkDust(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 3; i++) {
            sEnemyShot* p = MilkAddShot(pSet, (double)GetRand(480), -6.0, DX_PI / 2.0, 0.0,
                img_enemyShotSmallBall[kMilkColOrange]);
            p->param_d[0] = p->x;
            p->param_d[1] = 6.0 + GetRand(1400) / 100.0;
            p->param_d[2] = GetRand(628) / 100.0;
            p->param_d[3] = 0.55 + GetRand(60) / 100.0;
        }
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        const double nx = pShot->param_d[0] + pShot->param_d[1] * sin(0.04 * pShot->count + pShot->param_d[2]);
        const double ny = pShot->y + pShot->param_d[3];
        const double dx = nx - pShot->x;
        const double dy = ny - pShot->y;
        pShot->muki = atan2(dy, dx);
        pShot->speed = sqrt(dx * dx + dy * dy);
        pShot->x = nx;
        pShot->y = ny;
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕: 女の姿 (頭巾=白の中玉の輪 / 胴着=黄の菱形弾の輪 / エプロン=青の鱗弾の扇)
//  ShotSetのparam_i[0]: 何回目の一斉発射か(回転・揺れの位相に使う)
// ============================================================
static void ShotMilkBody(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int salvo = pSet->param_i[0];
        MilkPlaySound(sound_enemyShot_medium);

        // 頭巾: 白の中玉のリング
        for (int i = 0; i < 24/2; i++) {
            const double ang = 2.0 * DX_PI * i / 24.0*2 + salvo * 0.13;
            MilkAddShot(pSet, pSet->x + 14.0 * cos(ang), pSet->y + 14.0 * sin(ang), ang, 1.5,
                img_enemyShotMediumBall[kMilkColWhite]);
        }
        // 胴着: 黄の菱形弾のリング(頭巾と逆方向にずらして回す)
        for (int i = 0; i < 32/2; i++) {
            const double ang = 2.0 * DX_PI * (i + 0.5) / 32.0*2 - salvo * 0.17;
            MilkAddShot(pSet, pSet->x + 24.0 * cos(ang), pSet->y + 24.0 * sin(ang), ang, 2.0,
                img_enemyShotDiamond[kMilkColYellow]);
        }
        // エプロン: 青の鱗弾を下向きの扇状に2層。布が垂れるようにゆっくり加速する
        const double swivel = 0.2 * sin(salvo * 0.9);
        for (int layer = 0; layer < 2; layer++) {
            for (int i = 0; i < 25-10; i++) {
                const double ang = DX_PI / 2.0 + ((i - 12+5) + 0.5 * layer) / 12.0 * 1.2*3 + swivel;
                sEnemyShot* p = MilkAddShot(pSet, pSet->x, pSet->y + 20.0, ang, 1.7 + 0.5 * layer,
                    img_enemyShotScale[kMilkColBlue]);
                p->param_d[0] = 0.008;  // 加速度
                p->param_d[1] = 3.4;    // 速さの上限
            }
        }
    }
    MilkMoveLinear(pSet);
}

// ============================================================
//  弾幕: 器から溢れる全方位リング (白と青)
// ============================================================
static void ShotMilkOverflow(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        MilkPlaySound(sound_enemyShot_extreme);
        // 白小玉 / 青小玉 / 白中玉 / 青中玉 の4層。1層ごとに半ピッチずらす
        for (int layer = 0; layer < 4; layer++) {
            const double speed = 1.6 + 0.5 * layer;
            int kind;
            switch (layer) {
            case 0:  kind = img_enemyShotSmallBall[kMilkColWhite];  break;
            case 1:  kind = img_enemyShotSmallBall[kMilkColBlue];   break;
            case 2:  kind = img_enemyShotMediumBall[kMilkColWhite]; break;
            default: kind = img_enemyShotMediumBall[kMilkColBlue];  break;
            }
            for (int i = 0; i < 72; i++) {
                const double ang = 2.0 * DX_PI * ((double)i + 0.5 * (layer % 2)) / 72.0;
                MilkAddShot(pSet, pSet->x, pSet->y, ang, speed, kind);
            }
        }
        // 溢れ出す大きな雫
        for (int i = 0; i < 12; i++) {
            const double ang = 2.0 * DX_PI * (i + 0.25) / 12.0;
            MilkAddShot(pSet, pSet->x, pSet->y, ang, 1.0, img_enemyShotLargeBall[kMilkColWhite]);
        }
    }
    MilkMoveLinear(pSet);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Milkmaid_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.maxHp = enemy.hp = 200;
        // 器は最初に1つだけ作り、以後ずっと残る
        MilkCreateSet(ShotMilkBowl, MilkBowlX(count), kMilkBowlY);
    }
    enemy.x = MilkEnemyX(count);
    enemy.y = kMilkEnemyY;

    const int tc = (count - 1) % kMilkCycleLen;  // サイクル内の経過フレーム
    const int ph = tc / kMilkPhaseLen;           // 0:注ぐ 1:窓の光 2:あふれる
    const int pt = tc % kMilkPhaseLen;           // 段階内の経過フレーム

    // 牛乳の糸: 一定間隔でShotSetを作る(途切れ期間を除く)。第3段階は太い糸
    if (tc % kMilkStreamInterval == 0 && !(tc >= kMilkGapStart && tc < kMilkGapEnd)) {
        sEnemyShotSet* pSet = MilkCreateSet(ShotMilkStream, enemy.x, enemy.y + kMilkNozzleDY);
        pSet->param_i[0] = (ph == 2) ? 1 : 0;
        if (tc % 24 == 0) MilkPlaySound(sound_enemyShot_light);
    }

    // 第2段階: 窓の光とパン屑
    if (tc >= kMilkPhaseLen - 30 && tc < kMilkPhaseLen * 2 - 30 && (tc - (kMilkPhaseLen - 30)) % 100 == 0) {
        MilkPlaySound(sound_enemyCharge);  // 光が差す30フレーム前の予告音
    }
    if (ph == 1 && pt % 100 == 0) {
        sEnemyShotSet* pSet = MilkCreateSet(ShotMilkLight, enemy.x, enemy.y);
        pSet->param_i[0] = pt / 100;
        MilkPlaySound(sound_enemyShot_medium);
    }
    if (ph == 1 && pt % 12 == 0) {
        MilkCreateSet(ShotMilkDust, enemy.x, enemy.y);
    }

    // 第3段階: 女の姿(終盤の途切れ・リングと重ならないよう、最後の数回は出さない)
    if (ph == 2 && pt % 40 == 0 && pt < 500) {
        sEnemyShotSet* pSet = MilkCreateSet(ShotMilkBody, enemy.x, enemy.y);
        pSet->param_i[0] = pt / 40;
    }

    // 終盤: 糸の途切れの先頭が器に届いた瞬間、器から全方位リングが溢れる
    if (tc == kMilkGapStart + MilkFallFrames()) {
        MilkCreateSet(ShotMilkOverflow, MilkBowlX(count), kMilkBowlY - 4.0);
    }
}