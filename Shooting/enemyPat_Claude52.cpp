// enemyPat_HaneotoHachinojiMai.cpp
//
// 蜂モチーフ弾幕：「羽音の八の字舞(はねおとのはちのじまい)」
// ミツバチの8の字ダンス(waggle dance)をモチーフにした3フェーズ弾幕。
//
//   フェーズ1: 巣房展開   … 六角格子(2重リング)状に弾がじわっと滲み出て静止・微振動
//   フェーズ2: 8の字舞    … 働き蜂弾がレムニスケート軌道で8の字を描きつつ、自機を直接狙う
//                          針弾(刺突弾)を周期的に放つ
//   フェーズ3: 花粉散布   … 蜂本体の位置から自機を狙って花粉弾を放射しつつ、全弾が外へ弾け散る
//
// 素材選定方針:
//   ・巣房(ハニカム)  : img_enemyShotMediumOval  (黄/橙。蜜蝋の巣穴を丸っこい塊で表現)
//   ・蜂本体          : img_enemyShotMediumBall  (橙/白の明滅。交差点通過時に羽音の振動を演出)
//   ・花粉            : img_enemyShotSmallBall   (黄。小粒でばら撒き向き。自機狙いの扇状)
//   ・針(刺突弾)      : img_enemyShotBullet      (黒。細長く高速、自機へ直撃を狙う)
//   ・img_enemyShotLaser は規約により不使用(判定が実画像より大きいため)。
//
// 設計方針(既存パターン踏襲):
//   ・弾の位置は pShot->count から直接計算する純関数とし、速度積分(+=)は行わない。
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//     画面外弾の削除はメインルーチン側の仕様に委ねる。
//   ・GetRand(x) は 0〜x の x+1 通りを返す(リプレイ再現性のため必ずこれを使用)。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {
    // ---- ハニカム(巣房)関連 ----
    constexpr int    HONEY_RING1_NUM = 6;      // 内側リングの弾数
    constexpr int    HONEY_RING2_NUM = 6;      // 外側リングの弾数
    constexpr double  HONEY_R1 = 70.0;   // 内側リング半径
    constexpr double  HONEY_R2 = 140.0;  // 外側リング半径
    constexpr int    HONEY_EASE_FRAME = 70;     // 滲み出て定位置に着くまでのフレーム数
    constexpr int    HONEY_ACCEL_START = 300;    // 弾け飛び始めるフレーム(自弾count基準)
    constexpr double  HONEY_ACCEL = 0.006;  // 弾け飛ぶ加速係数

    // ---- 働き蜂(8の字ダンス本体)関連 ----
    constexpr int     BEE_NUM = 7;       // 働き蜂弾の数
    constexpr double  BEE_A = 150.0;   // レムニスケートのスケール
    constexpr double  BEE_OMEGA = 0.045;   // θの進行速度
    constexpr double  BEE_ROT_OMEGA = 0.0015;  // 長軸回転の速度
    constexpr int     BEE_GROW_FRAME = 80;      // 軌道が育ちきるまでのフレーム
    constexpr int     BEE_DISPERSE_START = 380;     // 軌道を膨らませ退場させ始めるフレーム
    constexpr double  BEE_DISPERSE_ACCEL = 0.02;     // 退場時の膨張係数

    // ---- 花粉散布関連 ----
    constexpr int     POLLEN_START_FRAME = 260;              // 散布開始フレーム(自弾count基準)
    constexpr int     POLLEN_END_FRAME = 370;               // 散布終了フレーム
    constexpr int     POLLEN_INTERVAL = 18;                // 散布間隔
    constexpr int     POLLEN_PER_BURST = 2;                 // 1蜂・1回あたりの花粉数
    constexpr double  POLLEN_SPREAD_HALF = 18.0 * DX_PI / 180.0; // 花粉の広がり角(半角)
    constexpr double  POLLEN_SPEED_MIN = 1.6;
    constexpr double  POLLEN_SPEED_RANGE = 1.2;

    // ---- 針(刺突弾)関連: 蜂本体から自機を直接狙う高速弾 ----
    constexpr int     STINGER_START_FRAME = 100;   // 撃ち始めるフレーム(自弾count基準)
    constexpr int     STINGER_END_FRAME = 380;   // 撃ち終えるフレーム
    constexpr int     STINGER_INTERVAL = 40;    // 発射間隔
    constexpr double  STINGER_SPEED = 3.2;   // 速度(直撃を狙う高速弾)

    // ---- 弾の役割 (param_i[0] に格納) ----
    constexpr int SHOT_ROLE_HONEY = 0;
    constexpr int SHOT_ROLE_BEE = 1;
    constexpr int SHOT_ROLE_POLLEN = 2;
    constexpr int SHOT_ROLE_STINGER = 3;
}

