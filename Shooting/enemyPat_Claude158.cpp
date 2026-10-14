#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕「ダブルスリット・オブザーバー」
//  二重スリット実験をモチーフにした全4フェーズのループ弾幕。
//
//  [準備]   スリット板(大玉の壁)が左右から組み上がる(1回だけ)
//  [粒子]   小玉が1発ずつスリットを通り抜ける(粒子としての振る舞い)
//  [干渉]   2つのスリットから波面が出て、画面下の「スクリーン線」で干渉縞を作る
//           強め合う位置は弾が密、弱め合う位置は弾が無い安置になる
//  [観測]   検出器(菱形弾)が現れると縞が消え、各スリットから粒子の帯が出る
//           最後は検出器が去り、一瞬だけ干渉縞が復活して一周
//
//  前提: 画面 480x480 / 60fps / 毎フレーム呼ばれる EnemyPat_DoubleSlit_Claude()
//  難易度や弾数は下の定数で調整できます。
// ============================================================

// ------------------------------------------------------------
//  定数(ファイルスコープ。Windows SDK との名前衝突を避けるため k + 長めの名前)
// ------------------------------------------------------------
// ---- スリット板の配置 ----
static constexpr double kSlitCenterX = 240.0;                         // スリット板の中心X
static constexpr double kSlitGap = 140.0;                         // 2つのスリットの中心間距離
static constexpr double kSlitLeftX = kSlitCenterX - kSlitGap / 2.0; // 左スリット中心X
static constexpr double kSlitRightX = kSlitCenterX + kSlitGap / 2.0; // 右スリット中心X
static constexpr double kSlitHalf = 44.0;                          // スリット中心から隣の壁弾の中心までの距離
static constexpr double kSlitWallY = 220.0;                         // 壁の中心Y
static constexpr double kSlitWallR = 20.0;                          // 壁に使う大玉の半径
static constexpr double kSlitWallStep = 28.0;                         // 壁の大玉の並べ間隔
static constexpr int    kSlitWallColor = 7;                           // 壁の色(7:黒)。見づらければ変更
static constexpr double kSlitWallSpeed = 10.0;                        // 壁弾が定位置へ向かう速さ
static constexpr int    kSlitWallSpawnInterval = 3;                   // 壁弾を出す間隔(フレーム)
static constexpr int    kSlitWallSideN = (int)((kSlitLeftX - kSlitHalf) / kSlitWallStep) + 1; // 片側の壁弾の数
static constexpr int    kSlitWallMidN = 3;                           // 2つのスリットの間の壁弾の数
static constexpr double kSlitSourceY = kSlitWallY + kSlitWallR + 2.0; // スリット通過後の波源Y(壁の真下)

// ---- 時間配分(フレーム) ----
static constexpr int kSlitWallFrames = 60;    // 壁の組み上げ待ち時間
static constexpr int kSlitParticleFrames = 360;   // 粒子フェーズ
static constexpr int kSlitWaveFrames = 720;   // 干渉フェーズ
static constexpr int kSlitObserveFrames = 480;   // 観測フェーズ
static constexpr int kSlitObserveBeamFrames = 400;   // 観測フェーズのうち粒子の帯を出す長さ(残りは縞が復活)
static constexpr int kSlitCycleFrames = kSlitParticleFrames + kSlitWaveFrames + kSlitObserveFrames;

// ---- 粒子フェーズ ----
static constexpr int    kSlitParticleInterval = 3;      // 粒子を出す間隔
static constexpr int    kSlitAimHalf = 14;     // スリット中心からの狙いのばらつき幅(片側px)
static constexpr double kSlitDiffract = 0.22;   // スリット通過直後に曲がる最大角度(rad)

