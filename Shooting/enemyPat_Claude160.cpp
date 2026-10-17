#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <string.h>

// ============================================================
//  弾幕「黄金角の結晶 ― Phyllotaxis Lattice」
//
//  ひまわりの種の配置(黄金角螺旋)を静止格子として展開し、
//  格子ごと回転・呼吸・ドリフトさせる。2枚目の逆回転格子を重ねてモアレを作り、
//  最後は中心へ収束して消える。乱数は一切使わず全て決定論的(TAS・リプレイ向け)。
//
//  ◆ 実装方針
//   格子の点 n の位置は「時刻 T だけから決まる式」で毎フレーム計算し直す。
//   画面外へ出て本体側に消された弾も、画面内に戻る位置になれば再生成されるので、
//   回転・ドリフトで格子が画面外に出入りしても画面全域が常に格子で覆われる。
//   (画面の隅に格子の無い安全地帯ができるのを防ぐため)
//
//  ◆ タイムライン(周期内の経過フレーム T)
//   Phase1    0〜300 : 展開。敵から弾を射出し、減速しながら各自の格子点へ到達して静止
//   Phase2  300〜360 : 結晶化。21本→34本→55本の螺旋腕を白く走査して見せる
//   Phase3  360〜1200: 格子全体の回転 + 呼吸(拡縮) + ドリフト
//   Phase4 1200〜1800: 逆位相格子(黄金角が逆向き・逆回転・菱形弾)を重ねる
//   Phase5 1800〜1920: 両格子が中心へ収束。中心の半径 PHYLLO_SAFE_RADIUS の円に入った弾は消える
//   そのあと余白を挟んで同じパターンを繰り返す
// ============================================================

// ------------------------------------------------------------
//  調整用の定数
// ------------------------------------------------------------
static const double PHYLLO_FIELD_CENTER_X = 240.0; // 格子の中心(ゲーム画面は 480x480)
static const double PHYLLO_FIELD_CENTER_Y = 240.0;
static const double PHYLLO_SCREEN_SIZE = 480.0;
static const double PHYLLO_EMIT_RADIUS = 340.0; // 中心から画面の隅までの距離(約339.4)

static const int    PHYLLO_T_CRYSTAL = 300;   // Phase2 開始
static const int    PHYLLO_T_MOTION = 360;   // Phase3 開始
static const int    PHYLLO_T_OVERLAY = 1200;  // Phase4 開始
static const int    PHYLLO_T_CONVERGE = 1800;  // Phase5 開始
static const int    PHYLLO_CONVERGE_FRAMES = 120;   // Phase5 の長さ
static const int    PHYLLO_T_END = PHYLLO_T_CONVERGE + PHYLLO_CONVERGE_FRAMES; // 1920
static const int    PHYLLO_CYCLE_FRAMES = PHYLLO_T_END + 60; // 繰り返しの周期(余白60f)
static const int    PHYLLO_FLIGHT_FRAMES = 90;    // 射出から格子点に静止するまでの時間
static const double PHYLLO_RAMP_FRAMES = 60.0;  // 回転・呼吸・ドリフトの立ち上がり時間
static const double PHYLLO_SAFE_RADIUS = 14.0;  // Phase5 で弾が消える中心円の半径
static const int    PHYLLO_BULLET_BUDGET = 3900;  // 2つの格子の合計弾数の上限(sEnemyShot プールは4096)

static const int    PHYLLO_LATTICE_MAIN = 0;     // 主格子(小玉)
static const int    PHYLLO_LATTICE_OVERLAY = 1;     // 逆位相格子(菱形弾)
static const int    PHYLLO_LATTICE_NUM = 2;
static const int    PHYLLO_MAX_POINTS = 5200;

static const double PHYLLO_VANISH_POS = -10000.0; // 弾を消すときの退避座標(消去は本体側に任せる)

// 格子ごとの設定
struct sPhylloSpec {
    double spacing;       // 点 i の半径 = spacing * sqrt(i)。小さいほど密
    double turnSign;      // +1: 黄金角が正方向 / -1: 逆方向
    int    pointNum;      // 計算する点の数(画面外に出ても再入場できる分まで含める)
    double spinDeg;       // 回転速度(度/フレーム)
    double breathAmp;     // 呼吸の振幅(拡縮率)
    double breathPeriod;  // 呼吸の周期(フレーム)
    double breathPhase;   // 呼吸の位相(ラジアン)
    double driftX;        // ドリフトの振幅 x
    double driftXPeriod;
    double driftY;        // ドリフトの振幅 y
    double driftYPeriod;
    double driftYPhase;
    int    motionStart;   // 回転・呼吸・ドリフトの開始フレーム
    int    emitStart;     // 射出の開始フレーム
    int    emitSpan;      // 射出にかける時間
    bool   emitOuterFirst;// true: 外側の点から射出 / false: 内側の点から射出
};

