// enemyPat_washingMachine.cpp
//
// 洗濯機をモチーフにした弾幕パターン
//
// ゲーム画面: 480 x 480  /  1 サイクル = 720 フレーム（@60fps ≈ 12 秒）でループ
//
// ┌──────────────────────────────────────────────────────────────┐
// │ フェーズ1「洗い」  phase 0〜299  (300F / 5 秒)                   │
// │   8 方向シアンリング弾を 30F ごとに発射。                           │
// │   プレイヤー方向を基準に毎回 π/8 ずつ回転 → 螺旋状弾幕。           │
// │   敵は sin 波で左右に揺れる（ドラム回転の振動）。                   │
// ├──────────────────────────────────────────────────────────────┤
// │ フェーズ2「すすぎ」 phase 300〜539  (240F / 4 秒)                  │
// │   12 方向泡弾（シアン / 白混在）を 20F ごとに発射。                 │
// │   速度にランダム幅を持たせ「泡が弾ける」感を演出。                   │
// ├──────────────────────────────────────────────────────────────┤
// │ フェーズ3「脱水」  phase 540〜719  (180F / 3 秒)                   │
// │   3 本腕の高速螺旋弾を 5F ごとに発射（1 秒 1 回転）。               │
// │   敵本体が時間経過とともに激しく振動（遠心力で暴れる洗濯機）。       │
// └──────────────────────────────────────────────────────────────┘
//
// ── 素材一覧（enemyPat_sampleForAI.cpp より） ─────────────────────
//   弾種: img_enemyShotSmallBall[c]   小玉
//         img_enemyShotMediumBall[c]  中玉
//         img_enemyShotLargeBall[c]   大玉
//         img_enemyShotBullet[c]      銃弾
//         img_enemyShotScale[c]       鱗弾
//         img_enemyShotDiamond[c]     菱形弾
//         c: 0赤 1黄 2緑 3シアン 4青 5マゼンタ 6白 7黒
//   SE:   sound_enemyShot_light / medium / heavy / extreme
//   関数: GetRand(x) → [0, x] の整数を一様乱数で返す
//         atan2 / cos / sin / DX_PI

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕パターン関数
// ============================================================

// ── フェーズ1: 洗い ─────────────────────────────────────────
// 8 方向シアン中玉リング
// pEnemyShotSet->muki = プレイヤー方向 + 累積回転角
//                       （EnemyPat_WashingMachine_Claude 側で毎スポーン π/8 ずつ増加）
static void ShotWashRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        const int N = 32;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (2.0 * DX_PI / N) * i;
            pEnemyShot->speed = 2.8;
            pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン（水色）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ── フェーズ2: すすぎ ───────────────────────────────────────
