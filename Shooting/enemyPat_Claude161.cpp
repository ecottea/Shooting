#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  銃符「ルナティックガン」
//
//  A: ボス中心に、間隔が狭い箇所と広い箇所が交互に並ぶ10way全方位弾を、
//     時計回りに回転させながら常時撃ち続ける。
//  B: ボスから自機に向かって荒く狙い、1辺8発の逆三角形(頂点が自機側)の
//     密な弾列を5つ、僅かに時間差を付けて撃つ(=1セット)。
//     1セット撃ったら間髪空けずにもう1セット撃ち、少し間を空ける、を繰り返す。
//
//  使用素材
//    A: 中玉(青) / 効果音 sound_enemyShot_light
//    B: 銃弾(赤) / 効果音 sound_enemyShot_medium
//       (銃弾は向きに沿って描画されるので、muki=進行方向 がそのまま見た目に合う)
// ============================================================

// ---------- 共通 ----------
static const double LUNAGUN_ENEMY_X = 240.0;   // ボス位置(画面上部中央で固定)
static const double LUNAGUN_ENEMY_Y = 70.0;

// ---------- A: 回転する狭広10way ----------
static const int    LUNAGUN_A_INTERVAL = 5;     // 発射間隔(フレーム)
static const int    LUNAGUN_A_WAY = 10;    // way数(2発1組なので偶数)
static const double LUNAGUN_A_SPEED = 2.5;   // 弾速
static const double LUNAGUN_A_ROTATE_DEG = 4.5;   // 1回の発射ごとの時計回り回転量(度)
static const double LUNAGUN_A_PAIR_HALF_DEG = 9.0;   // 組の中心から各弾までの角度(度)。狭い間隔=この2倍、広い間隔=360/組数-この2倍
static const int    LUNAGUN_A_COLOR = 4;     // 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
static const int    LUNAGUN_A_SOUND_EVERY = 3;     // 何回の発射に1回効果音を鳴らすか

// ---------- B: 逆三角形弾列 ----------
static const int    LUNAGUN_B_START = 60;    // Bの開始フレーム
static const int    LUNAGUN_B_SIDE = 8;     // 三角形の1辺の弾数(=行数)
static const double LUNAGUN_B_SPEED = 4.0;   // 弾速
static const double LUNAGUN_B_SPACING = 12.0;  // 横方向の弾間隔(px)
static const int    LUNAGUN_B_ROW_INTERVAL = 3;     // 行ごとの発射間隔(フレーム)。進行方向の弾間隔 = 弾速 × これ
static const int    LUNAGUN_B_TRI_NUM = 5;     // 1セットあたりの三角形の数
static const int    LUNAGUN_B_TRI_INTERVAL = 6;     // 三角形同士の時間差(フレーム)
static const int    LUNAGUN_B_SET_NUM = 2;     // 休止前に連続で撃つセット数
static const int    LUNAGUN_B_PAUSE = 60;    // セット連射後の休止(フレーム)
static const int    LUNAGUN_B_AIM_JITTER_DEG = 10*6;   // 狙いの荒さ。自機方向から±この角度(度)ランダムにずらす
static const int    LUNAGUN_B_COLOR = 0;     // 弾の色(Aと同じ並び)

// 1サイクル = 三角形を撃つ期間 + 休止
static const int LUNAGUN_B_FIRE_FRAMES = LUNAGUN_B_TRI_INTERVAL * LUNAGUN_B_TRI_NUM * LUNAGUN_B_SET_NUM;
static const int LUNAGUN_B_CYCLE = LUNAGUN_B_FIRE_FRAMES + LUNAGUN_B_PAUSE;

// ============================================================
//  補助関数
// ============================================================
static double LunaGunDegToRad(double deg)
{
    return deg / 180.0 * DX_PI;
}

