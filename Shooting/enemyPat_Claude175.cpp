#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：振り子の共鳴(ペンデュラムウェーブ)
//
//  画面上部に PW_NUM 本の振り子を横一列に吊るす。
//    糸   ： 白い小玉を等間隔に並べたもの
//    おもり： 中玉 + 周囲を回る小玉のリング(色は虹色グラデーション)
//  i 番目の振り子は PW_CYCLE フレームの間にちょうど (PW_FREQ_BASE + i) 回往復する。
//  そのため開始時と PW_CYCLE フレーム後は全振り子が同じ向きに揃い、
//  その間に位相がずれて「斜めの波 → うねり → 市松状の定在波 → 再整列」と変化していく。
//
//  追加攻撃:
//    ・おもりが振れの端に達した瞬間、菱形弾を3発真下へ落とす
//    ・整列の直前に、全おもりから自機狙いの3way(2速度)を撃つ
//
//  実装メモ:
//    ・振り子の弾は速度で動かさず、毎フレーム全て消去して座標を直接計算し直す
//      (画面外に出た弾がメインルーチンに消されても、戻ってきたときに糸が欠けないようにするため)
//    ・そのため振り子の弾の pEnemyShot->count は毎フレーム0に戻る(このパターンでは未使用)
// ============================================================

// ---------- 調整用パラメータ ----------
static constexpr int    PW_NUM = 14;      // 振り子の本数
static constexpr int    PW_FREQ_BASE = 4;       // 0番目の振り子の往復回数(i番目は PW_FREQ_BASE + i 回)
static constexpr int    PW_CYCLE = 2400/2;    // 全振り子が整列に戻るまでのフレーム数(60fpsで40秒)
static constexpr double PW_SUPPORT_Y = 24.0;    // 支点のy座標
static constexpr double PW_SPACING = 30.0;    // 支点の間隔(x方向)
static constexpr double PW_LEN_LONG = 420.0*0.95;   // 0番目の糸の長さ(最長)
static constexpr double PW_LEN_SHORT = 360.0*0.95;   // 最後の糸の長さ(最短)
static constexpr double PW_THETA_MAX = 0.34;    // 振れ幅の最大角度[rad](約19.5度)
static constexpr double PW_TIP_SPEED_MAX = 5.0;     // おもりの最高速度の上限[px/frame](速い振り子は振れ幅を自動で縮める)
static constexpr double PW_STRING_GAP = 14.0;    // 糸の弾の間隔
static constexpr double PW_BOB_GAP = 9.0;     // 糸の最後の弾とおもりの最小間隔
static constexpr int    PW_RING_NUM = 6;       // おもりのリングの小玉の数
static constexpr double PW_RING_R = 11.0;    // おもりのリングの半径
static constexpr double PW_RING_SPIN = 0.08;    // おもりのリングの回転速度[rad/frame]
static constexpr int    PW_INTRO = 90;      // 糸が伸びきるまでのフレーム数
static constexpr int    PW_CHARGE_LEAD = 120/2;     // 整列の何フレーム前に予告音を鳴らすか
static constexpr int    PW_VOLLEY_LEAD = 30;      // 整列の何フレーム前に自機狙い弾を撃つか
static constexpr int    PW_DROP_STOP = 150/3;     // 整列の何フレーム前から落下弾を止めるか(終盤を見やすくする)

// ---------- 共通ヘルパー ----------

