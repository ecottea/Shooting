// enemyPat_bee.cpp
// ============================================================
//  モチーフ：ハチ（蜂）
//
//  弾幕パターン一覧
//    ShotBeeSwarm  ─ 扇形に広がる黄色小玉の群れ弾
//                    pEnemyShot->count に連動した正弦波で蛇行させ、
//                    乱舞する蜂の群れを演出。
//                    muki フィールドに「展開角オフセット（rad）」を
//                    格納して方向計算に流用する。
//
//    ShotBeeSting  ─ プレイヤー照準を中心に ±20° の高速銃弾 × 5 本。
//                    ハチが刺針を突き刺す瞬発攻撃をイメージ。
//
//    ShotBeeHex    ─ 60° 間隔 × 6 方向に中玉を展開する六角形バースト。
//                    黄とマゼンタを交互配色してハチの縞模様を表現。
//                    1 頂点がプレイヤー方向を向き、発射ごとに 15° 回転。
//
//  敵の動き
//    横：240 ± 180px の正弦波（周期 360 フレーム）
//    縦：  60 ±  15px のホバリング振動（周期 90 フレーム）
//    → 花の上をゆったり旋回するハチのような軌道
//
//  弾幕発射タイミング
//    count % 50  == 0   ShotBeeSwarm（群れ弾）
//    count % 80  == 30  ShotBeeSting（針弾）
//    count % 120 == 60  ShotBeeHex（六角展開弾）
//
//  仕様メモ（共通）
//    ・count / pEnemyShotSet->count / pEnemyShot->count の
//      インクリメントはメインルーチンが行う。ここでは触らない。
//    ・画面外弾の消去もメインルーチンが行う。
//    ・弾の蛇行計算には pEnemyShot->count を使い、
//      グローバル count は使わない（位相不連続防止）。
// ============================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ─── 弾幕1: 群れ弾 (ShotBeeSwarm) ─────────────────────────
//
//  プレイヤー方向を軸に ±60° の扇形へ 8 発の黄色小玉を発射。
//  各弾は pEnemyShot->count に連動した正弦波でわずかに蛇行する。
//
//  フィールドの流用
//    pEnemyShotSet->muki : 発射時のプレイヤー照準角（固定、基準方向）
//    pEnemyShot->muki    : 展開角オフセット（更新ループでは基準に加算）
//
//  速度：中央寄りの弾ほど速くしてメリハリを付ける（1.7 〜 2.5）
// ─────────────────────────────────────────────────────────────
static void ShotBeeSwarm(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int N = 24;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // ±60° の扇形を N 等分した展開角オフセット（-π/3 〜 +π/3）
            // i=0 → -π/3、i=N-1 → +π/3、中央 i=3.5 → 0
            double spread = (i - (N - 1) * 0.5) / ((N - 1) * 0.5) * (DX_PI / 3.0 * 3);

            pEnemyShot->muki = spread;   // ← 方向でなく展開角オフセットとして格納
            pEnemyShot->speed = 2.5 - 0.8 * fabs(spread) / (DX_PI / 3.0);
            //  spread=0    → speed=2.5（最速、中央弾）
            //  spread=±π/3 → speed=1.7（最遅、端弾）
            pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 実際の射出角 = 基準方向 + オフセット + 正弦波による蛇行
        //   蛇行振幅 : ±0.3 rad（約 ±17°）
        //   蛇行周期 : 2π/0.08 ≈ 78 フレーム
        //   位相差   : muki*2.0 で各弾がバラバラに揺れる
        //   ※ グローバル count でなく pEnemyShot->count を使用
        double angle = pEnemyShotSet->muki
            + pEnemyShot->muki
            + 0.3 * sin(pEnemyShot->count * 0.08 + pEnemyShot->muki * 2.0);
        pEnemyShot->x += pEnemyShot->speed * cos(angle);
        pEnemyShot->y += pEnemyShot->speed * sin(angle);
        pEnemyShot = pEnemyShot->next;
    }
}