// 12 方向泡バースト（シアン / 白ランダム混在・速度ばらつきあり）
// pEnemyShotSet->muki = バースト基準角（ゆっくり回転）
static void ShotRinseBubble(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (2.0 * DX_PI / 12) * i;
            // 速度を 0.90〜1.80 でランダムにばらけさせ「泡が弾ける」感を出す
            pEnemyShot->speed = 0.9 + GetRand(6) * 0.15;
            // シアンと白をランダムに混ぜて石鹸泡を表現
            pEnemyShot->kind = (GetRand(1) == 0)
                ? img_enemyShotSmallBall[3]   // シアン
                : img_enemyShotSmallBall[6];  // 白

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ── フェーズ3: 脱水 ─────────────────────────────────────────
// 3 本腕の高速螺旋弾（青鱗弾）
// pEnemyShotSet->muki = 螺旋の現在角（EnemyPat_WashingMachine_Claude 側で毎スポーン π/6 増加）
// 音は EnemyPat_WashingMachine_Claude 側で管理するため、ここでは鳴らさない
static void ShotSpinDry(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        for (int j = 0; j < 5; j++) {
            for (int i = 0; i < 3; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->muki = pEnemyShotSet->muki + (2.0 * DX_PI / 3) * i + j * 0.31415;
                const double R = 100.0;
                pEnemyShot->x = pEnemyShotSet->x + R * cos(pEnemyShot->muki);
                pEnemyShot->y = pEnemyShotSet->y + R * sin(pEnemyShot->muki);
                pEnemyShot->speed = 4.0 + GetRand(1) * 0.5 + 10.0; // 4.0 or 4.5（わずかなばらつき）
                pEnemyShot->kind = img_enemyShotScale[4];   // 青の鱗弾

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  ヘルパー: sEnemyShotSet を生成して enemyShotSetHead に追加
// ============================================================
static void SpawnShotSet(double x, double y, double muki,
    void (*func)(sEnemyShotSet*))
{
    sEnemyShotSet* pSS = new sEnemyShotSet;
    pSS->count = 0;
    pSS->x = x;
    pSS->y = y;
    pSS->muki = muki;
    pSS->patternFunc = func;

    pSS->pEnemyShotHead = new sEnemyShot;
    pSS->pEnemyShotHead->prev = pSS->pEnemyShotHead;
    pSS->pEnemyShotHead->next = pSS->pEnemyShotHead;

    pSS->prev = enemyShotSetHead.prev;
    pSS->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSS;
    enemyShotSetHead.prev = pSS;
}

// ============================================================
//  敵本体パターン: 洗濯機
// ============================================================
void EnemyPat_WashingMachine_Claude()
{
    static double ringAngle;   // 洗いフェーズ: リング累積回転角
    static double spiralAngle; // 脱水フェーズ: 螺旋累積回転角

    // ── 初期化（count は 1 始まり）──────────────────────────
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 300;
        ringAngle = 0.0;
        spiralAngle = 0.0;
    }

    // ── フェーズ判定（720 フレームでループ）─────────────────
    const int phase = (count - 1) % 720;

    // ── 敵の動き ────────────────────────────────────────────
    if (phase < 540) {
        // フェーズ1 / 2: sin 波で左右に緩やかに揺れる（ドラム回転の振動）
        // 周期 180F（3 秒）、振幅 ±60px
        enemy.x = 240.0 + 60.0 * sin((double)(count - 1) * DX_PI / 90.0);
        enemy.y = 60.0;
    }
    else {
        // フェーズ3: 脱水の激しい振動
        // (phase - 540) / 6 で 0〜29 と徐々に増加し、上限 ±20px でクランプ
        int vib = (phase - 540) / 6;
        if (vib > 20) vib = 20;
        enemy.x = 240.0 + (double)(GetRand(vib * 2) - vib); // ±vib px でランダム振動
        enemy.y = 60.0 + (double)(GetRand(4) - 2);
    }

    // ── 弾幕発射 ────────────────────────────────────────────
    if (phase < 300) {
        // ── フェーズ1: 洗い ── 8 方向リングを 30F ごとに発射 ──
        // プレイヤー方向を基準にしつつ π/8 ずつ回転させ螺旋を形成。
        // 10 回発射で合計 225° 回転（多様な方向をカバー）。
        if (phase % 30 == 0) {
            double toPlayer = atan2(player.y - enemy.y, player.x - enemy.x);
            SpawnShotSet(enemy.x, enemy.y + 10.0,
                GetRand(359) / 360.0 * DX_PI, ShotWashRing);
            ringAngle += DX_PI / 8.0;
        }

    }
    else if (phase < 540) {
        // ── フェーズ2: すすぎ ── 泡バーストを 20F ごとに発射 ──
        // angle をゆっくり回転させてリングが単調にならないようにする。
        // (phase - 300) が 0〜239 → angle が 0〜2π に満たない程度に増加
        if ((phase - 300) % 20 == 0) {
            double angle = (double)(phase - 300) * DX_PI / 120.0;
            SpawnShotSet(enemy.x, enemy.y + 10.0,
                angle, ShotRinseBubble);
        }

        if (phase == 540 - 60) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
    else {
        // ── フェーズ3: 脱水 ── 3 本腕螺旋弾を 5F ごとに発射 ──
        // 角度を π/6 ずつ増やす → 1 回転 = 12 スポーン = 60F = 1 秒

        // 脱水突入の瞬間に大きな効果音を 1 回だけ鳴らす
        if (phase == 540) {
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        if ((phase - 540) % 1 == 0) {
            SpawnShotSet(enemy.x, enemy.y + 10.0,
                spiralAngle, ShotSpinDry);
            spiralAngle += DX_PI / 6.0 + 0.1;
        }

        // 30F ごとに発射音（phase == 540 は extreme で代用するためスキップ）
        if (phase > 540 && (phase - 540) % 30 == 0) {
            //PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
}