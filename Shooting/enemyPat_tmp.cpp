//=====================================================================
//  葉符「バーンズリーの庭園」 ―― バーンズリーのシダ モチーフ弾幕
//  (enemyPat_barnsleyFern.cpp)
//---------------------------------------------------------------------
//  【コンセプト】
//    バーンズリーのシダ(IFS: 反復関数系フラクタル)を「敵弾の点描」で
//    表現する。葉専用素材は使わず、既存の弾だけで自相似形を作る。
//
//    ボス直下に逆さに吊り下がった巨大シダが出現し、
//      Phase 1 萌出し : カオスゲームで生成した座標へ小玉を置き、
//                       根本(ボス側)から葉先へ「成長」させる
//      Phase 2 揺籃   : 根元を支点にシダ全体が揺れ、葉先から狙い弾
//      Phase 3 落葉   : 小葉の群れ(クラスタ)が1枚ずつ剥がれて翻り落下
//      Phase 4 散華   : 残った幹が葉先側から崩落し、ボスが3wayで締める
//    の後、再び成長から繰り返す(1サイクル約18秒)。
//
//  【使用素材】(enemyPat_sampleForAI.cpp の一覧より抜粋)
//    img_enemyShotSmallBall  小玉(2.5x2.5) … シダの点描(密集配置しても
//                                            隙間が残る最小の弾)
//    img_enemyShotMediumBall 中玉(7.0x7.0) … 茎(幹)だけ太く見せる
//    img_enemyShotBullet     銃弾(5.0x2.0) … 狙い弾/散華(向きが読める)
//    色: 1黄=右大葉 / 2緑=主葉・茎 / 3シアン=左大葉 / 4青=幹(最後まで残る)
//        6白=葉先からの狙い弾 / 0赤=ボス本体の3way
//    音: sound_enemyCharge     … サイクル開始の予告
//        sound_enemyShot_medium… 成長開始
//        sound_enemyShot_light  … 葉先からの狙い弾
//        sound_enemyShot_heavy  … 散華
//
//  【タイムライン】(pEnemyShotSet->count を FERN_CYCLE で割った剰余 t)
//      0- 300  萌出し: 600点を根本→葉先の順に出現(実質2点/フレーム)
//      300-    揺籃  : 120フレームかけて揺れを増幅(以後常時揺れる)
//      360- 880狙い弾: 40フレームごとに生存する葉先から最大8発
//      540- 885落葉  : 15フレームごとに小葉クラスタを1枚剥がす(葉先側から)
//      900- 960散華  : 幹を葉先側から順に自由飛行へ移行 + ボスから3way×2
//      960-1080余韻  → 1080で再成長(ループ)
//
//  【実装上の注意】
//    - count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//      画面外の弾の消去はメインルーチンが行うため、ここでは行わない。
//      (画面外消去に任せられるよう、離脱弾・飛行弾は画面外へ出して捨てる)
//    - GetRand(x) は 0〜x の x+1 種類の整数を返す。
//      ±k が欲しいときは GetRand(2*k) - k、確率は GetRand(999)/1000.0。
//      乱数はすべて GetRand 経由なのでリプレイ再現性が保たれる。
//    - ゲーム画面は 480x480。シダは S=36 倍で高さ約360px・幅約170px。
//    - 弾セットは1ボスにつき1つ。リトライ等で残った旧セットは
//      自動的に崩落させて自然消滅させる(s_activeSet を参照)。
//
//  【難易度調整のヒント】
//    Easy    : FERN_POINT_N 380 / FERN_SWAY_AMP 0.06 / FERN_VOLLEY_INT 60
//              / FERN_VOLLEY_SHOTS 5
//    Normal  : 既定値
//    Lunatic : FERN_POINT_N 850 / FERN_SWAY_AMP 0.16 / FERN_VOLLEY_INT 28
//              / FERN_VOLLEY_SHOTS 12
//    ※ FERN_POINT_N を変えても成長期は FERN_GROW_END で出し切る仕様なので
//      他の定数は変更不要(成長速度が自動で変わる)。
//=====================================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------- チューニング用定数 ----------------
static const int    FERN_POINT_N = 600;    // シダを構成する弾の総点数
static const int    FERN_CYCLE = 1080-300;   // 1サイクルの長さ(フレーム)
static const int    FERN_GROW_END = 300-200;    // 成長期の終了フレーム
static const int    FERN_VOLLEY_START = 360-200;    // 狙い弾の開始フレーム
static const int    FERN_VOLLEY_INT = 40/2;     // 狙い弾の間隔(フレーム)
static const int    FERN_VOLLEY_SHOTS = 8;      // 1回の狙い弾の弾数
static const int    FERN_DETACH_START = 540-200;    // 落葉の開始フレーム
static const int    FERN_DETACH_INT = 15;     // 落葉の間隔(フレーム)
static const int    FERN_BURST_START = 900-200;    // 散華の開始フレーム
static const double FERN_SCALE = 36.0;   // シダ座標→画面ピクセルの倍率
static const double FERN_SWAY_AMP = 0.10;   // 揺れの最大角(rad) 約5.7°
static const double FERN_SWAY_PERIOD = 300.0;  // 揺れの周期(フレーム)
static const double FERN_SIDE_X = 0.3;    // |x|がこれを超える点を「葉」とみなす

