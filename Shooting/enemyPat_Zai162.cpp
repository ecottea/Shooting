// ============================================================
//  enemyPat_Tmp.cpp
//  弾幕パターン:注乳「ホワイト・カラフェ」(牛乳を注ぐ女 モチーフ)
//
//  【デザイン概要】 専用素材を使わず、既存弾(すべて白)の組み合わせで
//   「ピッチャーからグラスへ牛乳を注ぐ」動作を表現する。
//
//   - 本流 : 大玉(白)を密に連射して1本の"牛乳の柱"を作り、ゆっくり落下させる
//            (10フレーム間隔・弾速1.5 → 弾間隔15px < 弾径20px で弾が重なり、
//             連続した液体の流れに見える)
//   - 飛沫 : 本流が仮想液面(グラスライン)に達した瞬間、鱗弾(白)が下方向に跳ねる
//            (コップに着いたしぶき。鱗弾は涙滴形なので飛沫に見立てた)
//   - 泡   : 小玉(白)が液面の下をゆっくり流れる(液体の質感を出す演出弾)
//   - 溢れ : 終盤、中玉(白)の泡が画面下から一斉に浮上し、残った本流と
//            上下挟撃を形成する(グラスから溢れ返る牛乳)
//
//  【サイクル】720フレームで1周(ボスが倒されるまでループ)
//   Phase A (  0-239) 注ぎ開始 : 画面中央で直下に注ぐ(微かに揺らぐ)
//   Phase B (240-479) スイング : 注ぎ位置が左右に1往復(x=90〜390、液柱のカーテン)
//   Phase C (480-599) 狙い注ぎ : 液面が上昇。予告音+細い雫で狙いを見せた後、
//                                プレイヤーを狙ってピッチャーを傾けて注ぐ
//   Phase D (600-719) 溢れ返り : 注ぎを止め、画面下から泡が浮上するフィニッシュ
//
//  【使用素材と選定理由】
//   弾: img_enemyShotLargeBall[6]  大玉・白   … 本流(牛乳)。最大サイズで太い流れを作る
//       img_enemyShotScale[6]      鱗弾・白   … 飛沫(しぶき)。涙滴形が水滴に見える
//       img_enemyShotSmallBall[6]  小玉・白   … 液面の泡/狙いの予告雫。小ささが泡らしい
//       img_enemyShotMediumBall[6] 中玉・白   … 溢れ返る泡。大きさで演出弾と区別
//       ※色をすべて白(6)に統一しているのは「牛乳」の白濁イメージのため。
//         弾種のサイズ差と動きの差で種別を判別できる。
//   音: sound_enemyShot_medium  … 注ぎ開始 / スイング開始
//       sound_enemyShot_light   … 着水音(飛沫発生時。45フレームに1回にレート制限)
//       sound_enemyCharge       … 狙い注ぎの予告音
//       sound_enemyShot_heavy   … 傾け完了・狙った方向への注ぎ再開
//       sound_enemyShot_extreme … 溢れ返り開始
//
//  【実装メモ】
//   - count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//     画面外弾の消去はメインルーチンが行うため、ここでは一切行わない。
//     (各弾セットは発射を止めれば弾が画面外に出て空になり、本体側で回収される前提)
//   - GetRand(x) は 0〜x の x+1 種類を返すため、範囲計算では注意して使用。
//     リプレイファイルにseed値が保存されるため再現性は保たれる。
//   - sEnemyShot/sEnemyShotSet はメモリプールから再利用されるため、
//     生成時に count と自由パラメータ(param_i/param_d)を明示的に初期化する。
//     (margin は既定値20.0のまま触らない)
//
//  【難易度調整ポイント】
//   - milk_STREAM_INTERVAL / milk_STREAM_SPEED … 本流の密度と速度
//   - 飛沫の方向数(8)と速度(2.2〜3.0) … 着水点周辺の圧
//   - milk_GLASS_RISE … 液面上昇による締め付けの強さ
//   - ShotOverflow の発生間隔(3フレーム)と速度(1.0〜2.0) … フィニッシュの密度
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---- 定数 ----
static const int    milk_CYCLE = 720;   // 1サイクルのフレーム数
static const int    milk_PHASE_B = 240;   // スイング開始
static const int    milk_PHASE_C = 480;   // 狙い開始(予告)
static const int    milk_TILT_END = 520;   // 傾け完了(注ぎ再開)
static const int    milk_PHASE_D = 600;   // 溢れ返り開始
static const double milk_GLASS_Y = 380.0; // 通常時のグラスライン(仮想液面)
static const double milk_GLASS_RISE = 60.0;  // 液面が上昇する量
static const int    milk_WHITE = 6;     // 弾の色:白
static const double milk_STREAM_SPEED = 1.5;   // 本流の弾速
static const int    milk_STREAM_INTERVAL = 10;    // 本流の発射間隔(フレーム)

