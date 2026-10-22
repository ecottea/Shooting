//-----------------------------------------------------------------------------
// enemyPat_snowCrystal.cpp
// 雪の結晶モチーフ弾幕『六華結晶(りっかけっしょう)』
//
// 【構成】3フェーズ + 背景演出
//   フェーズ1「成長」 : 敵の周囲に静止弾を配置し、雪の結晶が育っていく演出(約1秒)。
//                      6本の主軸(白の中玉) → 先端の菱形弾(シアン) → 側枝(シアンの小玉)
//                      の順に内側から生えていく。予告音(sound_enemyCharge)を鳴らす。
//   フェーズ2「開花」 : 全弾が一斉に放射状へ動き出す(見せ場)。
//                      1本の主軸は生成時に自機方向へ向く。
//   フェーズ3「揺らぎ」: 結晶全体をゆっくり回転させる(ウェーブごとに回転方向が反転)。
//                      しばらく進んだ先端の菱形弾から、六方向のリング弾(白の小玉)を展開。
//   背景演出「粉雪」  : 画面上部から青い小玉がゆっくり降り注ぐ(常時・別セット)。
//
// 【素材の選定】enemyPat_sampleForAI.cpp の素材一覧から抜粋して使用
//   主軸   : 中玉(7.0x7.0)   白[6]     … 結晶の「腕」になる太い線
//   側枝   : 小玉(2.5x2.5)   シアン[3] … 腕から±40°で枝分かれする細かい枝(雪の樹枝晶)
//   先端   : 菱形弾(4.5x2.5) シアン[3] … 腕の先の六角板(結晶のきらめく先端)
//   リング : 小玉(2.5x2.5)   白[6]     … 先端が展開する六角形の弾の柵
//   粉雪   : 小玉(2.5x2.5)   青[4]     … 背景の降雪(手前より暗い色で奥行きを出す)
//   ※大玉/銃弾/鱗弾/中楕円弾/短レーザーは雪の結晶の無機質な形を壊すため不使用。
//
// 【仕様上の分担】
//   - count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと、
//     画面外に出た弾の消去はメインルーチンが行うため、ここでは読むだけにしている。
//   - GetRand(x) は 0〜x の x+1 種類の整数を返すため、
//     「+オフセット」「+1」など範囲計算に注意して使用している。
//-----------------------------------------------------------------------------
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============ 調整用定数 ============
static const int    BLOOM_FRAME = 64;     // 開花するフレーム(pEnemyShotSet->count基準)
static const int    RING_FRAME = 100;    // 先端弾がリング弾を放つフレーム(pEnemyShot->count基準)
static const double AXIS_SPEED = 2.4;    // 主軸・先端の速度
static const double BRANCH_SPEED = 1.5;    // 側枝の速度(主軸より遅くして残雪のように尾を引かせる)
static const double RING_SPEED = 0.9;    // リング弾の速度(ゆっくり広がる六角形の柵)
static const double ROT_SPEED = 0.0025; // 開花後の回転速度(ラジアン/フレーム 約0.14°)

// 弾の役割(param_i[0]に格納)
static const int ROLE_AXIS = 0; // 主軸
static const int ROLE_BRANCH = 1; // 側枝
static const int ROLE_TIP = 2; // 先端(菱形弾)
static const int ROLE_RING = 3; // 先端から展開される六方向リング弾
static const int ROLE_SNOW = 4; // 粉雪

// 画面内(少し内側)に収まっているか
// 敵は画面上部にいるため、上向きの腕は画面外に出てしまう。
// 生成時にこの判定を行うことで、画面端で刈り取られた自然な見た目になる。
static bool InScreen(double x, double y)
{
    //return x >= 12.0 && x <= 468.0 && y >= 12.0 && y <= 468.0;
    return true;
}

// 弾を1発生成してセットの連結リストへ登録する
// speed        : 今の速度(成長中は0で静止させる)
// bloomSpeed   : 開花時に speed へ代入する値(param_d[0]に記憶)
static sEnemyShot* SpawnShot(sEnemyShotSet* pSet, double x, double y, double muki,
    double speed, double bloomSpeed, int kind, int role)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->param_d[0] = bloomSpeed; // 開花時の速度を記憶
    pEnemyShot->kind = kind;             // 弾の画像(種類+色)
    pEnemyShot->param_i[0] = role;       // 役割
    pEnemyShot->margin = 240;
    pEnemyShot->prev = pSet->pEnemyShotHead->prev;
    pEnemyShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pEnemyShot;
    pSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// 弾セットを生成してリストへ登録する
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc patternFunc,
    double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// 弾幕: 粉雪(背景として常に降り注ぐ)
