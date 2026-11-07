// EnemyPat_PythagoreanTheorem_MetaAI.cpp
// 三平方の定理の証明「並べ替えの証明」をモチーフにした弾幕パターン
//
// 証明の流れ:
//   1辺 a+b の大正方形の中に、直角三角形4つを置くと、真ん中に c^2 の正方形ができる
//   三角形4つを並べ替えると、残りが a^2 の正方形 + b^2 の正方形になる
//   よって a^2 + b^2 = c^2
//
// 幾何設定 (3:4:5 の直角三角形を拡大):
//   a = 90, b = 120, c = 150, 大正方形の1辺 = a+b = 210
//   中心 (240, 260), 大正方形は x:135-345, y:155-365
//   ゲーム画面は 480x480
//
// タイムライン (pEnemyShotSet->count 基準, 1セット約900フレームでループ):
//   0-59    : 出現。大正方形のガイド(短レーザー)表示 + 三角形が中心から広がって配置
//   60-359  : 保持 + 全体がゆっくり回転 (c^2 の状態) + 四隅からの自機狙い攻撃
//   360-479 : 並べ替え。三角形がスライドして a^2 + b^2 の状態へ。360で予告音
//   480-779 : 保持 + 逆回転で元の角度に戻る (a^2 + b^2 の状態) + 四隅からの自機狙い攻撃
//   780-899 : 退場。全弾が中心から外向きに飛んで画面外へ消える
//
// 回転中の攻撃 (安全すぎる問題への対策):
//   回転だけだと、図形と一緒に回れば避けられてしまう。そこで回転フェーズ限定で、
//   大正方形の四隅を砲台として、24フレームごとに各隅から自機狙いの小玉を1発ずつ撃つ。
//   四隅自体も図形と一緒に回転するため発射位置が常に変わり、ただ回るだけでは避けられない。
//   狙いは自機の現在位置 + 交互にずらす小さい角度 (-0.10 / 0 / +0.10 rad) で、
//   立ち止まっていると当たるが、動き続ければ避けられるバランスにした。
//   攻撃弾は図形の弾と区別するため橙の小玉にし、直進のみで図形の回転には連動しない。
//   並べ替え中は三角形の動きに集中してほしいので攻撃は止める。
//             ※ count, pEnemyShotSet->count, pEnemyShot->count の加算と
//                画面外の弾の消去はメインルーチン側で行われるため、ここでは行わない
//
// 素材選び:
//   三角形の辺 : 小玉(2.5x2.5) ... 円なので向きに依存せず、直線状に並べると辺に見える。
//                間隔10で点線にすることで、プレイヤーが隙間を抜けられる。
//                三角形ごとに色を変えた (0:赤, 1:黄, 2:緑, 3:シアン) ので、並べ替えで
//                どの三角形がどこへ動いたか追える。
//   頂点 : 中玉(7.0x7.0) 白 ... 辺の端点だけ大きくして、正方形の頂点を強調。
//          頂点は辺の弾と同じ位置計算なので、並べ替え後も自動で a^2 / b^2 の頂点になる。
//   大正方形 : 短レーザー(64.0x4.0) 白 ... 長いので1辺を4本でつなげると実線に見える。
//              ただし実線の壁だとプレイヤーが閉じ込められるため、0-59のガイド専用とし、
//              60以降は外向きに飛ばして消す。常設の壁にはしない。
//   回転中の攻撃弾 : 小玉(2.5x2.5) 橙 ... 図形の弾 (赤黄緑シアン・白) と色で区別できる。
//                橙は「危険」の合図として目立つ。直進弾なので muki / speed で動かす。
//   使わなかったもの:
//     大玉(20.0x20.0) ... 大きすぎて証明の図形が潰れる
//     銃弾 / 鱗弾 / 菱形弾 / 中楕円弾 ... 方向性のある形は回転で見た目が崩れ、幾何図形に不向き
//
// GetRand について:
//   GetRand(x) は 0 以上 x 以下の x+1 種類を返すため、座標計算に使うと
//   証明の図形が崩れる。ここでは幾何位置に乱数を一切使わず、完全固定配置にした。
//   そのため GetRand はこのファイルでは呼ばない。

