// ============================================================================
//  弾幕パターン「ペンデュラム・シンクロニシティ」(ペンデュラムウェーブ弾)
// ----------------------------------------------------------------------------
//  <モチーフ>
//    周期が僅かに異なる振り子を横一列に並べると、位相差の増減によって
//    「全同調 → 波の伝播 → グループ分裂 → 再同調」が繰り返される
//    (ペンデュラムウェーブ)。この位相変化をそのまま攻撃リズムにした弾幕。
//
//  <構成>
//    吊り元(アンカー)13箇所を画面上端に並べ、糸で吊るしたおもりが
//    振り子kごとに 1周期で (8 + k) 回往復する単振動をする。
//    t=0 で全て右に揃い、以後は位相差が波となって列を伝う。
//
//    攻撃は次の6層:
//      (1) 鉛直通過斉射   : おもりが最速点(真下)を通る瞬間の自機狙い。
//                           位相差のぶん発射順がずれるため、狙い弾が
//                           左→右→左と波のようにたなびく。
//      (2) 全同調カーテン (t%720==0)     : 全て揃い切った瞬間、全おもりが真下へ平行弾。
//      (3) 分裂扇弾       (t%720==240,480): 位相が3つに割れる瞬間、自機狙い3-way。
//      (4) 逆位相編み目   (t%720==360)   : 隣合うおもりが逆位相の瞬間、
//                           偶数番は左下・奇数番は右下へ撃ち交叉する編み目。
//      (5) 予告           (t%720==600)   : 再同調(次のカーテン)の2秒前に予告音。
//      (6) 終演炸裂       (t==2160)      : 再同調の瞬間に糸が切れ、おもりが
//                           砕け散る放射弾と共に構造弾を撤去。
//                           60フレームの間合いののち EnemyPat_PendulumWave_Zai が再詠唱する。
//
//  <使用素材の一覧(enemyPat_sampleForAI.cpp より把握)>
//    弾の種類と半径: 小玉(2.5x2.5) 中玉(7.0x7.0) 大玉(20.0x20.0) 銃弾(5.0x2.0)
//                    鱗弾(4.0x3.0) 菱形弾(4.5x2.5) 中楕円弾(10.5x7.0) 短レーザー(64.0x4.0)
//    弾の色:  0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
//    効果音:  sound_enemyShot_light / sound_enemyShot_medium / sound_enemyShot_heavy
//             sound_enemyShot_extreme / sound_enemyCharge(予告音)
//
//  <素材の選定>(おもり・糸の専用素材はないため既存弾の組み合わせで表現)
//    おもり        = 中玉[1]黄     : アンカー間隔25pxに対し大玉(半径20)では
//                                    隣のおもりと干渉するため中玉(半径7)を採用
//    糸            = 小玉[6]白     : アンカー〜おもりを結ぶ線上に7個並べた点線。
//                                    毎フレーム位置を引き直しおもりの動きに追従。
//                                    点と点の間が空いているので縫って避けられる
//    鉛直通過斉射  = 小玉[0]赤     : 細く速く、連射向き
//    全同調カーテン= 中玉[3]シアン : おもり(黄)と同型の色違い → 同調演出に一貫性
//    分裂扇弾      = 鱗弾[2]緑     : 扇状に広がる弾に細長い形状が合う
//    逆位相編み目  = 菱形弾[0]赤   : 斜め方向へ飛ぶ弾に菱形が合う
//    終演炸裂      = 小玉[1]黄     : おもりと同色 → おもりが砕けた見た目
//    効果音        = heavy:全同調カーテン / medium:扇弾・編み目 / extreme:終演炸裂
//                    enemyCharge:再同調の予告 / light:斉射が頻繁なため不使用
//    不使用        : 大玉(間隔25pxに対し過大) / 銃弾・中楕円弾・短レーザー(役割重複)
//                    / 黒・青(背景に紛れて視認性が低い)
//
//  <仕様上の注意>
//    - count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと、
//      画面外に出た弾の消去はメインルーチンで行われるため、ここでは行わない
//    - 構造弾(おもり・糸)は速度を持たず、角度の式から毎フレーム位置を直接設定する。
//      画面外に出るとメインルーチンに消去されてしまうため、振れ幅・糸の長さ・
//      アンカー配置は全位置が必ず画面内(480x480)に収まるよう設計してある
//      (例: 端のおもりの最左位置 = 90 - 170*sin(26°) ≒ 15.5、
//            最右位置 = 390 + 74.5 ≒ 464.5、おもりの最下点 y = 20 + 185 = 205)
//    - 終演時のみ構造弾を画面外(-9999,-9999)へ退避させ、消去はメインルーチンに委ねる
//    - GetRand(x) は 0 以上 x 以下の x+1 種類を返すため、
//      (GetRand(16) - 8) で -8～+8 の 17 通りになる(終演炸裂でのみ使用。
//      基本は位相を正確に揃えるため乱数を使わない)
//
//  <調整ガイド>
//    - 攻撃密度   : PD_SPD_AIM / PD_BASE(往復数=斉射頻度)
//    - テンポ     : PD_CYCLE(位相変化・イベント間隔・予告タイミングが全て追従)
//    - 振れの大きさ: PD_AMP_DEG(端のおもりが画面外に出ない範囲で大きくする)
//    - 弾幕の規模 : PD_N(変更する場合はアンカー配置と糸長の画面内収束を再確認)
// ============================================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================================
//  定数(チューニング箇所)
// ============================================================================
static const int    PD_N = 13;    // 振り子の本数(奇数:逆位相が綺麗に出る)
static const int    PD_STRAND = 7;     // 糸1本あたりの小玉の数
static const double PD_AMP_DEG = 26.0;  // 振れ幅(度)
static const int    PD_BASE = 8;     // 最長振り子の1周期あたりの往復数(以降+1ずつ)
static const int    PD_CYCLE = 720;   // 再同調までのフレーム数(60fpsで12秒)
static const int    PD_TOTAL = PD_CYCLE * 3;  // 1回の詠唱で3周期演じる
static const int    PD_RECAST = PD_TOTAL + 60; // 再詠唱間隔(終演+1秒の間合い)
static const double PD_ANCHOR_Y = 20.0;  // 吊り元の高さ
static const double PD_ANCHOR_X0 = 90.0;  // 左端のアンカーX
static const double PD_ANCHOR_DX = 25.0;  // アンカーの間隔
static const double PD_SPD_AIM = 3.2;   // 鉛直通過斉射の速さ
static const double PD_SPD_CURTAIN = 2.2;   // 全同調カーテンの速さ
static const double PD_SPD_WEAVE = 2.6;   // 逆位相編み目の速さ
static const double PD_SPD_FAN = 2.8;   // 分裂扇弾の速さ
static const double PD_SPD_BURST = 2.4;   // 終演炸裂の速さ

