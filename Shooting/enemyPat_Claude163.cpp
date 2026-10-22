#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  雪符「六花結晶・散華」
//
//  六角形の雪の結晶を敵弾の組み合わせで描き、
//    予告(種) → 開花(相似拡大) → 静止 → 崩壊(粉雪・氷柱・残響)
//  と進行する。サイズ・回転方向・向きの違う結晶を3輪、時間差で重ねる。
//
//  1輪 = 1つの sEnemyShotSet。全弾を count==0 で生成し、以降は
//  各弾が param で自分の役割を覚えて動く(ShotSetは弾が残る限り消えない)。
//
//  【sEnemyShotSet の param_d】
//    [0] 結晶の最終中心X   [1] 結晶の最終中心Y   [2] 結晶の半径R(px)
//    [3] 回転方向(+1.0 / -1.0)  [4] 初期回転角(rad)
//    ※ 生成位置は sEnemyShotSet の x, y に保持(敵の位置)
//
//  【sEnemyShot の param_i】
//    [0] 役割(eSnowPart)  [1] 氷柱の発射順(0〜23)
//    [2] 結晶から離れる時刻(ShotSetのcount基準)
//    [3] 状態(0:結晶に固定 1:離脱済み)
//    [4][5] 氷柱の明滅色(シアン/白)
//  【sEnemyShot の param_d】
//    [0][1] 結晶座標系での位置(R=等倍時のpx)  [2] 結晶座標系での向き(rad)
//    [3][4] 離脱後の速度成分  [5] 砕け時の飛散速度  [6][7] 粉雪の揺れの位相・周期
//    [8] 粉雪:現在の落下速度 / 残響:周回開始角
//    [9] 粉雪:落下速度の上限 / 残響:周回半径
// ============================================================

// ---------- タイミング(フレーム) ----------
static const int    SNOW_CRYSTAL_NUM = 3;     // 1サイクルで咲かせる結晶の数
static const int    SNOW_T_SEED = 40;    // 予告(種)の長さ
static const int    SNOW_T_BLOOM = 130;   // 開花完了
static const int    SNOW_T_BREAK = 190;   // 崩壊開始(静止は BLOOM〜BREAK)
static const int    SNOW_ICICLE_INTERVAL = 3;     // 氷柱を1本ずつ撃つ間隔
static const int    SNOW_ORBIT_FRAMES = 60;    // 残響の周回フレーム数
static const int    SNOW_START = 30;    // サイクル開始から1輪目までの待ち
static const int    SNOW_CYCLE = 800;   // 1サイクルの長さ

// ---------- 形状・動きの調整値 ----------
static const double SNOW_SCALE_SEED_BEGIN = 0.10;  // 種の初期スケール
static const double SNOW_SCALE_SEED_END = 0.18;  // 種の終了スケール(開花直前)
static const double SNOW_ROT_SEED = 0.15;  // 種の間の回転量(rad)
static const double SNOW_ROT_TOTAL = 1.20;  // 開花中の回転量(rad)
static const double SNOW_ORBIT_OMEGA = 0.02;  // 残響の角速度(rad/F)

// 弾の色(img_enemyShotXxx[色])  0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
static const int SNOW_COLOR_CYAN = 3;
static const int SNOW_COLOR_BLUE = 4;
static const int SNOW_COLOR_WHITE = 6;

// 結晶ごとの設定(小→中→大)
static const double SNOW_RADIUS[SNOW_CRYSTAL_NUM] = { 110.0, 145.0*1.1, 185.0*1.2 };
static const double SNOW_TARGET_Y[SNOW_CRYSTAL_NUM] = { 150.0, 170.0, 190.0 };
static const double SNOW_THETA0[SNOW_CRYSTAL_NUM] = {
    -DX_PI / 2.0,
    -DX_PI / 2.0 + DX_PI / 6.0,   // 30°ずらす
    -DX_PI / 2.0
};

// 主軸の中玉の位置(Rに対する比)。外側ほど少し詰める。0.4 / 0.7 は枝の付け根と一致させる
static const double SNOW_STEM_RATE[10] = {
    0.10, 0.20, 0.30, 0.40, 0.50, 0.60, 0.70, 0.78, 0.86, 0.93
};

// 弾の役割
enum eSnowPart {
    SNOW_PART_FLAKE = 0,  // 崩壊時に粉雪になる弾
    SNOW_PART_ICICLE,     // 枝先の菱形弾 → 自機狙いの氷柱
    SNOW_PART_TIP,        // 腕の先端の大玉 → 周回してから外へ飛ぶ残響
    SNOW_PART_CORE        // 中心核 → ゆっくり自機狙い
};

