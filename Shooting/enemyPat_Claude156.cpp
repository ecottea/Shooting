#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：定幅の波 ― ルーローの三角形
//
//  【幾何】
//    敵中心から半径 R の位置に頂点を3つ置く(頂点 V_i の角度 = theta + 120°*i)。
//    辺の長さ s = √3 * R。
//    円弧 j は「頂点 V_j を中心」「半径 s」「角度 phi = theta + 120°*j + 180° ± 30°」の60°分。
//    円弧上の点の「外向き法線」は phi そのもの。
//    各頂点では、両隣の円弧の端の法線に挟まれた60°(頂点の向き ± 30°)が空いている。
//    円弧3本×60° + 頂点3つ×60° = 360° なので、全弾を法線方向へ放つと
//    波面がルーローの三角形(の平行曲線)を保ったまま膨張する。
//
//  【流れ】 1つの波(ShotSet)の寿命
//    ① 形成(0〜49F)   : 4層の入れ子のルーロー輪郭を頂点→円弧の順に描き込む(静止)
//    ② 回転(50〜109F) : 全体が回転しながら加速し、少し膨らむ
//    ③ 射出(110F)     : 全弾を外向き法線へ放つ。頂点には60°の扇を追加する
//    波は2つ。第1波と第2波は回転方向が逆で、解放時の向きが60°ずれている。
//    第2波の解放直後は、外層の3頂点から菱形弾の自機狙いを4連射する。
// ============================================================

// ---------- 定数 ----------
static const double REU_SQRT3 = 1.7320508075688772;
static const double REU_BASE_RADIUS = 90.0;       // 最外層の頂点半径R
static const double REU_SWELL = 0.25;             // 回転中の膨張率(最大で 1 + REU_SWELL 倍)

static const int REU_LAYER_NUM = 4;               // 入れ子の層数
static const double REU_LAYER_SCALE[REU_LAYER_NUM] = { 1.00, 0.78, 0.56, 0.34 };
// 円弧1本あたりの弾数(外層ほど多く、弾の間隔がほぼ一定になるようにしている)
static const int REU_ARC_NUM[REU_LAYER_NUM] = { 36, 28, 20, 12 };
// 頂点1つあたりの扇の弾数(中央の1発は頂点の目印弾と兼用。必ず奇数にすること)
static const int REU_FAN_NUM[REU_LAYER_NUM] = { 13, 11, 9, 7 };
// 解放後の速さ[波][層](外層ほど速いので、層ごとに間隔が開いて入れ子の波になる)
static const double REU_LAYER_SPEED[2][REU_LAYER_NUM] = {
    { 2.8, 2.4, 2.0, 1.6 },
    { 3.0, 2.6, 2.2, 1.8 },
};
// 弾の色[波][層] 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
static const int REU_COLOR[2][REU_LAYER_NUM] = {
    { 0, 8, 1, 6 },   // 第1波: 暖色
    { 3, 4, 5, 6 },   // 第2波: 寒色
};
static const int REU_DIAMOND_COLOR = 1;           // 締めの自機狙い(菱形弾)の色

// フェーズのフレーム数
static const int REU_LAYER_DELAY = 5;             // 層ごとの描き始めのずれ
static const int REU_DRAW_FRAME = 30;             // 1層を描き切るのにかかる時間
static const int REU_FORM_FRAME = 50;             // 形成フェーズの長さ
static const int REU_ROT_FRAME = 60;              // 回転フェーズの長さ
static const int REU_RELEASE_FRAME = REU_FORM_FRAME + REU_ROT_FRAME;  // 射出するフレーム

// 締めの自機狙い
static const int REU_AIM_VOLLEY = 4;              // 連射回数
static const int REU_AIM_INTERVAL = 6;            // 連射間隔(F)
static const int REU_AIM_WAY = 5;                 // 1頂点1回あたりの弾数

// 敵本体
static const int REU_INTRO_FRAME = 40;            // 登場移動にかける時間
static const double REU_ENEMY_X = 240.0;
static const double REU_ENEMY_Y_START = 40.0;
static const double REU_ENEMY_Y_BATTLE = 130.0;   // 図形が画面内に収まる高さ(R*1.25 = 112.5 に余裕を持たせた位置)
static const int REU_CYCLE = 300;                 // 1サイクルの長さ
static const int REU_WAVE2_DELAY = 130;           // 第2波を出すタイミング(第1波の解放後)

// 弾ごとのパラメータの使い道
static const int REU_P_TYPE = 0;    // param_i[0]: 0=円弧上の弾, 1=頂点の目印弾
static const int REU_P_INDEX = 1;   // param_i[1]: 円弧番号j または 頂点番号i
static const int REU_P_LAYER = 2;   // param_i[2]: 層
static const int REU_P_STATE = 3;   // param_i[3]: 0=図形に追従中, 1=直進中
// param_d[0]: 円弧上の位置u(0〜1)

// ---------- 補助関数 ----------

