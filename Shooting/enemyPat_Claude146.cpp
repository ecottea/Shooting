#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕「分光(ディスパージョン)」
//
//  敵が白い光線(短レーザー)を撃ち、空中で揺れる三角プリズム(菱形弾)に当てる。
//  光線はプリズムを通ると7色(赤・橙・黄・緑・シアン・青・マゼンタ)の扇に分かれ、
//  色ごとに速さが違うので遠くへ行くほど帯が分かれていく。
//  プリズムは1往復して中央へ戻り、その後、第2のプリズムが画面中ほどに現れる。
//  広がった7色の弾は全て第2のプリズムへ曲がりながら収束し、白い小玉になって爆発。
//  最後に7色の中玉リングが広がり、虹の輪になる。
//  1サイクル PH_CYCLE_LEN フレームで繰り返す。
//
//  ShotSetの設計:
//   ・プリズム本体 : 1サイクルに最大2セット(ShotPrismBody)
//   ・光線1発分   : 2フレームごとに1セット(ShotPulse)。白い入射光が到着した瞬間に7色の弾を同じセットへ追加する
//   ・白いリング   : ShotFlash / 最後の虹リング: ShotRainbowRing
//  どのセットも、生成フレーム(count==0)で必ず弾を持つので、空のセットは生まれない。
//  消したい弾は x,y を画面の遥か外へ飛ばして、画面外消去(メインルーチン)に任せている。
// ============================================================

// ---- 配置 ----
static constexpr double PRISM_SRC_X = 240.0;        // 白色光の発射位置
static constexpr double PRISM_SRC_Y = 50.0;
static constexpr double PRISM_CENTER_X = 240.0;     // プリズム1の揺れの中心
static constexpr double PRISM_CENTER_Y = 150.0;
static constexpr double PRISM_SWING_W = 140.0;      // プリズム1の左右の振れ幅
static constexpr double PRISM_R = 32.0;             // プリズム1(三角形)の外接円半径
static constexpr double PRISM_ENTRY_OFFSET = 18.0;  // 白色光が消える位置(プリズム中心から手前側)
static constexpr double PRISM_EXIT_OFFSET = 24.0;   // 7色光が出てくる位置(プリズム中心から出口側)
static constexpr double PRISM2_X = 240.0;           // 第2のプリズム(収束点)
static constexpr double PRISM2_Y = 290.0;
static constexpr double PRISM2_R = 24.0;
static constexpr double PRISM_SPIN = 0.02;          // プリズム本体の自転(rad/frame)
static constexpr int    PRISM_EDGE_N = 16;          // 三角形の1辺あたりの菱形弾の数

// ---- 光線 ----
static constexpr double PRISM_WHITE_SPEED = 9.0;                    // 白い入射光の速さ
static constexpr double PRISM_FAN_STEP = 8.5 * DX_PI / 180.0;       // 隣り合う色の角度差
static constexpr double PRISM_SPEED_RED = 3.8;                      // 赤の速さ(最速)
static constexpr double PRISM_SPEED_STEP = 0.28;                    // 色が1つ進むごとに遅くなる量
static constexpr double PRISM_BULLET_SIDE = 5.0;                    // 銃弾を光線の左右にずらす距離
// 色の並び: 赤, 橙, 黄, 緑, シアン, 青, マゼンタ(紫の代わり)
static const int PRISM_COLOR[7] = { 0, 8, 1, 2, 3, 4, 5 };
static constexpr int PRISM_WHITE = 6;

// ---- 収束 ----
static constexpr double PRISM_STEER_SPEED = 4.5;    // 収束中の目標速度
static constexpr double PRISM_STEER_BLEND = 0.06;   // 速度ベクトルを目標へ寄せる割合/frame
static constexpr double PRISM_CAPTURE_R = 10.0;     // 収束点からこの距離以内で白い小玉に変わる

// ---- 時間割(1サイクル内のフレーム) ----
static constexpr int PH_ASSEMBLE_LEN = 60;                  // プリズム1が組み上がるまで
static constexpr int PH_DISPERSE = 90;                      // 分光開始
static constexpr int PH_SWING_LEN = 420;                    // プリズム1が1往復する時間
static constexpr int PH_EMIT_LEN = 400;                     // 光線を撃ち続ける時間
static constexpr int PH_CONVERGE = PH_DISPERSE + PH_SWING_LEN;  // 第2プリズム登場
static constexpr int PH_STEER = PH_CONVERGE + 20;           // 7色弾が収束し始める(プリズム1が消える)
static constexpr int PH_FINALE = PH_CONVERGE + 160;         // 虹のリング
static constexpr int PH_CYCLE_LEN = PH_FINALE + 150;        // 1サイクルの長さ

