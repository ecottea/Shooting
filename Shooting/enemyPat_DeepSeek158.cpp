#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// パターン名：『二重スリット・コヒーレンス』
// ============================================================

// ---- スリット位置（画面座標） ----
static const double SLIT_Y = 100.0;
static const double SLIT_X1 = 160.0;
static const double SLIT_X2 = 320.0;

// ---- 観測者が画面内にいるか ----
static bool g_observerActive = false;

// ============================================================
// ヘルパ：弾を1つ追加
// ============================================================
static sEnemyShot* AddEnemyShot(sEnemyShotSet* set)
{
    sEnemyShot* s = new sEnemyShot;
    s->prev = set->pEnemyShotHead->prev;
    s->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = s;
    set->pEnemyShotHead->prev = s;
    return s;
}

// ============================================================
// ヘルパ：このセットに属する全弾の座標を毎フレーム更新
// ============================================================
static void UpdateShots(sEnemyShotSet* set)
{
    sEnemyShot* s = set->pEnemyShotHead->next;
    while (s != set->pEnemyShotHead) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
        s = s->next;
    }
}

// ============================================================
// ヘルパ：弾幕セットを1つ追加
// ============================================================
static sEnemyShotSet* AddEnemyShotSet(sEnemyShotSet::PatternFunc func,
    double x, double y,
    double muki, int kind)
{
    sEnemyShotSet* set = new sEnemyShotSet;
    set->count = 0;
    set->patternFunc = func;
    set->x = x;
    set->y = y;
    set->muki = muki;
    set->kind = kind;
    set->pEnemyShotHead = new sEnemyShot;
    set->pEnemyShotHead->prev = set->pEnemyShotHead;
    set->pEnemyShotHead->next = set->pEnemyShotHead;
    set->prev = enemyShotSetHead.prev;
    set->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = set;
    enemyShotSetHead.prev = set;
    return set;
}

// ============================================================
// スリット板：大弾を横一列に並べ、2箇所だけ隙間を空ける
// （板は静止させたいので speed=0。UpdateShots は実質無移動）
// ============================================================
static void PatternSlitBoard(sEnemyShotSet* set)
{
    if (set->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const double wallX[] = {
            0.0,  40.0,  80.0, 120.0,
            200.0, 240.0, 280.0,
            360.0, 400.0, 440.0, 480.0
        };
        const int wallN = (int)(sizeof(wallX) / sizeof(wallX[0]));

        for (int i = 0; i < wallN; i++) {
            sEnemyShot* s = AddEnemyShot(set);
            s->x = wallX[i];
            s->y = SLIT_Y;
            s->muki = 0.0;
            s->speed = 0.0;
            s->kind = img_enemyShotLargeBall[6]; // 白
        }

        const double edgeX[] = {
            SLIT_X1 - 20.0, SLIT_X1 + 20.0,
            SLIT_X2 - 20.0, SLIT_X2 + 20.0
        };
        for (int i = 0; i < 4; i++) {
            sEnemyShot* s = AddEnemyShot(set);
            s->x = edgeX[i];
            s->y = SLIT_Y;
            s->muki = 0.0;
            s->speed = 0.0;
            s->kind = img_enemyShotMediumBall[3]; // シアン
        }
    }

    UpdateShots(set);
}

// ============================================================
// 波の放出：2つのスリットから下向き半円の波面を出す
// ============================================================
static void PatternWave(sEnemyShotSet* set)
{
    const int    WAVE_INTERVAL = 25;
    const int    N = 11;
    const double SPEED = 1.7;

    if (!g_observerActive && set->count % WAVE_INTERVAL == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const double slits[2] = { SLIT_X1, SLIT_X2 };

        for (int s = 0; s < 2; s++) {
            for (int i = 0; i < N; i++) {
                double ang = DX_PI * (double)i / (double)(N - 1);

                sEnemyShot* b = AddEnemyShot(set);
                b->x = slits[s];
                b->y = SLIT_Y + 20.0;
                b->muki = ang;
                b->speed = SPEED;
                b->kind = img_enemyShotSmallBall[3]; // シアン（波）
            }
        }
    }

    UpdateShots(set);
}

// ============================================================
// 観測者：目の形の弾を出してゆっくり降下、
// その間はスリットから粒子線を放つ
// ============================================================
static void PatternObserver(sEnemyShotSet* set)
{
    const int    LIFETIME = 350/2;
    const double SPEED = 1.3;

    if (set->count == 0) {
        g_observerActive = true;
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const double ox = set->x;
        const double oy = set->y;
        const double mk = DX_PI / 2.0;

        // 瞳（大弾）
        {
            sEnemyShot* b = AddEnemyShot(set);
            b->x = ox;
            b->y = oy;
            b->muki = mk;
            b->speed = SPEED;
            b->kind = img_enemyShotLargeBall[3]; // シアン
        }
        // 輪郭（上下左右の中弾で菱形の目）
        const double dx[4] = { 0.0,   0.0, -36.0,  36.0 };
        const double dy[4] = { -18.0,  18.0,   0.0,   0.0 };
        for (int i = 0; i < 4; i++) {
            sEnemyShot* e = AddEnemyShot(set);
            e->x = ox + dx[i];
            e->y = oy + dy[i];
            e->muki = mk;
            e->speed = SPEED;
            e->kind = img_enemyShotMediumBall[5]; // マゼンタ
        }
    }
    else if (set->count < LIFETIME) {
        // 粒子線（発射時点のプレイヤー位置を狙う）
        if (set->count % 15 == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            const double slits[2] = { SLIT_X1, SLIT_X2 };
            for (int s = 0; s < 2; s++) {
                double ang = atan2(player.y - (SLIT_Y + 20.0),
                    player.x - slits[s]);
                ang += (GetRand(20) - 10) / 180.0 * DX_PI;

                sEnemyShot* b = AddEnemyShot(set);
                b->x = slits[s];
                b->y = SLIT_Y + 20.0;
                b->muki = ang;
                b->speed = 4.0;
                b->kind = img_enemyShotSmallBall[0]; // 赤（粒子）
            }
        }
    }
    else if (set->count == LIFETIME) {
        g_observerActive = false;
    }

    // 既存の弾（目の形・粒子線）を毎フレーム動かす
    UpdateShots(set);
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_DoubleSlit_DeepSeek()
{
    static int  muki;
    static bool boardSpawned;
    static bool waveSpawned;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        boardSpawned = false;
        waveSpawned = false;
        g_observerActive = false;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (enemy.x > 400.0) muki = -1;
        if (enemy.x < 80.0) muki = 1;
    }

    if (count == 60 && !boardSpawned) {
        boardSpawned = true;
        AddEnemyShotSet(PatternSlitBoard, 240.0, SLIT_Y, 0.0, 0);
    }

    if (count == 120 && !waveSpawned) {
        waveSpawned = true;
        AddEnemyShotSet(PatternWave, 240.0, SLIT_Y, 0.0, 0);
    }

    if (count >= 300 && (count - 300) % 420 == 0) {
        AddEnemyShotSet(PatternObserver, 240.0, 60.0, 0.0, 0);
    }
}