// ---------------- サイクル内の状態(ファイルスコープ静的) ----------------
static double s_fx[FERN_POINT_N];    // シダ局所座標 x(右+, 左-, 0=幹)
static double s_fy[FERN_POINT_N];    // シダ局所座標 y(根本0 → 葉先10)
static int    s_part[FERN_POINT_N];  // 生成元: 0:茎 1:主葉 2:左大葉 3:右大葉
static int    s_clst[FERN_POINT_N];  // 落葉クラスタID 0:幹 1..12:左 13..24:右
static int    s_order[FERN_POINT_N]; // 成長順(fy昇順のインデックス)
static int    s_tipIdx[24];          // 葉先候補(各バンドの左右端)の点インデックス
static int    s_tipNum = 0;
static bool   s_detached[32];       // クラスタ離脱済みフラグ
static int    s_detachOrder[32];    // 落葉順に並べたクラスタID
static int    s_detachNum = 0;
static int    s_fernN = 0;      // 生成済み点数
static int    s_spawnIdx = 0;      // 出現済み点数(成長期進行)
static int    s_detachPos = 0;      // 次に落とすクラスタの位置
static sEnemyShotSet* s_activeSet = nullptr;  // 現在アクティブな弾セット

//---------------------------------------------------------------------
// 弾をセットの連結リストへ追加(sample の ShotScatter と同じ方式)
//---------------------------------------------------------------------
static sEnemyShot* FernAddShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* p = new sEnemyShot;
    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
    p->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = p;
    pEnemyShotSet->pEnemyShotHead->prev = p;
    return p;
}

//---------------------------------------------------------------------
// クラスタcidに点が存在するか
//---------------------------------------------------------------------
static bool FernClusterExists(int cid)
{
    for (int i = 0; i < s_fernN; i++) {
        if (s_clst[i] == cid) return true;
    }
    return false;
}

