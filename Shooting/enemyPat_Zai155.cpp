// 弾幕：「平安京の悪夢」風
// 基本パターン: 画面外含む正方形範囲(-60,-60)~(540,540)に赤レーザーの格子を出現させ、
//           1秒待つ → 中玉に変化 → T フレームかけて加速、T フレームかけて減速(元の位置へ
//           詰め直すように移動) → レーザーに戻す → 1秒待つ → 全消去 → 1秒待つ
// HP 2/3以上: 縦レーザー基本パターン + 自機の左右位置から上下方向の大玉列
// HP 2/3~1/3: x/y転置(横レーザー)基本パターン + 自機の上下位置から左右方向の大玉列
// HP 1/3以下: 両基本パターン同時(大玉列なし)
// ※ count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//    画面外の弾の消去はメインルーチンで行われる前提。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数・共有データ
// ============================================================
static const double FIELD_MIN = -60.0, FIELD_MAX = 540.0; // 出現範囲(画面+margin60)
static const int    W_FIXED = 60;   // 広めの間隔W(固定)
static const int    WAIT1 = 60;     // 出現後の待ち(1秒)
static const int    WAIT2 = 60;     // 移動終了後の待ち(1秒)
static const int    WAIT3 = 60;     // 消去後の待ち(1秒)
static const int    MAXN = 64;

// 基本パターン用の乱数表(スロット0:縦/通常、スロット1:横/転置)
static int gQ[2][MAXN]; // Q[k] = (P[k]-k) mod N
static int gR[2][MAXN]; // i方向シフト用ビット
static int gD[2][MAXN]; // j方向シフト用ビット

static void PlayShotSound(int sound)
{
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);
}

static sEnemyShot* AddShot(sEnemyShotSet* pSet)
{
    sEnemyShot* p = new sEnemyShot;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

static sEnemyShotSet* AddShotSet(sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = pSet->y = 0.0;
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

// ============================================================
//  基本パターン(レーザー格子 → 詰め移動)
// ============================================================
static void ShotHeiankyo(sEnemyShotSet* pSet)
{
    const int slot = pSet->param_i[0];
    const int N = pSet->param_i[1];
    const int nI = pSet->param_i[2];          // i方向(固定間隔W)の本数
    const int nJ = pSet->param_i[3];          // j方向(狭め間隔H)の本数
    const double W = (double)pSet->param_i[4];
    const double H = (double)pSet->param_i[5];
    const int T = pSet->param_i[6];
    const int horizontal = pSet->param_i[7];  // 0:縦レーザー 1:横レーザー(x/y転置)
    const int t1 = WAIT1;                     // 中玉へ変化
    const int t2 = t1 + 2 * T;                // 移動終了(レーザーへ戻す)
    const int t3 = t2 + WAIT2;                // 全消去
    // t3 + WAIT3 で基本パターン終了
    const int c = pSet->count;

    if (c == 0) {
        // レーザーの格子を出現(速度0)
        PlayShotSound(sound_enemyShot_heavy);
        for (int i = 0; i < nI; i++) {
            for (int j = 0; j < nJ; j++) {
                const int r = j % N;
                sEnemyShot* p = AddShot(pSet);
                double x0, y0, dx, dy, muki;
                if (horizontal == 0) {
                    x0 = FIELD_MIN + W * (i + 0.5);
                    y0 = FIELD_MIN + H * (j + 0.5);
                    muki = DX_PI / 2.0; // 下向き
                    dx = (gR[slot][r] ? 1.0 : -1.0) * W;
                    dy = (gD[slot][r] ? (double)gQ[slot][r]
                        : (double)(gQ[slot][r] - N)) * H;
                }
                else {
                    x0 = FIELD_MIN + H * (j + 0.5);
                    y0 = FIELD_MIN + W * (i + 0.5);
                    muki = 0.0; // 右向き(転置)
                    dx = (gD[slot][r] ? (double)gQ[slot][r]
                        : (double)(gQ[slot][r] - N)) * H;
                    dy = (gR[slot][r] ? 1.0 : -1.0) * W;
                }
                p->x = x0;  p->y = y0;
                p->muki = muki;
                p->speed = 0.0;
                p->kind = img_enemyShotLaser[0]; // 赤レーザー
                p->margin = 80;
                p->param_d[0] = x0; p->param_d[1] = y0; // 移動始点
                p->param_d[2] = dx; p->param_d[3] = dy; // 移動量
            }
        }
    }
    else if (c == t1) {
        // レーザー → 中玉に変化(移動開始)
        PlayShotSound(sound_enemyCharge);
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            p->kind = img_enemyShotMediumBall[0]; // 赤中玉
            p = p->next;
        }
    }
    else if (c > t1 && c <= t2) {
        // ゆっくり加速 → ゆっくり減速(サイン半波で補間)
        const double s = (c - t1) / (double)(2 * T);
        const double f = (1.0 - cos(DX_PI * s)) * 0.5;
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            p->x = p->param_d[0] + p->param_d[2] * f;
            p->y = p->param_d[1] + p->param_d[3] * f;
            if (c == t2) p->kind = img_enemyShotLaser[0]; // 中玉 → レーザー
            p = p->next;
        }
    }
    else if (c == t3) {
        // 全レーザー消去(自セットのリストから削除)
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            sEnemyShot* next = p->next;
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
            p = next;
        }
    }
    // t3 以降〜 t3+WAIT3 は空で待機、以降は何もしない
}