static const double PD_AMP = PD_AMP_DEG * DX_PI / 180.0;
static const double PD_MID = (PD_N - 1) / 2.0;   // 中央の振り子番号

// 弾の役割分類(param_i[0] に格納)
enum {
    PD_TYPE_BOB = 0,      // おもり(構造弾)
    PD_TYPE_STRAND = 1,   // 糸(構造弾)
    PD_TYPE_FREE = 2      // 発射後は等速直進する自由弾
};

// ============================================================================
//  補助関数
// ============================================================================

// 糸の長さ:中央が最も長く端ほど短い緩いアーチ。
// 振れ幅との積(おもりの横振れ)が常に画面内に収まる範囲で設定してある。
static double PendLen(int k)
{
    double u = (k - PD_MID) / PD_MID;      // -1.0 ～ +1.0
    return 185.0 - 15.0 * u * u;           // 170.0 ～ 185.0
}

// 振り子kのアンカーX座標
static double PendAnchorX(int k)
{
    return PD_ANCHOR_X0 + PD_ANCHOR_DX * k;
}

// 振り子kの時刻t(フレーム)における角度。
// 1周期PD_CYCLEのあいだに (PD_BASE + k) 回往復する → 隣との位相差がじわじわ開く
static double PendTheta(int k, int t)
{
    return PD_AMP * cos(2.0 * DX_PI * (PD_BASE + k) * t / (double)PD_CYCLE);
}

// 振り子kの時刻tにおけるおもりの座標
static void PendBobPos(int k, int t, double* pX, double* pY)
{
    double th = PendTheta(k, t);
    double L = PendLen(k);
    *pX = PendAnchorX(k) + L * sin(th);
    *pY = PD_ANCHOR_Y + L * cos(th);
}

// 効果音を鳴らす(鳴っている間は止めてから再生し直す)
static void PlayShotSound(int sound)
{
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);
}