// ---- 干渉フェーズ ----
static constexpr double kSlitFocusY = 400.0;   // 干渉縞が最も鮮明になる「スクリーン線」のY(自機の定位置あたり)
static constexpr double kSlitLambda = 56.0;    // 波長(経路差の単位)。小さいほど縞が細かく本数が増える
static constexpr int    kSlitFringeDx = 8;       // スクリーン線上の弾の間隔(px)。小さいほど高密度
static constexpr int    kSlitFringeArrive = 75;      // スリットからスクリーン線までの到達フレーム数
static constexpr int    kSlitFringeInterval = 10;      // 波面を出す間隔(フレーム)
static constexpr double kSlitFringeThreshold = 0.30;    // この強度未満は弾を出さない(=安置)
static constexpr double kSlitPhaseAmp = 0.20;    // 縞が左右に揺れる大きさ(波長単位)
static constexpr double kSlitPhasePeriod = 420.0;   // 縞が揺れる周期(フレーム)

// ---- 観測フェーズ ----
static constexpr int    kSlitDetectorN = 6;        // 検出器(菱形弾)の数(スリットごと)
static constexpr double kSlitDetectorR = 16.0;     // 検出器の公転半径
static constexpr double kSlitDetectorOmega = 0.07;     // 検出器の公転角速度(rad/フレーム)
static constexpr double kSlitDetectorOffsetY = 34.0;    // 壁の上端から検出器の中心までの距離
static constexpr int    kSlitDetectorBlink = 8;        // 検出器の点滅間隔
static constexpr int    kSlitBeamStart = 30;       // 観測フェーズ開始から帯を出し始めるまで
static constexpr int    kSlitBeamInterval = 4;        // 帯の弾列を出す間隔
static constexpr double kSlitBeamSpeed = 4.2;      // 帯の弾速
static constexpr double kSlitBeamSway = 0.50;     // 帯が首を振る最大角度(rad)
static constexpr double kSlitBeamPeriod = 300.0;    // 帯が首を振る周期(フレーム)