//---------------------------------------------------------------------
// カオスゲームでシダの点群を生成し、成長順・葉先・落葉順を準備する
// (1サイクルごとに呼ぶ。形は毎サイクル微妙に変わる)
//---------------------------------------------------------------------
static void FernGenerateCloud(void)
{
    double x = 0.0, y = 0.0;

    // 過渡状態を捨てるウォームアップ
    for (int i = 0; i < 20; i++) {
        double r = GetRand(999) / 1000.0;   // 0.000〜0.999
        double nx, ny;
        if (r < 0.01) { nx = 0.0;                  ny = 0.16 * y; }
        else if (r < 0.86) { nx = 0.85 * x + 0.04 * y;  ny = -0.04 * x + 0.85 * y + 1.6; }
        else if (r < 0.93) { nx = 0.20 * x - 0.26 * y;  ny = 0.23 * x + 0.22 * y + 1.6; }
        else { nx = -0.15 * x + 0.28 * y; ny = 0.26 * x + 0.24 * y + 0.44; }
        x = nx;  y = ny;
    }

    // 本生成
    s_fernN = 0;
    while (s_fernN < FERN_POINT_N) {
        double r = GetRand(999) / 1000.0;
        int    part;
        double nx, ny;
        if (r < 0.01) {        // 茎 f1(確率1%)
            nx = 0.0;                  ny = 0.16 * y;                    part = 0;
        }
        else if (r < 0.86) { // 主葉 f2(確率85%)
            nx = 0.85 * x + 0.04 * y;  ny = -0.04 * x + 0.85 * y + 1.6;  part = 1;
        }
        else if (r < 0.93) { // 左大葉 f3(確率7%)
            nx = 0.20 * x - 0.26 * y;  ny = 0.23 * x + 0.22 * y + 1.6;   part = 2;
        }
        else {               // 右大葉 f4(確率7%)
            nx = -0.15 * x + 0.28 * y; ny = 0.26 * x + 0.24 * y + 0.44;  part = 3;
        }
        x = nx;  y = ny;
        s_fx[s_fernN] = x;
        s_fy[s_fernN] = y;
        s_part[s_fernN] = part;

        // 落葉クラスタ: 幹から離れた左右の点をy方向バンド(0.9刻み)でグループ化
        int band = (int)((y - 1.5) / 0.9);
        if (band < 0)  band = 0;
        if (band > 11) band = 11;
        if (x < -FERN_SIDE_X) s_clst[s_fernN] = 1 + band;    // 左 1..12
        else if (x > FERN_SIDE_X) s_clst[s_fernN] = 13 + band;   // 右 13..24
        else                       s_clst[s_fernN] = 0;           // 幹(散華まで残る)
        s_fernN++;
    }

    // 成長順: fy昇順(根本→葉先)に挿入ソート → 吊り下がりシダが「成長」して見える
    for (int i = 0; i < FERN_POINT_N; i++) s_order[i] = i;
    for (int i = 1; i < FERN_POINT_N; i++) {
        int    v = s_order[i];
        double ky = s_fy[v];
        int    j = i - 1;
        while (j >= 0 && s_fy[s_order[j]] > ky) { s_order[j + 1] = s_order[j]; j--; }
        s_order[j + 1] = v;
    }

    // 葉先候補: 各yバンドで最も外側にある左右の点(狙い弾の発射位置)
    s_tipNum = 0;
    for (int b = 0; b < 12 && s_tipNum < 24; b++) {
        int minIdx = -1, maxIdx = -1;
        for (int i = 0; i < s_fernN; i++) {
            if (s_clst[i] == 0) continue;
            int band = (int)((s_fy[i] - 1.5) / 0.9);
            if (band < 0)  band = 0;
            if (band > 11) band = 11;
            if (band != b) continue;
            if (minIdx < 0 || s_fx[i] < s_fx[minIdx]) minIdx = i;
            if (maxIdx < 0 || s_fx[i] > s_fx[maxIdx]) maxIdx = i;
        }
        if (minIdx >= 0 && s_tipNum < 24) s_tipIdx[s_tipNum++] = minIdx;
        if (maxIdx >= 0 && s_tipNum < 24) s_tipIdx[s_tipNum++] = maxIdx;
    }

    // 落葉フラグと落葉順(=画面下側=葉先に近いバンドから順に落とす)を初期化
    for (int i = 0; i < 32; i++) s_detached[i] = false;
    s_detachNum = 0;
    s_detachPos = 0;
    for (int b = 11; b >= 0; b--) {
        if (FernClusterExists(1 + b))  s_detachOrder[s_detachNum++] = 1 + b;   // 左
        if (FernClusterExists(13 + b)) s_detachOrder[s_detachNum++] = 13 + b;  // 右
    }
}

