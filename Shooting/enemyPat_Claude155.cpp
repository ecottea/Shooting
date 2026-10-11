// ============================================================
//  弾幕：平安京の悪夢（東方星蓮船）風
//
//  ■ 基本パターン（レーザー格子の組み替え）
//    1. (-60,-60)〜(540,540) に、x方向間隔W(固定・広め)、y方向間隔H(毎回変化・狭め)で
//       下向きの赤レーザーを格子状に出現させ、速度0で1秒待つ。
//    2. 左からi番目・上からj=qN+r番目のレーザーを (i2, j2) へ移動させる。
//         R[r]=1 → i2=i+1 / R[r]=0 → i2=i-1
//         D[r]=1 → j2=j+Q[r] / D[r]=0 → j2=j+Q[r]-N
//         (Q[k]=(P[k]-k) mod N, Pは0〜N-1のランダム順列, R,Dはランダムビット列)
//       移動中はレーザーを中玉に変え、時間Tで加速→時間Tで減速(終端速度0)させ、
//       到着したら中玉をレーザーに戻す。
//    3. 1秒待ち → 全レーザー消去 → 1秒待ち → 終了。
//
//  ■ フェーズ
//    HP 2/3以上   : 基本パターン + 上下方向の大玉の列(自機と同じx、上端or下端から)
//    HP 2/3〜1/3  : x/y転置した基本パターン + 左右方向の大玉の列(自機と同じy、左端or右端から)
//    HP 1/3以下   : 基本パターンと転置パターンを同時に発生(大玉の列は無し)
//    N,P,R,D,H,T は基本パターンを撃つたびに再抽選する。Wは固定。
//    (HP1/3以下の同時発生では、2つの開始〜終了を揃えるためTだけ共通にしている)
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  調整用の定数
// ------------------------------------------------------------
static const int    kHeianScreenSize = 480;   // ゲーム画面は 480x480
static const int    kHeianGridMargin = 60;    // 格子を画面外に広げる幅
static const int    kHeianGridSize = kHeianScreenSize + 2 * kHeianGridMargin; // 600
static const int    kHeianWaitFrames = 60;    // 「1秒待つ」

static const int    kHeianSpacingW = 120;   // x方向の間隔W(固定)
static const int    kHeianSpacingHMin = 12;    // y方向の間隔Hの範囲(狭め)
static const int    kHeianSpacingHMax = 28;
static const int    kHeianNMin = 4;     // 整数Nの範囲
static const int    kHeianNMax = 12;
static const int    kHeianMoveTMin = 45;    // 加速時間=減速時間Tの範囲(フレーム)
static const int    kHeianMoveTMax = 90;

static const int    kHeianLaserColor = 0;     // 0:赤
static const int    kHeianBigBallColor = 4;     // 4:青

static const int    kHeianFirstDelay = 30;    // 戦闘開始から最初の基本パターンまで
static const int    kHeianColumnFirst = 60;    // 最初の大玉の列までのフレーム数
static const int    kHeianColumnSpawn = 100;   // 大玉の列を撃つ間隔
static const int    kHeianColumnBallCount = 20;    // 列を構成する大玉の数
static const int    kHeianColumnInterval = 8;     // 大玉を出す間隔(フレーム)
static const double kHeianColumnSpeed = 4.5;   // 大玉の速さ
static const double kHeianColumnEdgeOffset = 20.0; // 大玉の出現位置(画面端からの外側へのオフセット=大玉の半径)
static const double kHeianColumnSafeDist = 160.0; // 自機がこれより端に近いときは反対側の端から撃つ

// 変数 N に関する配列の最大サイズ
static const int    kHeianArrayMax = kHeianNMax;

