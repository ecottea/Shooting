// ============================================================================
//  enemyPat_tmp.cpp
//  弾幕パターン「三平方の定理の証明 ― ユークリッドの風車」
//
//  ▼モチーフ
//    直角三角形の3辺の上に正方形を載せた「風車図」を敵弾で描き、
//    a² + b² = c² を「弾数」で証明して見せる1スペル。
//    a=3, b=4, c=5 の直角三角形を使うので
//    a²の正方形 = 青中玉 9発、b²の正方形 = 緑中玉 16発、
//    その2枚が移動して c²の正方形 = 25発(金色)となり、数えて証明が終わる。
//
//  ▼進行(ショットセットの count 基準)
//    t=0    3頂点に大玉を鎮座(予告音)
//    t=2    三角形の3辺を菱形弾の一筆書きで描画(C→B→A→C)
//    t=90.. 各辺の外側に正方形の枠(小玉を密に敷く)。斜辺の正方形は枠だけ=証明すべき命題
//    t=150  a²の正方形に青9発を敷き詰め / t=165 b²の正方形に緑16発
//    t=185  予告音
//    t=200  直角頂点Cから斜辺へ垂線ビーム(赤銃弾)。c²を2つの長方形に分割
//    t=245  転送開始:9発と16発が直角頂点を経由して c² 内の長方形へ流れ込む
//    t=425  25発がちょうど揃う → 金色に変化(Q.E.D.)
//    t=455  完成した正方形が散開(締め弾幕)
//
//  ▼素材の抜粋(一覧の中から使用したもの)
//    大玉:3頂点         中玉:面積の敷き詰め・分点マーカー・祭り輪
//    小玉:正方形の枠     菱形弾:三角形の辺    銃弾:垂線ビームと自機狙い
//    色:0赤(垂線・雑弾) 1黄(Q.E.D.後のc²) 2緑(b²) 4青(a²)
//       5マゼンタ(三角形の辺) 6白(頂点・枠・分点) 8橙(祭り輪)
//    音:enemyCharge=予告 / light=枠・雑弾 / medium=敷き詰め・垂線・転送
//       heavy=Q.E.D. / extreme=散開
//
//  ▼仕様上の注意
//  ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと、
//    画面外に出た弾の消去はメインルーチンに任せる(本ファイルでは読むだけ)
//  ・静止表示している弾は、t=455 の散開(または敵撃破時)で必ず画面外へ
//    飛び出すので、メインルーチンの画面外消去で回収される
//  ・GetRand(x) は 0〜x の x+1 種を返すため、±3度の揺らぎは GetRand(6)-3、
//    ランダム方位は GetRand(628)/100.0 ラジアンとして使用している
//  ・線分は弾の間隔を弾粒径以下に詰めて描く(点線ではなく線に見えるように)。
//    菱形弾・銃弾など長軸のある弾は muki を進行方向(=線分方向)に合わせる
// ============================================================================

#include "gv.h"
#include <cmath>

#ifndef DX_PI
#define DX_PI 3.14159265358979
#endif

namespace {

    // 色(画像配列の添字 = 色)
    enum {
        COL_RED = 0, COL_YELLOW = 1, COL_GREEN = 2, COL_CYAN = 3, COL_BLUE = 4,
        COL_MAGENTA = 5, COL_WHITE = 6, COL_BLACK = 7, COL_ORANGE = 8
    };

    // pEnemyShot->param_i[0]:弾の役割
    enum { ROLE_FREE = 0, ROLE_VERTEX, ROLE_TRIEDGE, ROLE_SQEDGE, ROLE_FILL, ROLE_MARKER };

    // タイムライン(セットcount基準のフレーム)
    enum {
        T_EDGE = 2,                               // 三角形の一筆書き
        T_SQ_A = 90, T_SQ_B = 108, T_SQ_C = 126, // 正方形の枠(a²→b²→c²)
        T_FILL_A = 150, T_FILL_B = 165,             // 面積の敷き詰め
        T_CUE = 185,                             // 垂線の予告音
        T_BEAM = 200,                             // 垂線ビーム
        T_MARK = 209,                             // 垂線の足Hとc²奥面の分点H'
        T_MOVE = 245,                             // 転送開始
        T_QED = 425,                             // 完成(金色化)
        T_BURST = 455,                             // 散開
    };