static const sPhylloSpec PHYLLO_SPEC[PHYLLO_LATTICE_NUM] = {
    // 主格子: 密な格子。外側へ向かって順に展開する
    { 5.4, +1.0, 5200, +0.35, 0.06, 240.0, 0.0,    18.0, 311.0, 14.0, 197.0, 1.0, PHYLLO_T_MOTION,  0,                210, false },
    // 逆位相格子: 疎な格子。逆回転。外側から内側へ向かって展開する
    { 9.5, -1.0, 1900, -0.50, 0.05, 180.0, DX_PI,  20.0, 257.0, 16.0, 173.0, 2.1, PHYLLO_T_OVERLAY, PHYLLO_T_OVERLAY, 180, true  },
};

// 色(0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙)
// 隣り合う螺旋腕の番号は 21 を法として ±8 / ±13 ずれる。7 を法にすると ±1 になるので、
// 腕番号 % 7 でパレットを引くと隣の腕と色が1段ずつ変わる虹色の螺旋になる。
static const int PHYLLO_PALETTE_MAIN[7] = { 0, 8, 1, 2, 3, 4, 5 };
static const int PHYLLO_PALETTE_OVERLAY[3] = { 3, 4, 6 }; // 3 でも同様に ±1 になる
static const int PHYLLO_ARM_COUNTS[3] = { 21, 34, 55 }; // フィボナッチ数の螺旋腕

// ------------------------------------------------------------
//  内部テーブル(最初の呼び出しで1回だけ計算)
// ------------------------------------------------------------
static double s_baseX[PHYLLO_LATTICE_NUM][PHYLLO_MAX_POINTS];     // 格子点の基準座標(中心からの相対)
static double s_baseY[PHYLLO_LATTICE_NUM][PHYLLO_MAX_POINTS];
static double s_baseTheta[PHYLLO_LATTICE_NUM][PHYLLO_MAX_POINTS]; // 外向き方向
static int    s_emitTime[PHYLLO_LATTICE_NUM][PHYLLO_MAX_POINTS];  // 射出フレーム
static bool   s_tableReady = false;

static int    s_emitShift[PHYLLO_LATTICE_NUM];                    // 最初のフレームから弾が出るようにする前倒し量
static unsigned char s_alive[PHYLLO_LATTICE_NUM][PHYLLO_MAX_POINTS]; // その点の弾が今リストにあるか
static int    s_liveNum[PHYLLO_LATTICE_NUM];

struct sPhylloFrame {
    double cx, cy;       // 格子の中心
    double phi;          // 回転角
    double cosPhi, sinPhi;
    double scale;        // 呼吸と収束を合わせた拡大率
};

struct sPhylloPlace {
    double x, y, muki;
};

static void PhylloInitTable()
{
    if (s_tableReady) return;
    const double goldenAngle = DX_PI * (3.0 - sqrt(5.0)); // 約137.5078度
    for (int id = 0; id < PHYLLO_LATTICE_NUM; id++) {
        const sPhylloSpec& spec = PHYLLO_SPEC[id];
        for (int n = 0; n < spec.pointNum; n++) {
            const double index = (double)(n + 1); // 中心の弾(index 0)は置かない
            const double r = spec.spacing * sqrt(index);
            const double theta = spec.turnSign * goldenAngle * index;
            s_baseX[id][n] = r * cos(theta);
            s_baseY[id][n] = r * sin(theta);
            s_baseTheta[id][n] = theta;
            double ratio = (r < PHYLLO_EMIT_RADIUS ? r : PHYLLO_EMIT_RADIUS) / PHYLLO_EMIT_RADIUS;
            if (spec.emitOuterFirst) ratio = 1.0 - ratio;
            s_emitTime[id][n] = spec.emitStart + (int)(spec.emitSpan * ratio);
        }
    }
    s_tableReady = true;
}