// ---- 補助:再生中の音を止めてから鳴らす(重複再生対策) ----
static void MilkPlaySound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// ---- 補助:値を範囲内にクランプ ----
static double MilkClamp(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// ---- 仮想液面(グラスライン)のY座標 ----
// Phase C からサイクル終端まで毎フレーム0.25pxずつ上昇し(牛乳が満ちる)、
// 次サイクル開始で元に戻る(グラスが空いて再注ぎされる演出)。
static double MilkGlassY(int c)
{
    if (c < milk_PHASE_C) return milk_GLASS_Y;
    if (c <= milk_CYCLE)  return milk_GLASS_Y - (c - milk_PHASE_C) * (milk_GLASS_RISE / (milk_CYCLE - milk_PHASE_C));
    return milk_GLASS_Y;
}

// ---- 補助:弾を1発生成してセットに連結 ----
// (リンク処理はサンプルの実装に準拠。プール再利用対策として
//  count と自由パラメータを明示的に初期化する)
static sEnemyShot* MilkAddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->count = 0; // 初期化のみ(インクリメント自体はメインルーチンが行う)
    p->margin = 240;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// ---- 補助:弾セットを生成して全体リストに連結 ----
static sEnemyShotSet* MilkAddShotSet(void (*patternFunc)(sEnemyShotSet*), double x, double y, double muki, int kind)
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
    return p;
}