// 効果音を頭から鳴らし直す
static void ReuleauxPlaySE(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ShotSetの弾リストの末尾に弾を1つ追加する
static sEnemyShot* ReuleauxAddShot(sEnemyShotSet* pSet, int kind, double x, double y, double muki, double speed)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->kind = kind;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 図形に追従する弾(形成〜回転フェーズ中の弾)を追加する。位置は後で毎フレーム計算し直す
static void ReuleauxAddAttached(sEnemyShotSet* pSet, int kind, int type, int index, double u, int layer)
{
    sEnemyShot* pShot = ReuleauxAddShot(pSet, kind, pSet->x, pSet->y, 0.0, 0.0);
    pShot->param_i[REU_P_TYPE] = type;
    pShot->param_i[REU_P_INDEX] = index;
    pShot->param_i[REU_P_LAYER] = layer;
    pShot->param_i[REU_P_STATE] = 0;
    pShot->param_d[0] = u;
}

// 図形上の点(中心からの相対座標)と、その点での外向き法線の角度を求める
//   type 0: 円弧 index=j, u=0〜1(円弧上の位置)
//   type 1: 頂点 index=i
static void ReuleauxPoint(int type, int index, double u, double radius, double theta,
    double* pX, double* pY, double* pNormal)
{
    const double a = theta + 2.0 * DX_PI / 3.0 * index;
    if (type == 0) {
        // 円弧の中心は頂点V_j。半径 s=√3*R で、頂点とは反対側(a+180°)を中心に±30°描く
        const double vx = radius * cos(a);
        const double vy = radius * sin(a);
        const double s = REU_SQRT3 * radius;
        const double phi = a + DX_PI + (u - 0.5) * DX_PI / 3.0;
        *pX = vx + s * cos(phi);
        *pY = vy + s * sin(phi);
        *pNormal = phi;
    }
    else {
        *pX = radius * cos(a);
        *pY = radius * sin(a);
        *pNormal = a;
    }
}

// 弾の種類を選ぶ(種類ごとの画像配列から色を選ぶ)
static int ReuleauxSmall(int color) { return img_enemyShotSmallBall[color]; }
static int ReuleauxMedium(int color) { return img_enemyShotMediumBall[color]; }
static int ReuleauxLarge(int color) { return img_enemyShotLargeBall[color]; }

// 頂点の目印弾の種類: 最外層だけ大玉、内側は中玉
static int ReuleauxMarkerKind(int layer, int color)
{
    return (layer == 0) ? ReuleauxLarge(color) : ReuleauxMedium(color);
}

// ---------- 弾幕本体(1つの波) ----------
// pSet->param_i[0] : 波の番号(0=第1波, 1=第2波)
// pSet->param_d[0] : 射出時の図形の角度 theta
// pSet->param_d[1] : 回転量(符号付き。正で時計回り、負で反時計回り)
static void ShotReuleaux(sEnemyShotSet* pSet)
{
    const int c = pSet->count;
    const int wave = pSet->param_i[0];
    const double thetaRelease = pSet->param_d[0];
    const double rotTotal = pSet->param_d[1];

    if (c == 0) ReuleauxPlaySE(sound_enemyCharge);

    // ---------- ① 形成: 頂点→円弧の順に輪郭を描き込む ----------
    if (c < REU_FORM_FRAME) {
        for (int l = 0; l < REU_LAYER_NUM; l++) {
            const int start = l * REU_LAYER_DELAY;
            if (c < start || c >= start + REU_DRAW_FRAME) continue;
            const int color = REU_COLOR[wave][l];
            if (c == start) {
                // 頂点の目印弾
                for (int i = 0; i < 3; i++) {
                    ReuleauxAddAttached(pSet, ReuleauxMarkerKind(l, color), 1, i, 0.0, l);
                }
            }
            // 円弧: 弧0→弧1→弧2 の順につながっているので、通し番号で順に出していく
            const int n = REU_ARC_NUM[l];
            const int total = n * 3;
            const int lo = total * (c - start) / REU_DRAW_FRAME;
            const int hi = total * (c - start + 1) / REU_DRAW_FRAME;
            for (int idx = lo; idx < hi; idx++) {
                const int j = idx / n;
                const int m = idx % n;
                ReuleauxAddAttached(pSet, ReuleauxSmall(color), 0, j, (m + 0.5) / n, l);
            }
        }
    }

    // ---------- ② 回転: 角度と膨張量を決める(進行度pは2乗で加速) ----------
    double p = 0.0;
    if (c >= REU_FORM_FRAME) {
        double t = (double)(c - REU_FORM_FRAME) / (double)REU_ROT_FRAME;
        if (t > 1.0) t = 1.0;
        p = t * t;
    }
    const double theta = thetaRelease - rotTotal * (1.0 - p);
    const double swell = 1.0 + REU_SWELL * p;
    const bool isRelease = (c == REU_RELEASE_FRAME);

    // ---------- 全弾の更新 ----------
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[REU_P_STATE] == 0) {
            // 図形に追従: 毎フレーム解析的に位置を求める(速度の積み上げをしないので誤差が出ない)
            const int layer = pShot->param_i[REU_P_LAYER];
            const double radius = REU_BASE_RADIUS * REU_LAYER_SCALE[layer] * swell;
            double px, py, normal;
            ReuleauxPoint(pShot->param_i[REU_P_TYPE], pShot->param_i[REU_P_INDEX], pShot->param_d[0],
                radius, theta, &px, &py, &normal);
            pShot->x = pSet->x + px;
            pShot->y = pSet->y + py;
            pShot->muki = normal;   // 外向き法線(=この弾が最終的に飛ぶ向き)
            if (isRelease) {
                // ③ 射出: 法線方向へ直進させる
                pShot->speed = REU_LAYER_SPEED[wave][layer];
                pShot->param_i[REU_P_STATE] = 1;
            }
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // ---------- ③ 射出: 頂点の扇を追加(新しい弾は次フレームから動く) ----------
    if (isRelease) {
        ReuleauxPlaySE(sound_enemyShot_heavy);
        for (int l = 0; l < REU_LAYER_NUM; l++) {
            const int color = REU_COLOR[wave][l];
            const double radius = REU_BASE_RADIUS * REU_LAYER_SCALE[l] * swell;
            const int fan = REU_FAN_NUM[l];
            for (int i = 0; i < 3; i++) {
                double px, py, normal;
                ReuleauxPoint(1, i, 0.0, radius, theta, &px, &py, &normal);
                for (int m = 0; m < fan; m++) {
                    if (m == fan / 2) continue;   // 中央は頂点の目印弾が担当
                    // 頂点の向き ± 30° を等間隔に埋める
                    const double off = ((m + 0.5) / fan - 0.5) * DX_PI / 3.0;
                    sEnemyShot* pFan = ReuleauxAddShot(pSet, ReuleauxMedium(color),
                        pSet->x + px, pSet->y + py, normal + off, REU_LAYER_SPEED[wave][l]);
                    pFan->param_i[REU_P_STATE] = 1;
                }
            }
        }
    }

    // ---------- 第2波のみ: 外層の頂点から菱形弾の自機狙いを連射 ----------
    if (wave == 1 && c >= REU_RELEASE_FRAME) {
        const int since = c - REU_RELEASE_FRAME;
        if (since % REU_AIM_INTERVAL == 0 && since / REU_AIM_INTERVAL < REU_AIM_VOLLEY) {
            const int volley = since / REU_AIM_INTERVAL;
            ReuleauxPlaySE(sound_enemyShot_light);
            const double radius = REU_BASE_RADIUS * REU_LAYER_SCALE[0] * swell;
            for (int i = 0; i < 3; i++) {
                double px, py, normal;
                ReuleauxPoint(1, i, 0.0, radius, theta, &px, &py, &normal);
                const double x = pSet->x + px;
                const double y = pSet->y + py;
                const double aim = atan2(player.y - y, player.x - x);
                for (int w = 0; w < REU_AIM_WAY; w++) {
                    const double off = (w - (REU_AIM_WAY - 1) / 2.0) * (DX_PI / 30.0);   // 6°刻み
                    sEnemyShot* pDia = ReuleauxAddShot(pSet, img_enemyShotDiamond[REU_DIAMOND_COLOR],
                        x, y, aim + off, 3.0 + 0.4 * volley);
                    pDia->param_i[REU_P_STATE] = 1;
                }
            }
        }
    }
}

// 波(ShotSet)を1つ生成する。wave 0:第1波(時計回り) / 1:第2波(反時計回り・解放時の向きが60°ずれる)
static void ReuleauxSpawnWave(int wave)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotReuleaux;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = wave;
    pEnemyShotSet->param_i[0] = wave;
    // 第1波は頂点が真上、第2波はそこから60°ずらす(第1波の頂点の扇の継ぎ目を第2波の円弧が塞ぐ)
    pEnemyShotSet->param_d[0] = (wave == 0) ? -DX_PI / 2.0 : -DX_PI / 2.0 + DX_PI / 3.0;
    pEnemyShotSet->param_d[1] = (wave == 0) ? DX_PI : -DX_PI;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ---------- 敵本体のパターン ----------
void EnemyPat_ReuleauxTriangle_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = REU_ENEMY_X;
        enemy.y = REU_ENEMY_Y_START;
        enemy.maxHp = enemy.hp = 200;   // 200から変更しないこと
    }
    else if (count <= REU_INTRO_FRAME) {
        // 図形が画面内に収まる高さまで、減速しながら降りてくる
        const double t = (double)count / (double)REU_INTRO_FRAME;
        const double e = 1.0 - (1.0 - t) * (1.0 - t);
        enemy.y = REU_ENEMY_Y_START + (REU_ENEMY_Y_BATTLE - REU_ENEMY_Y_START) * e;
    }

    // 登場移動が終わったら、REU_CYCLE フレーム周期で第1波→第2波を繰り返す
    const int t = count - REU_INTRO_FRAME - 1;
    if (t >= 0) {
        const int phase = t % REU_CYCLE;
        if (phase == 0) ReuleauxSpawnWave(0);
        else if (phase == REU_WAVE2_DELAY) ReuleauxSpawnWave(1);
    }
}