    const int EUCLID_CYCLE = 560; // 本スペルの周期(フレーム)

    // 線分を弾で描く間隔。弾粒径以下に詰めて「点線」ではなく「線」に見えるようにする
    const double EDGE_SPACING = 5.0*2; // 三角形の辺(菱形弾:長軸4.5px)
    const double FRAME_SPACING = 3.0*2; // 正方形の枠(小玉:直径2.5px)

    // 効果音再生(鳴りっぱなし防止)
    void PlayS(int handle)
    {
        if (CheckSoundMem(handle)) StopSoundMem(handle);
        PlaySoundMem(handle, DX_PLAYTYPE_BACK);
    }

    double Rad(double deg) { return deg * DX_PI / 180.0; }

    // 弾を生成してセットのリストへ繋ぐ(sample と同じ繋ぎ方)
    sEnemyShot* SpawnShot(sEnemyShotSet* S, double x, double y, double muki, double speed, int kind)
    {
        sEnemyShot* p = new sEnemyShot;
        p->x = x;  p->y = y;  p->muki = muki;  p->speed = speed;  p->kind = kind;
        p->prev = S->pEnemyShotHead->prev;
        p->next = S->pEnemyShotHead;
        S->pEnemyShotHead->prev->next = p;
        S->pEnemyShotHead->prev = p;
        return p;
    }

    // 1フレーム分 (tx,ty) へ接近する。到達したら true
    bool StepToward(sEnemyShot* p, double tx, double ty, double speed)
    {
        double dx = tx - p->x, dy = ty - p->y;
        double d = sqrt(dx * dx + dy * dy);
        p->muki = atan2(dy, dx); // 移動中も常に進行方向を向く
        if (d <= speed) { p->x = tx; p->y = ty; return true; }
        p->x += dx / d * speed;
        p->y += dy / d * speed;
        return false;
    }

} // namespace