// 巣房弾: 中心から滲み出て定位置で微振動し、やがて外へ弾け飛ぶ(すべてcountの純関数)
static void UpdateHoneycombShot(sEnemyShot* pShot, double cx, double cy)
{
    const double dx = pShot->param_d[0];
    const double dy = pShot->param_d[1];
    const double vibPhase = pShot->param_d[2];
    const int c = pShot->count;

    double ratio;
    if (c < HONEY_EASE_FRAME) {
        double t = (double)c / HONEY_EASE_FRAME;
        ratio = 1.0 - (1.0 - t) * (1.0 - t); // ease-out
    }
    else {
        ratio = 1.0;
    }

    double extra = 0.0;
    if (c > HONEY_ACCEL_START) {
        double t = (double)(c - HONEY_ACCEL_START);
        extra = HONEY_ACCEL * t * t;
    }

    double vib = 0.0;
    if (c >= HONEY_EASE_FRAME && c <= HONEY_ACCEL_START) {
        vib = 2.0 * sin(0.06 * c + vibPhase);
    }

    const double len = sqrt(dx * dx + dy * dy) + 1e-6;
    const double nx = dx / len;
    const double ny = dy / len;

    pShot->x = cx + dx * ratio + nx * (extra + vib);
    pShot->y = cy + dy * ratio + ny * (extra + vib);
}

// 蜂本体弾: レムニスケート(8の字)軌道 + 長軸のゆっくりした回転(すべてcountの純関数)
static void UpdateBeeShot(sEnemyShot* pShot, double cx, double cy)
{
    const double phase = pShot->param_d[0];
    const double aScale = pShot->param_d[1];
    const int c = pShot->count;

    double growRatio = (c < BEE_GROW_FRAME) ? (double)c / BEE_GROW_FRAME : 1.0;
    double a = BEE_A * aScale * growRatio;

    double theta = BEE_OMEGA * c + phase;
    double s = sin(theta), co = cos(theta);
    double denom = 1.0 + s * s;

    double lx = a * co / denom;
    double ly = a * s * co / denom;

    // 長軸方向の回転(次の花粉散布方向を予告する)
    double phi = BEE_ROT_OMEGA * c;
    double rx = lx * cos(phi) - ly * sin(phi);
    double ry = lx * sin(phi) + ly * cos(phi);

    // フェーズ終盤: 軌道半径そのものを膨らませて自然に画面外へ退場させる
    if (c > BEE_DISPERSE_START) {
        double t = (double)(c - BEE_DISPERSE_START);
        double disperse = 1.0 + BEE_DISPERSE_ACCEL * t * t / (a + 1.0);
        rx *= disperse;
        ry *= disperse;
    }

    pShot->x = cx + rx;
    pShot->y = cy + ry;

    // 交差点(原点付近)通過の瞬間だけ明滅させ、羽音の振動を演出
    bool blinking = (fabs(co) < 0.12);
    pShot->kind = blinking ? img_enemyShotMediumBall[6] : img_enemyShotMediumBall[8];

    // 花粉散布時に使う長軸方向をここに保存しておく
    pShot->param_d[2] = phi;
}

// 花粉弾・針弾 共通: 発生時の位置・方向・速度から直線飛翔(countの純関数、速度積分はしない)
static void UpdateStraightShot(sEnemyShot* pShot)
{
    const double sx = pShot->param_d[0];
    const double sy = pShot->param_d[1];
    const int c = pShot->count;

    pShot->x = sx + pShot->speed * cos(pShot->muki) * c;
    pShot->y = sy + pShot->speed * sin(pShot->muki) * c;
}