// ─── 弾幕2: 針弾 (ShotBeeSting) ────────────────────────────
//
//  プレイヤー照準を中心に ±20° の扇形で 5 本の高速銃弾を発射。
//  ハチが一直線に突進して刺す瞬発的な攻撃。
//
//  使用弾種：img_enemyShotBullet[1]（黄・銃弾）
//  速度    ：4.5（高速・直線）
// ─────────────────────────────────────────────────────────────
static void ShotBeeSting(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int    N = 5;
        const double spreadMax = DX_PI / 9.0; // ±20°

        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double offset = (i - (N - 1) * 0.5) / ((N - 1) * 0.5) * spreadMax;
            pEnemyShot->muki = pEnemyShotSet->muki + offset;
            pEnemyShot->speed = 4.5;
            pEnemyShot->kind = img_enemyShotBullet[1]; // 黄・銃弾（刺針のイメージ）

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


// ─── 弾幕3: 六角展開弾 (ShotBeeHex) ────────────────────────
//
//  60° 間隔 × 6 方向に中玉を展開する六角形バースト。
//  pEnemyShotSet->muki を六角形の基準角とすることで、
//  呼び出し側から「どの頂点がプレイヤーを向くか」を制御できる。
//
//  配色：偶数インデックス=黄、奇数=マゼンタ（ハチの縞模様）
//  速度：2.2
// ─────────────────────────────────────────────────────────────
static void ShotBeeHex(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 60° 刻みで 6 方向。pEnemyShotSet->muki を基準角として加算
            pEnemyShot->muki = pEnemyShotSet->muki + i * (DX_PI / 3.0);
            pEnemyShot->speed = 2.2;
            // 黄とマゼンタ交互（蜂の縞模様イメージ）
            pEnemyShot->kind = (i % 2 == 0)
                ? img_enemyShotMediumBall[1]    // 黄
                : img_enemyShotMediumBall[5];   // マゼンタ

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


// ─── 共通ヘルパー: 弾幕セット生成 (spawnShotSet) ────────────
//
//  EnemyPat_Tmp 内で繰り返す「sEnemyShotSet の確保 → 初期化
//  → グローバルリストへ挿入」をまとめた内部関数。
// ─────────────────────────────────────────────────────────────
static void spawnShotSet(void(*func)(sEnemyShotSet*),
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


// ─── 敵本体パターン (EnemyPat_Tmp) ─────────────────────────
//
//  ホバリング移動しながら 3 種の弾幕を周期的に放つハチ型ボス。
//
//  移動軌道
//    enemy.x = 240 + 180 * sin(count * π/180)
//      → 周期 360 フレーム で左右 ±180px を往復（常に画面内）
//    enemy.y =  60 +  15 * sin(count * π/45)
//      → 周期  90 フレーム で上下  ±15px を振動（ホバリング感）
//    ※ sin の引数に global count を使うが、これは"位置"計算なので
//      弾の位相不連続問題は発生しない。
//
//  弾幕発射タイミング
//    count % 50  == 0   ShotBeeSwarm（群れ弾 ×8）
//    count % 80  == 30  ShotBeeSting（針弾  ×5）
//    count % 120 == 60  ShotBeeHex  （六角  ×6、発射ごとに 15° 回転）
// ─────────────────────────────────────────────────────────────
void EnemyPat_Bee_Claude2()
{
    static double guruguru = DX_PI / 2;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;

        guruguru = DX_PI / 2;
    }

    // ホバリング移動
    enemy.x = 240.0 + 15.0 * sin(count * DX_PI / 180.0);
    enemy.y = 240.0 + 15.0 * sin(count * DX_PI / 45.0);

    double toPlayer = atan2(player.y - enemy.y, player.x - enemy.x);

    // 群れ弾：50 フレームごと（最初の発射 = count 50）
    if (count % 5 == 0) {
        spawnShotSet(ShotBeeSwarm, enemy.x, enemy.y, guruguru);
        guruguru += 0.05;
    }

    // 針弾：80 フレームごと、30 フレームずらして発射（最初 = count 30）
    //if (count % 80 == 30) {
    //    spawnShotSet(ShotBeeSting, enemy.x, enemy.y, toPlayer);
    //}

    // 六角展開弾：120 フレームごと（最初 = count 60）
    //   hexPhase で発射回数を数え、15° ずつ六角形を回転させる。
    //   → 同じパターンの繰り返しにならず変化が生まれる。
    //if (count % 120 == 60) {
    //    int    hexPhase = count / 120;   // 0, 1, 2, ... と増加
    //    double hexBase = toPlayer + hexPhase * (DX_PI / 12.0); // +15°/回
    //    spawnShotSet(ShotBeeHex, enemy.x, enemy.y, hexBase);
    //}
}