// ============================================================================
//  証明の上演そのもの(1つのショットセットで完結するステートマシン)
//  セットの原点 = 直角三角形の直角頂点C
//  param_d[0] = 目盛りu(敷き詰め間隔)  param_d[1] = 反転係数(+1/-1)
//  param_i[0] = 散開済みフラグ
// ============================================================================
static void ShotEuclid(sEnemyShotSet* S)
{
    const int    t = S->count;
    const double u = S->param_d[0];
    const double fl = S->param_d[1];

    // ---- 風車図の幾何(絶対座標) ----------------------------------------
    // 3:4:5 の直角三角形。C=直角頂点、B=横に3u、A=下に4u
    const double Cx = S->x, Cy = S->y;
    const double Bx = S->x + fl * 3.0 * u, By = S->y;
    const double Ax = S->x, Ay = S->y + 4.0 * u;
    const double c = 5.0 * u;                             // 斜辺
    const double bax = (Ax - Bx) / c, bay = (Ay - By) / c; // B→A の単位ベクトル
    double nx = bay, ny = -bax;                             // 斜辺の法線(外向きへ補正)
    if (nx * (Cx - Bx) + ny * (Cy - By) > 0.0) { nx = -nx; ny = -ny; }
    const double BH = 9.0 * u / 5.0;    // B→垂線の足H = a²/c
    const double AH = 16.0 * u / 5.0;   // A→垂線の足  = b²/c

    // ---- 散開(Q.E.D.の締め / 敵撃破時の後片付けも兼ねる) ----------------
    if (S->param_i[0] == 0 && (t >= T_BURST || enemy.hp <= 0)) {
        S->param_i[0] = 1;
        PlayS(sound_enemyShot_extreme);
        const double Ox = Bx + bax * (c / 2.0) + nx * (c / 2.0); // c²の中心
        const double Oy = By + bay * (c / 2.0) + ny * (c / 2.0);
        const double Gx = (Cx + Bx + Ax) / 3.0;                  // 三角形の重心
        const double Gy = (Cy + By + Ay) / 3.0;
        for (sEnemyShot* p = S->pEnemyShotHead->next; p != S->pEnemyShotHead; p = p->next) {
            const bool  fromC2 = (p->param_i[0] == ROLE_FILL || p->param_i[0] == ROLE_MARKER);
            const double cx = fromC2 ? Ox : Gx;
            const double cy = fromC2 ? Oy : Gy;
            const double dx = p->x - cx, dy = p->y - cy;
            const double d = sqrt(dx * dx + dy * dy);
            p->muki = (d < 1.0) ? GetRand(628) / 100.0 : atan2(dy, dx); // GetRand(628)=0..628
            switch (p->param_i[0]) {
            case ROLE_FILL:   p->speed = 4.2 + 0.3 * (p->param_i[3] % 3); break;
            case ROLE_MARKER: p->speed = 4.8; break;
            case ROLE_VERTEX: p->speed = 3.0; break;
            default:          p->speed = 3.4; break;
            }
            p->param_i[0] = ROLE_FREE; // 以後は等速直線で画面外へ(消去はメイン側)
        }
        // 祭りの輪(橙色中玉18発)。GetRand(6)-3 で -3..+3 度の揺らぎ
        // (速度3.2なら最も画面外まで遠い角度でも約150フレームで回収される)
        for (int i = 0; i < 18; i++) {
            SpawnShot(S, Ox, Oy, Rad(i * 20.0) + Rad((double)(GetRand(6) - 3)), 3.2,
                img_enemyShotMediumBall[COL_ORANGE]);
        }
    }

    // ---- タイムライン(各1回だけ発火) ------------------------------------
    // t=0:開演。3頂点に大玉
    if (t == 0) {
        PlayS(sound_enemyCharge);
        const double V[3][2] = { { Cx, Cy }, { Bx, By }, { Ax, Ay } };
        for (int i = 0; i < 3; i++) {
            sEnemyShot* p = SpawnShot(S, V[i][0], V[i][1], 0.0, 0.0,
                img_enemyShotLargeBall[COL_WHITE]);
            p->param_i[0] = ROLE_VERTEX;
            p->param_d[0] = V[i][0];
            p->param_d[1] = V[i][1];
        }
    }

    // t=2:三角形の辺を一筆書き(C→B→A→C)。菱形弾が頂点から頂点へ流れる
    if (t == T_EDGE) {
        const double E[3][4] = { { Cx, Cy, Bx, By }, { Bx, By, Ax, Ay }, { Ax, Ay, Cx, Cy } };
        int delay = 0;
        for (int e = 0; e < 3; e++) {
            const double sx = E[e][0], sy = E[e][1], ex = E[e][2], ey = E[e][3];
            const double len = sqrt((ex - sx) * (ex - sx) + (ey - sy) * (ey - sy));
            const double ux = (ex - sx) / len, uy = (ey - sy) / len;
            const double sp = len / 21.0;         // 1辺を21フレームで描く
            const int    N = (int)(len / EDGE_SPACING + 0.5) - 1; // 中間点の弾数(静止時の間隔≈EDGE_SPACING)
            int gap = (int)(EDGE_SPACING / sp + 0.5); // 出発間隔(停止後の間隔がEDGE_SPACINGになるよう)
            if (gap < 1) gap = 1;
            for (int i = 0; i < N; i++) {
                sEnemyShot* p = SpawnShot(S, sx, sy, atan2(uy, ux), 0.0, img_enemyShotDiamond[COL_MAGENTA]);
                p->param_i[0] = ROLE_TRIEDGE;
                p->param_i[1] = delay + i * gap;        // 出発までの遅延
                p->param_d[0] = sx;
                p->param_d[1] = sy;
                p->param_d[2] = ux;
                p->param_d[3] = uy;
                p->param_d[4] = len * (i + 1) / (N + 1); // 自分の停止位置
                p->param_d[5] = sp;
            }
            delay += 21; // 次の辺は、直前の辺の弾が頂点へ流れ着いてから
        }
    }

    // 正方形の枠:小玉を弾粒径以下の間隔で周に敷き詰める。c²は枠だけで中身が空 = 「証明すべきこと」
    if (t == T_SQ_A || t == T_SQ_B || t == T_SQ_C) {
        double cn[4][2];
        if (t == T_SQ_A) {          // a²:辺CBの外側(上)
            cn[0][0] = Cx;          cn[0][1] = Cy;
            cn[1][0] = Bx;          cn[1][1] = By;
            cn[2][0] = Bx;          cn[2][1] = By - 3.0 * u;
            cn[3][0] = Cx;          cn[3][1] = Cy - 3.0 * u;
        }
        else if (t == T_SQ_B) {   // b²:辺CAの外側(反転側)
            cn[0][0] = Cx;          cn[0][1] = Cy;
            cn[1][0] = Ax;          cn[1][1] = Ay;
            cn[2][0] = Ax - fl * 4.0 * u; cn[2][1] = Ay;
            cn[3][0] = Cx - fl * 4.0 * u; cn[3][1] = Cy;
        }
        else {                    // c²:斜辺BAの外側
            cn[0][0] = Bx;          cn[0][1] = By;
            cn[1][0] = Ax;          cn[1][1] = Ay;
            cn[2][0] = Ax + nx * c;   cn[2][1] = Ay + ny * c;
            cn[3][0] = Bx + nx * c;   cn[3][1] = By + ny * c;
        }
        PlayS(sound_enemyShot_light);
        for (int k = 0; k < 4; k++) {
            const int k2 = (k + 1) % 4;
            const double ex = cn[k2][0] - cn[k][0], ey = cn[k2][1] - cn[k][1];
            const double len = sqrt(ex * ex + ey * ey);
            const double mx = ex / len, my = ey / len;
            const int    N = (int)(len / FRAME_SPACING + 0.5); // 始点を含む。終点は次の辺の始点が受ける
            for (int j = 0; j < N; j++) {
                const double d = len * j / N;
                sEnemyShot* p = SpawnShot(S, cn[k][0] + mx * d, cn[k][1] + my * d,
                    atan2(my, mx), 0.0, img_enemyShotSmallBall[COL_WHITE]);
                p->param_i[0] = ROLE_SQEDGE;
                p->param_d[0] = p->x;
                p->param_d[1] = p->y;
            }
        }
    }

    // 敷き詰め:a²=青9発(3x3)/ b²=緑16発(4x4)。c²への転送先もここで決める
    if (t == T_FILL_A) {
        PlayS(sound_enemyShot_medium);
        for (int i = 0; i < 9; i++) {
            const int ix = i % 3, iy = i / 3;
            const double px = Cx + fl * (ix + 0.5) * u;
            const double py = Cy - (iy + 0.5) * u;
            // 転送先:c²内の「垂線より左の長方形(面積a²)」に3x3で詰め直す
            const double co = BH * (ix + 0.5) / 3.0; // 斜辺に沿った位置
            const double no = c * (iy + 0.5) / 3.0; // 法線方向の位置
            sEnemyShot* p = SpawnShot(S, px, py, 0.0, 0.0, img_enemyShotMediumBall[COL_BLUE]);
            p->param_i[0] = ROLE_FILL;  p->param_i[2] = 0;  p->param_i[3] = i;
            p->param_d[0] = px;                   p->param_d[1] = py;  // 出発地
            p->param_d[2] = Bx + bax * co + nx * no;  p->param_d[3] = By + bay * co + ny * no; // 行き先
            p->param_d[4] = Cx;                   p->param_d[5] = Cy;  // 経由点(直角頂点)
        }
    }
    if (t == T_FILL_B) {
        PlayS(sound_enemyShot_medium);
        for (int i = 0; i < 16; i++) {
            const int ix = i % 4, iy = i / 4;
            const double px = Cx - fl * (ix + 0.5) * u;
            const double py = Cy + (iy + 0.5) * u;
            // 転送先:c²内の「垂線より右の長方形(面積b²)」に4x4で詰め直す
            const double co = BH + AH * (ix + 0.5) / 4.0;
            const double no = c * (iy + 0.5) / 4.0;
            sEnemyShot* p = SpawnShot(S, px, py, 0.0, 0.0, img_enemyShotMediumBall[COL_GREEN]);
            p->param_i[0] = ROLE_FILL;  p->param_i[2] = 0;  p->param_i[3] = 9 + i;
            p->param_d[0] = px;                   p->param_d[1] = py;
            p->param_d[2] = Bx + bax * co + nx * no;  p->param_d[3] = By + bay * co + ny * no;
            p->param_d[4] = Cx;                   p->param_d[5] = Cy;
        }
    }

    // 垂線:直角頂点Cから斜辺へ赤い銃弾ビーム。c²を2つの長方形に分割する
    if (t == T_CUE) PlayS(sound_enemyCharge);
    if (t >= T_BEAM && t < T_BEAM + 180 && t % 5 == 0) { // 毎フレーム発射。弾の並び間隔=速度なので線が繋がって見える
        if (t == T_BEAM) PlayS(sound_enemyShot_medium);
        sEnemyShot* p = SpawnShot(S, Cx, Cy, atan2(ny, nx), 9.0/5, img_enemyShotBullet[COL_RED]);
        p->param_i[0] = ROLE_FREE; // 貫通して画面外へ(消去はメイン側)
    }
    // 垂線の足Hと、c²の奥面にある分点H'を白い中玉でマーク
    if (t == T_MARK) {
        const double hx = Bx + bax * BH, hy = By + bay * BH;
        for (int m = 0; m < 2; m++) {
            sEnemyShot* p = SpawnShot(S, hx + nx * c * m, hy + ny * c * m, 0.0, 0.0,
                img_enemyShotMediumBall[COL_WHITE]);
            p->param_i[0] = ROLE_MARKER;
            p->param_d[0] = p->x;
            p->param_d[1] = p->y;
        }
    }

    // 完成:青9+緑16の25発が揃った時点で金色へ(Q.E.D.)
    if (t == T_QED) {
        PlayS(sound_enemyShot_heavy);
        for (sEnemyShot* p = S->pEnemyShotHead->next; p != S->pEnemyShotHead; p = p->next) {
            if (p->param_i[0] == ROLE_FILL) {
                p->x = p->param_d[2];  p->y = p->param_d[3]; // 万が一輸送中でも強スナップ
                p->param_i[2] = 3;
                p->kind = img_enemyShotMediumBall[COL_YELLOW];
            }
        }
    }

    // ---- 毎フレームの弾の挙動 --------------------------------------------
    for (sEnemyShot* p = S->pEnemyShotHead->next; p != S->pEnemyShotHead; p = p->next) {
        switch (p->param_i[0]) {
        case ROLE_FREE:
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            break;

        case ROLE_TRIEDGE: {   // 辺を一筆書きする弾
            const int d0 = p->param_i[1];
            if (p->count >= d0) {
                double prog = p->param_d[5] * (p->count - d0);
                if (prog > p->param_d[4]) prog = p->param_d[4];
                p->x = p->param_d[0] + p->param_d[2] * prog;
                p->y = p->param_d[1] + p->param_d[3] * prog;
            }
            break;
        }

        case ROLE_FILL: {      // 面積の弾:待機 → 直角頂点経由でc²へ輸送 → 整列
            const int stage = p->param_i[2];
            if (stage == 0) {
                const int dep = (p->param_i[3] < 9)
                    ? T_MOVE + 3 * p->param_i[3]
                    : T_MOVE + 27 + 3 * (p->param_i[3] - 9);
                p->x = p->param_d[0] + 1.5 * sin(p->count * 0.2);  // 待機中は脈打つ
                p->y = p->param_d[1] + 1.5 * cos(p->count * 0.2);
                if (t >= dep) p->param_i[2] = 1;
            }
            else if (stage == 1) {
                if (StepToward(p, p->param_d[4], p->param_d[5], 4.5)) p->param_i[2] = 2;
            }
            else if (stage == 2) {
                if (StepToward(p, p->param_d[2], p->param_d[3], 4.0)) p->param_i[2] = 3;
            }
            else {
                p->x = p->param_d[2] + 1.2 * sin(p->count * 0.25 + p->param_i[3]); // 整列後も微脈動
                p->y = p->param_d[3] + 1.2 * cos(p->count * 0.25 + p->param_i[3]);
            }
            break;
        }

        default:               // VERTEX / SQEDGE / MARKER:その場に静止
            break;
        }
    }
}

