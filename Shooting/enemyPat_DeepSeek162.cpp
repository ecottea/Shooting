#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：乳白の静謐「ポアリング・ミルクメイド」
//   - 上部：白い大玉でミルクメイドと壺のシルエットを一時描画
//   - 中央：壺の口から白い中玉が細い牛乳液となって落下
//   - 下部：床で白い小玉リングの波紋が広がり、自機狙いの飛沫が跳ねる
//  使用素材：大玉・中玉・小玉のみ（すべて白=色6）
// ============================================================

// --- 補助 ---
static void InsertShot(sEnemyShotSet* set, double x, double y,
    double muki, double speed, int kind, int tag)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = tag; // 0=牛乳液, 1=波紋, 2=飛沫, 99=シルエット
    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
}

static void DeleteShotInList(sEnemyShot* p)
{
    p->prev->next = p->next;
    p->next->prev = p->prev;
    delete p;
}

static sEnemyShotSet* CreateSet(double x, double y, double muki,
    sEnemyShotSet::PatternFunc func, int kind)
{
    sEnemyShotSet* set = new sEnemyShotSet;
    set->count = 0;
    set->x = x;
    set->y = y;
    set->muki = muki;
    set->patternFunc = func;
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
//  シルエット：白い大玉を低速停止配置してミルクメイドと壺を描く
//  一定時間で消去（スペル背景のような演出）
// ============================================================
static void ShotMaidSilhouette(sEnemyShotSet* pSet)
{
    const int WHITE = 6;
    const int LIFETIME = 180; // 約3秒

    if (pSet->count == 0) {
        struct P { double x, y; };
        const P pts[] = {
            // 頭
            {  0, -55}, {  0, -45}, {  0, -35},
            // 首・肩
            {  0, -28}, {-22, -22}, { 22, -22},
            // 胴
            {-15, -10}, { 15, -10}, {  0, -10},
            // スカート
            {  0,   5}, {-15,  20}, {-28,  32},
            { 15,  20}, { 28,  32}, {  0,  25}, {  0,  35},
            // 右腕（壺へ伸ばす）
            { 30, -15}, { 40,  -5},
            // 壺（縦のライン）
            { 50,   0}, { 50,  10}, { 50,  18},
        };
        const int n = (int)(sizeof(pts) / sizeof(pts[0]));
        for (int i = 0; i < n; i++) {
            InsertShot(pSet, pSet->x + pts[i].x*2, pSet->y + pts[i].y*2,
                0.0, 0.0, img_enemyShotLargeBall[WHITE], 99);
        }
    }

    // 時間経過で全消去
    if (pSet->count == LIFETIME) {
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            sEnemyShot* next = p->next;
            DeleteShotInList(p);
            p = next;
        }
    }
}

// ============================================================
//  牛乳注ぎ：牛乳液＋波紋＋飛沫
// ============================================================
static void ShotMilkPour(sEnemyShotSet* pSet)
{
    const int WHITE = 6;
    const double FLOOR_Y = 400.0;     // 床の高さ
    const double MILK_SPEED = 4.0;    // 牛乳液の落下速度

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // --- 牛乳液（中玉・白）を細い線状に落とす ---
    if (pSet->count % 5 == 0) {
        double jitter = (GetRand(10) - 5) * 0.4; // ±2px の揺らぎ
        InsertShot(pSet, pSet->x + jitter, pSet->y,
            DX_PI / 2.0, MILK_SPEED,
            img_enemyShotMediumBall[WHITE], 0);
    }

    // --- 床の波紋（小玉リング・白） ---
    //   牛乳液が床に届き始めてから周期的に発生
    if (pSet->count >= 80 && pSet->count % 40 == 0) {
        for (int i = 0; i < 12; i++) {
            InsertShot(pSet, pSet->x, FLOOR_Y,
                i * (2.0 * DX_PI / 12.0),
                2.0, img_enemyShotSmallBall[WHITE], 1);
        }
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // --- 跳ねる飛沫（自機狙い3-way・小玉・白） ---
    if (pSet->count >= 100 && pSet->count % 50 == 0) {
        double baseAngle = atan2(player.y - FLOOR_Y, player.x - pSet->x);
        for (int i = -1; i <= 1; i++) {
            InsertShot(pSet, pSet->x, FLOOR_Y,
                baseAngle + i * (12.0 / 180.0 * DX_PI),
                3.0, img_enemyShotSmallBall[WHITE], 2);
        }
    }

    // --- 弾の更新（位置・床到達判定・減速・寿命消去） ---
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;

        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);

        if (p->param_i[0] == 0) {
            // 牛乳液：床に到達したら消去（波紋と飛沫で表現）
            if (p->y >= FLOOR_Y) {
                DeleteShotInList(p);
            }
        }
        else if (p->param_i[0] == 1) {
            // 波紋：減速しきったら消去
            p->speed *= 0.98;
            if (p->speed < 0.3) DeleteShotInList(p);
        }
        else if (p->param_i[0] == 2) {
            // 飛沫：緩やかに減速し、十分遅くなったら消去
            p->speed *= 0.99;
            if (p->speed < 0.5) DeleteShotInList(p);
        }

        p = next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Milkmaid_DeepSeek()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しない
    }

    // 開始直後にシルエットと牛乳注ぎを1セットずつ生成
    if (count == 30) {
        // シルエット：ミルクメイド本体（左上寄りに構図）
        CreateSet(enemy.x - 30.0, enemy.y, 0.0, ShotMaidSilhouette, 0);
        // 牛乳注ぎ：壺の口付近（シルエットの壺位置に合わせる）
        CreateSet(enemy.x + 20.0, enemy.y + 10.0, 0.0, ShotMilkPour, 0);
    }
}