// ============================================================
//  共通ヘルパー
// ============================================================
static double Prism_Dist(double dx, double dy)
{
    return sqrt(dx * dx + dy * dy);
}

// ShotSetを作ってメインのリストにつなぐ
static sEnemyShotSet* Prism_NewSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// ShotSetの末尾に弾を1つ追加する
static sEnemyShot* Prism_AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->margin = 80;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 全ての弾を向きと速さのまま直進させる
static void Prism_MoveStraight(sEnemyShotSet* pSet)
{
    sEnemyShot* pHead = pSet->pEnemyShotHead;
    for (sEnemyShot* pShot = pHead->next; pShot != pHead; pShot = pShot->next) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
    }
}

static void Prism_PlaySound(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ---- プリズム1の動き(分光フェーズ内の時刻 tp の関数) ----
// tp=0で中央から右へ動き始め、PH_SWING_LEN で1往復して中央に戻る
static double Prism_SwingAngle(double tp)
{
    if (tp < 0.0) tp = 0.0;
    if (tp > PH_SWING_LEN) tp = PH_SWING_LEN;
    return 2.0 * DX_PI * tp / (double)PH_SWING_LEN;
}

static void Prism_Center(double tp, double* px, double* py)
{
    *px = PRISM_CENTER_X + PRISM_SWING_W * sin(Prism_SwingAngle(tp));
    *py = PRISM_CENTER_Y;
}

// 白色光がプリズムに入る点(中心から発射位置側へ少し戻った位置)
static void Prism_Entry(double tp, double* ex, double* ey)
{
    double px, py;
    Prism_Center(tp, &px, &py);
    const double dx = px - PRISM_SRC_X;
    const double dy = py - PRISM_SRC_Y;
    const double d = Prism_Dist(dx, dy);
    *ex = px - dx / d * PRISM_ENTRY_OFFSET;
    *ey = py - dy / d * PRISM_ENTRY_OFFSET;
}

// ============================================================
//  弾幕：プリズム本体(白い菱形弾の三角形 + 頂点の中玉)
//   param_i[0]: 0=プリズム1, 1=プリズム2
//   param_i[1]: サイクル開始count   param_i[2]: 組み上げ開始(サイクル内時刻)
//   param_i[3]: 組み上げにかける時間  param_i[4]: 消える時刻(サイクル内時刻)
//   param_i[5]: 生成時のcount        param_d[0]: 外接円半径
//  弾ごと param_i[0]: 0=辺の菱形弾, 1=頂点の中玉   param_i[1]: 辺の番号   param_i[2]: 辺上の位置
//        param_i[15]: 9=消去済み     param_d[0],[1]: 組み上げ前の位置
// ============================================================
static void ShotPrismBody(sEnemyShotSet* pEnemyShotSet)
{
    const int which = pEnemyShotSet->param_i[0];
    const int cycleStart = pEnemyShotSet->param_i[1];
    const int asmStart = pEnemyShotSet->param_i[2];
    const int asmLen = pEnemyShotSet->param_i[3];
    const int vanishAt = pEnemyShotSet->param_i[4];
    const int createdAt = pEnemyShotSet->param_i[5];
    const double radius = pEnemyShotSet->param_d[0];
    const int tc = createdAt + pEnemyShotSet->count - cycleStart;   // サイクル内の時刻

    if (pEnemyShotSet->count == 0) {
        for (int s = 0; s < 3; s++) {
            for (int j = 0; j < PRISM_EDGE_N; j++) {
                // 画面内のランダムな位置から飛んできて三角形になる(画面外だとすぐ消されるので画面内に置く)
                const double sx = 20 + GetRand(440);
                const double sy = 20 + GetRand(280);
                sEnemyShot* pEdge = Prism_AddShot(pEnemyShotSet, sx, sy, 0.0, 0.0, img_enemyShotDiamond[PRISM_WHITE]);
                pEdge->param_i[0] = 0;
                pEdge->param_i[1] = s;
                pEdge->param_i[2] = j;
                pEdge->param_d[0] = sx;
                pEdge->param_d[1] = sy;
            }
            const double vsx = 20 + GetRand(440);
            const double vsy = 20 + GetRand(280);
            sEnemyShot* pVertex = Prism_AddShot(pEnemyShotSet, vsx, vsy, 0.0, 0.0, img_enemyShotMediumBall[PRISM_WHITE]);
            pVertex->param_i[0] = 1;
            pVertex->param_i[1] = s;
            pVertex->param_d[0] = vsx;
            pVertex->param_d[1] = vsy;
        }
    }

    // プリズムの中心と向き
    double cx, cy, phi;
    if (which == 0) {
        Prism_Center(tc - PH_DISPERSE, &cx, &cy);
        phi = -DX_PI * 0.5 + PRISM_SPIN * tc;
    }
    else {
        cx = PRISM2_X;
        cy = PRISM2_Y;
        phi = DX_PI * 0.5 - PRISM_SPIN * 2.0 * (tc - asmStart);   // プリズム2は逆向きに速く回る
    }
    double corX[3], corY[3];
    for (int s = 0; s < 3; s++) {
        const double a = phi + s * 2.0 * DX_PI / 3.0;
        corX[s] = cx + radius * cos(a);
        corY[s] = cy + radius * sin(a);
    }

    // 組み上げの進み具合(0→1, なめらか)
    double u = (double)(tc - asmStart) / (double)asmLen;
    if (u < 0.0) u = 0.0;
    if (u > 1.0) u = 1.0;
    const double e = u * u * (3.0 - 2.0 * u);

    const bool vanish = (tc >= vanishAt);
    sEnemyShot* pHead = pEnemyShotSet->pEnemyShotHead;
    for (sEnemyShot* pShot = pHead->next; pShot != pHead; pShot = pShot->next) {
        if (pShot->param_i[15] == 9) continue;
        if (vanish) {
            // 画面外へ飛ばして、画面外消去に任せる
            pShot->x = -10000.0;
            pShot->y = -10000.0;
            pShot->param_i[15] = 9;
            continue;
        }
        const int s = pShot->param_i[1];
        const int s2 = (s + 1) % 3;
        double tx, ty;
        if (pShot->param_i[0] == 0) {
            const double r = (double)pShot->param_i[2] / (double)PRISM_EDGE_N;
            tx = corX[s] + (corX[s2] - corX[s]) * r;
            ty = corY[s] + (corY[s2] - corY[s]) * r;
            pShot->muki = atan2(corY[s2] - corY[s], corX[s2] - corX[s]);    // 辺の向き
        }
        else {
            tx = corX[s];
            ty = corY[s];
            pShot->muki = phi + s * 2.0 * DX_PI / 3.0;
        }
        pShot->x = pShot->param_d[0] + (tx - pShot->param_d[0]) * e;
        pShot->y = pShot->param_d[1] + (ty - pShot->param_d[1]) * e;
    }
}

// ============================================================
//  弾幕：光線1発分(白い入射光 → 到着した瞬間に7色の光線が出る)
//   セット param_i[0]: 白い入射光が到着する呼び出し番号(count)
//                [1]: 生成時のcount  [2]: 光線の通し番号  [3]: サイクル開始count
//   弾ごと param_i[15]: 0=7色光線(収束で曲がる), 1=白い入射光, 2=白い小玉(直進), 9=消去済み
// ============================================================

// 7色の弾を1発、収束点へ寄せる。収束点に着いたらtrueを返す
static bool Prism_Steer(sEnemyShot* pShot)
{
    const double dx = PRISM2_X - pShot->x;
    const double dy = PRISM2_Y - pShot->y;
    const double d = Prism_Dist(dx, dy);
    if (d < PRISM_CAPTURE_R) return true;

    double vx = pShot->speed * cos(pShot->muki);
    double vy = pShot->speed * sin(pShot->muki);
    const double tvx = dx / d * PRISM_STEER_SPEED;
    const double tvy = dy / d * PRISM_STEER_SPEED;
    vx += (tvx - vx) * PRISM_STEER_BLEND;
    vy += (tvy - vy) * PRISM_STEER_BLEND;
    pShot->muki = atan2(vy, vx);        // 曲がった向きに合わせて絵も回す
    pShot->speed = Prism_Dist(vx, vy);
    pShot->x += vx;
    pShot->y += vy;
    return false;
}

static void ShotPulse(sEnemyShotSet* pEnemyShotSet)
{
    const int createdAt = pEnemyShotSet->param_i[1];
    const int pulseIdx = pEnemyShotSet->param_i[2];
    const int cycleStart = pEnemyShotSet->param_i[3];
    const int t0 = createdAt - cycleStart - PH_DISPERSE;    // 生成時の、分光フェーズ内の時刻

    if (pEnemyShotSet->count == 0) {
        // プリズムは動いているので、到着時刻のプリズム位置を繰り返し計算して、何フレームで届くかを決める
        int n = 1;
        for (int it = 0; it < 4; it++) {
            double tx, ty;
            Prism_Entry(t0 + n - 1, &tx, &ty);
            n = (int)ceil(Prism_Dist(tx - PRISM_SRC_X, ty - PRISM_SRC_Y) / PRISM_WHITE_SPEED);
            if (n < 1) n = 1;
        }
        double tx, ty;
        Prism_Entry(t0 + n - 1, &tx, &ty);
        const double dist = Prism_Dist(tx - PRISM_SRC_X, ty - PRISM_SRC_Y);
        pEnemyShotSet->param_i[0] = n - 1;      // n回動いた呼び出し(count==n-1)で目標点に着く

        sEnemyShot* pWhite = Prism_AddShot(pEnemyShotSet, PRISM_SRC_X, PRISM_SRC_Y,
            atan2(ty - PRISM_SRC_Y, tx - PRISM_SRC_X), dist / n, img_enemyShotLaser[PRISM_WHITE]);
        pWhite->param_i[15] = 1;

        if (pulseIdx % 4 == 0) Prism_PlaySound(sound_enemyShot_light);
    }

    const int tc = createdAt + pEnemyShotSet->count - cycleStart;
    const bool steering = (tc >= PH_STEER);
    bool arrived = false;

    sEnemyShot* pHead = pEnemyShotSet->pEnemyShotHead;
    for (sEnemyShot* pShot = pHead->next; pShot != pHead; pShot = pShot->next) {
        switch (pShot->param_i[15]) {
        case 1:     // 白い入射光
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pEnemyShotSet->count >= pEnemyShotSet->param_i[0]) {
                // プリズムに入った。画面外へ飛ばして消す
                arrived = true;
                pShot->x = -10000.0;
                pShot->y = -10000.0;
                pShot->speed = 0.0;
                pShot->param_i[15] = 9;
            }
            break;
        case 9:     // 消去済み
            break;
        case 2:     // 白い小玉(収束後)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        default:    // 7色光線
            if (steering) {
                if (Prism_Steer(pShot)) {
                    // 第2のプリズムに着いた: 白い小玉になって四方へ散る
                    pShot->x = PRISM2_X;
                    pShot->y = PRISM2_Y;
                    pShot->kind = img_enemyShotSmallBall[PRISM_WHITE];
                    pShot->muki = GetRand(3599) / 3600.0 * 2.0 * DX_PI;
                    pShot->speed = 1.2 + GetRand(220) / 100.0;
                    pShot->param_i[15] = 2;
                }
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            break;
        }
    }

    if (arrived) {
        // 白い光がプリズムを通り抜けた: 到着時刻のプリズムの出口から7色の扇を出す
        const int tpA = t0 + pEnemyShotSet->param_i[0];
        double px, py;
        Prism_Center(tpA, &px, &py);
        const double thIn = atan2(py - PRISM_SRC_Y, px - PRISM_SRC_X);
        for (int i = 0; i < 7; i++) {
            const double th = thIn + (i - 3) * PRISM_FAN_STEP;
            const double cs = cos(th);
            const double sn = sin(th);
            const double spd = PRISM_SPEED_RED - i * PRISM_SPEED_STEP;
            const double sx = px + cs * PRISM_EXIT_OFFSET;
            const double sy = py + sn * PRISM_EXIT_OFFSET;
            Prism_AddShot(pEnemyShotSet, sx, sy, th, spd, img_enemyShotLaser[PRISM_COLOR[i]]);
            if (pulseIdx % 3 == 0) {
                // 3発に1回、光線の両脇に銃弾を添えて太く見せる
                Prism_AddShot(pEnemyShotSet, sx - sn * PRISM_BULLET_SIDE, sy + cs * PRISM_BULLET_SIDE, th, spd,
                    img_enemyShotBullet[PRISM_COLOR[i]]);
                Prism_AddShot(pEnemyShotSet, sx + sn * PRISM_BULLET_SIDE, sy - cs * PRISM_BULLET_SIDE, th, spd,
                    img_enemyShotBullet[PRISM_COLOR[i]]);
            }
        }
    }
}

// ============================================================
//  弾幕：プリズムの出口から散る白い小玉のリング(2層)
//   pEnemyShotSet->muki: リングの回転オフセット
// ============================================================
static void ShotFlash(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        Prism_PlaySound(sound_enemyShot_medium);
        const int n = 48;
        const double step = 2.0 * DX_PI / n;
        for (int layer = 0; layer < 2; layer++) {
            for (int j = 0; j < n; j++) {
                const double ang = pEnemyShotSet->muki + step * j + step * 0.5 * layer;
                Prism_AddShot(pEnemyShotSet,
                    pEnemyShotSet->x + 10.0 * cos(ang), pEnemyShotSet->y + 10.0 * sin(ang),
                    ang, 1.8 + 0.9 * layer, img_enemyShotSmallBall[PRISM_WHITE]);
            }
        }
    }
    Prism_MoveStraight(pEnemyShotSet);
}