// ------------------------------------------------------------
// 本流:大玉(白)を密に連ねた「牛乳の柱」
// param_i[0] : 0=本流の弾 / 1=飛沫(着水判定の対象外) / 2=泡
// param_i[1] : 前回の着水音フレーム(レート制限用)
// param_d[0] : 現在の注ぎ角度
// param_d[2] : 狙い角度(Phase C 開始時にプレイヤー位置から算出)
// ------------------------------------------------------------
static void ShotMilkStream(sEnemyShotSet* pEnemyShotSet)
{
    const int c = pEnemyShotSet->count;
    double landX[8];
    double landY[8];
    int landNum = 0;

    // ---- 弾の移動 + 着水判定 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double prevY = pShot->y;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        // 本流の弾が下向きに液面を通過した瞬間を記録(飛沫の発生点)
        if (pShot->param_i[0] == 0 &&
            pShot->muki > DX_PI * 0.25 && pShot->muki < DX_PI * 0.75) {
            double gy = MilkGlassY(c);
            if (prevY < gy && pShot->y >= gy && landNum < 8) {
                landX[landNum] = pShot->x;
                landY[landNum] = gy;
                landNum++;
            }
        }
        pShot = pShot->next;
    }

    // ---- 飛沫:着水点から鱗弾(白)が下方向に跳ねる ----
    for (int j = 0; j < landNum; j++) {
        // 着水音(連続再生を避けるため45フレームに1回に制限)
        if (c - pEnemyShotSet->param_i[1] >= 45) {
            pEnemyShotSet->param_i[1] = c;
            MilkPlaySound(sound_enemyShot_light);
        }
        for (int i = 0; i < 8; i++) {
            // 下向き半円(約9度〜171度)に8方向+ランダム揺らぎ
            // ※GetRand(10) は 0〜10 の11種を返すので -5〜+5 になる
            double m = DX_PI * (0.05 + 0.9 * i / 7.0) + (GetRand(10) - 5) / 180.0 * DX_PI;
            m = MilkClamp(m, DX_PI * 0.05, DX_PI * 0.95); // 水平より上へ飛ばさない
            double spd = 2.2 + GetRand(80) / 100.0;
            sEnemyShot* p = MilkAddShot(pEnemyShotSet, landX[j], landY[j], m, spd, img_enemyShotScale[milk_WHITE]);
            p->param_i[0] = 1; // 飛沫フラグ(着水判定の対象外)
        }
    }

    // ---- 注ぎ角度の制御(Phase C:プレイヤーを狙う) ----
    if (c == milk_PHASE_C) {
        double sx = enemy.x;
        double sy = enemy.y + 25.0;
        // 真上や水平より上へは注がない(下方向側だけに限定)
        if (player.y > sy + 40.0) pEnemyShotSet->param_d[2] = atan2(player.y - sy, player.x - sx);
        else                      pEnemyShotSet->param_d[2] = DX_PI / 2.0;
    }
    if (c < milk_PHASE_C) {
        pEnemyShotSet->param_d[0] = DX_PI / 2.0; // Phase A/B は直下注ぎ
    }
    else {
        // 40フレームかけて角度を補間(ピッチャーを傾ける予告動作)
        double k = (c - milk_PHASE_C) / 40.0;
        if (k > 1.0) k = 1.0;
        pEnemyShotSet->param_d[0] = DX_PI / 2.0 + (pEnemyShotSet->param_d[2] - DX_PI / 2.0) * k;
    }

    // ---- 予告:傾けている角度に細い雫(小玉)を流して狙いを見せる ----
    if (c >= milk_PHASE_C && c < milk_TILT_END && c % 8 == 0) {
        MilkAddShot(pEnemyShotSet, enemy.x, enemy.y + 25.0, pEnemyShotSet->param_d[0], 3.0, img_enemyShotSmallBall[milk_WHITE]);
    }

    // ---- 本流の発射(大玉・白) ----
    // 10フレームに1発。弾間隔15pxに対して弾径20pxなので弾が重なり、
    // 1本の連続した"牛乳の柱"に見える。
    // Phase C の傾け中(480〜519)は一旦注ぎを止めて予告に使う。
    bool pour = (c < milk_PHASE_C) || (c >= milk_TILT_END && c < milk_PHASE_D);
    if (pour && c % milk_STREAM_INTERVAL == 0) {
        MilkAddShot(pEnemyShotSet, enemy.x, enemy.y + 25.0, pEnemyShotSet->param_d[0], milk_STREAM_SPEED, img_enemyShotLargeBall[milk_WHITE]);
    }
}

// ------------------------------------------------------------
// 液面の泡:小玉(白)がグラスライン下をゆっくり流れる(演出弾)
// param_i[0] : 2=泡(着水判定の対象外)
// param_d[0] : さざ波の位相
// ------------------------------------------------------------
static void ShotFoam(sEnemyShotSet* pEnemyShotSet)
{
    const int c = pEnemyShotSet->count;

    // ---- 移動:横へゆっくり流れつつ上下にさざ波 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki) + sin(pShot->count * 0.05 + pShot->param_d[0]) * 0.15;
        pShot = pShot->next;
    }

    // ---- 発射:注ぎ期間中のみ 15フレームに1個 ----
    if (c < milk_PHASE_D && c % 15 == 0) {
        // GetRand(x) は 0〜x の x+1 種類を返すので範囲計算に注意
        double fx = 140.0 + GetRand(200);                    // グラス領域 x=140〜340
        double fy = MilkGlassY(c) + 10.0 + GetRand(30);      // 液面の少し下
        int    dir = GetRand(1);                              // 0:右流れ / 1:左流れ
        double m = MilkClamp((dir == 0 ? 0.0 : DX_PI) + (GetRand(20) - 10) / 180.0 * DX_PI,
            DX_PI * 0.03, DX_PI * 0.97);   // ゆるく下向きに流す
        double spd = 0.4 + GetRand(30) / 100.0;
        sEnemyShot* p = MilkAddShot(pEnemyShotSet, fx, fy, m, spd, img_enemyShotSmallBall[milk_WHITE]);
        p->param_d[0] = GetRand(628) / 100.0; // 揺れの位相 0〜6.28
        p->param_i[0] = 2;
    }
}