// 結晶全体の座標系(中心・スケール・回転角)
struct sSnowFrame {
    double cx, cy, scale, theta;
};

// ------------------------------------------------------------
//  補助関数
// ------------------------------------------------------------
static void SnowPlaySE(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// curをtargetへ角度の最短経路でrate(0〜1)だけ近づける
static double SnowAngleApproach(double cur, double target, double rate)
{
    double d = target - cur;
    while (d > DX_PI)  d -= DX_PI * 2.0;
    while (d < -DX_PI) d += DX_PI * 2.0;
    return cur + d * rate;
}

// 時刻tにおける結晶の座標系を求める
//   t <  SEED : 敵の位置で小さな種が溜まる(ゆっくり拡大・回転)
//   t <  BLOOM: 3次イーズアウトで一気に開花しつつ最終位置へ移動・回転、最後は急減速して止まる
//   t >= BLOOM: 完全に静止
static sSnowFrame CalcSnowFrame(const sEnemyShotSet* pSet, int t)
{
    const double sx = pSet->x, sy = pSet->y;
    const double tx = pSet->param_d[0], ty = pSet->param_d[1];
    const double dir = pSet->param_d[3];
    const double theta0 = pSet->param_d[4];

    double e, scale, rot;
    if (t < SNOW_T_SEED) {
        const double u = (double)t / (double)SNOW_T_SEED;
        e = 0.0;
        scale = SNOW_SCALE_SEED_BEGIN + (SNOW_SCALE_SEED_END - SNOW_SCALE_SEED_BEGIN) * u;
        rot = SNOW_ROT_SEED * u;
    }
    else if (t < SNOW_T_BLOOM) {
        const double u = (double)(t - SNOW_T_SEED) / (double)(SNOW_T_BLOOM - SNOW_T_SEED);
        const double v = 1.0 - u;
        e = 1.0 - v * v * v;
        scale = SNOW_SCALE_SEED_END + (1.0 - SNOW_SCALE_SEED_END) * e;
        rot = SNOW_ROT_SEED + SNOW_ROT_TOTAL * e;
    }
    else {
        e = 1.0;
        scale = 1.0;
        rot = SNOW_ROT_SEED + SNOW_ROT_TOTAL;
    }

    sSnowFrame f;
    f.cx = sx + (tx - sx) * e;
    f.cy = sy + (ty - sy) * e;
    f.scale = scale;
    f.theta = theta0 + dir * rot;
    return f;
}

// ShotSetの弾リスト末尾に弾を1つ追加する
static sEnemyShot* AddSnowShot(sEnemyShotSet* pSet, int part, int img,
    double lx, double ly, double localMuki, int releaseTime)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = pSet->x;
    pShot->y = pSet->y;
    pShot->muki = localMuki;
    pShot->speed = 0.0;
    pShot->kind = img;
    pShot->param_i[0] = part;
    pShot->param_i[2] = releaseTime;
    pShot->param_i[3] = 0;
    pShot->param_d[0] = lx;
    pShot->param_d[1] = ly;
    pShot->param_d[2] = localMuki;
    // 粉雪用の乱数(毎回同じ個数を消費する。GetRand(x)は0〜xのx+1通り)
    pShot->param_d[5] = 1.0 + GetRand(100) / 100.0;        // 飛散速度 1.0〜2.0
    pShot->param_d[6] = GetRand(628) / 100.0;              // 揺れの位相 0〜6.28
    pShot->param_d[7] = 0.05 + GetRand(40) / 1000.0;       // 揺れの角速度 0.05〜0.09
    pShot->param_d[9] = 0.9 + GetRand(60) / 100.0;         // 落下速度の上限 0.9〜1.5
    pShot->margin = 240;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 粉雪になる弾を追加する。外側ほど早く砕け(約30F)、さらに0〜7Fのばらつきを付ける
static void AddSnowFlake(sEnemyShotSet* pSet, int img, double lx, double ly,
    double localMuki, double R)
{
    double rn = sqrt(lx * lx + ly * ly) / R;
    if (rn > 1.0) rn = 1.0;
    const int releaseTime = SNOW_T_BREAK + (int)(30.0 * (1.0 - rn)) + GetRand(7);
    AddSnowShot(pSet, SNOW_PART_FLAKE, img, lx, ly, localMuki, releaseTime);
}

// 結晶1輪分の弾(約205発)を生成する
static void SpawnSnowCrystal(sEnemyShotSet* pSet)
{
    const double R = pSet->param_d[2];

    // 中心核: 大玉(白)
    AddSnowShot(pSet, SNOW_PART_CORE, img_enemyShotLargeBall[SNOW_COLOR_WHITE],
        0.0, 0.0, 0.0, SNOW_T_BREAK + 20);

    // 枝の設定: [0]=内側 [1]=外側
    const double branchJunction[2] = { 0.40, 0.70 };  // 主軸上の付け根(Rに対する比)
    const double branchLength[2] = { 0.26, 0.20 };  // 枝の長さ(Rに対する比)。隣の腕の枝と交差しない長さ
    const int    branchCount[2] = { 5, 4 };        // 枝の弾数(最後の1発が菱形弾)

    for (int a = 0; a < 6; a++) {
        const double alpha = a * DX_PI / 3.0;         // 腕の向き
        const double ca = cos(alpha), sa = sin(alpha);

        // 主軸: 中玉(白)x10
        for (int i = 0; i < 10; i++) {
            const double r = R * SNOW_STEM_RATE[i];
            AddSnowFlake(pSet, img_enemyShotMediumBall[SNOW_COLOR_WHITE],
                r * ca, r * sa, alpha, R);
        }

        // 腕の先端: 大玉(白)。崩壊時に残響になる
        AddSnowShot(pSet, SNOW_PART_TIP, img_enemyShotLargeBall[SNOW_COLOR_WHITE],
            R * ca, R * sa, alpha, SNOW_T_BREAK);

        // 枝: 小玉(シアン) + 先端に菱形弾(シアン)
        for (int b = 0; b < 2; b++) {
            const double jx = R * branchJunction[b] * ca;
            const double jy = R * branchJunction[b] * sa;
            for (int s = 0; s < 2; s++) {
                const double side = (s == 0) ? 1.0 : -1.0;
                const double bAng = alpha + side * DX_PI / 3.0;   // 枝の伸びる向き
                const double cb = cos(bAng), sb = sin(bAng);
                for (int k = 1; k <= branchCount[b]; k++) {
                    const double d = R * branchLength[b] * (double)k / (double)branchCount[b];
                    const double lx = jx + d * cb;
                    const double ly = jy + d * sb;
                    if (k == branchCount[b]) {
                        // 氷柱(菱形弾): 腕→内外→左右の順に時間差で発射
                        const int order = a * 4 + b * 2 + s;       // 0〜23
                        sEnemyShot* pTip = AddSnowShot(pSet, SNOW_PART_ICICLE,
                            img_enemyShotDiamond[SNOW_COLOR_CYAN], lx, ly, bAng,
                            SNOW_T_BREAK + order * SNOW_ICICLE_INTERVAL);
                        pTip->param_i[1] = order;
                        pTip->param_i[4] = SNOW_COLOR_CYAN;
                        pTip->param_i[5] = SNOW_COLOR_WHITE;
                    }
                    else {
                        AddSnowFlake(pSet, img_enemyShotSmallBall[SNOW_COLOR_CYAN],
                            lx, ly, bAng, R);
                    }
                }
            }
        }

        // 内側の六角リング: 隣り合う腕の25%地点を結ぶ辺に鱗弾(青)x3
        {
            const double alpha2 = (a + 1) * DX_PI / 3.0;
            const double v0x = R * 0.25 * ca, v0y = R * 0.25 * sa;
            const double v1x = R * 0.25 * cos(alpha2), v1y = R * 0.25 * sin(alpha2);
            const double edgeMuki = alpha + DX_PI * 2.0 / 3.0;   // 辺の接線方向
            for (int k = 1; k <= 3; k++) {
                const double tt = k / 4.0;
                AddSnowFlake(pSet, img_enemyShotScale[SNOW_COLOR_BLUE],
                    v0x + (v1x - v0x) * tt, v0y + (v1y - v0y) * tt, edgeMuki, R);
            }
        }

        // 輝き1: 主軸に沿った短レーザー(白)
        AddSnowFlake(pSet, img_enemyShotLaser[SNOW_COLOR_WHITE],
            R * 0.55 * ca, R * 0.55 * sa, alpha, R);

        // 輝き2: 腕と腕の間(30°方向)に接線向きの短レーザー(白)。結晶の外枠になる
        {
            const double bis = alpha + DX_PI / 6.0;
            AddSnowFlake(pSet, img_enemyShotLaser[SNOW_COLOR_WHITE],
                R * 0.93 * cos(bis), R * 0.93 * sin(bis), bis + DX_PI / 2.0, R);
        }
    }
}

// 結晶から離れる瞬間の初期化
static void ReleaseSnowShot(sEnemyShotSet* pSet, sEnemyShot* pShot, bool* pIcicleSe)
{
    pShot->param_i[3] = 1;
    const double fcx = pSet->param_d[0], fcy = pSet->param_d[1];

    switch (pShot->param_i[0]) {
    case SNOW_PART_ICICLE: {
        // 自機狙い。腕ごとに少しだけ角度をずらして扇状にばらけさせる
        const int arm = pShot->param_i[1] / 4;
        const double spread = ((double)arm - 2.5) * 0.03;
        pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x) + spread;
        pShot->speed = 2.0;
        pShot->kind = img_enemyShotDiamond[pShot->param_i[4]];
        if (pShot->param_i[1] % 4 == 0) *pIcicleSe = true;
        break;
    }
    case SNOW_PART_TIP: {
        const double dx = pShot->x - fcx, dy = pShot->y - fcy;
        pShot->param_d[8] = atan2(dy, dx);              // 周回開始角
        pShot->param_d[9] = sqrt(dx * dx + dy * dy);    // 周回半径
        pShot->param_d[3] = 0.0;
        pShot->param_d[4] = 0.0;
        break;
    }
    case SNOW_PART_CORE: {
        pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
        pShot->speed = 1.0;
        break;
    }
    default: {
        // 粉雪: 結晶の中心から外向きに飛び散る初速を与える
        double dx = pShot->x - fcx, dy = pShot->y - fcy;
        double len = sqrt(dx * dx + dy * dy);
        if (len < 0.001) { dx = 0.0; dy = -1.0; len = 1.0; }
        pShot->param_d[3] = dx / len * pShot->param_d[5];
        pShot->param_d[4] = dy / len * pShot->param_d[5];
        pShot->param_d[8] = 0.0;                        // 落下速度は0から加速
        break;
    }
    }
}

