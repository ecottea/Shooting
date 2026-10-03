#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// プリズム・スペクトル弾幕
//   大玉(白)        : プリズムの3頂点
//   中玉(シアン)    : プリズムの辺
//   小玉(白)        : 入射光
//   中玉(赤/緑/青)  : 屈折光
//   小玉(赤黄緑青紫): 分散スペクトル
// ============================================================

// 弾を弾幕セットに追加するヘルパー
//   type : 0=頂点, 1=辺, 2=入射光, 3=屈折光, 4=屈折済み, 5=分散光
static void AddShot(sEnemyShotSet* pSet,
    double x, double y, double muki, double speed,
    int kind, int type, int sub)
{
    sEnemyShot* s = new sEnemyShot;
    s->x = x;
    s->y = y;
    s->muki = muki;
    s->speed = speed;
    s->kind = kind;
    s->param_i[0] = type;
    s->param_i[1] = sub;
    s->prev = pSet->pEnemyShotHead->prev;
    s->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = s;
    pSet->pEnemyShotHead->prev = s;
}

// プリズム弾幕パターン本体
static void ShotPrism(sEnemyShotSet* pSet)
{
    const double cx = pSet->x;
    const double cy = pSet->y;
    const double R = 70.0;                 // プリズム半径
    const double a0 = pSet->muki;           // 回転オフセット
    const double TWO_PI_3 = 2.0 * DX_PI / 3.0;

    // ---- プリズム生成 ----
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 3頂点（大玉・白）
        for (int i = 0; i < 3; i++) {
            double ang = a0 + i * TWO_PI_3;
            AddShot(pSet, cx + R * cos(ang), cy + R * sin(ang),
                ang, 0.0, img_enemyShotLargeBall[6], 0, i);
        }
        // 辺（中玉・シアン）各辺3個
        for (int i = 0; i < 3; i++) {
            int j = (i + 1) % 3;
            double ax = cx + R * cos(a0 + i * TWO_PI_3);
            double ay = cy + R * sin(a0 + i * TWO_PI_3);
            double bx = cx + R * cos(a0 + j * TWO_PI_3);
            double by = cy + R * sin(a0 + j * TWO_PI_3);
            for (int k = 1; k <= 3; k++) {
                double t = k / 4.0;
                AddShot(pSet, ax * (1 - t) + bx * t, ay * (1 - t) + by * t,
                    0.0, 0.0, img_enemyShotMediumBall[3], 1, i);
            }
        }
    }

    // ---- 入射光（50フレーム間隔・頂点から中心へ） ----
    if (pSet->count >= 40 && (pSet->count - 40) % 25 == 0 && pSet->count <= 480) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        int v = ((pSet->count - 40) / 25) % 3;
        double ang = a0 + v * TWO_PI_3;
        for (int k = 0; k < 6; k++) {
            double d = 180.0 + k * 14.0;    // 外側から
            AddShot(pSet, cx + d * cos(ang), cy + d * sin(ang),
                ang + DX_PI, 4.0, img_enemyShotSmallBall[6], 2, v);
            pSet->pEnemyShotHead->prev->margin = 240;
        }
    }

    // ---- 各ショットの更新 ----
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;   // 挿入に備えて退避

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        int type = pShot->param_i[0];

        // 入射光が中心付近に達したら屈折（赤緑青の中玉を3方向へ）
        if (type == 2 && pSet->count <= 480) {
            double dx = pShot->x - cx;
            double dy = pShot->y - cy;
            if (dx * dx + dy * dy < 400.0) {   // 中心から20px以内
                int v = pShot->param_i[1];
                pShot->param_i[0] = -1;        // フラグ消費
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                int colors[3] = { 0, 2, 4 };   // 赤・緑・青
                for (int d = 0; d < 3; d++) {
                    double refAng = a0 + v * TWO_PI_3
                        + (d - 1) * DX_PI / 5.0;
                    AddShot(pSet, cx, cy, refAng, 2.5,
                        img_enemyShotMediumBall[colors[d]], 3, 0);
                }
            }
        }

        // 屈折光が一定距離進んだら分散（小玉5色の扇状）
        if (type == 3 && pShot->count > 50) {
            pShot->param_i[0] = 4;             // 分裂済み
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            int colors[5] = { 0, 1, 2, 4, 5 }; // 赤黄緑青マゼンタ
            for (int d = 0; d < 5; d++) {
                double dispAng = pShot->muki + (d - 2) * DX_PI / 14.0;
                AddShot(pSet, pShot->x, pShot->y, dispAng, 1.5,
                    img_enemyShotSmallBall[colors[d]], 5, 0);
            }
        }

        // プリズム崩壊（一定時間経過で頂点・辺が外側へ）
        if (pSet->count > 480
            && (type == 0 || type == 1) && pShot->speed == 0.0) {
            double dx = pShot->x - cx;
            double dy = pShot->y - cy;
            pShot->muki = atan2(dy, dx);
            pShot->speed = 3.0;
        }

        pShot = nextShot;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Prism_DeepSeek()
{
    static int    shot_count;
    static double prism_angle;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;       // 200から変更しないこと
        shot_count = 0;
        prism_angle = -DX_PI / 2.0;         // 頂点が上向き
    }
    else {
        // 左右にゆっくり揺れる
        enemy.x = 240.0 + 180.0 * sin(count * 0.015);
        enemy.y = 60.0 + 15.0 * sin(count * 0.030);
    }

    // プリズムサイクル開始（360フレームごと・30度ずつ回転）
    if (count % 360 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPrism;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 60.0;   // プリズム中心を少し下へ
        pEnemyShotSet->muki = prism_angle;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        prism_angle += DX_PI / 6.0;          // 30度回転
    }
}