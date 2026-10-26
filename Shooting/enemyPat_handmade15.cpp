#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：鎖状ランダムドットステレオグラム
// 3D像は静止、鎖ごとにランダム速度で落下

// (x,y) が3つの塗りつぶし円の内側なら s=12、外側なら s=0
static int StereoS(double x, double y)
{
    const double CX = 185.0;   // 頭の中心X（さらに左へ）
    const double CY = 300.0;   // 頭の中心Y
    const double R_head = 100.0;   // 頭の半径（拡大）
    const double R_ear = 64.0;    // 耳の半径（拡大）
    const double EDX = 113.0;   // 耳の水平オフセット（比率維持）
    const double EDY = 113.0;   // 耳の垂直オフセット（比率維持）

    const double cx[3] = { CX,       CX - EDX, CX + EDX };
    const double cy[3] = { CY,       CY - EDY, CY - EDY };
    const double rr[3] = { R_head,   R_ear,    R_ear };

    for (int i = 0; i < 3; i++) {
        double dx = x - cx[i];
        double dy = y - cy[i];
        if (dx * dx + dy * dy < rr[i] * rr[i]) {
            return 12;   // 塗りつぶし内部はすべて同じ奥行き
        }
    }
    return 0;            // 背景
}

static void ShotStereogram(sEnemyShotSet* pEnemyShotSet)
{
    const int    W = 480, H = 480;
    const int    D = 120;
    const int    N_COLORS = 9;
    const int    MAX_CHAINS = 450;
    const int    SAFE_MAX = 3500;

    // ---- 鎖キャッシュ（初回フレームで一度だけ生成） ----
    static double ch_x0[MAX_CHAINS];
    static double ch_y0[MAX_CHAINS];
    static int    ch_c[MAX_CHAINS];
    static double ch_v[MAX_CHAINS];
    static int    ch_n = 0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int SPAWN_H = H / 4;   // 画面上側1/4の高さ(120)

        ch_n = 0;
        for (int i = 0; i < MAX_CHAINS; i++) {
            ch_x0[ch_n] = (double)GetRand(D - 1);
            ch_y0[ch_n] = (double)GetRand(SPAWN_H - 1);   // ← 0〜119 に限定
            ch_c[ch_n] = GetRand(N_COLORS - 1);
            ch_v[ch_n] = 0.1 + GetRand(2500) / 1000.0;
            ch_n++;
        }
    }

    // ---- 前フレームの弾を全消去 ----
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nx = p->next;
        p->prev->next = p->next;
        p->next->prev = p->prev;
        delete p;
        p = nx;
    }

    // ---- 鎖を現在位置から再構築 ----
    int total = 0;
    for (int i = 0; i < ch_n && total < SAFE_MAX; i++) {
        double y = fmod(ch_y0[i] + ch_v[i] * (double)pEnemyShotSet->count,
            (double)H);
        double x = ch_x0[i];

        while (x < W && total < SAFE_MAX) {
            sEnemyShot* q = new sEnemyShot;
            q->x = x;
            q->y = y;
            q->muki = 0.0;
            q->speed = 0.0;
            q->kind = img_enemyShotSmallBall[ch_c[i]];
            q->param_i[0] = 0;
            q->prev = pEnemyShotSet->pEnemyShotHead->prev;
            q->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = q;
            pEnemyShotSet->pEnemyShotHead->prev = q;
            total++;

            // 現在位置でのずらし量（奥行き場は画面固定 → 3D像は静止）
            int s = StereoS(x, y);
            x += (double)(D - s);
        }
    }

    // ---- 赤大玉 2 個を末尾に再生成（描画順で最前面） ----
    for (int i = 0; i < 2; i++) {
        sEnemyShot* q = new sEnemyShot;
        q->x = 240.0 - D * 0.5 + i * D;
        q->y = 80.0;
        q->muki = 0.0;
        q->speed = 0.0;
        q->kind = img_enemyShotLargeBall[0];
        q->param_i[0] = 1;
        q->prev = pEnemyShotSet->pEnemyShotHead->prev;
        q->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = q;
        pEnemyShotSet->pEnemyShotHead->prev = q;
    }
}

void EnemyPat_RDS()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ---- 弾幕セットは count==60 で一度だけ生成 ----
    if (count == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotStereogram;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 240.0;
        pEnemyShotSet->muki = 0.0;
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