// 弾1発の更新
static void UpdateSnowShot(sEnemyShotSet* pSet, sEnemyShot* pShot, int t,
    const sSnowFrame& f, bool* pIcicleSe)
{
    const int part = pShot->param_i[0];

    // ---- 結晶に固定されている間 ----
    if (pShot->param_i[3] == 0) {
        if (t < pShot->param_i[2]) {
            const double c = cos(f.theta), s = sin(f.theta);
            const double lx = pShot->param_d[0], ly = pShot->param_d[1];
            pShot->x = f.cx + f.scale * (lx * c - ly * s);
            pShot->y = f.cy + f.scale * (lx * s + ly * c);
            pShot->muki = f.theta + pShot->param_d[2];   // 結晶の回転に合わせて向きも回す
            // 静止中は枝先の菱形弾を6Fごとにシアン/白で明滅させる
            if (part == SNOW_PART_ICICLE && t >= SNOW_T_BLOOM) {
                const int col = (((t - SNOW_T_BLOOM) / 6) % 2 == 0) ? pShot->param_i[4] : pShot->param_i[5];
                pShot->kind = img_enemyShotDiamond[col];
            }
            return;
        }
        ReleaseSnowShot(pSet, pShot, pIcicleSe);
    }

    // ---- 離脱後 ----
    const int dt = t - pShot->param_i[2];

    switch (part) {
    case SNOW_PART_ICICLE: {
        // 加速しながら直進
        pShot->speed += 0.04;
        if (pShot->speed > 5.0) pShot->speed = 5.0;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        break;
    }
    case SNOW_PART_CORE: {
        pShot->speed += 0.03;
        if (pShot->speed > 3.5) pShot->speed = 3.5;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        break;
    }
    case SNOW_PART_TIP: {
        const double fcx = pSet->param_d[0], fcy = pSet->param_d[1];
        const double dir = pSet->param_d[3];
        const double r = pShot->param_d[9];
        if (dt < SNOW_ORBIT_FRAMES) {
            // 結晶の中心を軸に円運動(結晶と同じ回転方向)
            const double ang = pShot->param_d[8] + dir * SNOW_ORBIT_OMEGA * (double)dt;
            pShot->x = fcx + r * cos(ang);
            pShot->y = fcy + r * sin(ang);
            pShot->muki = ang + dir * DX_PI / 2.0;       // 進行方向(接線)
            pShot->speed = r * SNOW_ORBIT_OMEGA;
        }
        else {
            if (dt == SNOW_ORBIT_FRAMES) {
                // 周回終了: 接線速度を引き継いで外へ飛び出す
                const double angEnd = pShot->param_d[8] + dir * SNOW_ORBIT_OMEGA * (double)(SNOW_ORBIT_FRAMES - 1);
                pShot->param_d[3] = -dir * SNOW_ORBIT_OMEGA * r * sin(angEnd);
                pShot->param_d[4] = dir * SNOW_ORBIT_OMEGA * r * cos(angEnd);
            }
            const double dx = pShot->x - fcx, dy = pShot->y - fcy;
            double len = sqrt(dx * dx + dy * dy);
            if (len < 0.001) len = 1.0;
            pShot->param_d[3] += 0.05 * dx / len;        // 外向きに加速
            pShot->param_d[4] += 0.05 * dy / len;
            pShot->x += pShot->param_d[3];
            pShot->y += pShot->param_d[4];
            pShot->muki = atan2(pShot->param_d[4], pShot->param_d[3]);
            pShot->speed = sqrt(pShot->param_d[3] * pShot->param_d[3] + pShot->param_d[4] * pShot->param_d[4]);
        }
        break;
    }
    default: {
        // 粉雪: 飛散速度が減衰しつつ、ゆっくり加速して落下。横にはsin波で揺れる
        pShot->param_d[3] *= 0.96;
        pShot->param_d[4] *= 0.96;
        pShot->param_d[8] += 0.02;
        if (pShot->param_d[8] > pShot->param_d[9]) pShot->param_d[8] = pShot->param_d[9];
        double amp = 0.45;
        if (dt < 30) amp *= (double)dt / 30.0;           // 揺れは徐々に強くする
        const double swayVx = amp * sin(pShot->param_d[6] + pShot->param_d[7] * (double)dt);
        const double mvx = pShot->param_d[3] + swayVx;
        const double mvy = pShot->param_d[4] + pShot->param_d[8];
        pShot->x += mvx;
        pShot->y += mvy;
        // 向きは実際の進行方向へなめらかに合わせる(レーザー・鱗弾が急に回らないように)
        pShot->muki = SnowAngleApproach(pShot->muki, atan2(mvy, mvx), 0.1);
        pShot->speed = sqrt(mvx * mvx + mvy * mvy);
        break;
    }
    }
}