// 弾をセットのリスト末尾へ追加する
static sEnemyShot* AddShot(sEnemyShotSet* pSet, int type, int kind,
    double x, double y, double muki, double speed)
{
    sEnemyShot* p = new sEnemyShot;
    p->kind = kind;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->param_i[0] = type;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 自由弾(発射後は等速直進)を追加する
static void FireShot(sEnemyShotSet* pSet, int kind,
    double x, double y, double muki, double speed)
{
    AddShot(pSet, PD_TYPE_FREE, kind, x, y, muki, speed);
}

// 構造弾(おもり+糸)を13本ぶん生成する
static void CreatePendulums(sEnemyShotSet* pSet)
{
    for (int k = 0; k < PD_N; k++) {
        double ax = PendAnchorX(k);
        double L = PendLen(k);
        // おもり
        sEnemyShot* pBob = AddShot(pSet, PD_TYPE_BOB, img_enemyShotMediumBall[1],
            ax, PD_ANCHOR_Y + L, 0.0, 0.0);
        pBob->param_i[1] = k;
        // 糸:アンカーとおもりを結ぶ線上に小玉を等間隔に並べた点線
        for (int i = 1; i <= PD_STRAND; i++) {
            sEnemyShot* pStrand = AddShot(pSet, PD_TYPE_STRAND, img_enemyShotSmallBall[6],
                ax, PD_ANCHOR_Y, 0.0, 0.0);
            pStrand->param_i[1] = k;
            pStrand->param_i[2] = i;
        }
    }
}

// ============================================================================
//  位相イベント:波の状態が「揃い/分裂/逆位相」に変化する瞬間の攻撃
// ============================================================================
static void FirePhaseEvent(sEnemyShotSet* pSet, int t)
{
    const int phase = t % PD_CYCLE;

    if (phase == 0) {
        // 全同調カーテン:全おもりが同じ角度で揃い切った瞬間、一斉に真下へ。
        // おもりは右寄りの斜め一列なので、弾も斜めに並ぶカーテンになる。
        PlayShotSound(sound_enemyShot_heavy);
        for (int k = 0; k < PD_N; k++) {
            double bx, by;
            PendBobPos(k, t, &bx, &by);
            FireShot(pSet, img_enemyShotMediumBall[3], bx, by, DX_PI * 0.5, PD_SPD_CURTAIN);
        }
    }
    else if (phase == PD_CYCLE / 3 || phase == PD_CYCLE * 2 / 3) {
        // 分裂扇弾:位相が3つに割れる(隣と120度ずつ差がつく)瞬間、
        // 各おもりが自機狙いの3-wayを放つ。
        PlayShotSound(sound_enemyShot_medium);
        for (int k = 0; k < PD_N; k++) {
            double bx, by;
            PendBobPos(k, t, &bx, &by);
            double aim = atan2(player.y - by, player.x - bx);
            for (int j = -1; j <= 1; j++) {
                FireShot(pSet, img_enemyShotScale[2], bx, by, aim + j * 0.3, PD_SPD_FAN);
            }
        }
    }
    else if (phase == PD_CYCLE / 2) {
        // 逆位相編み目:隣合うおもりが逆方向に振れ切った瞬間、
        // 右に寄っている偶数番は左下へ、左に寄っている奇数番は右下へ撃ち、
        // 弾が交叉する編み目を作る。
        PlayShotSound(sound_enemyShot_medium);
        for (int k = 0; k < PD_N; k++) {
            double bx, by;
            PendBobPos(k, t, &bx, &by);
            double muki = (k % 2 == 0) ? (DX_PI * 0.75) : (DX_PI * 0.25);
            FireShot(pSet, img_enemyShotDiamond[0], bx, by, muki, PD_SPD_WEAVE);
        }
    }
}

// ============================================================================
//  弾幕パターン本体(弾セットごとに毎フレーム呼ばれる)
// ============================================================================
static void ShotPendulumWave(sEnemyShotSet* pEnemyShotSet)
{
    const int t = pEnemyShotSet->count;

    // ---- 終演:最後の一撃を放って構造弾を撤去する ------------------------
    if (t >= PD_TOTAL) {
        if (t == PD_TOTAL) {
            // 再同調の瞬間に糸が切れ、おもりが砕け散る放射弾
            PlayShotSound(sound_enemyShot_extreme);
            for (int k = 0; k < PD_N; k++) {
                double bx, by;
                PendBobPos(k, t - 1, &bx, &by);
                for (int j = 0; j < 6; j++) {
                    // GetRand(x)は0～xのx+1種類を返す → GetRand(16)-8 は -8～+8 の17通り
                    double jitter = (GetRand(16) - 8) * 0.01;
                    FireShot(pEnemyShotSet, img_enemyShotSmallBall[1], bx, by,
                        j * DX_PI / 3.0 + jitter, PD_SPD_BURST);
                }
            }
        }
        // 構造弾(おもり・糸)を画面外へ退避させる。
        // 画面外の弾の消去はメインルーチンの仕事のため、ここでは消さずに
        // 画面外へ追い出すだけ(次フレーム以降でメインルーチンが消去する)。
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] != PD_TYPE_FREE) {
                pShot->x = -9999.0;
                pShot->y = -9999.0;
            }
            pShot = pShot->next;
        }
        return;
    }

    // ---- 初回:構造弾を生成する ------------------------------------------
    if (t == 0) {
        CreatePendulums(pEnemyShotSet);
    }

    // ---- 再同調の予告音(次の全同調カーテンの2秒前) ----------------------
    if (t % PD_CYCLE == PD_CYCLE - 120) {
        PlayShotSound(sound_enemyCharge);
    }

    // ---- 位相イベント ----------------------------------------------------
    FirePhaseEvent(pEnemyShotSet, t);

    // ---- 毎フレーム:構造弾の位置更新と自由弾の移動 ----------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == PD_TYPE_FREE) {
            // 発射済みの弾は等速直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            const int    k = pShot->param_i[1];
            const double th_now = PendTheta(k, t);
            const double th_prev = PendTheta(k, t - 1);
            const double L = PendLen(k);
            const double ax = PendAnchorX(k);
            const double bx = ax + L * sin(th_now);
            const double by = PD_ANCHOR_Y + L * cos(th_now);

            if (pShot->param_i[0] == PD_TYPE_BOB) {
                // おもり:角度の式に沿って毎フレーム位置を直接設定する。
                // 位置は設計上常に画面内に収まっており、
                // メインルーチンの画面外消去に引っかからない。
                pShot->x = bx;
                pShot->y = by;

                // 鉛直通過斉射:真下(最速点)を通過する瞬間に自機狙いを1発。
                // 上り(−→+)は撃たない:発射数を半分にし、
                // 波のたなびきを読みやすくするため。
                // 位相差のぶん発射順がずれ、狙い弾が左→右→左へたなびく。
                if (t > 0 && th_prev > 0.0 && th_now <= 0.0) {
                    FireShot(pEnemyShotSet, img_enemyShotSmallBall[0], bx, by,
                        atan2(player.y - by, player.x - bx), PD_SPD_AIM);
                }
            }
            else {
                // 糸:アンカー〜おもりを結ぶ線上に置いた点線。
                // 毎フレーム引き直すので、おもりの動きに常に追従する。
                const double f = pShot->param_i[2] / (double)(PD_STRAND + 1);
                pShot->x = ax + (bx - ax) * f;
                pShot->y = PD_ANCHOR_Y + (by - PD_ANCHOR_Y) * f;
            }
        }
        pShot = pShot->next;
    }
}

// ============================================================================
//  敵本体のパターン
// ============================================================================
void EnemyPat_PendulumWave_Zai()
{
    if (count == 1) {
        // 出現位置とHP(200から変更しないこと)
        enemy.x = 240.0;
        enemy.y = 34.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 吊り台(ボス)は上部中央をゆっくり揺れるだけの演出。
        // アンカーは画面に固定してあるので、弾幕の形には影響しない。
        enemy.x = 240.0 + 28.0 * sin(2.0 * DX_PI * (count - 1) / 600.0);
    }

    // 詠唱:初回と、終演炸裂+1秒の間合いが過ぎたあとに弾セットを新規作成する。
    // (count - 1) が PD_RECAST の整数倍になるフレームでちょうど1回だけ成立する。
    if ((count - 1) % PD_RECAST == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPendulumWave;
        pEnemyShotSet->x = 240.0;   // アンカー座標はパターン関数内の定数で管理
        pEnemyShotSet->y = PD_ANCHOR_Y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        // 弾セットをグローバルリストへ登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