// ============================================================
//  弾幕：虹のリング(7色の中玉リング + 白い大玉の核)
//   速さが色ごとに違うので、広がるほど赤が内側・マゼンタが外側の虹の輪になる
// ============================================================
static void ShotRainbowRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        Prism_PlaySound(sound_enemyShot_extreme);
        for (int k = 0; k < 7; k++) {
            const int n = 36 + 6 * k;
            const double step = 2.0 * DX_PI / n;
            const double offset = (k % 2) * step * 0.5 + k * 0.05;  // 隣のリングと半歩ずらして、すき間の通り道を作る
            const double spd = 1.4 + 0.45 * k;
            for (int j = 0; j < n; j++) {
                const double ang = offset + step * j;
                Prism_AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, ang, spd,
                    img_enemyShotMediumBall[PRISM_COLOR[k]]);
            }
        }
        for (int j = 0; j < 8; j++) {
            const double ang = DX_PI / 8.0 + j * DX_PI / 4.0;
            Prism_AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, ang, 1.6,
                img_enemyShotLargeBall[PRISM_WHITE]);
        }
    }
    Prism_MoveStraight(pEnemyShotSet);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Prism_Claude()
{
    static int cycleStart;  // 現在のサイクルが始まったcount
    static int pulseIdx;    // サイクル内の光線の通し番号

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.maxHp = enemy.hp = 200;   // 200から変更しないこと
        cycleStart = 1;
        pulseIdx = 0;
    }
    enemy.x = 240.0;
    enemy.y = 40.0;

    if (count - cycleStart >= PH_CYCLE_LEN) {
        cycleStart = count;
        pulseIdx = 0;
    }
    const int tc = count - cycleStart;  // サイクル内の時刻

    // ① 予告: プリズム1を組み上げる
    if (tc == 0) {
        Prism_PlaySound(sound_enemyCharge);
        sEnemyShotSet* pBody = Prism_NewSet(ShotPrismBody, 0.0, 0.0, 0.0);
        pBody->param_i[0] = 0;
        pBody->param_i[1] = cycleStart;
        pBody->param_i[2] = 0;
        pBody->param_i[3] = PH_ASSEMBLE_LEN;
        pBody->param_i[4] = PH_STEER;       // 収束が始まるころに消える
        pBody->param_i[5] = count;
        pBody->param_d[0] = PRISM_R;
    }

    // ② 分光: 2フレームごとに白い光線を1発撃つ
    const int tp = tc - PH_DISPERSE;        // 分光フェーズ内の時刻
    if (tp >= 0 && tp < PH_EMIT_LEN) {
        if (tp % 5 == 0) {
            sEnemyShotSet* pPulse = Prism_NewSet(ShotPulse, PRISM_SRC_X, PRISM_SRC_Y, 0.0);
            pPulse->param_i[1] = count;
            pPulse->param_i[2] = pulseIdx++;
            pPulse->param_i[3] = cycleStart;
        }
        // 100フレームごとに、プリズムから白い小玉のリングを散らす
        if (tp % 100 == 50) {
            double px, py;
            Prism_Center(tp, &px, &py);
            const double offset = GetRand(99) / 100.0 * (2.0 * DX_PI / 48.0);
            Prism_NewSet(ShotFlash, px, py, offset);
        }
    }

    // ③ 収束: 第2のプリズムを組み上げる
    if (tc == PH_CONVERGE) {
        Prism_PlaySound(sound_enemyShot_heavy);
        sEnemyShotSet* pBody2 = Prism_NewSet(ShotPrismBody, 0.0, 0.0, 0.0);
        pBody2->param_i[0] = 1;
        pBody2->param_i[1] = cycleStart;
        pBody2->param_i[2] = PH_CONVERGE;
        pBody2->param_i[3] = 40;
        pBody2->param_i[4] = PH_FINALE;     // 虹のリングが出る瞬間に消える
        pBody2->param_i[5] = count;
        pBody2->param_d[0] = PRISM2_R;
    }

    // ④ フィナーレ: 虹のリング
    if (tc == PH_FINALE) {
        Prism_NewSet(ShotRainbowRing, PRISM2_X, PRISM2_Y, 0.0);
    }
}