#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：居合斬り・三日月
//  刀を既存弾の組み合わせで表現し、それを振り抜くことで
//  斬撃の軌跡と衝撃波を発生させるパターン。
// ============================================================

// ---- 弾を1つ追加 ----
static sEnemyShot* AddEnemyShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ---- 弾をリストから外して delete ----
static void RemoveEnemyShot(sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev->next = pEnemyShot->next;
    pEnemyShot->next->prev = pEnemyShot->prev;
    delete pEnemyShot;
}

// ---- 刀を組み立てる ----
//  param_i[0] = 1 : 刀弾
//  param_d[0] : 柄元からの距離（半径方向）
//  param_d[1] : 刀身に垂直なオフセット（反り）
static void BuildKatana(sEnemyShotSet* pEnemyShotSet)
{
    // 刀身：短レーザー（白）×3 を重ねて1本の刃に見せる
    const double bladeR[3] = { 24.0, 56.0, 88.0 };
    const double bladePerp[3] = { 0.0,  2.0,  4.0 };
    for (int i = 0; i < 3; i++) {
        sEnemyShot* p = AddEnemyShot(pEnemyShotSet);
        p->kind = img_enemyShotLaser[6]; // 白
        p->speed = 0.0;
        p->param_i[0] = 1;
        p->param_d[0] = bladeR[i];
        p->param_d[1] = bladePerp[i];
    }
    // 切っ先：小玉（白）
    {
        sEnemyShot* p = AddEnemyShot(pEnemyShotSet);
        p->kind = img_enemyShotSmallBall[6]; // 白
        p->speed = 0.0;
        p->param_i[0] = 1;
        p->param_d[0] = 124.0;
        p->param_d[1] = 6.0;
    }
    // 鍔：中玉（橙）
    {
        sEnemyShot* p = AddEnemyShot(pEnemyShotSet);
        p->kind = img_enemyShotMediumBall[8]; // 橙
        p->speed = 0.0;
        p->param_i[0] = 1;
        p->param_d[0] = 0.0;
        p->param_d[1] = 0.0;
    }
    // 柄：小玉（黄）×4
    const double handleR[4] = { -12.0, -22.0, -32.0, -42.0 };
    for (int i = 0; i < 4; i++) {
        sEnemyShot* p = AddEnemyShot(pEnemyShotSet);
        p->kind = img_enemyShotSmallBall[1]; // 黄
        p->speed = 0.0;
        p->param_i[0] = 1;
        p->param_d[0] = handleR[i];
        p->param_d[1] = 0.0;
    }
}

// ---- 斬撃の軌跡を弧状に配置 ----
static void PlaceTrail(sEnemyShotSet* pEnemyShotSet, double startDeg, double endDeg)
{
    const int    N = 7*2;
    const double R = 80.0;
    for (int i = 0; i < N; i++) {
        double t = (double)i / (double)(N - 1);
        double a = (startDeg + (endDeg - startDeg) * t) / 180.0 * DX_PI;
        sEnemyShot* p = AddEnemyShot(pEnemyShotSet);
        p->kind = img_enemyShotSmallBall[6]; // 白
        p->x = pEnemyShotSet->x + R * cos(a);
        p->y = pEnemyShotSet->y + R * sin(a);
        p->speed = 0.0;
        p->param_i[0] = 2;   // 軌跡弾
        p->param_i[1] = 30;  // 寿命（フレーム）
    }
}

// ---- 衝撃波を発生 ----
static void SpawnImpact(sEnemyShotSet* pEnemyShotSet)
{
    const double px = pEnemyShotSet->x;
    const double py = pEnemyShotSet->y;

    // 全方位の中弾（シアン）
    const int N = 8*2;
    for (int i = 0; i < N; i++) {
        double a = (i * 2.0 * DX_PI / N) + DX_PI / N;
        sEnemyShot* p = AddEnemyShot(pEnemyShotSet);
        p->kind = img_enemyShotMediumBall[3]; // シアン
        p->x = px;
        p->y = py;
        p->muki = a;
        p->speed = 2.5;
    }

    // 軌跡弾の位置を収集
    double tx[32], ty[32];
    int nTrail = 0;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead && nTrail < 32) {
        if (pShot->param_i[0] == 2) {
            tx[nTrail] = pShot->x;
            ty[nTrail] = pShot->y;
            nTrail++;
        }
        pShot = pShot->next;
    }

    // 軌跡から扇状に小弾（白）
    for (int i = 0; i < nTrail; i++) {
        double base = atan2(ty[i] - py, tx[i] - px);
        for (int k = -1; k <= 1; k++) {
            sEnemyShot* p = AddEnemyShot(pEnemyShotSet);
            p->kind = img_enemyShotSmallBall[6]; // 白
            p->x = tx[i];
            p->y = ty[i];
            p->muki = base + k * 25.0 / 180.0 * DX_PI;
            p->speed = 3.0;
        }
    }
}