// ------------------------------------------------------------
//  共通ヘルパー
// ------------------------------------------------------------
// 効果音を頭から鳴らし直す
static void PlaySoundRestart(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ShotSet を生成してリストの末尾に繋ぐ
static sEnemyShotSet* CreateHeianShotSet(sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// 弾を ShotSet のリスト末尾に繋ぐ
static void AddHeianShot(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->margin = 60;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 移動の進行度(0〜1)。時間 u=0〜2T のうち、前半Tは一定加速、後半Tは一定減速(終端速度0)
static double HeianMoveProgress(int u, int T)
{
    if (u <= T) {
        double v = (double)u / T;
        return 0.5 * v * v;
    }
    double v = (double)(2 * T - u) / T;
    return 1.0 - 0.5 * v * v;
}

// 速度の倍率(0〜1)。u=T で最大1
static double HeianMoveSpeedRate(int u, int T)
{
    return (u <= T) ? (double)u / T : (double)(2 * T - u) / T;
}

// ------------------------------------------------------------
//  弾幕：基本パターン(レーザー格子の組み替え)
//    param_i[0] = T(加速時間=減速時間)
//    param_i[1] = 転置フラグ(0:通常=下向きレーザー, 1:x/y転置=右向きレーザー)
//  各レーザーの param_d: [0]=始点x [1]=始点y [2]=終点x [3]=終点y [4]=移動方向 [5]=移動距離
// ------------------------------------------------------------
static void ShotHeianLaserGrid(sEnemyShotSet* pEnemyShotSet)
{
    const int    T = pEnemyShotSet->param_i[0];
    const bool   transposed = (pEnemyShotSet->param_i[1] != 0);
    const int    c = pEnemyShotSet->count;
    const int    moveStart = kHeianWaitFrames;           // 出現して1秒待った後に移動開始
    const int    moveEnd = moveStart + 2 * T;          // 加速T + 減速T
    const int    eraseAt = moveEnd + kHeianWaitFrames; // 移動後1秒待って全消去
    const double laserMuki = transposed ? 0.0 : DX_PI / 2.0; // 下向き(転置時は右向き)
    sEnemyShot* const pHead = pEnemyShotSet->pEnemyShotHead;

    // --- 出現：レーザーを格子状に並べる(速度0) ---
    if (c == 0) {
        PlaySoundRestart(sound_enemyCharge);

        // N, P, R, D, Q, H を毎回ランダムに決める
        const int N = kHeianNMin + GetRand(kHeianNMax - kHeianNMin);
        const int H = kHeianSpacingHMin + GetRand(kHeianSpacingHMax - kHeianSpacingHMin);
        int P[kHeianArrayMax], R[kHeianArrayMax], D[kHeianArrayMax], Q[kHeianArrayMax];
        for (int k = 0; k < N; k++) P[k] = k;
        for (int k = N - 1; k > 0; k--) { // Fisher-Yates シャッフル (GetRand(k) は 0〜k)
            int m = GetRand(k);
            int tmp = P[k]; P[k] = P[m]; P[m] = tmp;
        }
        for (int k = 0; k < N; k++) {
            R[k] = GetRand(1);
            D[k] = GetRand(1);
            Q[k] = ((P[k] - k) % N + N) % N;
        }

        const int cols = kHeianGridSize / kHeianSpacingW + 1; // 左からの個数
        const int rows = kHeianGridSize / H + 1;              // 上からの個数
        for (int i = 0; i < cols; i++) {
            for (int j = 0; j < rows; j++) {
                const int r = j % N;                          // j = qN + r
                const int i2 = i + (R[r] ? 1 : -1);
                const int j2 = j + Q[r] - (D[r] ? 0 : N);

                // a: W方向(通常ならx), b: H方向(通常ならy)。転置時は x/y を入れ替える
                const double a0 = -kHeianGridMargin + kHeianSpacingW * i;
                const double b0 = -kHeianGridMargin + H * j;
                const double a1 = -kHeianGridMargin + kHeianSpacingW * i2;
                const double b1 = -kHeianGridMargin + H * j2;
                const double x0 = transposed ? b0 : a0;
                const double y0 = transposed ? a0 : b0;
                const double x1 = transposed ? b1 : a1;
                const double y1 = transposed ? a1 : b1;

                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = x0;
                pEnemyShot->y = y0;
                pEnemyShot->muki = laserMuki;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotLaser[kHeianLaserColor];
                pEnemyShot->param_d[0] = x0;
                pEnemyShot->param_d[1] = y0;
                pEnemyShot->param_d[2] = x1;
                pEnemyShot->param_d[3] = y1;
                pEnemyShot->param_d[4] = atan2(y1 - y0, x1 - x0);
                pEnemyShot->param_d[5] = sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
                AddHeianShot(pEnemyShotSet, pEnemyShot);
            }
        }
    }

    // --- 移動：レーザー→中玉に変え、加速T→減速Tで移動、到着したらレーザーに戻す ---
    if (c >= moveStart && c <= moveEnd) {
        if (c == moveStart) PlaySoundRestart(sound_enemyShot_medium);
        const int    u = c - moveStart;
        const double prog = HeianMoveProgress(u, T);
        const double rate = HeianMoveSpeedRate(u, T);
        for (sEnemyShot* pShot = pHead->next; pShot != pHead; pShot = pShot->next) {
            const double x0 = pShot->param_d[0], y0 = pShot->param_d[1];
            const double x1 = pShot->param_d[2], y1 = pShot->param_d[3];
            pShot->x = x0 + (x1 - x0) * prog;
            pShot->y = y0 + (y1 - y0) * prog;
            if (c == moveEnd) {
                // 到着：中玉をレーザーに戻す
                pShot->kind = img_enemyShotLaser[kHeianLaserColor];
                pShot->muki = laserMuki;
                pShot->speed = 0.0;
            }
            else {
                if (c == moveStart) pShot->kind = img_enemyShotMediumBall[kHeianLaserColor];
                pShot->muki = pShot->param_d[4];                       // 進行方向
                pShot->speed = pShot->param_d[5] / T * rate;           // 現在の速さ
            }
        }
    }

    // --- 消去：全レーザーを消す(以後1秒は何も無い。次のパターンは本体側が時間で管理) ---
    if (c == eraseAt) {
        sEnemyShot* pShot = pHead->next;
        while (pShot != pHead) {
            sEnemyShot* pNext = pShot->next;
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = pNext;
        }
    }
}

// ------------------------------------------------------------
//  弾幕：大玉の列
//    ShotSet の x,y = 出現位置(画面端), muki = 自機方向(上下 or 左右)
//    kHeianColumnInterval フレームごとに大玉を1個出し、同じ向きへ等速で進める
// ------------------------------------------------------------
static void ShotHeianBigBallColumn(sEnemyShotSet* pEnemyShotSet)
{
    const int c = pEnemyShotSet->count;
    if (c < kHeianColumnBallCount * kHeianColumnInterval && c % kHeianColumnInterval == 0) {
        if (c == 0) PlaySoundRestart(sound_enemyShot_heavy);
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = kHeianColumnSpeed;
        pEnemyShot->kind = img_enemyShotLargeBall[kHeianBigBallColor];
        AddHeianShot(pEnemyShotSet, pEnemyShot);
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  生成用ヘルパー
// ------------------------------------------------------------
static void SpawnHeianLaserGrid(bool transposed, int T)
{
    sEnemyShotSet* pEnemyShotSet = CreateHeianShotSet(ShotHeianLaserGrid);
    pEnemyShotSet->param_i[0] = T;
    pEnemyShotSet->param_i[1] = transposed ? 1 : 0;
}

// vertical=true : 自機と同じxの上端/下端から、上下方向に撃つ
// vertical=false: 自機と同じyの左端/右端から、左右方向に撃つ
static void SpawnHeianBigBallColumn(bool vertical)
{
    // 自機位置のうち、列の進行方向に沿った座標 (縦ならy, 横ならx)
    const double along = vertical ? player.y : player.x;
    bool fromFirstEdge = (GetRand(1) == 0); // 上端 or 左端
    // 自機が端に近すぎて避けようがない場合は反対側の端から撃つ
    if (fromFirstEdge && along < kHeianColumnSafeDist) fromFirstEdge = false;
    else if (!fromFirstEdge && along > kHeianScreenSize - kHeianColumnSafeDist) fromFirstEdge = true;
    const double edge = fromFirstEdge ? -kHeianColumnEdgeOffset
        : kHeianScreenSize + kHeianColumnEdgeOffset;

    sEnemyShotSet* pEnemyShotSet = CreateHeianShotSet(ShotHeianBigBallColumn);
    if (vertical) {
        pEnemyShotSet->x = player.x;
        pEnemyShotSet->y = edge;
    }
    else {
        pEnemyShotSet->x = edge;
        pEnemyShotSet->y = player.y;
    }
    // 自機を狙う(自機と同じ左右位置/上下位置から撃つので真上下/真左右になる)
    pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Heiankyo_Claude()
{
    static int nextPatternCount; // 次の基本パターンを撃つ count
    static int nextColumnCount;  // 次の大玉の列を撃つ count

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        nextPatternCount = kHeianFirstDelay;
        nextColumnCount = kHeianColumnFirst;
    }

    // フェーズ判定 0:HP2/3以上, 1:HP2/3〜1/3, 2:HP1/3以下
    int phase;
    if (enemy.hp * 3 >= enemy.maxHp * 2) phase = 0;
    else if (enemy.hp * 3 > enemy.maxHp) phase = 1;
    else phase = 2;

    // 基本パターン：前のパターンが終わったら次を撃つ(N,P,R,D,H は各パターン内で抽選、Tはここで抽選)
    if (count >= nextPatternCount) {
        const int T = kHeianMoveTMin + GetRand(kHeianMoveTMax - kHeianMoveTMin);
        switch (phase) {
        case 0:
            SpawnHeianLaserGrid(false, T);
            break;
        case 1:
            SpawnHeianLaserGrid(true, T);
            break;
        default:
            SpawnHeianLaserGrid(false, T);
            SpawnHeianLaserGrid(true, T);
            break;
        }
        // 出現1秒待ち + 移動2T + 到着後1秒待ち + 消去後1秒待ち
        nextPatternCount = count + 3 * kHeianWaitFrames + 2 * T;
    }

    // 大玉の列：HP1/3以下では撃たない
    if (phase != 2 && count >= nextColumnCount) {
        SpawnHeianBigBallColumn(phase == 0);
        nextColumnCount = count + kHeianColumnSpawn;
    }
}