//---------------------------------------------------------------------
// クラスタcid(小葉の群れ)をシダから切り離して落葉状態へ移行する
// 群れ内の弾は重心の移動+個々のオフセットの自転で隊列を保ったまま翻る
//---------------------------------------------------------------------
static void FernDetachCluster(sEnemyShotSet* pEnemyShotSet, int cid,
    double baseX, double baseY, double dth)
{
    // 1) 現在の重心を求める
    double cx = 0.0, cy = 0.0;
    int n = 0;
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->param_i[0] == 0 && p->param_i[1] == cid) { cx += p->x; cy += p->y; n++; }
        p = p->next;
    }
    s_detached[cid] = true;
    if (n == 0) return;
    cx /= n;  cy /= n;

    // 2) 群れ全体で共有する運動パラメータ(全弾に同じ値 → 隊列が崩れない)
    //    揺れの角速度 dth を引き継いで、切り離し瞬間の慣性も表現する
    double vx0 = (GetRand(60) - 30) / 100.0 - dth * (cy - baseY);        // -0.30..0.30 ±慣性
    double vy0 = 0.40 + GetRand(40) / 100.0 + dth * (cx - baseX);        //  0.40..0.79 ±慣性
    double frq = 0.05 + GetRand(40) / 1000.0;                            // 翻り角速度 0.050..0.089 rad/f
    double amp = 0.60 + GetRand(60) / 100.0;                             // 翻れ幅 0.60..1.19
    double omg = (0.006 + GetRand(14) / 1000.0)                          // 自転角速度 0.006..0.019 rad/f
        * ((GetRand(1) == 0) ? -1.0 : 1.0);

    // 3) 各弾を落葉弾(param_i[0]==2)へ移行
    p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->param_i[0] == 0 && p->param_i[1] == cid) {
            p->param_i[0] = 2;
            p->param_d[0] = vx0;       // 横方向の基本速度
            p->param_d[1] = vy0;       // 縦方向速度(毎フレーム重力で加速)
            p->param_d[2] = 0.0;       // 翻り位相
            p->param_d[3] = frq;       // 翻り角速度
            p->param_d[4] = amp;       // 翻れ幅
            p->param_d[5] = cx;        // 群れ重心(毎フレーム積分)
            p->param_d[6] = cy;
            p->param_d[7] = omg;       // 自転角速度
            p->param_d[8] = p->x - cx; // 重心からのオフセット(毎フレーム回転)
            p->param_d[9] = p->y - cy;
        }
        p = p->next;
    }
}