static double PhylloWrapPi(double a)
{
    while (a > DX_PI) a -= 2.0 * DX_PI;
    while (a < -DX_PI) a += 2.0 * DX_PI;
    return a;
}

static bool PhylloInScreen(double x, double y)
{
    return x >= 0.0 && x <= PHYLLO_SCREEN_SIZE && y >= 0.0 && y <= PHYLLO_SCREEN_SIZE;
}

// 時刻 T における格子全体の状態
static void PhylloMakeFrame(int id, int T, sPhylloFrame* f)
{
    const sPhylloSpec& spec = PHYLLO_SPEC[id];
    double tau = (double)(T - spec.motionStart);
    if (tau < 0.0) tau = 0.0;

    // 立ち上がり: 角速度は 0 から二次関数で増やし、その積分を回転角にする
    double spinIntegral, amp;
    if (tau < PHYLLO_RAMP_FRAMES) {
        const double u = tau / PHYLLO_RAMP_FRAMES;
        spinIntegral = tau * tau * tau / (3.0 * PHYLLO_RAMP_FRAMES * PHYLLO_RAMP_FRAMES);
        amp = u * u * (3.0 - 2.0 * u); // smoothstep
    }
    else {
        spinIntegral = PHYLLO_RAMP_FRAMES / 3.0 + (tau - PHYLLO_RAMP_FRAMES);
        amp = 1.0;
    }

    const double twoPi = 2.0 * DX_PI;
    f->phi = spec.spinDeg * DX_PI / 180.0 * spinIntegral;
    f->cosPhi = cos(f->phi);
    f->sinPhi = sin(f->phi);
    double scale = 1.0 + amp * spec.breathAmp * sin(twoPi * tau / spec.breathPeriod + spec.breathPhase);
    double offsetX = amp * spec.driftX * sin(twoPi * tau / spec.driftXPeriod);
    double offsetY = amp * spec.driftY * sin(twoPi * tau / spec.driftYPeriod + spec.driftYPhase);

    // Phase5: 中心へ収束(ドリフトも 0 に戻す)
    double contract = 1.0;
    if (T > PHYLLO_T_CONVERGE) {
        contract = 1.0 - (double)(T - PHYLLO_T_CONVERGE) / (double)PHYLLO_CONVERGE_FRAMES;
        if (contract < 0.0) contract = 0.0;
    }
    f->cx = PHYLLO_FIELD_CENTER_X + offsetX * contract;
    f->cy = PHYLLO_FIELD_CENTER_Y + offsetY * contract;
    f->scale = scale * contract;
}

// 点 n の、時刻 T での位置と向き
static void PhylloPlace(const sEnemyShotSet* pEnemyShotSet, const sPhylloFrame& f, int id, int n, int T, sPhylloPlace* out)
{
    const double bx = s_baseX[id][n];
    const double by = s_baseY[id][n];
    const double targetX = f.cx + f.scale * (f.cosPhi * bx - f.sinPhi * by);
    const double targetY = f.cy + f.scale * (f.sinPhi * bx + f.cosPhi * by);

    // 向き: 通常は格子の外向き。収束中は内向きへ回す
    double radial = s_baseTheta[id][n] + f.phi;
    if (T >= PHYLLO_T_CONVERGE) {
        double w = (double)(T - PHYLLO_T_CONVERGE) / 20.0;
        if (w > 1.0) w = 1.0;
        radial += DX_PI * w;
    }

    const int emitTime = s_emitTime[id][n] - s_emitShift[id];
    const int arriveTime = emitTime + PHYLLO_FLIGHT_FRAMES;
    if (T >= arriveTime) {
        out->x = targetX;
        out->y = targetY;
        out->muki = radial;
        return;
    }

    // 展開中: 敵の位置から目標点へ、減速しながら飛ぶ(ease-out)
    double u = (double)(T - emitTime) / (double)PHYLLO_FLIGHT_FRAMES;
    if (u < 0.0) u = 0.0;
    if (u > 1.0) u = 1.0;
    const double e = 1.0 - (1.0 - u) * (1.0 - u);
    out->x = pEnemyShotSet->x + (targetX - pEnemyShotSet->x) * e;
    out->y = pEnemyShotSet->y + (targetY - pEnemyShotSet->y) * e;
    // 向き: 飛んでいる間は進行方向、到達に近づくほど格子の外向きへ寄せる
    const double travel = atan2(targetY - pEnemyShotSet->y, targetX - pEnemyShotSet->x);
    out->muki = radial + PhylloWrapPi(travel - radial) * (1.0 - u);
}