// 弾をShotSetの弾リスト末尾に連結する
static void LunaGunLinkShot(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ShotSetを生成してenemyShotSetHeadのリスト末尾に連結する
static sEnemyShotSet* LunaGunCreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = 0;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// ============================================================
//  弾幕A: 狭広10way全方位弾(1回分)
//    pEnemyShotSet->muki    : 今回の基準角(これを時計回りに回していく)
//    pEnemyShotSet->param_i[0] : 0以外なら効果音を鳴らす
// ============================================================
static void ShotLunaticRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (pEnemyShotSet->param_i[0] != 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        const int pairNum = LUNAGUN_A_WAY / 2;
        for (int i = 0; i < LUNAGUN_A_WAY; i++) {
            int pair = i / 2;                           // 何組目か
            double side = (i % 2 == 0) ? -1.0 : 1.0;    // 組の中心から見て反時計側(-)か時計側(+)か
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 組の中心を等間隔に並べ、その両側に振り分ける → 狭い間隔と広い間隔が交互に並ぶ
            pEnemyShot->muki = pEnemyShotSet->muki
                + 2.0 * DX_PI * pair / pairNum
                + side * LunaGunDegToRad(LUNAGUN_A_PAIR_HALF_DEG);
            pEnemyShot->speed = LUNAGUN_A_SPEED;
            pEnemyShot->kind = img_enemyShotMediumBall[LUNAGUN_A_COLOR];
            LunaGunLinkShot(pEnemyShotSet, pEnemyShot);
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕B: 逆三角形弾列(1つ分)
//    pEnemyShotSet->muki    : 三角形の進行方向(自機狙い+ブレ)
//    pEnemyShotSet->param_i[0] : 0以外なら効果音を鳴らす
//
//  頂点(1発)を先頭に、ROW_INTERVAL フレームごとに 2発, 3発, ... 8発 の行を
//  同じ地点から同じ速度で撃ち出す。進行方向に対して横にだけ位置をずらすので、
//  弾は等速直進のまま、頂点が自機側を向いた三角形(=逆三角形)になる。
// ============================================================
static void ShotLunaticTriangle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % LUNAGUN_B_ROW_INTERVAL == 0) {
        int row = pEnemyShotSet->count / LUNAGUN_B_ROW_INTERVAL; // 0:頂点 〜 SIDE-1:底辺
        if (row < LUNAGUN_B_SIDE) {
            if (row == 0 && pEnemyShotSet->param_i[0] != 0) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
            for (int j = 0; j <= row; j++) {
                // 進行方向に対して垂直な向き (-sin, cos) に中央揃えでずらす
                double lateral = (j - row / 2.0) * LUNAGUN_B_SPACING;
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x - lateral * sin(pEnemyShotSet->muki);
                pEnemyShot->y = pEnemyShotSet->y + lateral * cos(pEnemyShotSet->muki);
                pEnemyShot->muki = pEnemyShotSet->muki;  // 銃弾は進行方向に向ける
                pEnemyShot->speed = LUNAGUN_B_SPEED;
                pEnemyShot->kind = img_enemyShotBullet[LUNAGUN_B_COLOR];
                LunaGunLinkShot(pEnemyShotSet, pEnemyShot);
            }
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_LunaticGun_Claude()
{
    static double ringAngle; // Aの基準角(増える方向が画面上での時計回り: y軸が下向きのため)
    static int    ringCount; // Aの発射回数

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = LUNAGUN_ENEMY_X;
        enemy.y = LUNAGUN_ENEMY_Y;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        ringAngle = DX_PI / 2.0;      // 最初の基準角は真下
        ringCount = 0;
    }

    // ---- A: 常時撃ち続ける ----
    if (count % LUNAGUN_A_INTERVAL == 1) {
        sEnemyShotSet* pEnemyShotSet = LunaGunCreateShotSet(ShotLunaticRing, enemy.x, enemy.y, ringAngle);
        pEnemyShotSet->param_i[0] = (ringCount % LUNAGUN_A_SOUND_EVERY == 0) ? 1 : 0;
        ringAngle += LunaGunDegToRad(LUNAGUN_A_ROTATE_DEG); // 時計回りに回転
        ringCount++;
    }

    // ---- B: (三角形×5)×2セットを連続で撃つ → 休止 を繰り返す ----
    int bTime = count - LUNAGUN_B_START;
    if (bTime >= 0) {
        int cycleTime = bTime % LUNAGUN_B_CYCLE;
        if (cycleTime < LUNAGUN_B_FIRE_FRAMES && cycleTime % LUNAGUN_B_TRI_INTERVAL == 0) {
            int triIndex = cycleTime / LUNAGUN_B_TRI_INTERVAL; // サイクル内で何個目の三角形か
            // 自機狙い。GetRand(2*J) は 0〜2J の 2J+1 通りなので、-J 〜 +J の対称なブレになる。
            double aim = atan2(player.y - enemy.y, player.x - enemy.x);
            aim += LunaGunDegToRad(GetRand(LUNAGUN_B_AIM_JITTER_DEG * 2) - LUNAGUN_B_AIM_JITTER_DEG);
            sEnemyShotSet* pEnemyShotSet = LunaGunCreateShotSet(ShotLunaticTriangle, enemy.x, enemy.y, aim);
            pEnemyShotSet->param_i[0] = (triIndex % LUNAGUN_B_TRI_NUM == 0) ? 1 : 0; // セットの先頭だけ効果音
        }
    }
}