// 効果音の再生(連続で鳴らしても重ならないように止めてから鳴らす)
static void PlaySE(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 振り子 i の色(虹色: 赤→橙→黄→緑→シアン→青→マゼンタ)
static int PendulumColor(int i)
{
    static const int rainbow[7] = { 0, 8, 1, 2, 3, 4, 5 };
    return rainbow[i * 7 / PW_NUM];
}

// 弾を1つ追加する。画面から十分離れた位置の弾は生成しない。
static void AddEnemyShot(sEnemyShot* pHead, double x, double y, double muki, double speed, int kind)
{
    if (x < -16.0 || x > 496.0 || y < -16.0 || y > 496.0) return;
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pHead->prev;
    pEnemyShot->next = pHead;
    pHead->prev->next = pEnemyShot;
    pHead->prev = pEnemyShot;
}

// ShotSet を新しく作って末尾に追加する
static void CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ShotSet 内の全ての弾を向き・速さの通りに直進させる
static void MoveShotsStraight(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------- 振り子の状態計算 ----------
struct sPendulumState {
    double sx, sy;   // 支点の座標
    double len;      // 糸の長さ
    double theta;    // 鉛直下向きからの角度[rad](+で右に振れる)
    double dtheta;   // 角速度[rad/frame]
};

// 時刻 t(フレーム)における i 番目の振り子の状態を返す
static sPendulumState GetPendulum(int i, int t)
{
    sPendulumState ps;
    const double pi2 = DX_PI * 2.0;
    const int f = PW_FREQ_BASE + i;                                  // 1サイクル中の往復回数
    ps.sx = 240.0 + (i - (PW_NUM - 1) / 2.0) * PW_SPACING;
    ps.sy = PW_SUPPORT_Y;
    ps.len = PW_LEN_LONG + (PW_LEN_SHORT - PW_LEN_LONG) * (double)i / (PW_NUM - 1);
    const double w = pi2 * f / PW_CYCLE;                             // 角周波数[rad/frame]
    double amp = PW_TIP_SPEED_MAX / (ps.len * w);                    // おもりの最高速度が上限を超えないよう振れ幅を制限
    if (amp > PW_THETA_MAX) amp = PW_THETA_MAX;
    const int ph = (f * (t % PW_CYCLE)) % PW_CYCLE;                  // 位相を整数で持つ(長時間でも誤差が溜まらない)
    const double phase = pi2 * ph / PW_CYCLE;
    ps.theta = amp * cos(phase);                                     // t=0 では全員が右端に揃っている
    ps.dtheta = -amp * w * sin(phase);
    return ps;
}

// ---------- 弾幕：振り子本体(糸とおもり) ----------
static void ShotPendulum(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pHead = pEnemyShotSet->pEnemyShotHead;

    // 前フレームの弾を全て消去
    sEnemyShot* pShot = pHead->next;
    while (pShot != pHead) {
        sEnemyShot* pNext = pShot->next;
        delete pShot;
        pShot = pNext;
    }
    pHead->prev = pHead;
    pHead->next = pHead;

    const int t = pEnemyShotSet->count;
    // 最初は支点から糸がするっと伸びる(予告)
    double grow = 1.0;
    if (t < PW_INTRO) {
        const double g = (double)t / PW_INTRO;
        grow = 1.0 - (1.0 - g) * (1.0 - g);
    }

    for (int i = 0; i < PW_NUM; i++) {
        const sPendulumState ps = GetPendulum(i, t);
        const double sinT = sin(ps.theta);
        const double cosT = cos(ps.theta);
        const double reach = ps.len * grow;                          // 現在の糸の長さ
        const double angSpeed = fabs(ps.dtheta);
        // 弾の向きは「実際に動いている方向」(接線方向)に合わせる
        const double muki = (ps.dtheta >= 0.0) ? -ps.theta : DX_PI - ps.theta;
        const int color = PendulumColor(i);

        // 糸：白い小玉
        for (int k = 1; k * PW_STRING_GAP < reach - PW_BOB_GAP; k++) {
            const double s = k * PW_STRING_GAP;
            AddEnemyShot(pHead, ps.sx + s * sinT, ps.sy + s * cosT, muki, s * angSpeed, img_enemyShotSmallBall[6]);
        }

        // おもり：中玉 + 周囲を回る小玉のリング
        const double bx = ps.sx + reach * sinT;
        const double by = ps.sy + reach * cosT;
        const double bobSpeed = reach * angSpeed;
        AddEnemyShot(pHead, bx, by, muki, bobSpeed, img_enemyShotMediumBall[color]);
        for (int k = 0; k < PW_RING_NUM; k++) {
            const double a = DX_PI * 2.0 * k / PW_RING_NUM + t * PW_RING_SPIN;
            AddEnemyShot(pHead, bx + PW_RING_R * cos(a), by + PW_RING_R * sin(a), muki, bobSpeed, img_enemyShotSmallBall[color]);
        }
    }
}

// ---------- 弾幕：おもりからの落下弾(菱形弾を3発、真下へ) ----------
static void ShotDrop(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySE(sound_enemyShot_light);
        for (int i = 0; i < 3; i++) {
            AddEnemyShot(pEnemyShotSet->pEnemyShotHead, pEnemyShotSet->x, pEnemyShotSet->y,
                DX_PI / 2.0, 1.5 + 0.8 * i, img_enemyShotDiamond[pEnemyShotSet->kind]);
        }
    }
    MoveShotsStraight(pEnemyShotSet);
}

// ---------- 弾幕：整列直前の自機狙い(3way x 2速度) ----------
static void ShotVolley(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySE(sound_enemyShot_heavy);
        static const double speeds[2] = { 2.4, 3.2 };
        for (int w = 0; w < 2; w++) {
            for (int k = -1; k <= 1; k++) {
                const double muki = pEnemyShotSet->muki + k * 9.0 / 180.0 * DX_PI;
                AddEnemyShot(pEnemyShotSet->pEnemyShotHead, pEnemyShotSet->x, pEnemyShotSet->y,
                    muki, speeds[w], img_enemyShotMediumOval[pEnemyShotSet->kind]);
            }
        }
    }
    MoveShotsStraight(pEnemyShotSet);
}