// 弾の色。Phase2 の間だけ主格子の螺旋腕を白く走査する
static int PhylloColor(int id, int index, int T)
{
    if (id == PHYLLO_LATTICE_MAIN) {
        if (T >= PHYLLO_T_CRYSTAL && T < PHYLLO_T_MOTION) {
            const int sweep = (T - PHYLLO_T_CRYSTAL) / 20; // 0:21本 1:34本 2:55本
            const int frame = (T - PHYLLO_T_CRYSTAL) % 20;
            const int arms = PHYLLO_ARM_COUNTS[sweep];
            const int armNo = index % arms;
            const int lo = frame * arms / 20;
            const int hi = (frame + 1) * arms / 20 - 1;
            if (armNo >= lo && armNo <= hi) return 6; // 白
        }
        return PHYLLO_PALETTE_MAIN[(index % 21) % 7];
    }
    return PHYLLO_PALETTE_OVERLAY[(index % 21) % 3];
}

static int PhylloShotKind(int id, int index, int T)
{
    const int color = PhylloColor(id, index, T);
    // 主格子: 小玉(2.5x2.5) / 逆位相格子: 菱形弾(4.5x2.5)
    if (id == PHYLLO_LATTICE_MAIN) return img_enemyShotSmallBall[color];
    return img_enemyShotDiamond[color];
}

// 弾幕: 黄金角格子(1つのShotSetが1枚の格子を受け持つ)
//   kind     = 格子の種類(PHYLLO_LATTICE_xxx)
//   param_i[1] = この ShotSet を作った時点の周期内フレーム(T = count + param_i[1])
static void ShotPhyllotaxis(sEnemyShotSet* pEnemyShotSet)
{
    const int id = pEnemyShotSet->kind;
    const int T = pEnemyShotSet->count + pEnemyShotSet->param_i[1];
    const sPhylloSpec& spec = PHYLLO_SPEC[id];

    if (pEnemyShotSet->count == 0) {
        PhylloInitTable();
        memset(s_alive[id], 0, sizeof(s_alive[id]));
        s_liveNum[id] = 0;
        // 最初の呼び出しで最低1発は出るよう、射出スケジュールを前倒しする
        sPhylloFrame f0;
        PhylloMakeFrame(id, T, &f0);
        int firstEmit = 0x7fffffff;
        for (int n = 0; n < spec.pointNum; n++) {
            const double x = f0.cx + f0.scale * (f0.cosPhi * s_baseX[id][n] - f0.sinPhi * s_baseY[id][n]);
            const double y = f0.cy + f0.scale * (f0.sinPhi * s_baseX[id][n] + f0.cosPhi * s_baseY[id][n]);
            if (PhylloInScreen(x, y) && s_emitTime[id][n] < firstEmit) firstEmit = s_emitTime[id][n];
        }
        s_emitShift[id] = firstEmit - T;
        if (s_emitShift[id] < 0) s_emitShift[id] = 0;
    }

    sEnemyShot* pHead = pEnemyShotSet->pEnemyShotHead;

    // 終了: 残っている弾を全て画面外へ退避(消去は本体側)
    if (T >= PHYLLO_T_END) {
        for (sEnemyShot* pShot = pHead->next; pShot != pHead; pShot = pShot->next) {
            pShot->x = PHYLLO_VANISH_POS;
            pShot->y = PHYLLO_VANISH_POS;
        }
        s_liveNum[id] = 0;
        return;
    }

    sPhylloFrame f;
    PhylloMakeFrame(id, T, &f);
    memset(s_alive[id], 0, sizeof(s_alive[id]));

    // 既存の弾を更新
    int liveNum = 0;
    for (sEnemyShot* pShot = pHead->next; pShot != pHead; pShot = pShot->next) {
        const int n = pShot->param_i[0];
        s_alive[id][n] = 1;
        liveNum++;

        sPhylloPlace place;
        PhylloPlace(pEnemyShotSet, f, id, n, T, &place);

        // Phase5: 中心の安全円に入った弾は消える
        if (T >= PHYLLO_T_CONVERGE) {
            const double dx = place.x - f.cx;
            const double dy = place.y - f.cy;
            if (dx * dx + dy * dy < PHYLLO_SAFE_RADIUS * PHYLLO_SAFE_RADIUS) {
                pShot->x = PHYLLO_VANISH_POS;
                pShot->y = PHYLLO_VANISH_POS;
                continue;
            }
        }

        const double mx = place.x - pShot->x;
        const double my = place.y - pShot->y;
        pShot->speed = sqrt(mx * mx + my * my); // 実際の移動量
        pShot->x = place.x;
        pShot->y = place.y;
        pShot->muki = place.muki;
        pShot->kind = PhylloShotKind(id, n + 1, T);
    }
    s_liveNum[id] = liveNum;

    // Phase5 では新しい弾を追加しない(収束で密度が上がるので)
    if (T >= PHYLLO_T_CONVERGE) return;

    // 射出時刻を迎えていて、画面内に位置する点の弾を生成する
    // (画面外に出て消された点も、画面内に戻れば再生成される)
    for (int n = 0; n < spec.pointNum; n++) {
        if (s_alive[id][n]) continue;
        if (T < s_emitTime[id][n] - s_emitShift[id]) continue;

        sPhylloPlace place;
        PhylloPlace(pEnemyShotSet, f, id, n, T, &place);
        if (!PhylloInScreen(place.x, place.y)) continue;
        if (s_liveNum[PHYLLO_LATTICE_MAIN] + s_liveNum[PHYLLO_LATTICE_OVERLAY] >= PHYLLO_BULLET_BUDGET) break;

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = place.x;
        pEnemyShot->y = place.y;
        pEnemyShot->muki = place.muki;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = PhylloShotKind(id, n + 1, T);
        pEnemyShot->param_i[0] = n; // 格子点の番号
        pEnemyShot->prev = pHead->prev;
        pEnemyShot->next = pHead;
        pHead->prev->next = pEnemyShot;
        pHead->prev = pEnemyShot;
        s_liveNum[id]++;
    }
}