//---------------------------------------------------------------------
// シダ弾幕本体(弾セットのパターン関数: メインルーチンから毎フレーム呼ばれる)
// 弾の役割 param_i[0]: 0=シダ着生(アンカー追従) / 1=自由飛行 / 2=落葉
//---------------------------------------------------------------------
static void ShotFernGarden(sEnemyShotSet* pEnemyShotSet)
{
    const bool active = (s_activeSet == pEnemyShotSet);
    const int  t = pEnemyShotSet->count % FERN_CYCLE;

    const double baseX = enemy.x;        // 基準位置はボスに追従させる
    const double baseY = enemy.y + 6.0;
    double th = 0.0, dth = 0.0;          // 揺れ角とその角速度

    if (!active) {
        // リトライ等で取り残された旧セット: 着生弾をその場で崩落させて自然消滅
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) {
                p->param_i[0] = 1;
                p->muki = DX_PI / 2.0;                       // 真下へ
                p->speed = 1.0 + GetRand(100) / 100.0;
            }
            p = p->next;
        }
    }
    else {
        // ---- サイクル替わり: 点群の再生成と状態リセット(予告音) ----
        if (t == 0 || s_fernN == 0) {
            FernGenerateCloud();
            s_spawnIdx = 0;
            s_detachPos = 0;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        if (t == 1) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // ---- 揺れ角(成長終了から120フレームかけて増幅) ----
        if (t >= FERN_GROW_END) {
            double u = (double)(t - FERN_GROW_END);
            double ramp = u / 120.0;
            if (ramp > 1.0) ramp = 1.0;
            th = FERN_SWAY_AMP * ramp * sin(u * 2.0 * DX_PI / FERN_SWAY_PERIOD);
            dth = FERN_SWAY_AMP * ramp * (2.0 * DX_PI / FERN_SWAY_PERIOD)
                * cos(u * 2.0 * DX_PI / FERN_SWAY_PERIOD);
        }

        // ---- Phase 1: 萌出し(根本→葉先の順に出現。総点数に合わせて自動で速度調整) ----
        if (t < FERN_GROW_END) {
            int quota = (t + 1) * s_fernN / FERN_GROW_END;   // このフレームまでの累計出現数
            while (s_spawnIdx < quota) {
                int    pi = s_order[s_spawnIdx++];
                double ox = s_fx[pi] * FERN_SCALE;
                double oy = s_fy[pi] * FERN_SCALE;
                sEnemyShot* p = FernAddShot(pEnemyShotSet);
                p->x = baseX + ox * cos(th) - oy * sin(th);
                p->y = baseY + ox * sin(th) + oy * cos(th);
                p->muki = 0.0;
                p->speed = 0.0;
                p->param_i[0] = 0;              // 着生弾
                p->param_i[1] = s_clst[pi];     // 落葉クラスタID
                p->param_d[0] = s_fx[pi];       // アンカー(シダ局所座標)
                p->param_d[1] = s_fy[pi];
                if (s_part[pi] == 0) p->kind = img_enemyShotMediumBall[2]; // 茎: 中玉・緑
                else if (s_clst[pi] == 0) p->kind = img_enemyShotSmallBall[4];  // 幹: 小玉・青
                else if (s_part[pi] == 2) p->kind = img_enemyShotSmallBall[3];  // 左大葉: 小玉・シアン
                else if (s_part[pi] == 3) p->kind = img_enemyShotSmallBall[1];  // 右大葉: 小玉・黄
                else                      p->kind = img_enemyShotSmallBall[2];  // 主葉: 小玉・緑
            }
        }
    }

    // ---- 全弾の移動(役割ごとに更新。リスト自体は操作しない) ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int role = pShot->param_i[0];
        if (role == 0) {
            // 着生弾: アンカーを基準位置+揺れ角で再配置(アンカーは不変)
            double ox = pShot->param_d[0] * FERN_SCALE;
            double oy = pShot->param_d[1] * FERN_SCALE;
            pShot->x = baseX + ox * cos(th) - oy * sin(th);
            pShot->y = baseY + ox * sin(th) + oy * cos(th);
        }
        else if (role == 1) {
            // 自由飛行弾(狙い弾・散華): 等速直進。画面外はメインルーチンが消去する
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 落葉弾: 群れの重心が漂い、個々のオフセットが自転する
            pShot->param_d[2] += pShot->param_d[3];
            double vx = pShot->param_d[0] + pShot->param_d[4] * sin(pShot->param_d[2]);
            pShot->param_d[1] += 0.012;                     // 重力
            if (pShot->param_d[1] > 1.8) pShot->param_d[1] = 1.8;
            pShot->param_d[5] += vx;
            pShot->param_d[6] += pShot->param_d[1];
            double c = cos(pShot->param_d[7]);
            double s2 = sin(pShot->param_d[7]);
            double ox = pShot->param_d[8];
            double oy = pShot->param_d[9];
            pShot->param_d[8] = ox * c - oy * s2;
            pShot->param_d[9] = ox * s2 + oy * c;
            pShot->x = pShot->param_d[5] + pShot->param_d[8];
            pShot->y = pShot->param_d[6] + pShot->param_d[9];
        }
        pShot = pShot->next;
    }

    if (!active) return;   // 以下の出現処理はアクティブなセットのみ

    // ---- Phase 2: 葉先からの狙い弾(落ちた葉の葉先からは撃てない) ----
    if (t >= FERN_VOLLEY_START && t < FERN_BURST_START
        && (t - FERN_VOLLEY_START) % FERN_VOLLEY_INT == 0 && s_tipNum > 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int k = 0; k < FERN_VOLLEY_SHOTS; k++) {
            int pi = s_tipIdx[GetRand(s_tipNum - 1)];    // 0..s_tipNum-1 を一様選択
            if (s_detached[s_clst[pi]]) continue;
            double ox = s_fx[pi] * FERN_SCALE;
            double oy = s_fy[pi] * FERN_SCALE;
            double sx = baseX + ox * cos(th) - oy * sin(th);
            double sy = baseY + ox * sin(th) + oy * cos(th);
            sEnemyShot* p = FernAddShot(pEnemyShotSet);
            p->x = sx;
            p->y = sy;
            p->muki = atan2(player.y - sy, player.x - sx);
            p->speed = 3.0;
            p->param_i[0] = 1;
            p->kind = img_enemyShotBullet[6];            // 銃弾・白
        }
    }

    // ---- Phase 3: 落葉(小葉クラスタが1枚ずつ剥がれる) ----
    if (t >= FERN_DETACH_START && t < FERN_BURST_START
        && (t - FERN_DETACH_START) % FERN_DETACH_INT == 0
        && s_detachPos < s_detachNum) {
        FernDetachCluster(pEnemyShotSet, s_detachOrder[s_detachPos++], baseX, baseY, dth);
    }

    // ---- Phase 4: 散華(残った幹が葉先側から順に崩落) ----
    if (t >= FERN_BURST_START && t < FERN_BURST_START + 60) {
        double fyTh = 10.5 * (1.0 - (double)(t - FERN_BURST_START) / 60.0);
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0 && pShot->param_d[1] >= fyTh) {
                double dx = pShot->x - baseX;
                double dy = pShot->y - baseY;
                pShot->param_i[0] = 1;                   // 自由飛行へ移行
                pShot->muki = atan2(dy, dx);            // 根本から離れる外向き(下方向)
                pShot->speed = 2.0 + GetRand(150) / 100.0;
            }
            pShot = pShot->next;
        }
    }
    if (t == FERN_BURST_START + 60) {
        // 最終掃引: 幹の根元側の残弾もすべて崩落させる
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                double dx = pShot->x - baseX;
                double dy = pShot->y - baseY;
                pShot->param_i[0] = 1;
                pShot->muki = atan2(dy, dx);
                pShot->speed = 1.0 + GetRand(100) / 100.0;   // ゆっくり下って余韻へ
            }
            pShot = pShot->next;
        }
    }
    if (t == FERN_BURST_START) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    // ボス本体から3way(散華の締め)
    if (t == FERN_BURST_START || t == FERN_BURST_START + 50) {
        double ax = enemy.x;
        double ay = enemy.y + 10.0;
        double aim = atan2(player.y - ay, player.x - ax);
        for (int k = -1; k <= 1; k++) {
            sEnemyShot* p = FernAddShot(pEnemyShotSet);
            p->x = ax;
            p->y = ay;
            p->muki = aim + k * (DX_PI / 12.0);         // ±15°
            p->speed = 4.0;
            p->param_i[0] = 1;
            p->kind = img_enemyShotBullet[0];            // 銃弾・赤
        }
    }
}

//---------------------------------------------------------------------
// 敵本体のパターン: 葉符「バーンズリーの庭園」
//---------------------------------------------------------------------
void EnemyPat_Tmp()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと

        // シダ全体で弾セット1つ(全弾はこのセットのリストで管理する)
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFernGarden;
        pEnemyShotSet->x = enemy.x;      // 初期基準位置(実行時は enemy に追従)
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
        s_activeSet = pEnemyShotSet;
    }
    else {
        // ゆっくり漂い、吊り下がったシダが呼吸するように動く
        enemy.y = 40.0;
        enemy.x = 240.0 + 24.0 * sin((count - 1) * 2.0 * DX_PI / 720.0);
    }
}