static void ShotKonayuki(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % 20 == 0) {
        // GetRand(x) は 0〜x の x+1 種類を返すので範囲に注意
        double x = 10 + GetRand(460);                                  // 10〜470
        double y = 10 + GetRand(10);                                   // 10〜20(画面上端の内側)
        double muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;   // 真下から±20°
        double speed = 0.7 + GetRand(50) / 100.0;                          // 0.70〜1.20
        SpawnShot(pEnemyShotSet, x, y, muki, speed, speed,
            img_enemyShotSmallBall[4], ROLE_SNOW);                   // 青の小玉
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 弾幕: 六華結晶
static void ShotRikka(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;
    const double cx = pEnemyShotSet->x;
    const double cy = pEnemyShotSet->y;
    const double base = pEnemyShotSet->muki; // 6本の主軸のうち1本が自機方向

    // ============================================================
    // フェーズ1: 結晶の成長(全弾静止して形を作る)
    // ============================================================
    if (pEnemyShotSet->count < BLOOM_FRAME) {
        if (pEnemyShotSet->count == 0) {
            // 予告音(結晶が育つ合図)
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        // 主軸: 2フレームに1リングずつ、内側から距離20〜125まで成長する
        if (pEnemyShotSet->count <= 14 && pEnemyShotSet->count % 2 == 0) {
            double d = 20.0 + (pEnemyShotSet->count / 2) * 15.0;
            for (int k = 0; k < 6; k++) {
                double ang = base + k * (DX_PI / 3.0);
                double x = cx + d * cos(ang);
                double y = cy + d * sin(ang);
                if (InScreen(x, y)) {
                    SpawnShot(pEnemyShotSet, x, y, ang, 0.0, AXIS_SPEED,
                        img_enemyShotMediumBall[6], ROLE_AXIS);  // 白の中玉
                }
            }
        }
        // 先端: 距離135に菱形弾(結晶の先の六角板)
        if (pEnemyShotSet->count == 20) {
            for (int k = 0; k < 6; k++) {
                double ang = base + k * (DX_PI / 3.0);
                double x = cx + 135.0 * cos(ang);
                double y = cy + 135.0 * sin(ang);
                if (InScreen(x, y)) {
                    SpawnShot(pEnemyShotSet, x, y, ang, 0.0, AXIS_SPEED,
                        img_enemyShotDiamond[3], ROLE_TIP);      // シアンの菱形弾
                }
            }
        }
        // 側枝: 内側から順に、軸上の位置から±40°へ小玉を配置する
        if (pEnemyShotSet->count >= 24 && pEnemyShotSet->count <= 58 &&
            pEnemyShotSet->count % 2 == 0) {
            int idx = (pEnemyShotSet->count - 24) / 2; // 0〜17
            int axis = idx % 6;                         // 対象の軸
            int level = idx / 6;                         // 0〜2(内側→外側)
            double d = 45.0 + level * 30.0;            // 軸上の位置 45, 75, 105
            double ang = base + axis * (DX_PI / 3.0);
            double x = cx + d * cos(ang);
            double y = cy + d * sin(ang);
            if (InScreen(x, y)) {
                for (int s = -1; s <= 1; s += 2) {
                    double bang = ang + s * (40.0 * DX_PI / 180.0);
                    SpawnShot(pEnemyShotSet, x, y, bang, 0.0, BRANCH_SPEED,
                        img_enemyShotSmallBall[3], ROLE_BRANCH); // シアンの小玉
                }
            }
        }
        return; // 成長が終わるまで全弾は静止したまま
    }

    // ============================================================
    // フェーズ2: 開花(全弾が一斉に動き出す)
    // ============================================================
    if (pEnemyShotSet->count == BLOOM_FRAME) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    // 先端のリング弾展開に合わせた音(先端弾はセットcount=20生成なので、
    // pEnemyShot->count==100 は セットcount==120 前後になる)
    if (pEnemyShotSet->count == 120) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 結晶全体をゆっくり回転させる(ウェーブごとに回転方向を反転)
    double rot = (pEnemyShotSet->kind % 2 == 0) ? ROT_SPEED : -ROT_SPEED;
    pEnemyShotSet->muki += rot;

    // ============================================================
    // 全弾の更新
    // ============================================================
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 開花の瞬間: 記憶しておいた速度を一斉に代入する
        if (pEnemyShotSet->count == BLOOM_FRAME) {
            pShot->speed = pShot->param_d[0];
        }
        // 先端弾はしばらく進んだ後、六方向のリング弾(白の小玉)を展開する
        // 軸と軸の「間」(30°側)に放つことで、腕の隙間への逃げ込みを塞ぐ
        if (pShot->param_i[0] == ROLE_TIP && pShot->param_i[1] == 0 &&
            pShot->count >= RING_FRAME) {
            pShot->param_i[1] = 1; // 発射は1回だけ
            if (InScreen(pShot->x, pShot->y)) {
                for (int k = 0; k < 6; k++) {
                    double ang = pEnemyShotSet->muki + DX_PI / 6.0 + k * (DX_PI / 3.0);
                    SpawnShot(pEnemyShotSet, pShot->x, pShot->y, ang, RING_SPEED, RING_SPEED,
                        img_enemyShotSmallBall[6], ROLE_RING);
                }
            }
        }
        pShot->muki += rot;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン: 雪の結晶『六華結晶』
void EnemyPat_Snowflake_Zai()
{
    static int muki;
    static int waveCount;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 170.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        waveCount = 0;

        // 粉雪(背景演出)用のセットを1つだけ生成。
        // 20フレームごとに弾を1発落とすので弾リストが空にならず、
        // 敵が倒されるまで降り続く。
        CreateShotSet(ShotKonayuki, enemy.x, enemy.y, DX_PI / 2.0, 0);
    }
    else {
        // 雪片が漂うようにゆっくり揺れながら移動する
        enemy.x += 0.5 * (double)muki;
        enemy.y = 170.0 + sin(count * 0.02) * 6.0;
        if (count % 150 == 75) muki *= -1;
    }

    // 結晶ウェーブ: 4秒(240フレーム)間隔で生成。
    // 生成時に1本の主軸を自機方向へ向ける(その軸に避け意識が集まる)。
    if (count % 120 == 61) {
        double sy = enemy.y + 10.0;
        double aim = atan2(player.y - sy, player.x - enemy.x);
        CreateShotSet(ShotRikka, enemy.x, sy, aim, waveCount++);
    }
}