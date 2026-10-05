#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

int K;

// 弾を1つ追加するヘルパー
static void AddShot(sEnemyShotSet* set, double x, double y, double muki, double speed, int kind,
    int param0 = 0, int param1 = 0, double paramd0 = 0.0)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = param0;
    p->param_i[1] = param1;
    p->param_d[0] = paramd0;
    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
}

// サボテンの節を追加（大玉/中玉＋周囲の針）
// rx, ry は set の位置からの相対座標
static void AddCactusSegment(sEnemyShotSet* set, double rx, double ry, bool isLarge)
{
    double x = set->x + rx;
    double y = set->y + ry;

    int bodyKind = isLarge ? img_enemyShotLargeBall[2] : img_enemyShotMediumBall[2];
    // 幹・腕の本体：param_i[0]=2 で後で落下させる
    AddShot(set, x, y, 0.0, 0.0, bodyKind, 2, 0, 0.0);

    double radius = isLarge ? 24.0 : 16.0;
    for (int i = 0; i < 8*K; i++) {
        double ang = i * DX_PI / 4.0/K;
        double nx = x + radius * cos(ang);
        double ny = y + radius * sin(ang);
        // 針：param_i[0]=1, param_i[1]=0（未発射）
        AddShot(set, nx, ny, ang, 0.0, img_enemyShotSmallBall[6], 1, 0, radius);
    }
}

// サボテン弾幕本体
static void ShotCactus(sEnemyShotSet* set)
{
    int t = set->count % 300;

    // --- 予告音 ---
    if (t == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- 成長：幹 ---
    if (t >= 10 && t <= 34 && (t - 10) % 8 == 0) {
        int idx = (t - 10) / 8; // 0..3
        AddCactusSegment(set, 0.0, idx * 30.0, true);
    }

    // --- 成長：左右の腕 ---
    if (t == 42) {
        AddCactusSegment(set, -40.0, 40.0, false);
        AddCactusSegment(set, 40.0, 40.0, false);
    }
    if (t == 50) {
        AddCactusSegment(set, -40.0, 10.0, false);
        AddCactusSegment(set, 40.0, 10.0, false);
    }

    // --- 腕の針：自機狙い5-way ---
    if (t == 120) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double bx[2] = { -40.0, 40.0 };
        for (int side = 0; side < 2; side++) {
            double sx = set->x + bx[side];
            double sy = set->y + 10.0;
            double base = atan2(player.y - sy, player.x - sx);
            for (int i = -2; i <= 2; i++) {
                double ang = base + i * 10.0 / 180.0 * DX_PI;
                AddShot(set, sx, sy, ang, 4.0, img_enemyShotBullet[1], 3, 0, 0.0);
            }
        }
    }

    // --- 全身の針：一斉発射 ---
    if (t == 160) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* p = set->pEnemyShotHead->next;
        while (p != set->pEnemyShotHead) {
            if (p->param_i[0] == 1 && p->param_i[1] == 0) {
                p->param_i[1] = 1;
                p->speed = 2.5 + GetRand(100) / 100.0; // 2.5～3.5
                // muki は配置時に外向きで設定済み
            }
            p = p->next;
        }
    }

    // --- 崩壊：本体が落下 ---
    if (t == 220) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* p = set->pEnemyShotHead->next;
        while (p != set->pEnemyShotHead) {
            if (p->param_i[0] == 2 && p->param_i[1] == 0) {
                p->param_i[1] = 1;
                p->speed = 2.0;
                p->muki = DX_PI / 2.0; // 下向き
            }
            p = p->next;
        }

        K++; K++;
    }

    // --- 全弾移動 ---
    sEnemyShot* p = set->pEnemyShotHead->next;
    while (p != set->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// 敵本体のパターン
void EnemyPat_Cactus_DeepSeek()
{
    static bool spawned = false;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        spawned = false;
        K = 1;
    }

    // 敵は上部中央で静止
    enemy.x = 240.0;
    enemy.y = 140.0;

    if (!spawned) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCactus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
        spawned = true;
    }
}