// enemyPat_kinkakuji.cpp
// 新難題「金閣寺の一枚天井」（蓬莱山輝夜）
// 東方文花帖 Level 9 - Scene 6

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ==============================================================
// 天井弾幕：横一列の黄色大玉
//
// 画面幅の約半分（≈224px）に9発を等間隔で展開し、真下へ落下させる。
// kind : 左右交互インデックス（偶数→左半分中心、奇数→右半分中心）
// ==============================================================
static void ShotCeilingWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int    NUM = 9;
        const double STEP = 28.0; // 弾間隔：9発×28px = 224px（画面幅480の約半分）
        // 左側の列：x=120 を中心（x=8〜232）
        // 右側の列：x=360 を中心（x=248〜472）
        double cx = (pEnemyShotSet->kind % 2 == 0) ? 80.0 : 400.0; // ※ナーフ

        for (int i = 0; i < NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + (i - (NUM - 1) * 0.5) * STEP;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI * 0.5;              // 真下
            pEnemyShot->speed = 1.7;
            pEnemyShot->kind = img_enemyShotLargeBall[1]; // 黄色大玉

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

// ==============================================================
// バラ撒き弾幕：4方向回転
//
// 基準角を中心に4方向（90°間隔）それぞれへ、ランダム拡散付きで発射。
// 毎フレーム基準角を変化させることで弾幕全体が回転する。
//
// kind      : 弾色インデックス（2=緑、3=シアン、4=青）
// param_d[0]: 発射基準角（ラジアン）
// param_d[1]: 弾速
// ==============================================================
static void ShotRotatingScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 同フレームに複数色のセットが生成されるため、
        // 緑弾セット（kind==2）のみ発射音を鳴らして重複再生を防ぐ。
        if (pEnemyShotSet->kind == 2)
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int    BULLETS_PER_DIR = 3;    // 1方向あたりの弾数
        const double SPREAD = 0.35; // 方向あたりの広がり角（ラジアン）
        double baseAngle = pEnemyShotSet->param_d[0];
        double spd = pEnemyShotSet->param_d[1];

        for (int dir = 0; dir < 4; dir++) {
            double dirAngle = baseAngle + dir * (DX_PI * 0.5);
            for (int j = 0; j < BULLETS_PER_DIR; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // GetRand(200) は 0〜200 → (0〜200)-100 = -100〜100
                pEnemyShot->muki = dirAngle + (GetRand(200) - 100) / 100.0 * SPREAD;
                // GetRand(40) は 0〜40 → 速度に ±0.4 の幅
                pEnemyShot->speed = spd + GetRand(40) / 100.0;
                pEnemyShot->kind = img_enemyShotMediumBall[pEnemyShotSet->kind];

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

// ==============================================================
// ヘルパー：sEnemyShotSet を生成してグローバルリストに末尾挿入
// ==============================================================
static sEnemyShotSet* SpawnShotSet(void(*func)(sEnemyShotSet*),
    double x, double y, int kind)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = x;
    p->y = y;
    p->kind = kind;

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;

    return p;
}

// ==============================================================
// 敵本体パターン：新難題「金閣寺の一枚天井」（蓬莱山輝夜）
//
// ◆ 弾幕構成
//   [天井弾] 28フレームに1回、横一列の黄色大玉を左→右→左…と交互に発射。
//            各列は画面幅の約半分を覆い、真下へゆっくり落下する。
//
//   [バラ撒き] 7フレームに1回、4方向へランダム拡散弾を発射。
//              基準角を毎回変化させることで弾幕全体が回転して見える。
//
// ◆ フェーズ移行（hp=200 基準）
//   Phase 1: hp > 114  → 緑弾のみ（時計回り）
//   Phase 2: hp > 57   → 緑＋シアン弾追加（反時計回り）
//   Phase 3: hp ≤ 57   → さらに青弾追加（時計回り・やや遅め）
// ==============================================================
void EnemyPat_Kinkakuji_Claude()
{
    static int    muki;         // 左右移動方向（+1 or -1）
    static int    wall_idx;     // 天井弾幕の左右交互インデックス
    static double green_angle;  // 緑バラ撒き基準角（時計回り → 増加）
    static double cyan_angle;   // シアンバラ撒き基準角（反時計回り → 減少）
    static double blue_angle;   // 青バラ撒き基準角（時計回り・緑より遅め）

    // ---- 初期化 ----
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        wall_idx = 0;
        green_angle = 0.0;
        cyan_angle = 0.0;
        blue_angle = 0.0;
    }
    else {
        // 画面上部をゆっくり左右往復
        enemy.x += 0.6 * (double)muki;
        if (enemy.x > 380.0) muki = -1;
        if (enemy.x < 100.0) muki = 1;
    }

    // ---- フェーズ判定 ----
    // 原作の「残り7枚→4枚→2枚」を hp=200 基準で再現
    const int HP_PH2 = 200 * 4 / 7; // ≈ 114
    const int HP_PH3 = 200 * 2 / 7; // ≈  57
    int phase = (enemy.hp <= HP_PH3) ? 3
        : (enemy.hp <= HP_PH2) ? 2
        : 1;

    // ---- 天井弾幕（横一列黄色大玉）: 28フレームに1回 ----
    if (count % 28 == 0) {
        SpawnShotSet(ShotCeilingWall, enemy.x, enemy.y + 10.0, wall_idx++);
    }

    // ---- バラ撒き弾幕（4方向回転）: 7フレームに1回 ----
    if (count % 10 == 0) { // ※ナーフ

        // [Phase 1〜3共通] 緑弾：時計回り（angle 増加）
        green_angle += 0.04;
        {
            sEnemyShotSet* p = SpawnShotSet(ShotRotatingScatter,
                enemy.x, enemy.y + 10.0, 2); // 2=緑
            p->param_d[0] = green_angle;
            p->param_d[1] = 2.2;
        }

        // [Phase 2〜3] シアン弾：反時計回り（angle 減少）
        if (phase >= 2) {
            cyan_angle -= 0.033;
            sEnemyShotSet* p = SpawnShotSet(ShotRotatingScatter,
                enemy.x, enemy.y + 10.0, 3); // 3=シアン
            p->param_d[0] = cyan_angle;
            p->param_d[1] = 2.2;
        }

        // [Phase 3] 青弾：時計回り・緑より若干遅め
        if (phase >= 3) {
            blue_angle += 0.028;
            sEnemyShotSet* p = SpawnShotSet(ShotRotatingScatter,
                enemy.x, enemy.y + 10.0, 4); // 4=青
            p->param_d[0] = blue_angle;
            p->param_d[1] = 1.9;
        }
    }
}