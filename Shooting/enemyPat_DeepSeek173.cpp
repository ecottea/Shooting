#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 五線譜弾幕「流譜・プレスト」
//   五線を速く流し、単音符・和音（3音同時）・小節線の
//   3種を組み合わせて密度とタイミングを強化した版。
// ============================================================

// リンクリスト末尾へ弾を追加
static void AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static sEnemyShotSet* CreateSet(sEnemyShotSet::PatternFunc func,
    double x, double y, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->kind = kind;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

static void MoveShots(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 五線：速度を上げ、小節の切れ目を短く設定
// ------------------------------------------------------------
static void ShotStaffLine(sEnemyShotSet* pEnemyShotSet)
{
    int phase = pEnemyShotSet->count % 120;
    if (phase < 100 && (pEnemyShotSet->count % 2) == 0) {
        AddShot(pEnemyShotSet, 490.0, pEnemyShotSet->y,
            DX_PI, 2.5, img_enemyShotSmallBall[3]); // シアン
    }
    MoveShots(pEnemyShotSet);
}

// ------------------------------------------------------------
// 音符の形を組み立てる（符頭に param_i[0]=1 でマーク）
// ------------------------------------------------------------
static void BuildNote(sEnemyShotSet* pSet, double bx, double by, int colorIdx)
{
    AddShot(pSet, bx, by, DX_PI, 2.5, img_enemyShotMediumBall[colorIdx]);
    pSet->pEnemyShotHead->prev->param_i[0] = 1; // 符頭

    for (int i = 1; i <= 5; i++) {
        AddShot(pSet, bx + 7.0, by - i * 4.0,
            DX_PI, 2.5, img_enemyShotSmallBall[colorIdx]); // 茎
    }
    for (int i = 1; i <= 3; i++) {
        AddShot(pSet, bx + 7.0 + i * 3.0, by - 20.0 + i * 3.0,
            DX_PI, 2.5, img_enemyShotSmallBall[colorIdx]); // 旗
    }
}

static void FindHead(sEnemyShotSet* pSet, double* hx, double* hy)
{
    *hx = pSet->x; *hy = pSet->y;
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 1) { *hx = p->x; *hy = p->y; return; }
        p = p->next;
    }
}

// ------------------------------------------------------------
// 単音符：5-way 自機狙い
// ------------------------------------------------------------
static void ShotNote(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        BuildNote(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, 8); // 橙
    }
    if (pEnemyShotSet->count == 35) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double hx, hy;
        FindHead(pEnemyShotSet, &hx, &hy);
        double base = atan2(player.y - hy, player.x - hx);
        for (int k = -2; k <= 2; k++) {
            AddShot(pEnemyShotSet, hx, hy,
                base + k * 12.0 / 180.0 * DX_PI, 3.5,
                img_enemyShotBullet[5]); // マゼンタ
        }
    }
    MoveShots(pEnemyShotSet);
}

// ------------------------------------------------------------
// 和音音符：広角5-way（壁を作る）
// ------------------------------------------------------------
static void ShotChordNote(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        BuildNote(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, 5); // マゼンタ
    }
    if (pEnemyShotSet->count == 35) {
        if (pEnemyShotSet->kind == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        double hx, hy;
        FindHead(pEnemyShotSet, &hx, &hy);
        double base = atan2(player.y - hy, player.x - hx);
        for (int k = -2; k <= 2; k++) {
            AddShot(pEnemyShotSet, hx, hy,
                base + k * 22.0 / 180.0 * DX_PI, 3.0,
                img_enemyShotBullet[5]);
        }
    }
    MoveShots(pEnemyShotSet);
}

// ------------------------------------------------------------
// 小節線：隙間付きの縦一列（白の小弾）
//   param_i[1] に隙間中心の y を格納
// ------------------------------------------------------------
static void ShotBarLine(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int gapY = pEnemyShotSet->param_i[1];
        int gapHalf = 40;
        for (int y = 5; y <= 475; y += 15) {
            if (y >= gapY - gapHalf && y <= gapY + gapHalf) continue;
            AddShot(pEnemyShotSet, pEnemyShotSet->x, (double)y,
                DX_PI, 2.5, img_enemyShotSmallBall[6]); // 白
        }
    }
    MoveShots(pEnemyShotSet);
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_StaffNotation_DeepSeek()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // 五線 5本
        for (int i = 0; i < 5; i++) {
            CreateSet(ShotStaffLine, 480.0, 200.0 + i * 20.0, i);
        }
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 単音符：高頻度でランダムな線の上に出現
    if (count % 25 == 1) {
        double noteY = 200.0 + GetRand(4) * 20.0;
        CreateSet(ShotNote, 490.0, noteY, 0);
    }

    // 和音：3音を線0・2・4に同時出現（広角壁）
    if (count % 100 == 50) {
        CreateSet(ShotChordNote, 490.0, 200.0, 0);
        CreateSet(ShotChordNote, 490.0, 240.0, 1);
        CreateSet(ShotChordNote, 490.0, 280.0, 2);
    }

    // 小節線：隙間付きの縦列を周期的に流す
    if (count % 200 == 100) {
        int gapY = 50 + GetRand(7) * 55; // 50〜435 の8通り
        sEnemyShotSet* pSet = CreateSet(ShotBarLine, 490.0, 0.0, 0);
        pSet->param_i[1] = gapY;
    }
}