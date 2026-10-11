#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <vector>

// 平安京の悪夢 - 東方星蓮船風
// Wは固定、H,T,N,P,R,Dは毎回変化

static const double W_FIXED = 40.0; // 広め固定

// ------------------------------------------------------------
// 大玉列用パターン（等速直進）
// ------------------------------------------------------------
static void Pat_Large(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// 基本グリッドパターン
// ------------------------------------------------------------
static void Pat_Grid(sEnemyShotSet* pSet)
{
    int T = pSet->param_i[0];
    int transposed = pSet->param_i[1]; // 0:通常(下向き) 1:転置(右向き)

    int c = pSet->count;
    int t0 = 60; // 出現待機
    int t1 = t0 + 2 * T; // 移動終了
    int t2 = t1 + 60; // 移動後待機
    int t3 = t2 + 60; // 消去後待機

    if (c < t0) {
        // 出現位置で静止
        sEnemyShot* s = pSet->pEnemyShotHead->next;
        while (s != pSet->pEnemyShotHead) {
            s->x = s->param_d[0];
            s->y = s->param_d[1];
            s->speed = 0.0;
            s = s->next;
        }
    }
    else if (c < t1) {
        int tm = c - t0; // 0.. 2T-1
        double u = (double)tm / (double)(2 * T);
        // ゆっくり加速・減速: sinイージング (両端速度0)
        double f = (1.0 - cos(DX_PI * u)) * 0.5;

        sEnemyShot* s = pSet->pEnemyShotHead->next;
        while (s != pSet->pEnemyShotHead) {
            if (tm == 0) {
                // レーザー -> 中玉(赤)
                s->kind = img_enemyShotMediumBall[0];
            }
            double x0 = s->param_d[0];
            double y0 = s->param_d[1];
            double x2 = s->param_d[2];
            double y2 = s->param_d[3];
            s->x = x0 + (x2 - x0) * f;
            s->y = y0 + (y2 - y0) * f;
            s = s->next;
        }
    }
    else if (c < t2) {
        sEnemyShot* s = pSet->pEnemyShotHead->next;
        while (s != pSet->pEnemyShotHead) {
            if (c == t1) {
                // 中玉 -> レーザーに戻す
                s->kind = img_enemyShotLaser[0];
            }
            s->x = s->param_d[2];
            s->y = s->param_d[3];
            s->speed = 0.0;
            s = s->next;
        }
    }
    else if (c == t2) {
        // 全レーザー消去
        sEnemyShot* cur = pSet->pEnemyShotHead->next;
        while (cur != pSet->pEnemyShotHead) {
            sEnemyShot* nxt = cur->next;
            cur->prev->next = cur->next;
            cur->next->prev = cur->prev;
            delete cur;
            cur = nxt;
        }
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    }
    else {
        // t2 < c < t3 : 消去後待機、何もしない
        // t3以降も空のまま残置（プール4096あるため問題なし）
    }
}

// ------------------------------------------------------------
// グリッド生成ヘルパ
// intervalX, intervalY は画面上の間隔
// transposedフラグは向きだけに使う（0:下 1:右）
// ------------------------------------------------------------
static sEnemyShotSet* CreateGridSet(double intervalX, double intervalY,
    int N, const std::vector<int>& P, const std::vector<int>& R,
    const std::vector<int>& D, const std::vector<int>& Q,
    int T, int transposed)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = Pat_Grid;
    pSet->x = 0; pSet->y = 0; pSet->muki = 0;
    pSet->param_i[0] = T;
    pSet->param_i[1] = transposed;
    pSet->param_d[0] = intervalX;
    pSet->param_d[1] = intervalY;
   
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    // (-60,-60)～(540,540) に配置
    int cols = (int)(600.0 / intervalX) + 1;
    int rows = (int)(600.0 / intervalY) + 1;

    for (int i = 0; i < cols; i++) {
        for (int j = 0; j < rows; j++) {
            int r = j % N;
            int i2 = i + (R[r] == 1 ? 1 : -1);
            int j2 = j + (D[r] == 1 ? Q[r] : Q[r] - N);

            double x0 = -60.0 + i * intervalX;
            double y0 = -60.0 + j * intervalY;
            double x2 = -60.0 + i2 * intervalX;
            double y2 = -60.0 + j2 * intervalY;

            sEnemyShot* s = new sEnemyShot;
            s->x = x0;
            s->y = y0;
            s->muki = (transposed == 0) ? DX_PI * 0.5 : 0.0; // 下 or 右
            s->speed = 0.0;
            s->kind = img_enemyShotLaser[0]; // 赤レーザー
            s->margin = 60;
            s->param_d[0] = x0;
            s->param_d[1] = y0;
            s->param_d[2] = x2;
            s->param_d[3] = y2;
            s->param_i[0] = r;

            s->prev = pSet->pEnemyShotHead->prev;
            s->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = s;
            pSet->pEnemyShotHead->prev = s;
        }
    }

    // グローバルリストに登録
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// ------------------------------------------------------------
// 大玉列生成
// ------------------------------------------------------------
static void SpawnVerticalColumn()
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = Pat_Large;
    pSet->x = 0; pSet->y = 0; pSet->muki = 0;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    double px = player.x;
    if (px < 0) px = 0; if (px > 480) px = 480;

    int fromBottom = (player.y < 240.0) ? 1 : 0;
    int K = 6;
    double spacing = 32.0;

    for (int k = 0; k < K; k++) {
        sEnemyShot* s = new sEnemyShot;
        s->x = px;
        s->y = fromBottom ? 540.0 + k * spacing : -60.0 - k * spacing;
        s->muki = fromBottom ? -DX_PI * 0.5 : DX_PI * 0.5;
        s->speed = 2.5 + GetRand(100) / 100.0;
        s->kind = img_enemyShotLargeBall[4];
        s->margin = 60;
        s->prev = pSet->pEnemyShotHead->prev;
        s->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = s;
        pSet->pEnemyShotHead->prev = s;
    }

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

static void SpawnHorizontalColumn()
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = Pat_Large;
    pSet->x = 0; pSet->y = 0; pSet->muki = 0;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    double py = player.y;
    if (py < 0) py = 0; if (py > 480) py = 480;

    int fromRight = (player.x < 240.0) ? 1 : 0;
    int K = 6;
    double spacing = 32.0;

    for (int k = 0; k < K; k++) {
        sEnemyShot* s = new sEnemyShot;
        s->y = py;
        s->x = fromRight ? 540.0 + k * spacing : -60.0 - k * spacing;
        s->muki = fromRight ? DX_PI : 0.0;
        s->speed = 2.5 + GetRand(100) / 100.0;
        s->kind = img_enemyShotLargeBall[1];
        s->margin = 60;
        s->prev = pSet->pEnemyShotHead->prev;
        s->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = s;
        pSet->pEnemyShotHead->prev = s;
    }

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_Heiankyo_MetaAI()
{
    static int gridCooldown = 0;
    static int largeCooldown = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        gridCooldown = 0;
        largeCooldown = 0;
    }
    else {
        enemy.x += sin(count * 0.02) * 0.6;
    }

    int hp = enemy.hp;
    int maxHp = enemy.maxHp;
    int phase = 0;
    if (hp > maxHp * 2 / 3) phase = 0;
    else if (hp > maxHp / 3) phase = 1;
    else phase = 2;

    // ---- 基本パターン生成 ----
    if (gridCooldown <= 0) {
        int N = 3 + GetRand(4);
        double H = 16.0 + GetRand(16);
        int T = 45 + GetRand(45-20)+20;
        double W = W_FIXED;

        std::vector<int> P(N), R(N), D(N), Q(N);
        for (int i = 0; i < N; i++) P[i] = i;
        for (int i = N - 1; i >= 1; i--) {
            int j = GetRand(i);
            int tmp = P[i]; P[i] = P[j]; P[j] = tmp;
        }
        for (int i = 0; i < N; i++) {
            R[i] = GetRand(1);
            D[i] = GetRand(1);
        }
        for (int i = 0; i < N; i++) {
            Q[i] = (P[i] - i) % N;
            if (Q[i] < 0) Q[i] += N;
        }

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        if (phase == 0) {
            CreateGridSet(W, H, N, P, R, D, Q, T, 0);
        }
        else if (phase == 1) {
            CreateGridSet(H*2, W, N, P, R, D, Q, T, 1);
        }
        else {
            CreateGridSet(W, H, N, P, R, D, Q, T, 0);
            CreateGridSet(H*2, W, N, P, R, D, Q, T, 1);
        }

        gridCooldown = 180 + 2 * T;
    }
    else {
        gridCooldown--;
    }

    // ---- 大玉列 ----
    if (phase == 0) {
        if (largeCooldown <= 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            SpawnVerticalColumn();
            largeCooldown = 70;
        }
        else largeCooldown--;
    }
    else if (phase == 1) {
        if (largeCooldown <= 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            SpawnHorizontalColumn();
            largeCooldown = 70;
        }
        else largeCooldown--;
    }
}