// ------------------------------------------------------------
// 溢れ返り:中玉(白)の泡が画面下から一斉に浮上する(フィニッシュ)
// param_d[0] : 揺れの位相
// ------------------------------------------------------------
static void ShotOverflow(sEnemyShotSet* pEnemyShotSet)
{
    const int c = pEnemyShotSet->count;

    // ---- 移動:下からゆっくり、左右に揺れながら浮上 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki) + sin(pShot->count * 0.08 + pShot->param_d[0]) * 0.5;
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }

    // ---- 発射:120フレーム間、3フレームに1個 ----
    if (c < 120 && c % 3 == 0) {
        double bx = 20.0 + GetRand(440);                     // x=20〜460
        double m = -DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
        double spd = 1.0 + GetRand(100) / 100.0;
        sEnemyShot* p = MilkAddShot(pEnemyShotSet, bx, 485.0, m, spd, img_enemyShotMediumBall[milk_WHITE]);
        p->param_d[0] = GetRand(628) / 100.0; // 揺れの位相 0〜6.28
    }
}

// ------------------------------------------------------------
// 敵本体のパターン:注乳「ホワイト・カラフェ」
// (720フレームを1サイクルとして、ボスが倒されるまでループする)
// ------------------------------------------------------------
void EnemyPat_Milkmaid_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    const int t = (count - 1) % milk_CYCLE; // サイクル内の経過フレーム

    // ---- ボス移動(注ぎ口=ピッチャーの位置) ----
    if (t < milk_PHASE_B) {
        // Phase A:画面中央付近で微かに揺れる
        enemy.x = 240.0 + sin(t * 0.05) * 8.0;
    }
    else if (t < milk_PHASE_C) {
        // Phase B:x=90〜390 を 1往復するスイング
        enemy.x = 240.0 + sin((t - milk_PHASE_B) * (DX_PI * 2.0 / (milk_PHASE_C - milk_PHASE_B))) * 150.0*1.4;
    }
    else {
        // Phase C/D:中央で構えて注ぐ
        enemy.x = 240.0;
    }

    // ---- フェーズ境界ごとの弾セット生成と演出音 ----
    if (t == 0) {
        // 注ぎ開始:本流+液面の泡
        MilkAddShotSet(ShotMilkStream, enemy.x, enemy.y + 25.0, DX_PI / 2.0, 0);
        MilkAddShotSet(ShotFoam, 240.0, 0.0, 0.0, 1);
        MilkPlaySound(sound_enemyShot_medium);
    }
    else if (t == milk_PHASE_B) {
        MilkPlaySound(sound_enemyShot_medium); // スイング開始
    }
    else if (t == milk_PHASE_C) {
        MilkPlaySound(sound_enemyCharge);      // 狙い注ぎの予告音
    }
    else if (t == milk_TILT_END) {
        MilkPlaySound(sound_enemyShot_heavy);  // 傾け完了、狙った方向へ注ぎ直し
    }
    else if (t == milk_PHASE_D) {
        // 注ぎ終了→グラスから溢れ返る
        MilkAddShotSet(ShotOverflow, 240.0, 0.0, 0.0, 2);
        MilkPlaySound(sound_enemyShot_extreme);
    }
}