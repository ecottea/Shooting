// 罔両「禅寺に棲む妖蝶」(東方妖々夢 Phantasm・八雲紫 スペルカードNo.136)
//
// 調査結果の要点:
//   ・青蝶弾が外へ飛んで止まり、その後ゆっくり紫のほうへ戻ってくる(画面外からの不意打ちが特徴)
//   ・赤レーザーとワインダー(うねる弾列)を併用する
//   ・音のタイミングで撃ち分けられ、予告音に合わせて動く
//
// 使用素材:
//   弾  : 中玉(青)=青蝶弾の代用 / 鱗弾(マゼンタ)=ワインダー / 短レーザー(赤)=赤レーザー
//   効果音: sound_enemyCharge(予告音) / sound_enemyShot_light(ワインダー)
//           sound_enemyShot_medium(蝶弾) / sound_enemyShot_heavy(赤レーザー)
//
// 仕様メモ:
//   ・count, pEnemyShotSet->count, pEnemyShot->count のインクリメントと画面外の弾の消去はメインルーチン側
//   ・画面外に出た弾はメイン側で消されるため、蝶弾は画面内で止まるように移動距離を制限している
//   ・全ての弾で muki を進行方向に設定している
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {

    // ---------------------------------------------------------------
    // 定数
    // ---------------------------------------------------------------
    // ゲーム画面
    constexpr double kZenScreenW = 480.0;
    constexpr double kZenScreenH = 480.0;
    constexpr double kZenScreenMargin = 8.0; // 蝶弾が止まる位置の画面端からの余白

    // 弾の色(img_enemyShotXxx[色] の添字)
    constexpr int kZenColorRed = 0;
    constexpr int kZenColorBlue = 4;
    constexpr int kZenColorMagenta = 5;

    // 弾の種類(ZenBulletImage で画像を選ぶのに使う)
    constexpr int kZenTypeMediumBall = 0;
    constexpr int kZenTypeScale = 1;
    constexpr int kZenTypeLaser = 2;

    // 紫の位置
    constexpr double kZenBossX = 240.0;
    constexpr double kZenBossY = 90.0;

    // 1周期のフレーム数
    constexpr int kZenCycleFrames = 240;

    // 青蝶弾のリング
    constexpr int    kZenRingStart = 20;       // 1発目を撃つフレーム
    constexpr int    kZenRingInterval = 30;    // リング同士の間隔
    constexpr int    kZenRingCount = 3;        // 1周期に撃つリング数
    constexpr int    kZenRingBullets = 48;     // 1リングの弾数
    constexpr double kZenRingMaxDist = 130.0;  // 1リング目の最大移動距離
    constexpr double kZenRingDistStep = 40.0;  // リングごとの移動距離の増分
    constexpr int    kZenRingOutFrames = 60;   // 外へ飛んで止まるまで
    constexpr int    kZenRingHoldFrames = 20;  // 止まっている時間
    constexpr double kZenReturnAccel = 0.025;  // 戻り始めの加速度
    constexpr double kZenReturnMaxSpeed = 1.3; // 戻りの最高速度(ゆっくり)
    constexpr double kZenReturnVanishDist = 14.0; // 紫にこの距離まで戻ったら消す

    // ワインダー(左右対称にうねる自機狙いの弾列)
    constexpr int    kZenWinderStart = 90;
    constexpr int    kZenWinderFrames = 90;    // 弾列を出し続けるフレーム数
    constexpr int    kZenWinderInterval = 2;   // 発射間隔
    constexpr int    kZenWinderPeriodEmits = 24; // うねり1周分の発射回数
    constexpr double kZenWinderSwing = 0.8;    // うねりの振れ幅(rad)
    constexpr int    kZenWinderWidth = 3;      // 1回に撃つ弾数(弾列の太さ)
    constexpr double kZenWinderSpacing = 0.05; // 弾列内の弾の角度間隔(rad)
    constexpr double kZenWinderSpeed = 2.6;

    // 赤レーザー(自機狙いの扇)
    constexpr int    kZenLaserStart = 185;
    constexpr int    kZenLaserInterval = 12;
    constexpr int    kZenLaserVolleys = 4;
    constexpr int    kZenLaserWays = 13;
    constexpr double kZenLaserSpacing = 0.07*1.5;  // 扇の角度間隔(rad)
    constexpr double kZenLaserSpeed = 3.4;
    constexpr int    kZenLaserWarning = 10;    // 予告音を鳴らすフレーム数(発射の何フレーム前か)

    // ---------------------------------------------------------------
    // 共通処理
    // ---------------------------------------------------------------
    // 種類と色から弾画像を返す
    int ZenBulletImage(int type, int color)
    {
        switch (type) {
        case kZenTypeScale: return img_enemyShotScale[color];
        case kZenTypeLaser: return img_enemyShotLaser[color];
        default:            return img_enemyShotMediumBall[color];
        }
    }

    // 効果音を頭から鳴らし直す
    void ZenPlaySe(int soundHandle)
    {
        if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
        PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
    }

    // ShotSetを生成して敵弾セットのリストに繋ぐ
    sEnemyShotSet* ZenCreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = func;
        pEnemyShotSet->x = x;
        pEnemyShotSet->y = y;
        pEnemyShotSet->muki = muki;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
        return pEnemyShotSet;
    }

    // 弾をShotSetの弾リストの末尾に繋ぐ
    void ZenLinkShot(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
    {
        pEnemyShot->margin = 120;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // (x, y) から向き muki へ進んだとき、画面内の余白を保って止まれる距離を返す
    double ZenClampTravel(double x, double y, double muki, double maxDist)
    {
        const double dx = cos(muki);
        const double dy = sin(muki);
        double limit = maxDist;
        if (dx > 1e-9) {
            limit = fmin(limit, (kZenScreenW - kZenScreenMargin - x) / dx);
        }
        else if (dx < -1e-9) {
            limit = fmin(limit, (kZenScreenMargin - x) / dx);
        }
        if (dy > 1e-9) {
            limit = fmin(limit, (kZenScreenH - kZenScreenMargin - y) / dy);
        }
        else if (dy < -1e-9) {
            limit = fmin(limit, (kZenScreenMargin - y) / dy);
        }
        return limit < 0.0 ? 0.0 : limit;
    }

    // ---------------------------------------------------------------
    // 弾幕：青蝶弾のリング(外へ飛ぶ → 止まる → 紫へゆっくり戻る)
    //   ShotSet: param_i[0]=弾数, param_d[0]=最大移動距離, muki=リングの基準角
    //   Shot   : param_d[0]=初速, param_d[1],[2]=戻り先(紫の位置)
    // ---------------------------------------------------------------
    void ShotButterflyRing(sEnemyShotSet* pEnemyShotSet)
    {
        if (pEnemyShotSet->count == 0) {
            const int n = pEnemyShotSet->param_i[0];
            for (int i = 0; i < n; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                const double muki = pEnemyShotSet->muki + 2.0 * DX_PI * i / n;
                // 画面内で止まれる距離に制限する(画面外に出るとメイン側で消されてしまうため)
                const double dist = ZenClampTravel(pEnemyShotSet->x, pEnemyShotSet->y, muki, pEnemyShotSet->param_d[0]);
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = muki;
                // 速度を一次関数で0まで落とす。移動距離の合計は初速 * (kZenRingOutFrames + 1) / 2
                pEnemyShot->param_d[0] = 2.0 * dist / (kZenRingOutFrames + 1);
                pEnemyShot->param_d[1] = pEnemyShotSet->x;
                pEnemyShot->param_d[2] = pEnemyShotSet->y;
                pEnemyShot->speed = pEnemyShot->param_d[0];
                pEnemyShot->kind = ZenBulletImage(kZenTypeMediumBall, kZenColorBlue);
                ZenLinkShot(pEnemyShotSet, pEnemyShot);
            }
        }
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next; // 削除に備えて先に控える
            const int t = pShot->count;
            if (t < kZenRingOutFrames) {
                // 外へ飛びながら減速
                pShot->speed = pShot->param_d[0] * (1.0 - (double)t / kZenRingOutFrames);
            }
            else if (t < kZenRingOutFrames + kZenRingHoldFrames) {
                // 停止
                pShot->speed = 0.0;
            }
            else {
                // 紫のほうへ加速しながら戻る
                const double dx = pShot->param_d[1] - pShot->x;
                const double dy = pShot->param_d[2] - pShot->y;
                if (sqrt(dx * dx + dy * dy) < kZenReturnVanishDist) {
                    // 紫まで戻ったので消す
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                    pShot = pNext;
                    continue;
                }
                pShot->muki = atan2(dy, dx);
                const double accel = (t - kZenRingOutFrames - kZenRingHoldFrames + 1) * kZenReturnAccel;
                pShot->speed = accel < kZenReturnMaxSpeed ? accel : kZenReturnMaxSpeed;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pNext;
        }
    }

    // ---------------------------------------------------------------
    // 弾幕：等速直進の扇(ワインダーと赤レーザーで共用)
    //   ShotSet: param_i[0]=弾数, param_i[1]=弾の種類, param_i[2]=色,
    //            param_d[0]=弾同士の角度間隔, param_d[1]=速さ, muki=扇の中心角
    // ---------------------------------------------------------------
    void ShotStraight(sEnemyShotSet* pEnemyShotSet)
    {
        if (pEnemyShotSet->count == 0) {
            const int n = pEnemyShotSet->param_i[0];
            for (int i = 0; i < n; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = pEnemyShotSet->muki + (i - (n - 1) * 0.5) * pEnemyShotSet->param_d[0];
                pEnemyShot->speed = pEnemyShotSet->param_d[1];
                pEnemyShot->kind = ZenBulletImage(pEnemyShotSet->param_i[1], pEnemyShotSet->param_i[2]);
                ZenLinkShot(pEnemyShotSet, pEnemyShot);
            }
        }
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }

} // namespace

// ---------------------------------------------------------------
// 敵本体のパターン: 罔両「禅寺に棲む妖蝶」
//   1周期(240フレーム)の流れ
//     0   予告音
//     20  青蝶弾のリングを30フレーム間隔で3発(外へ飛び、止まり、紫へゆっくり戻る)
//     90  ワインダー開始(自機狙いを基準に左右対称にうねる弾列を90フレーム)
//     175 予告音
//     185 赤レーザーの扇(自機狙い)を12フレーム間隔で4発
// ---------------------------------------------------------------
void EnemyPat_Zendera_Claude()
{
    static double ringBase;   // 青蝶弾リングの基準角(周期ごとにランダム)
    static double winderBase; // ワインダーの基準角(開始時の自機方向)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = kZenBossX;
        enemy.y = kZenBossY;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        ringBase = 0.0;
        winderBase = DX_PI / 2.0;
    }

    const int t = (count - 1) % kZenCycleFrames;

    // 予告音
    if (t == 0 || t == kZenLaserStart - kZenLaserWarning) {
        ZenPlaySe(sound_enemyCharge);
    }

    // 青蝶弾のリング
    if (t == kZenRingStart) {
        // GetRand(359) は 0〜359 の360通り
        ringBase = GetRand(359) * DX_PI / 180.0;
    }
    if (t >= kZenRingStart && (t - kZenRingStart) % kZenRingInterval == 0) {
        const int k = (t - kZenRingStart) / kZenRingInterval;
        if (k < kZenRingCount) {
            // リングごとに基準角を 1/3 ステップずつずらして隙間を埋める
            const double muki = ringBase + 2.0 * DX_PI / kZenRingBullets * k / kZenRingCount;
            sEnemyShotSet* pEnemyShotSet = ZenCreateShotSet(ShotButterflyRing, enemy.x, enemy.y, muki);
            pEnemyShotSet->param_i[0] = kZenRingBullets;
            pEnemyShotSet->param_d[0] = kZenRingMaxDist + kZenRingDistStep * k;
            ZenPlaySe(sound_enemyShot_medium);
        }
    }

    // ワインダー
    if (t == kZenWinderStart) {
        winderBase = atan2(player.y - enemy.y, player.x - enemy.x);
    }
    if (t >= kZenWinderStart && t < kZenWinderStart + kZenWinderFrames && (t - kZenWinderStart) % kZenWinderInterval == 0) {
        const int k = (t - kZenWinderStart) / kZenWinderInterval;
        const double swing = kZenWinderSwing * sin(2.0 * DX_PI * k / kZenWinderPeriodEmits);
        for (int side = 0; side < 2; side++) {
            // 2本の弾列は基準角を挟んで左右対称にうねる
            const double muki = winderBase + (side == 0 ? swing : -swing);
            sEnemyShotSet* pEnemyShotSet = ZenCreateShotSet(ShotStraight, enemy.x, enemy.y, muki);
            pEnemyShotSet->param_i[0] = kZenWinderWidth;
            pEnemyShotSet->param_i[1] = kZenTypeScale;
            pEnemyShotSet->param_i[2] = kZenColorMagenta;
            pEnemyShotSet->param_d[0] = kZenWinderSpacing;
            pEnemyShotSet->param_d[1] = kZenWinderSpeed;
        }
        if (k % 6 == 0) ZenPlaySe(sound_enemyShot_light);
    }

    // 赤レーザー
    if (t >= kZenLaserStart && (t - kZenLaserStart) % kZenLaserInterval == 0) {
        const int k = (t - kZenLaserStart) / kZenLaserInterval;
        if (k < kZenLaserVolleys) {
            // GetRand(20) は 0〜20 の21通り。-10〜+10 を100で割って ±0.1rad のぶれにする
            const double aim = atan2(player.y - enemy.y, player.x - enemy.x) + (GetRand(20) - 10) / 100.0;
            sEnemyShotSet* pEnemyShotSet = ZenCreateShotSet(ShotStraight, enemy.x, enemy.y, aim);
            pEnemyShotSet->param_i[0] = kZenLaserWays;
            pEnemyShotSet->param_i[1] = kZenTypeLaser;
            pEnemyShotSet->param_i[2] = kZenColorRed;
            pEnemyShotSet->param_d[0] = kZenLaserSpacing;
            pEnemyShotSet->param_d[1] = kZenLaserSpeed;
            ZenPlaySe(sound_enemyShot_heavy);
        }
    }
}