// ------------------------------------------------------------
//  弾幕: 結晶1輪
// ------------------------------------------------------------
static void ShotSnowCrystal(sEnemyShotSet* pEnemyShotSet)
{
    const int t = pEnemyShotSet->count;

    if (t == 0) {
        SnowPlaySE(sound_enemyCharge);                   // 予告音
        SpawnSnowCrystal(pEnemyShotSet);
    }
    else if (t == SNOW_T_SEED) {
        SnowPlaySE(sound_enemyShot_heavy);               // 開花
    }
    else if (t == SNOW_T_BREAK) {
        SnowPlaySE(sound_enemyShot_medium);              // 砕け散り
    }

    const sSnowFrame frame = CalcSnowFrame(pEnemyShotSet, t);
    bool icicleSe = false;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        UpdateSnowShot(pEnemyShotSet, pShot, t, frame, &icicleSe);
        pShot = pShot->next;
    }

    if (icicleSe) SnowPlaySE(sound_enemyShot_light);     // 氷柱の発射音(腕ごとに1回)
}

// 結晶1輪を生成する(index: 0=小 1=中 2=大, cycleNo: 何サイクル目か)
static void CreateSnowCrystal(int index, int cycleNo)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotSnowCrystal;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = index;
    pEnemyShotSet->param_d[0] = 240.0;                   // 結晶の最終中心(画面中央)
    pEnemyShotSet->param_d[1] = SNOW_TARGET_Y[index];
    pEnemyShotSet->param_d[2] = SNOW_RADIUS[index];
    // 回転方向は 時計回り→反時計回り→時計回り とし、サイクルごとに反転する
    pEnemyShotSet->param_d[3] = (((index + cycleNo) % 2) == 0) ? 1.0 : -1.0;
    pEnemyShotSet->param_d[4] = SNOW_THETA0[index];
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Snowflake_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    else {
        // 上部でゆるやかに左右へ揺れる
        enemy.x = 240.0 + 40.0 * sin(count * 0.012);
    }

    // 前の結晶が砕け始めるタイミング(SNOW_T_BREAK後)で次の結晶を咲かせる
    const int cycleFrame = (count - 1) % SNOW_CYCLE;
    const int cycleNo = (count - 1) / SNOW_CYCLE;
    for (int i = 0; i < SNOW_CRYSTAL_NUM; i++) {
        if (cycleFrame == SNOW_START + i * SNOW_T_BREAK) {
            CreateSnowCrystal(i, cycleNo);
        }
    }
}