// ============================================================
//  自機を狙った大玉の列(フェーズ0:上下方向 / フェーズ1:左右方向)
// ============================================================
static void ShotAimedColumn(sEnemyShotSet* pSet)
{
    const int c = pSet->count;
    // param_i[0]: 0=上下方向 1=左右方向、param_i[1]: 撃ち続けるフレーム数
    if (c < pSet->param_i[1] && c % 60 == 0) {
        PlayShotSound(sound_enemyShot_medium);
        const int mode = pSet->param_i[0];
        const int edge = GetRand(1);        // 上下(左右)どちらの端からか
        const int n = 3 + GetRand(2);       // 列をなす弾の数
        const double speed = 2.0 + GetRand(15) / 10.0-1;
        for (int k = 0; k < n; k++) {
            sEnemyShot* p = AddShot(pSet);
            p->kind = img_enemyShotLargeBall[0]; // 赤大玉
            p->speed = speed;
            if (mode == 0) {
                p->x = player.x; // 自機と同じ左右位置
                if (edge == 0) { p->y = -20.0 - 40.0 * k; p->muki = DX_PI / 2.0; }
                else { p->y = 500.0 + 40.0 * k; p->muki = -DX_PI / 2.0; }
            }
            else {
                p->y = player.y; // 自機と同じ上下位置
                if (edge == 0) { p->x = -20.0 - 40.0 * k; p->muki = 0.0; }
                else { p->x = 500.0 + 40.0 * k; p->muki = DX_PI; }
            }
        }
    }
    // 大玉の列を等速直線移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  基本パターン1回分の生成(N,P,R,D,Q,H を乱数で決定)
// ============================================================
static void SpawnBasicPattern(int slot, int T, int horizontal)
{
    // 狭めの間隔H(600が割り切れる値から選択)
    static const int hList[6] = { 12, 15, 20, 24, 25, 30 };
    const int H = hList[GetRand(5)];
    const int lines = 600 / H; // j方向(転置時はx方向)の本数

    // N の候補: lines の約数、かつ N*H <= 120(はみ出し抑制)
    int cand[32], ncand = 0;
    for (int n = 2; n <= lines; n++) {
        if (lines % n == 0 && n * H <= 120) cand[ncand++] = n;
    }
    if (ncand == 0) cand[ncand++] = 2; // 念のため(通常発生しない)
    const int N = cand[GetRand(ncand - 1)];

    // ランダム順列 P を Fisher-Yates で生成 → Q[k] = (P[k]-k) mod N
    int perm[MAXN];
    for (int k = 0; k < N; k++) perm[k] = k;
    for (int k = N - 1; k > 0; k--) {
        const int j = GetRand(k);
        const int t = perm[k]; perm[k] = perm[j]; perm[j] = t;
    }
    for (int k = 0; k < N; k++) {
        gQ[slot][k] = ((perm[k] - k) % N + N) % N;
        gR[slot][k] = GetRand(1);
        gD[slot][k] = GetRand(1);
    }

    sEnemyShotSet* pSet = AddShotSet(ShotHeiankyo);
    pSet->param_i[0] = slot;
    pSet->param_i[1] = N;
    pSet->param_i[2] = 600 / W_FIXED; // i方向本数(=10)
    pSet->param_i[3] = lines;
    pSet->param_i[4] = W_FIXED;
    pSet->param_i[5] = H;
    pSet->param_i[6] = T;
    pSet->param_i[7] = horizontal;
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Heiankyo_Zai()
{
    static int nextCycle = 0;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        nextCycle = 1;
    }
    // ボスをゆっくり揺らす
    enemy.x = 240.0 + 60.0 * sin(count * 0.015);
    enemy.y = 40.0 + 10.0 * sin(count * 0.03);

    if (count < nextCycle) return;

    // HP割合によるフェーズ判定
    const double ratio = enemy.hp / (double)enemy.maxHp;
    const int phase = (ratio * 3.0 >= 2.0) ? 0 : (ratio * 3.0 > 1.0 ? 1 : 2);

    // サイクル長(1回の基本パターンの長さ)を決定し、終わり次第次を生成
    const int T = 50 + GetRand(50);
    const int cycleLen = WAIT1 + 2 * T + WAIT2 + WAIT3;
    nextCycle = count + cycleLen;

    if (phase != 1) SpawnBasicPattern(0, T, 0); // 通常(下向きレーザー)
    if (phase != 0) SpawnBasicPattern(1, T, 1); // x/y転置(横向きレーザー)
    if (phase != 2) {
        // 大玉の列(フェーズ2では撃たない)
        sEnemyShotSet* pSet = AddShotSet(ShotAimedColumn);
        pSet->param_i[0] = phase;    // 0:上下方向 1:左右方向
        pSet->param_i[1] = cycleLen; // このサイクルの間だけ撃つ
    }
}