// ------------------------------------------------------------
//  共通ヘルパー
// ------------------------------------------------------------
// 効果音を鳴らす(重なってうるさくならないよう、鳴っていたら止めてから再生)
static void SlitPlaySound(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ShotSet を作ってリストの末尾に連結する
static sEnemyShotSet* SlitNewShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = 0;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// ShotSet に弾を1発追加してリストの末尾に連結する
static sEnemyShot* SlitAddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// 全弾を等速直進させる
static void SlitMoveAll(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  [準備] スリット板(壁)
//  左右の端から大玉が滑り込み、中央の仕切りは上から降りてくる。
//  壁弾は定位置に着いたら静止し、パターン終了まで残る。
//  param_d[0],[1] = 定位置(x,y) / param_i[0] = 0:移動中 1:到着済み
// ------------------------------------------------------------
static void ShotSlitWall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % kSlitWallSpawnInterval == 0) {
        const int k = pEnemyShotSet->count / kSlitWallSpawnInterval;
        const int wallKind = img_enemyShotLargeBall[kSlitWallColor];
        if (k < kSlitWallSideN) {
            // 左右の端から1個ずつ。外側から内側へ向かって埋まっていく
            const double lx = (kSlitLeftX - kSlitHalf) - (kSlitWallSideN - 1 - k) * kSlitWallStep;
            sEnemyShot* pLeft = SlitAddShot(pEnemyShotSet, -10.0, kSlitWallY, 0.0, kSlitWallSpeed, wallKind);
            pLeft->param_d[0] = lx;
            pLeft->param_d[1] = kSlitWallY;
            sEnemyShot* pRight = SlitAddShot(pEnemyShotSet, 490.0, kSlitWallY, DX_PI, kSlitWallSpeed, wallKind);
            pRight->param_d[0] = 2.0 * kSlitCenterX - lx;
            pRight->param_d[1] = kSlitWallY;
        }
        else if (k < kSlitWallSideN + kSlitWallMidN) {
            // 2つのスリットの間の仕切りは上から降りてくる
            const int j = k - kSlitWallSideN;
            const double midLeft = kSlitLeftX + kSlitHalf;
            const double midRight = kSlitRightX - kSlitHalf;
            const double mx = midLeft + (midRight - midLeft) * j / (double)(kSlitWallMidN - 1);
            sEnemyShot* pMid = SlitAddShot(pEnemyShotSet, mx, -10.0, DX_PI / 2.0, kSlitWallSpeed, wallKind);
            pMid->param_d[0] = mx;
            pMid->param_d[1] = kSlitWallY;
        }
    }
    // 定位置へ移動。着いたら止まる
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            const double dx = pShot->param_d[0] - pShot->x;
            const double dy = pShot->param_d[1] - pShot->y;
            const double dist = sqrt(dx * dx + dy * dy);
            if (dist <= pShot->speed) {
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
                pShot->speed = 0.0;
                pShot->param_i[0] = 1;
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  [粒子] スリットを通り抜ける粒子(小玉・白)
//  ShotSet 作成時の設定: param_i[0] = 同時に出す粒子の数 / param_i[1] = 1なら効果音
//  敵からいずれかのスリットへ向けて撃ち、壁を抜けた直後にわずかに進路が曲がる(回折)。
//  壁に当たる弾は最初から作らないので、壁側では弾が消える処理は不要。
//  param_i[0](弾側) = 0:未通過 1:通過済み
// ------------------------------------------------------------
static void ShotSlitParticle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (pEnemyShotSet->param_i[1]) SlitPlaySound(sound_enemyShot_light);
        for (int i = 0; i < pEnemyShotSet->param_i[0]; i++) {
            const double slitX = (GetRand(1) == 0) ? kSlitLeftX : kSlitRightX;
            const double targetX = slitX + GetRand(2 * kSlitAimHalf) - kSlitAimHalf;
            const double ox = pEnemyShotSet->x + GetRand(6) - 3;
            const double oy = pEnemyShotSet->y;
            const double muki = atan2(kSlitWallY - oy, targetX - ox);
            const double speed = 2.6 + GetRand(80) / 100.0;
            SlitAddShot(pEnemyShotSet, ox, oy, muki, speed, img_enemyShotSmallBall[6]);
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 壁の下端を抜けた瞬間に1回だけ進路を少し曲げる
        if (pShot->param_i[0] == 0 && pShot->y >= kSlitWallY + kSlitWallR) {
            pShot->param_i[0] = 1;
            pShot->muki += (GetRand(100) - 50) / 50.0 * kSlitDiffract;
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  [干渉] 波面(干渉縞)
//  ShotSet 作成時の設定: param_d[0] = 縞の位相ずらし(波長単位)
//
//  2つのスリットから、スクリーン線(y = kSlitFocusY)上の各点 X へ向けて弾を撃つ。
//  同じ X へ向かう2つの弾は同時に X に到着するよう速さを揃えてある。
//  X ごとの強度は、2つのスリットからの経路差 D = rA - rB から
//      I = cos^2( pi * (D / 波長 + 位相) )
//  で決める。強度が高い X ほど目立つ弾(赤 > 橙 > 黄)にし、低い X には弾を出さない。
//  その結果、スクリーン線付近に「どちらのスリットからの弾も来ない安置」が縞状にできる。
//  縞は波面ごとに位相をずらすことで、ゆっくり左右に揺れる。
// ------------------------------------------------------------
static void ShotSlitFringe(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        SlitPlaySound(sound_enemyShot_light);
        const double phase = pEnemyShotSet->param_d[0];
        const double dyFocus = kSlitFocusY - kSlitSourceY;
        for (int X = 0; X <= 480; X += kSlitFringeDx) {
            // 両スリットからスクリーン線上の点 X までの距離と経路差
            const double dxA = (double)X - kSlitLeftX;
            const double dxB = (double)X - kSlitRightX;
            const double rA = sqrt(dxA * dxA + dyFocus * dyFocus);
            const double rB = sqrt(dxB * dxB + dyFocus * dyFocus);
            const double D = rA - rB;
            double intensity = cos(DX_PI * (D / kSlitLambda + phase));
            intensity *= intensity;
            if (intensity < kSlitFringeThreshold) continue; // 暗線: 弾なし(安置)

            // 強度に応じた弾の種類と色  色: 0:赤 1:黄 8:橙
            int kind;
            if (intensity < 0.60)      kind = img_enemyShotSmallBall[1];  // 弱い明線: 小玉・黄
            else if (intensity < 0.90) kind = img_enemyShotMediumBall[8]; // 明線: 中玉・橙
            else                       kind = img_enemyShotMediumBall[0]; // 縞の中心: 中玉・赤

            for (int s = 0; s < 2; s++) {
                const double sx = (s == 0) ? kSlitLeftX : kSlitRightX;
                const double vx = ((double)X - sx) / kSlitFringeArrive; // 1フレームあたりの速度
                const double vy = dyFocus / kSlitFringeArrive;
                SlitAddShot(pEnemyShotSet, sx, kSlitSourceY, atan2(vy, vx), sqrt(vx * vx + vy * vy), kind);
            }
        }
    }
    SlitMoveAll(pEnemyShotSet);
}

// ------------------------------------------------------------
//  [観測] 検出器(菱形弾・シアン/白で点滅)
//  各スリットの上で公転し、kSlitObserveBeamFrames を過ぎると上方向へ去っていく。
//  param_d[0],[1] = 公転中心(x,y) / param_d[2] = 現在の角度 / param_d[3] = 回転方向(+1/-1)
//  param_i[0] = 0:公転中 1:退場中
// ------------------------------------------------------------
static void ShotSlitDetector(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        SlitPlaySound(sound_enemyShot_heavy);
        for (int s = 0; s < 2; s++) {
            const double cx = (s == 0) ? kSlitLeftX : kSlitRightX;
            const double cy = kSlitWallY - kSlitWallR - kSlitDetectorOffsetY;
            const double dir = (s == 0) ? 1.0 : -1.0; // 左右で逆回転
            for (int k = 0; k < kSlitDetectorN; k++) {
                const double a = 2.0 * DX_PI * k / kSlitDetectorN;
                sEnemyShot* p = SlitAddShot(pEnemyShotSet,
                    cx + kSlitDetectorR * cos(a), cy + kSlitDetectorR * sin(a),
                    a + dir * DX_PI / 2.0, kSlitDetectorR * kSlitDetectorOmega,
                    img_enemyShotDiamond[3]);
                p->param_d[0] = cx;
                p->param_d[1] = cy;
                p->param_d[2] = a;
                p->param_d[3] = dir;
            }
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            if (pShot->count >= kSlitObserveBeamFrames) {
                // 観測終了: 上方向へ退場
                const double a = pShot->param_d[2];
                pShot->param_i[0] = 1;
                pShot->muki = atan2(-fabs(sin(a)), cos(a));
                pShot->speed = 1.0;
            }
            else {
                const double dir = pShot->param_d[3];
                pShot->param_d[2] += kSlitDetectorOmega * dir;
                const double a = pShot->param_d[2];
                pShot->x = pShot->param_d[0] + kSlitDetectorR * cos(a);
                pShot->y = pShot->param_d[1] + kSlitDetectorR * sin(a);
                pShot->muki = a + dir * DX_PI / 2.0; // 公転の接線方向
                // 点滅(シアン <-> 白)
                pShot->kind = img_enemyShotDiamond[((pShot->count / kSlitDetectorBlink) % 2 == 0) ? 3 : 6];
            }
        }
        else {
            pShot->speed += 0.08; // 徐々に加速して去る
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  [観測] 粒子の帯(銃弾・白)
//  ShotSet 作成時の設定: param_d[0] = 左スリットの射出角 / param_d[1] = 右スリットの射出角 / param_i[0] = 1なら効果音
//  各スリットから銃弾を横一列に並べて撃つ。射出角は左右で逆向きに首を振る。
// ------------------------------------------------------------
static void ShotSlitBeam(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (pEnemyShotSet->param_i[0]) SlitPlaySound(sound_enemyShot_medium);
        for (int s = 0; s < 2; s++) {
            const double sx = (s == 0) ? kSlitLeftX : kSlitRightX;
            const double angle = pEnemyShotSet->param_d[s];
            for (int k = -3; k <= 3; k++) {
                SlitAddShot(pEnemyShotSet, sx + k * 6.0, kSlitSourceY, angle, kSlitBeamSpeed, img_enemyShotBullet[6]);
            }
        }
    }
    SlitMoveAll(pEnemyShotSet);
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_DoubleSlit_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = kSlitCenterX;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 300; // 200から変更しないこと
        // スリット板(壁)。以降ずっと残るので、このShotSetは1回だけ作る
        SlitNewShotSet(ShotSlitWall, kSlitCenterX, kSlitWallY);
        SlitPlaySound(sound_enemyCharge);
    }

    // 壁が組み上がるまで待つ
    const int t = count - 1 - kSlitWallFrames;
    if (t < 0) return;

    // 以降は [粒子] -> [干渉] -> [観測] を繰り返す
    const int u = t % kSlitCycleFrames; // 周期内の経過フレーム
    const int waveStart = kSlitParticleFrames;
    const int waveEnd = waveStart + kSlitWaveFrames;
    const int observeStart = waveEnd;
    const int reviveStart = observeStart + kSlitObserveBeamFrames;
    const int observeEnd = observeStart + kSlitObserveFrames;

    // ---- [粒子] ----
    if (u < kSlitParticleFrames) {
        if (u % kSlitParticleInterval == 0) {
            sEnemyShotSet* p = SlitNewShotSet(ShotSlitParticle, enemy.x, enemy.y + 10.0);
            p->param_i[0] = 1 + u / 90;          // 時間とともに同時発射数が 1 -> 4 に増える
            p->param_i[1] = (u % 12 == 0) ? 1 : 0; // 効果音は間引く
        }
    }

    // ---- [干渉] と、[観測] の最後に縞が復活する区間 ----
    if (u == waveStart) SlitPlaySound(sound_enemyCharge);
    const bool fringeOn = (u >= waveStart && u < waveEnd) || (u >= reviveStart && u < observeEnd);
    if (fringeOn && u % kSlitFringeInterval == 0) {
        sEnemyShotSet* p = SlitNewShotSet(ShotSlitFringe, enemy.x, kSlitSourceY);
        p->param_d[0] = kSlitPhaseAmp * sin(2.0 * DX_PI * u / kSlitPhasePeriod);
    }

    // ---- [観測] ----
    if (u >= observeStart && u < observeEnd) {
        const int u4 = u - observeStart;
        if (u4 == 0) {
            SlitPlaySound(sound_enemyCharge);
            SlitNewShotSet(ShotSlitDetector, kSlitCenterX, kSlitWallY);
        }
        if (u4 >= kSlitBeamStart && u4 < kSlitObserveBeamFrames && (u4 - kSlitBeamStart) % kSlitBeamInterval == 0) {
            sEnemyShotSet* p = SlitNewShotSet(ShotSlitBeam, kSlitCenterX, kSlitSourceY);
            const double sway = kSlitBeamSway * sin(2.0 * DX_PI * u4 / kSlitBeamPeriod);
            p->param_d[0] = DX_PI / 2.0 + sway; // 左スリット
            p->param_d[1] = DX_PI / 2.0 - sway; // 右スリット(逆向きに首を振る)
            p->param_i[0] = ((u4 - kSlitBeamStart) % (kSlitBeamInterval * 4) == 0) ? 1 : 0;
        }
    }
}