// enemyPat_tmp.cpp
//
// 砂時計モチーフ弾幕パターン
//
//  ■ パターンA: ShotHourglassSand（砂の流れ）
//      発射角度の扇幅が「広い→首（細）→広い」と変化し、
//      砂時計を流れ落ちる砂を黄・橙の小球で表現する。
//
//  ■ パターンB: ShotHourglassEdge（砂時計の輪郭）
//      左縁・右縁の2弾流がX座標を cos 曲線で変化させながら
//      垂直落下する。スナップショット上では
//        上端（広い）→ 首（狭い）→ 下端（広い）
//      という砂時計のシルエットが浮かび上がる。
//
// ゲーム画面: 480×480  /  弾速の単位: px/frame
//

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>


// ----------------------------------------------------------------
// ヘルパー: ショットをセットの双方向リストに追加
// ----------------------------------------------------------------
static void AddShot(sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}


// ----------------------------------------------------------------
// ヘルパー: ショットセットを生成してリストに追加
// ----------------------------------------------------------------
static void SpawnShotSet(sEnemyShotSet::PatternFunc func,
    double x, double y, double muki)
{
    sEnemyShotSet* s = new sEnemyShotSet;
    s->count = 0;
    s->patternFunc = func;
    s->x = x;
    s->y = y;
    s->muki = muki;

    s->pEnemyShotHead = new sEnemyShot;
    s->pEnemyShotHead->prev = s->pEnemyShotHead;
    s->pEnemyShotHead->next = s->pEnemyShotHead;

    s->prev = enemyShotSetHead.prev;
    s->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = s;
    enemyShotSetHead.prev = s;
}


// ================================================================
//  【弾幕A】 砂の流れ  ShotHourglassSand
//
//  フェーズ              フレーム    扇幅の変化
//  ─────────────────────────────────────────────────────
//  上部砂室              [0,  56)   68° → 0°  (t² で急速に絞る)
//  首                    [56, 84)   ≈ 3° 固定  (ほぼ真下)
//  下部砂室              [84,140)   0° → 68°  (t² でゆっくり広げる)
//
//  各フェーズとも 4フレームおきに弾を生成。
//  扇の発射数も幅に連動して 9→2（首）→9 と変化する。
// ================================================================
static void ShotHourglassSand(sEnemyShotSet* pEnemyShotSet)
{
    const int TOP = 56;
    const int NECK = 84;
    const int END = 140;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 4フレームおきに弾を生成（END フレームで打ち止め）
    if (pEnemyShotSet->count < END && pEnemyShotSet->count % 4 == 0) {

        const double BASE = DX_PI / 2.0;  // 真下
        double spread;
        int    n;
        int    col;

        if (pEnemyShotSet->count < TOP) {
            // 上部砂室: t² で扇が急速に絞られる（砂が首へ集まる）
            double t = (double)pEnemyShotSet->count / TOP;        // 0 → 1
            spread = (1.0 - t * t) * 68.0 / 180.0 * DX_PI;    // 68° → 0°
            n = 2 + (int)((1.0 - t) * 7.9);               // 9 → 2
            col = 1;  // 黄
        }
        else if (pEnemyShotSet->count < NECK) {
            // 首: ほぼ真下に収束
            spread = 3.0 / 180.0 * DX_PI;
            n = 3;
            col = 8;  // 橙
        }
        else {
            // 下部砂室: t² で扇がゆっくり広がる（砂が下室へ流れ込む）
            double t = (double)(pEnemyShotSet->count - NECK) / (END - NECK);
            spread = t * t * 68.0 / 180.0 * DX_PI;             // 0° → 68°
            n = 2 + (int)(t * 7.9);                        // 2 → 9
            col = 1;  // 黄
        }

        for (int i = 0; i < n; i++) {
            double angle = (n > 1)
                ? BASE - spread + 2.0 * spread * i / (n - 1)
                : BASE;
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                angle, 2.5, img_enemyShotSmallBall[col]);
        }
    }

    // 全弾を移動
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}


// ================================================================
//  【弾幕B】 砂時計の輪郭  ShotHourglassEdge
//
//  左縁・右縁の2弾流が x 座標を
//      w(t) = WMAX * |cos(π·t)|
//  に従って変化させながら真下へ落下する。
//
//  スナップショット（古い弾 = 画面下 / 新しい弾 = 画面上）:
//    t=0.0 (古)  … w=WMAX  (上端: 最も広い)   橙
//    t=0.5       … w=0     (首: 消滅点)         白
//    t=1.0 (新)  … w=WMAX  (下端: 最も広い)   黄
//  → 砂時計のシルエットが浮かび上がる
// ================================================================
static void ShotHourglassEdge(sEnemyShotSet* pEnemyShotSet)
{
    const int    DUR = 120;
    const double WMAX = 150.0;  // 最大水平幅 (px)
    const double SPD = 2.5;
    const double DOWN = DX_PI / 2.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count < DUR && pEnemyShotSet->count % 3 == 0) {
        double t = (double)pEnemyShotSet->count / DUR;  // 0 → 1
        double w = WMAX * fabs(cos(DX_PI * t));
        int    col = (t < 0.5) ? 8 : 1;  // 上半: 橙, 下半: 黄

        if (w < 2.0) {
            // 首: 両縁が重なるため中央1発 (白) に置き換え
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                DOWN, SPD, img_enemyShotSmallBall[6]);
        }
        else {
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x - w, pEnemyShotSet->y,
                DOWN, SPD, img_enemyShotSmallBall[col]);  // 左縁
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + w, pEnemyShotSet->y,
                DOWN, SPD, img_enemyShotSmallBall[col]);  // 右縁
        }
    }

    // 全弾を移動
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}


// ================================================================
//  敵本体パターン
// ================================================================
void EnemyPat_Hourglass_Claude()
{
    // ── 初期化 ─────────────────────────────────────────────
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 画面上部を sin 曲線でゆっくり左右に往復（振幅 ±90px、周期 360f）
        enemy.x = 240.0 + 90.0 * sin(count * DX_PI / 180.0);
        enemy.y = 55.0;
    }

    const double ex = enemy.x;
    const double ey = enemy.y + 10.0;
    const double dn = DX_PI / 2.0;

    // ── 弾幕A: 砂の流れ（180フレームおきに発射）────────────
    if (count % 180 == 10)
        SpawnShotSet(ShotHourglassSand, ex, ey, dn);

    // ── 弾幕B: 砂時計の輪郭（弾幕Aと90フレームずれで発射）──
    if (count % 180 == 100)
        SpawnShotSet(ShotHourglassEdge, ex, ey, dn);
}