// ---- 刀弾を全消去 ----
static void RemoveKatana(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        if (pShot->param_i[0] == 1) {
            RemoveEnemyShot(pShot);
        }
        pShot = pNext;
    }
}

// ---- 弾幕パターン本体 ----
static void ShotKatana(sEnemyShotSet* pEnemyShotSet)
{
    // 刀はボスに追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    const double PX = pEnemyShotSet->x;
    const double PY = pEnemyShotSet->y;
    const int    t = pEnemyShotSet->count;

    const double A = pEnemyShotSet->param_d[1]; // 開始角(度)
    const double B = pEnemyShotSet->param_d[2]; // 終了角(度)

    // タイムライン
    const int T_BUILD1 = 10;  // 刀生成（1本目）
    const int T_SLASH1S = 24;  // 斬撃1 開始
    const int T_SLASH1E = 30;  // 斬撃1 終了
    const int T_TRAIL1 = 31;  // 軌跡設置1
    const int T_IMPACT1 = 34;  // 衝撃波1
    const int T_BUILD2 = 42;  // 刀生成（2本目）
    const int T_SLASH2S = 56;  // 斬撃2 開始
    const int T_SLASH2E = 62;  // 斬撃2 終了
    const int T_TRAIL2 = 63;  // 軌跡設置2
    const int T_IMPACT2 = 66;  // 衝撃波2

    // ---- 現在の刃の角度を計算 ----
    double angleDeg = A;
    if (t >= T_SLASH1S && t <= T_SLASH1E) {
        double p = (double)(t - T_SLASH1S) / (double)(T_SLASH1E - T_SLASH1S);
        p = 1.0 - (1.0 - p) * (1.0 - p); // easeOutQuad
        angleDeg = A + (B - A) * p;
    }
    else if (t > T_SLASH1E && t < T_SLASH2S) {
        angleDeg = B;
    }
    else if (t >= T_SLASH2S && t <= T_SLASH2E) {
        double p = (double)(t - T_SLASH2S) / (double)(T_SLASH2E - T_SLASH2S);
        p = 1.0 - (1.0 - p) * (1.0 - p);
        angleDeg = B + (A - B) * p;
    }
    else if (t > T_SLASH2E) {
        angleDeg = A;
    }
    const double angleRad = angleDeg / 180.0 * DX_PI;

    // ---- イベント ----
    if (t == T_BUILD1 || t == T_BUILD2) {
        BuildKatana(pEnemyShotSet);
        //if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        //PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (t == T_SLASH1S || t == T_SLASH2S) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    if (t == T_TRAIL1) PlaceTrail(pEnemyShotSet, A, B);
    if (t == T_TRAIL2) PlaceTrail(pEnemyShotSet, B, A);
    if (t == T_IMPACT1 || t == T_IMPACT2) {
        RemoveKatana(pEnemyShotSet);
        SpawnImpact(pEnemyShotSet);
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // ---- 弾の更新 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 1) {
            // 刀弾：剛体回転（刃の角度に合わせて位置を再計算）
            double r = pShot->param_d[0];
            double perp = pShot->param_d[1];
            pShot->x = PX + r * cos(angleRad) + perp * (-sin(angleRad));
            pShot->y = PY + r * sin(angleRad) + perp * (cos(angleRad));
            pShot->muki = angleRad; // 画像の回転に使用
        }
        else if (pShot->param_i[0] == 2) {
            // 軌跡弾：寿命で消える（画面外に出ないため自前で delete）
            if (pShot->count > pShot->param_i[1]) {
                RemoveEnemyShot(pShot);
            }
        }
        else {
            // 通常弾（衝撃波）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Sword_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.8 * (double)muki;
        if (enemy.x > 360.0) muki = -1;
        if (enemy.x < 120.0) muki = 1;
    }

    // 90フレーム周期で居合斬りを放つ
    if (count >= 60 && count % 90 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKatana;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        // 斬撃の向きを交互に切り替える
        if (shot_count % 2 == 1) {
            pEnemyShotSet->param_d[1] = 30.0;   // 右下から
            pEnemyShotSet->param_d[2] = 150.0;  // 左上へ
        }
        else {
            pEnemyShotSet->param_d[1] = 150.0;  // 左下から
            pEnemyShotSet->param_d[2] = 30.0;   // 右上へ
        }

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}