#include "DxLib.h"
#include "gv.h"
#include <cmath>

static const double PYTH_A = 90.0;    // 直角三角形の短い辺
static const double PYTH_B = 120.0;   // 直角三角形の長い辺
static const double PYTH_C = 150.0;   // 斜辺 (90-120-150 は 3:4:5)
static const double PYTH_HALF = 105.0; // (a+b)/2

// 三角形の頂点を求める
// tri: 0-3, cfg: 0=初期配置(c^2), 1=並べ替え後(a^2+b^2)
// v0:直角の頂点, v1:脚Aの端点, v2:脚Bの端点 (中心原点のローカル座標)
static void GetTriVerts(int tri, int cfg, double v0[2], double v1[2], double v2[2])
{
    const double H = PYTH_HALF;
    const double A = PYTH_A;
    const double B = PYTH_B;
    if (cfg == 0) {
        switch (tri) {
        case 0: // 左上
            v0[0] = -H;     v0[1] = -H;
            v1[0] = -H + A; v1[1] = -H;
            v2[0] = -H;     v2[1] = -H + B;
            break;
        case 1: // 右上
            v0[0] = H;     v0[1] = -H;
            v1[0] = H - B; v1[1] = -H;
            v2[0] = H;     v2[1] = -H + A;
            break;
        case 2: // 右下
            v0[0] = H;     v0[1] = H;
            v1[0] = H;     v1[1] = H - B;
            v2[0] = H - A; v2[1] = H;
            break;
        default: // 左下
            v0[0] = -H;     v0[1] = H;
            v1[0] = -H + B; v1[1] = H;
            v2[0] = -H;     v2[1] = H - A;
            break;
        }
    }
    else {
        switch (tri) {
        case 0: // 右上の長方形の上側三角形へスライド
            v0[0] = H;     v0[1] = -H;
            v1[0] = H - A; v1[1] = -H;
            v2[0] = H;     v2[1] = -H + B;
            break;
        case 1: // 右上の長方形の下側三角形へスライド
            v0[0] = H - A; v0[1] = H - A;
            v1[0] = H - A; v1[1] = -H;
            v2[0] = H;     v2[1] = H - A;
            break;
        case 2: // 左下の長方形の上側三角形へスライド
            v0[0] = H - A; v0[1] = H - A;
            v1[0] = -H;    v1[1] = H - A;
            v2[0] = H - A; v2[1] = H;
            break;
        default: // 左下はそのままアンカーとして残る (I3 と F3 は同一)
            v0[0] = -H;     v0[1] = H;
            v1[0] = -H + B; v1[1] = H;
            v2[0] = -H;     v2[1] = H - A;
            break;
        }
    }
}

// 三角形ごとの脚の長さ。初期も並べ替え後も同じ長さを保つ。
static void GetTriLens(int tri, double* pLenA, double* pLenB)
{
    if (tri == 0) {
        *pLenA = PYTH_A;
        *pLenB = PYTH_B;
    }
    else {
        *pLenA = PYTH_B;
        *pLenB = PYTH_A;
    }
}

// 三角形の周上、直角頂点から距離 dist の点 (ローカル座標、並べ替え補間 s 付き)
// 周長 = a + b + c = 360。弾は 10 間隔で 36 個 / 三角形。
static void CalcTriLocal(int tri, double dist, double s, double out[2])
{
    double v0a[2], v1a[2], v2a[2];
    double v0b[2], v1b[2], v2b[2];
    GetTriVerts(tri, 0, v0a, v1a, v2a);
    GetTriVerts(tri, 1, v0b, v1b, v2b);

    double v0[2], v1[2], v2[2];
    for (int i = 0; i < 2; i++) {
        v0[i] = v0a[i] * (1.0 - s) + v0b[i] * s;
        v1[i] = v1a[i] * (1.0 - s) + v1b[i] * s;
        v2[i] = v2a[i] * (1.0 - s) + v2b[i] * s;
    }

    double lenA = 0.0, lenB = 0.0;
    GetTriLens(tri, &lenA, &lenB);

    double t = 0.0;
    if (dist < lenA) {
        t = dist / lenA;
        out[0] = v0[0] + (v1[0] - v0[0]) * t;
        out[1] = v0[1] + (v1[1] - v0[1]) * t;
    }
    else if (dist < lenA + PYTH_C) {
        t = (dist - lenA) / PYTH_C;
        out[0] = v1[0] + (v2[0] - v1[0]) * t;
        out[1] = v1[1] + (v2[1] - v1[1]) * t;
    }
    else {
        t = (dist - lenA - PYTH_C) / lenB;
        out[0] = v2[0] + (v0[0] - v2[0]) * t;
        out[1] = v2[1] + (v0[1] - v2[1]) * t;
    }
}