// ------------------------------------------------------------
//  敵本体
// ------------------------------------------------------------
static void PhylloPlaySound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

static void PhylloCreateShotSet(int latticeId, int cycleFrame)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotPhyllotaxis;
    pEnemyShotSet->x = enemy.x;            // 射出位置
    pEnemyShotSet->y = enemy.y + 10.0;
    pEnemyShotSet->muki = 0.0;             // 未使用(向きは弾ごとに設定する)
    pEnemyShotSet->kind = latticeId;
    pEnemyShotSet->param_i[1] = cycleFrame - 1; // T = count + param_i[1]
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_GeometryTAS_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    // 周期内のフレーム(1始まり)。1周期が終わったら同じパターンを繰り返す
    const int t = (count - 1) % PHYLLO_CYCLE_FRAMES + 1;

    // Phase1: 主格子の展開
    if (t == 1) {
        PhylloPlaySound(sound_enemyCharge);
        PhylloCreateShotSet(PHYLLO_LATTICE_MAIN, t);
    }
    if (t > 1 && t <= PHYLLO_SPEC[PHYLLO_LATTICE_MAIN].emitSpan && t % 12 == 1) {
        PhylloPlaySound(sound_enemyShot_light);
    }

    // Phase2: 結晶化 / Phase3: 回転と呼吸の開始
    if (t == PHYLLO_T_CRYSTAL + 1) PhylloPlaySound(sound_enemyShot_heavy);
    if (t == PHYLLO_T_MOTION + 1) PhylloPlaySound(sound_enemyShot_medium);

    // Phase4: 逆位相格子の展開
    if (t == PHYLLO_T_OVERLAY - 9) PhylloPlaySound(sound_enemyCharge);
    if (t == PHYLLO_T_OVERLAY + 1) {
        PhylloPlaySound(sound_enemyShot_heavy);
        PhylloCreateShotSet(PHYLLO_LATTICE_OVERLAY, t);
    }
    if (t > PHYLLO_T_OVERLAY + 1 && t <= PHYLLO_T_OVERLAY + PHYLLO_SPEC[PHYLLO_LATTICE_OVERLAY].emitSpan && (t - PHYLLO_T_OVERLAY) % 12 == 1) {
        PhylloPlaySound(sound_enemyShot_light);
    }

    // Phase5: 収束
    if (t == PHYLLO_T_CONVERGE + 1) PhylloPlaySound(sound_enemyShot_extreme);
}