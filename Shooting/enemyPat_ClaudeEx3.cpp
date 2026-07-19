// enemyPat_Tmp.cpp
//
// 達人王風弾幕ボス「星炎大王」
//
//  フェーズ1 (HP > 270): 回転リング連射 + 3-way 自機狙い扇
//  フェーズ2 (HP 136~270): 回転リング連射 + 5-way 自機狙い扇 + 2腕螺旋
//  フェーズ3 (HP <= 135): 回転リング連射 + 7-way 自機狙い扇 + 2腕螺旋 + 壁弾
//
// ゲーム画面: 480 x 480
// count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
// および画面外弾消去はメインルーチンで行われる。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ヘルパー: sEnemyShotSet を生成してリストに挿入
// ============================================================
static void SpawnShotSet(
    void (*patternFunc)(sEnemyShotSet*),
    double x, double y, double muki, int kind)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = patternFunc;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->kind = kind;

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
}

// ============================================================
//  弾幕1: 回転リング連射
//
//  12方向 × 4ボレー (5フレーム間隔)。
//  ボレーごとに 15° 回転して "花びら" 状の軌跡を描く。
//  弾は低速発射 → 30フレーム後から加速（達人王っぽい緩急）。
//  シアン中玉。
// ============================================================
static void ShotRotatingRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 5フレームごとに 1ボレー、計4回 (count 0, 5, 10, 15)
    if (pEnemyShotSet->count < 20 && pEnemyShotSet->count % 5 == 0) {
        int    volley = pEnemyShotSet->count / 5;
        double baseAngle = pEnemyShotSet->muki + volley * (DX_PI / 12.0); // 15°ずつ回転

        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + i * (2.0 * DX_PI / 12.0);
            pEnemyShot->speed = 1.5; // 低速スタート
            pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾移動: 30フレーム経過後から加速
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->count >= 30 && pEnemyShot->speed < 4.5) {
            pEnemyShot->speed += 0.08;
        }
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  弾幕2: 自機狙い N-way 扇形弾
//
//  kind に way 数を渡す。10° 間隔で扇状に発射。
//  高速赤銃弾。
// ============================================================
static void ShotAimedFan(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int    ways = (pEnemyShotSet->kind > 0) ? pEnemyShotSet->kind : 3;
        double step = DX_PI / 18.0; // 10°間隔
        double baseAngle = pEnemyShotSet->muki - (ways - 1) * step * 0.5;

        for (int i = 0; i < ways; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + i * step;
            pEnemyShot->speed = 4.5;
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤銃弾

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

// ============================================================
//  弾幕3: 2腕螺旋弾幕
//
//  90フレーム間、毎フレーム 2腕(180°対称)に各1発を発射。
//  1フレームごとに 12° 回転するため3周分の螺旋軌跡になる。
//  黄小玉。
// ============================================================
static void ShotSpiralArms(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count < 90) {
        if (pEnemyShotSet->count == 0)
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // pEnemyShotSet->count を基準にすることで
        // グローバル count の跳びによる位置ワープを回避する
        double angle = pEnemyShotSet->muki
            + pEnemyShotSet->count * (DX_PI / 15.0); // 12°/frame

        for (int arm = 0; arm < 2; arm++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle + arm * DX_PI; // 2腕 = 180°対称
            pEnemyShot->speed = 3.2;
            pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄小玉

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

// ============================================================
//  弾幕4: 壁弾（画面横一列 10発、ほぼ真下）
//
//  x を 0~480 に均等配置し、±10° の揺らぎと
//  2.0~3.0 のランダム速度で下方向に押し流す。
//  避けルートを塞ぐ達人王風カバー弾。
//  青大玉。
// ============================================================
static void ShotScatterWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 10; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 24.0 + i * 48.0; // 0~480 を 10 等分
            pEnemyShot->y = pEnemyShotSet->y;
            // ±10° の揺らぎ付き真下
            pEnemyShot->muki = DX_PI * 0.5
                + (GetRand(20) - 10) * DX_PI / 180.0;
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0; // 2.0~3.0
            pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青大玉

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

// ============================================================
//  敵本体: 星炎大王
//
//  HP 400。フェーズが進むにつれて移動速度・弾幕密度が上昇。
//  フェーズ3 では縦揺れを追加し、壁弾も解禁される。
// ============================================================
void EnemyPat_Tatsujinou_Claude()
{
    static double moveDir;
    static int    phase3EntryCount; // フェーズ3 突入時の count を記録

    // ---- 初期化 ----
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 300;
        moveDir = 1.0;
        phase3EntryCount = -1;
    }

    // ---- フェーズ判定 ----
    int phase;
    if (enemy.hp > 200) phase = 1;
    else if (enemy.hp > 100) phase = 2;
    else                     phase = 3;

    // フェーズ3 に突入した瞬間の count を保存
    if (phase == 3 && phase3EntryCount < 0)
        phase3EntryCount = count;

    // ---- 移動 ----
    //  左右振り子。フェーズが上がるほど速くなる。
    double moveSpd = (phase == 1) ? 1.5 : (phase == 2) ? 2.0 : 2.5;
    enemy.x += moveSpd * moveDir;
    if (enemy.x > 400.0 || enemy.x < 80.0) moveDir *= -1.0;

    //  フェーズ3 のみ縦揺れを追加。
    //  グローバル count を sin に直接使うと突入時に位置跳びが起きるため
    //  突入時からの相対フレーム数 localT を使用する。
    if (phase == 3) {
        int localT = count - phase3EntryCount;
        enemy.y = 60.0 + 20.0 * sin(localT * DX_PI / 90.0);
    }

    // ---- 自機狙い向き ----
    double shotX = enemy.x;
    double shotY = enemy.y + 15.0;
    double aimMuki = atan2(player.y - shotY, player.x - shotX);

    // ---- 弾幕スポーン ----

    if (phase == 1) {
        // 回転リング: 90f ごと
        if (count % 90 == 1)
            SpawnShotSet(ShotRotatingRing, enemy.x, enemy.y,
                GetRand(360) * DX_PI / 180.0, 0);
        // 3-way 自機狙い: 45f ごと (リングと 23f ずらす)
        if (count % 45 == 23)
            SpawnShotSet(ShotAimedFan, shotX, shotY, aimMuki, 3);
    }
    else if (phase == 2) {
        // 回転リング: 70f ごと (インターバル短縮)
        if (count % 70 == 1)
            SpawnShotSet(ShotRotatingRing, enemy.x, enemy.y,
                GetRand(360) * DX_PI / 180.0, 0);
        // 5-way 自機狙い: 35f ごと
        if (count % 35 == 15)
            SpawnShotSet(ShotAimedFan, shotX, shotY, aimMuki, 5);
        // 2腕螺旋: 150f ごと
        if (count % 150 == 75)
            SpawnShotSet(ShotSpiralArms, enemy.x, enemy.y,
                GetRand(360) * DX_PI / 180.0, 0);
    }
    else { // phase == 3
        // 回転リング: 50f ごと (さらに短縮)
        if (count % 50 == 1)
            SpawnShotSet(ShotRotatingRing, enemy.x, enemy.y,
                GetRand(360) * DX_PI / 180.0, 0);
        // 7-way 自機狙い: 25f ごと
        if (count % 25 == 10)
            SpawnShotSet(ShotAimedFan, shotX, shotY, aimMuki, 7);
        // 2腕螺旋: 120f ごと
        if (count % 120 == 60)
            SpawnShotSet(ShotSpiralArms, enemy.x, enemy.y,
                GetRand(360) * DX_PI / 180.0, 0);
        // 壁弾: 100f ごと (フェーズ3 限定)
        if (count % 100 == 80)
            SpawnShotSet(ShotScatterWall, shotX, shotY, 0.0, 0);
    }
}