// ============================================================================
//  合間の自機狙い3way(講釈の合間の軽いプレッシャー)
// ============================================================================
static void ShotHarass(sEnemyShotSet* S)
{
    if (S->count == 0) {
        PlayS(sound_enemyShot_light);
        for (int i = 0; i < 3; i++) {
            sEnemyShot* p = SpawnShot(S, S->x, S->y, S->muki + (i - 1) * 0.21, 2.7,
                img_enemyShotBullet[COL_RED]);
            p->param_i[0] = ROLE_FREE;
        }
    }
    for (sEnemyShot* p = S->pEnemyShotHead->next; p != S->pEnemyShotHead; p = p->next) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
    }
}

// ============================================================================
//  敵本体
// ============================================================================
void EnemyPat_PythagoreanTheorem_Zai()
{
    static int cycle; // 何周目の「証明」か(図形の大きさと向きを切り替える)

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        cycle = 0;
    }

    // 敵はほとんど動かない(図形はセット原点の固定座標に描かれる)
    enemy.x = 240.0 + 12.0 * sin(count * 0.017);
    enemy.y = 48.0 + 4.0 * sin(count * 0.031);

    // 周期ごとに「証明」を上演する
    if ((count - 1) % EUCLID_CYCLE == 0) {
        sEnemyShotSet* S = new sEnemyShotSet;
        S->count = 0;
        S->patternFunc = ShotEuclid;
        S->x = 240.0;              // 直角頂点C(図形の錨)
        S->y = 130.0;
        S->muki = 0.0;
        S->kind = cycle;
        S->alive = 620;            // 上演+散開弾の回収まで存在を保証
        S->param_d[0] = (cycle % 2 == 0) ? 24.0 : 20.0; // 目盛り(敷き詰め間隔)
        S->param_d[1] = (cycle % 2 == 0) ? 1.0 : -1.0;  // 奇数周期は左右反転
        S->param_i[0] = 0;

        S->pEnemyShotHead = new sEnemyShot;
        S->pEnemyShotHead->prev = S->pEnemyShotHead;
        S->pEnemyShotHead->next = S->pEnemyShotHead;

        S->prev = enemyShotSetHead.prev;
        S->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = S;
        enemyShotSetHead.prev = S;

        cycle++;
    }

    // 合間の自機狙い3way
    if (count >= 100 && count % 50 == 30) {
        sEnemyShotSet* S = new sEnemyShotSet;
        S->count = 0;
        S->patternFunc = ShotHarass;
        S->x = enemy.x;
        S->y = enemy.y + 14.0;
        S->muki = atan2(player.y - S->y, player.x - S->x);
        S->kind = 0;

        S->pEnemyShotHead = new sEnemyShot;
        S->pEnemyShotHead->prev = S->pEnemyShotHead;
        S->pEnemyShotHead->next = S->pEnemyShotHead;

        S->prev = enemyShotSetHead.prev;
        S->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = S;
        enemyShotSetHead.prev = S;
    }
}