// 弾幕本体: 羽音の八の字舞
static void ShotHaneotoHachinojiMai(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;
    const double cx = pEnemyShotSet->x;
    const double cy = pEnemyShotSet->y;

    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light/medium/heavy/extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // --- フェーズ1: 巣房(ハニカム) 内側リング ---
        for (int i = 0; i < HONEY_RING1_NUM; i++) {
            pShot = new sEnemyShot;
            double ang = DX_PI / 180.0 * (60.0 * i);
            pShot->param_i[0] = SHOT_ROLE_HONEY;
            pShot->param_d[0] = HONEY_R1 * cos(ang);
            pShot->param_d[1] = HONEY_R1 * sin(ang);
            pShot->param_d[2] = GetRand(628) / 100.0; // 振動位相をばらけさせる
            pShot->kind = img_enemyShotMediumOval[1];  // 黄: 巣房
            pShot->x = cx;
            pShot->y = cy;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // --- フェーズ1: 巣房(ハニカム) 外側リング ---
        for (int i = 0; i < HONEY_RING2_NUM; i++) {
            pShot = new sEnemyShot;
            double ang = DX_PI / 180.0 * (60.0 * i + 30.0);
            pShot->param_i[0] = SHOT_ROLE_HONEY;
            pShot->param_d[0] = HONEY_R2 * cos(ang);
            pShot->param_d[1] = HONEY_R2 * sin(ang);
            pShot->param_d[2] = GetRand(628) / 100.0;
            pShot->kind = img_enemyShotMediumOval[8]; // 橙: 外側の巣房
            pShot->x = cx;
            pShot->y = cy;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // --- フェーズ2: 働き蜂(8の字ダンス本体) ---
        for (int i = 0; i < BEE_NUM; i++) {
            pShot = new sEnemyShot;
            pShot->param_i[0] = SHOT_ROLE_BEE;
            pShot->param_d[0] = 2.0 * DX_PI / BEE_NUM * i; // 位相
            pShot->param_d[1] = 0.9 + GetRand(20) / 100.0; // aのばらつき(0.90〜1.09)
            pShot->kind = img_enemyShotMediumBall[8];       // 橙: 蜂本体
            pShot->x = cx;
            pShot->y = cy;
            pShot->margin = 480;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // --- フェーズ2: 針弾(蜂の一撃) — 自機を直接狙う高速弾を周期的に放つ ---
    if (pEnemyShotSet->count >= STINGER_START_FRAME &&
        pEnemyShotSet->count <= STINGER_END_FRAME &&
        (pEnemyShotSet->count - STINGER_START_FRAME) % STINGER_INTERVAL == 0) {

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == SHOT_ROLE_BEE) {
                sEnemyShot* stinger = new sEnemyShot;
                stinger->param_i[0] = SHOT_ROLE_STINGER;
                stinger->param_d[0] = pShot->x; // 発生時座標をスナップショット
                stinger->param_d[1] = pShot->y;
                stinger->muki = atan2(player.y - pShot->y, player.x - pShot->x); // 自機を直接狙う
                stinger->speed = STINGER_SPEED;
                stinger->kind = img_enemyShotBullet[7]; // 黒: 針
                stinger->x = pShot->x;
                stinger->y = pShot->y;

                stinger->prev = pEnemyShotSet->pEnemyShotHead->prev;
                stinger->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = stinger;
                pEnemyShotSet->pEnemyShotHead->prev = stinger;
            }
            pShot = pShot->next;
        }
    }

    // --- フェーズ3: 花粉散布 (蜂本体の現在位置から自機を狙って一定間隔で発生) ---
    if (pEnemyShotSet->count >= POLLEN_START_FRAME &&
        pEnemyShotSet->count <= POLLEN_END_FRAME &&
        (pEnemyShotSet->count - POLLEN_START_FRAME) % POLLEN_INTERVAL == 0) {

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == SHOT_ROLE_BEE) {
                // 自機を狙う方向を基準に、扇状にばら撒く
                double baseDir = atan2(player.y - pShot->y, player.x - pShot->x);

                for (int k = 0; k < POLLEN_PER_BURST; k++) {
                    sEnemyShot* pollen = new sEnemyShot;
                    double spread = (GetRand(200) / 100.0 - 1.0) * POLLEN_SPREAD_HALF;

                    pollen->param_i[0] = SHOT_ROLE_POLLEN;
                    pollen->param_d[0] = pShot->x; // 発生時座標をスナップショット
                    pollen->param_d[1] = pShot->y;
                    pollen->muki = baseDir + spread;
                    pollen->speed = POLLEN_SPEED_MIN + GetRand(100) / 100.0 * POLLEN_SPEED_RANGE;
                    pollen->kind = img_enemyShotSmallBall[1]; // 黄: 花粉
                    pollen->x = pShot->x;
                    pollen->y = pShot->y;

                    pollen->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pollen->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pollen;
                    pEnemyShotSet->pEnemyShotHead->prev = pollen;
                }
            }
            pShot = pShot->next;
        }
    }

    // --- 全弾の位置更新(役割ごとに分岐) ---
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case SHOT_ROLE_HONEY:
            UpdateHoneycombShot(pShot, cx, cy);
            break;
        case SHOT_ROLE_BEE:
            UpdateBeeShot(pShot, cx, cy);
            break;
        case SHOT_ROLE_POLLEN:
        case SHOT_ROLE_STINGER:
            UpdateStraightShot(pShot);
            break;
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Bee_Claude()
{
    static int sway;
    constexpr int CYCLE = 480; // ダンス1サイクルの長さ(この間隔で新しい舞を開始)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        sway = 1;
    }
    else {
        enemy.x += 0.5 * (double)sway;
        if (count % 180 == 90) sway *= -1;
    }

    if (count % CYCLE == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHaneotoHachinojiMai;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 220.0; // 8の字舞の中心(画面中央よりやや上)
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}