static double SmoothStep(double t)
{
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t * t * (3.0 - 2.0 * t);
}

// 弾幕: 三平方の定理 (並べ替えの証明)
static void ShotPythagoras(sEnemyShotSet* pEnemyShotSet)
{
    const int c = pEnemyShotSet->count;
    const double cx = pEnemyShotSet->x;
    const double cy = pEnemyShotSet->y;
    const double dir = (pEnemyShotSet->kind % 2 == 0) ? 1.0 : -1.0;

    if (c == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 三角形4つ: 周長360を10間隔で36個ずつ = 144個
        const int triColor[4] = { 0, 1, 2, 3 }; // 赤, 黄, 緑, シアン
        for (int tri = 0; tri < 4; tri++) {
            double lenA = 0.0, lenB = 0.0;
            GetTriLens(tri, &lenA, &lenB);
            for (int k = 0; k < 36; k++) {
                double dist = k * 10.0;
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx;
                pEnemyShot->y = cy;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->param_i[0] = 0; // 0:三角形の辺の弾
                pEnemyShot->param_i[1] = tri;
                pEnemyShot->param_d[0] = dist;

                // 頂点 (dist が 0 / lenA / lenA+c) だけ中玉・白で強調
                bool isVertex = (dist == 0.0) || (dist == lenA) || (dist == lenA + PYTH_C);
                if (isVertex) {
                    pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白
                }
                else {
                    pEnemyShot->kind = img_enemyShotSmallBall[triColor[tri]];
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 大正方形のガイド: 1辺210を短レーザー4本でつなぐ = 16本
        for (int side = 0; side < 4; side++) {
            for (int j = 0; j < 4; j++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx;
                pEnemyShot->y = cy;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->param_i[0] = 1; // 1:大正方形のガイドレーザー
                pEnemyShot->param_i[1] = side;
                pEnemyShot->param_d[0] = (double)j;
                pEnemyShot->kind = img_enemyShotLaser[6]; // 白

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 並べ替え開始の予告音
    if (c == 360) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // フェーズごとの回転角 theta と並べ替え補間 s と出現/退場係数
    double theta = 0.0;
    double s = 0.0;
    double appearEase = 1.0;
    double exitDist = 0.0;
    if (c < 60) {
        double e = c / 60.0;
        appearEase = 1.0 - (1.0 - e) * (1.0 - e);
        theta = 0.0;
        s = 0.0;
    }
    else if (c < 360) {
        theta = (c - 60) * 0.002 * dir;
        s = 0.0;
    }
    else if (c < 480) {
        theta = 300 * 0.002 * dir;
        s = SmoothStep((c - 360) / 120.0);
    }
    else if (c < 780) {
        theta = (300 * 0.002 - (c - 480) * 0.002) * dir;
        s = 1.0;
    }
    else {
        theta = 0.0;
        s = 1.0;
        exitDist = (c - 780) * 2.0;
    }

    const double cosT = cos(theta);
    const double sinT = sin(theta);

    // 回転フェーズ限定の攻撃: 大正方形の四隅から自機狙い
    const bool isRotPhase = (c >= 60 && c < 360) || (c >= 480 && c < 780);
    if (isRotPhase && (c % 24 == 0)) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // ボレーごとの角度ずらし: -0.10, 0, +0.10 を循環
        const int volley = c / 24;
        const double aimOffset = ((volley % 3) - 1) * 0.10;
        const double cornerLocal[4][2] = {
            { -PYTH_HALF, -PYTH_HALF },
            {  PYTH_HALF, -PYTH_HALF },
            {  PYTH_HALF,  PYTH_HALF },
            { -PYTH_HALF,  PYTH_HALF },
        };
        for (int ci = 0; ci < 4; ci++) {
            double lx = cornerLocal[ci][0];
            double ly = cornerLocal[ci][1];
            double sx = cx + (lx * cosT - ly * sinT);
            double sy = cy + (lx * sinT + ly * cosT);

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = sx;
            pEnemyShot->y = sy;
            pEnemyShot->muki = atan2(player.y - sy, player.x - sx) + aimOffset;
            pEnemyShot->speed = 2.8;
            pEnemyShot->param_i[0] = 2; // 2:回転中の攻撃弾 (直進)
            pEnemyShot->param_i[1] = ci;
            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 三角形の弾
            double local[2];
            CalcTriLocal(pShot->param_i[1], pShot->param_d[0], s, local);
            double rx = local[0] * cosT - local[1] * sinT;
            double ry = local[0] * sinT + local[1] * cosT;
            double tx = cx + rx;
            double ty = cy + ry;
            if (c < 60) {
                tx = cx + (tx - cx) * appearEase;
                ty = cy + (ty - cy) * appearEase;
            }
            else if (c >= 780) {
                double dx = tx - cx;
                double dy = ty - cy;
                double len = sqrt(dx * dx + dy * dy);
                if (len > 1.0e-6) {
                    tx += dx / len * exitDist;
                    ty += dy / len * exitDist;
                }
                else {
                    ty -= exitDist;
                }
            }
            pShot->x = tx;
            pShot->y = ty;
        }
        else if (pShot->param_i[0] == 1) {
            // 大正方形のガイドレーザー
            const double H = PYTH_HALF;
            const double step = (H * 2.0) / 4.0; // 52.5
            double off = -H + (pShot->param_d[0] + 0.5) * step;
            double lx = 0.0, ly = 0.0, sideAngle = 0.0;
            switch (pShot->param_i[1]) {
            case 0: // 上辺: 左から右
                lx = off; ly = -H; sideAngle = 0.0;
                break;
            case 1: // 右辺: 上から下
                lx = H; ly = off; sideAngle = DX_PI / 2.0;
                break;
            case 2: // 下辺: 右から左
                lx = -off; ly = H; sideAngle = DX_PI;
                break;
            default: // 左辺: 下から上
                lx = -H; ly = -off; sideAngle = -DX_PI / 2.0;
                break;
            }
            double rx = lx * cosT - ly * sinT;
            double ry = lx * sinT + ly * cosT;
            double tx = cx + rx;
            double ty = cy + ry;
            if (c >= 60) {
                // ガイドは60フレームで役目を終え、外向きに速く逃がす
                double dx = tx - cx;
                double dy = ty - cy;
                double len = sqrt(dx * dx + dy * dy);
                double out = (c - 60) * 4.0;
                if (len > 1.0e-6) {
                    tx += dx / len * out;
                    ty += dy / len * out;
                }
                else {
                    ty -= out;
                }
            }
            pShot->x = tx;
            pShot->y = ty;
            pShot->muki = sideAngle + theta;
        }
        else {
            // 回転中の攻撃弾: 図形の回転に連動せず、撃った方向へ直進のみ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_PythagoreanTheorem_MetaAI()
{
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        shot_count = 0;
    }
    else {
        // 敵は上部でゆっくり左右に漂うだけ。証明の図形は (240,260) に固定する。
        enemy.x = 240.0 + 30.0 * sin(count * 0.01);
        enemy.y = 40.0;
    }

    // 960フレームごとに1セット。初回は count==60 で出現。
    if (count % 960 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPythagoras;
        pEnemyShotSet->x = 240.0; // 証明の図形の中心
        pEnemyShotSet->y = 260.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++; // 偶数/奇数で回転方向を反転
        pEnemyShotSet->alive = 900; // 1セット分の存在保証

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}