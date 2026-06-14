// enemyPat_blizzard.cpp
// 吹雪をモチーフにした弾幕パターン「白銀の嵐」
//
// 弾幕A ShotSnowCrystal  : 雪の結晶（6本腕×2リング、右横風ドリフト）
// 弾幕B ShotBlizzardStream: 吹雪の流れ（扇状3波、強横風）
// 弾幕C ShotFrostSpiral  : 霜の渦（3腕らせん・加速弾）
//
// フェーズ0 300f: 穏やかな横揺れ → 結晶を20fごとに放つ
// フェーズ1 300f: 速い往復      → 吹雪の流れを15fごとに放つ
// フェーズ2 300f: 素早い横断    → 霜の渦を90fごとに起動
// → フェーズはループ
//
// 注意: グローバル count をそのまま sin の引数に使うと
//       フェーズ切替時に敵がワープするため、localTime で管理。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕A: 雪の結晶 (ShotSnowCrystal)
//
//  発射時:
//    外リング … 菱形弾（白）×6  / 60度間隔 / speed 2.8
//    内リング … 小玉（シアン）×6 / 30度ずらし / speed 1.6
//  更新:
//    全弾に +0.35 px/f の右横風を加える
// ============================================================
static void ShotSnowCrystal(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 外リング: 菱形弾（白）×6 ― 結晶の主軸
        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * (DX_PI / 3.0);
            pEnemyShot->speed = 2.8;
            pEnemyShot->count = 0;
            pEnemyShot->kind = img_enemyShotDiamond[6]; // 白

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 内リング: 小玉（シアン）×6 ― 結晶の枝（30度ずらし・低速）
        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (i + 0.5) * (DX_PI / 3.0);
            pEnemyShot->speed = 1.6;
            pEnemyShot->count = 0;
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム更新: 右方向の横風で徐々に流れる
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->count++;
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki) + 0.35; // 右横風
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  弾幕B: 吹雪の流れ (ShotBlizzardStream)
//
//  発射時 (count 0〜2 の3フレーム連射):
//    小玉（白・シアン交互）×7発 / 扇状 (±0.175π) / speed 2.0〜3.0
//    発射点に ±20px のランダムブレあり（猛吹雪らしい乱れ）
//  更新:
//    全弾に +0.5 px/f の強い右横風を加える
// ============================================================
static void ShotBlizzardStream(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 3フレームにわたり毎フレーム7発発射
    if (pEnemyShotSet->count < 3) {
        if (pEnemyShotSet->count == 0) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        const int    num = 7;
        const double spread = DX_PI * 0.35; // 扇の広がり角 (±0.175π)

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            // GetRand(x) は 0〜x を返すので、中央寄せは -半分
            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(16) - 8;
            pEnemyShot->muki = pEnemyShotSet->muki
                + spread * (i / (double)(num - 1) - 0.5);
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0; // 2.0〜3.0
            pEnemyShot->count = 0;
            // 白・シアンを交互に配置
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotSmallBall[6]  // 白
                : img_enemyShotSmallBall[3]; // シアン

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム更新: 強い右横風
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->count++;
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki) + 0.5; // 強横風
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  弾幕C: 霜の渦 (ShotFrostSpiral)
//
//  発射時 (count 0〜59 の60フレーム持続):
//    鱗弾（青・シアン・白）×3方向 / 120度間隔
//    0.06 rad/f で回転させてらせん軌跡を作る
//    初速 1.0 → 徐々に 3.5 まで加速（氷の刃が勢いをつける）
// ============================================================
static void ShotFrostSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 60フレーム間、毎フレーム3方向に発射
    if (pEnemyShotSet->count < 60) {
        if (pEnemyShotSet->count == 0) {
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        const int numArms = 3;
        const int armColors[3] = { 4, 3, 6 }; // 青, シアン, 白

        for (int i = 0; i < numArms; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 各腕は120度間隔、フレームごとに0.06rad回転してらせんを形成
            pEnemyShot->muki = pEnemyShotSet->muki
                + i * (2.0 * DX_PI / numArms)
                + pEnemyShotSet->count * 0.06;
            pEnemyShot->speed = 1.0; // 初速は遅め（後から加速）
            pEnemyShot->count = 0;
            pEnemyShot->kind = img_enemyShotScale[armColors[i]];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム更新: 速度上限3.5まで0.05ずつ加速
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->count++;
        if (pEnemyShot->speed < 3.5) {
            pEnemyShot->speed += 0.05;
        }
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体: 吹雪の精霊 (EnemyPat_Blizzard_Claude)
//
//  ゲーム画面は 480×480。
//  フェーズ内時間 localTime (1〜300) を使い、
//  グローバル count をそのまま sin に渡さない設計にすることで
//  フェーズ切替時の位置ワープを防ぐ。
// ============================================================
void EnemyPat_Blizzard_Claude()
{
    static int localTime;    // フェーズ内タイマー (1〜PHASE_LEN)
    static int phase;        // 現在フェーズ (0〜2)
    static int crystalAngle; // 結晶の角度交互フラグ (0 or 1)

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        localTime = 0;
        phase = 0;
        crystalAngle = 0;
    }

    localTime++;

    const int PHASE_LEN = 300; // 1フェーズの長さ（フレーム）

    // フェーズ切替
    if (localTime > PHASE_LEN) {
        localTime = 1;
        phase = (phase + 1) % 3;
    }

    // t: フェーズ内の進行を 0〜π に正規化
    // ※ localTime=1 で必ず DX_PI/PHASE_LEN から始まるためワープなし
    const double t = (double)localTime / PHASE_LEN * DX_PI;

    // ――― 敵の移動 ―――
    switch (phase) {
    case 0: // 穏やかな横揺れ（結晶フェーズ）
        enemy.x = 240.0 + 160.0 * sin(t * 2.0);
        enemy.y = 60.0 + 20.0 * sin(t);
        break;
    case 1: // 高い位置で速く往復しながら徐々に降りる（吹雪フェーズ）
        enemy.x = 240.0 + 130.0 * cos(t * 3.0);
        enemy.y = 50.0 + 45.0 * sin(t);
        break;
    case 2: // 素早い横断＋縦の細かい揺れ（渦フェーズ）
        enemy.x = 240.0 + 180.0 * sin(t);
        enemy.y = 65.0 + 25.0 * sin(t * 5.0);
        break;
    }

    // ――― 弾幕発射 ―――
    sEnemyShotSet* pEnemyShotSet;

    switch (phase) {

        // フェーズ0: 雪の結晶を20fごとに放つ
        // 0度 / 30度 を交互に使い、発射のたびに結晶の向きが変わる
    case 0:
        if (localTime % 20 == 1) {
            pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSnowCrystal;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->muki = crystalAngle * (DX_PI / 6.0); // 0 or 30度
            crystalAngle = 1 - crystalAngle;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        break;

        // フェーズ1: 吹雪の流れを15fごとに放つ（プレイヤーへ自機狙い）
    case 1:
        if (localTime % 15 == 1) {
            pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotBlizzardStream;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->muki = atan2(player.y - (enemy.y + 10.0),
                player.x - enemy.x);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        break;

        // フェーズ2: 霜の渦を90fごとに起動（開始角度はランダム）
        // 1フェーズ内に4回起動し、渦が重なり合う複雑な弾幕になる
    case 2:
        if (localTime % 90 == 1) {
            pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotFrostSpiral;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            // GetRand(62) は 0〜62 → /10.0 で 0.0〜6.2 rad (ほぼ全周)
            pEnemyShotSet->muki = GetRand(62) / 10.0;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        break;
    }
}