// ---------- 敵本体のパターン ----------
void EnemyPat_PendulumWave_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        PlaySE(sound_enemyCharge);    // 糸が伸び始める予告音
        CreateShotSet(ShotPendulum, 0.0, 0.0, 0.0, 0); // 振り子本体は1つのShotSetで最後まで管理する
        return;
    }

    const int t = count - 1;            // ShotPendulum 側の時刻と揃える
    const int tc = t % PW_CYCLE;        // 現在のサイクル内の時刻

    // 整列直前の予告音
    if (tc == PW_CYCLE - PW_CHARGE_LEAD) {
        PlaySE(sound_enemyCharge);
    }

    if (tc == PW_CYCLE - PW_VOLLEY_LEAD) {
        // 全おもりから自機狙い弾
        for (int i = 0; i < PW_NUM; i++) {
            const sPendulumState ps = GetPendulum(i, t);
            const double bx = ps.sx + ps.len * sin(ps.theta);
            const double by = ps.sy + ps.len * cos(ps.theta);
            if (bx < 0.0 || bx > 480.0) continue;
            const double aim = atan2(player.y - by, player.x - bx);
            CreateShotSet(ShotVolley, bx, by, aim, PendulumColor(i));
        }
    }
    else if (t >= PW_INTRO && tc < PW_CYCLE - PW_DROP_STOP) {
        // おもりが振れの端(速度0)に達した瞬間に真下へ落とす
        for (int i = 0; i < PW_NUM; i++) {
            const int f = PW_FREQ_BASE + i;
            // 端に達するフレームは 2*f*t/PW_CYCLE の整数部が増えるフレーム(整数演算で厳密に検出)
            if ((2 * f * t) / PW_CYCLE == (2 * f * (t - 1)) / PW_CYCLE) continue;
            const sPendulumState ps = GetPendulum(i, t);
            const double bx = ps.sx + ps.len * sin(ps.theta);
            const double by = ps.sy + ps.len * cos(ps.theta);
            if (bx < 8.0 || bx > 472.0) continue;
            CreateShotSet(ShotDrop, bx, by + PW_RING_R + 3.0, DX_PI / 2.0, PendulumColor(i));
        }
    }
}