#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <algorithm>

// ============================================================
//  弾幕：シダの胞子（バーンズリーのシダ）
//
//  バーンズリーのシダ(4つのアフィン変換の反復)で求めた点に弾を置き、
//  敵本体から吊り下がるシダを弾の点描で描く。
//    フェーズ1 成長   (0〜359f)   根元から葉先に向かって弾が置かれていく
//    フェーズ2 風     (360〜479f) 葉先ほど大きく左右に揺れる(予告)
//    フェーズ3 胞子散布 (480〜719f) 葉先から順に弾が離れて降り注ぎ、
//                                  茎からも扇状の胞子弾が出る
//  使用素材：
//    小玉・緑   … 葉
//    中玉・橙   … 茎(兼、胞子の発射元)
//    中玉・白   … 茎から出る胞子弾
// ============================================================

// ---- タイムライン(弾幕セットの count 基準) ----
static const int FERN_GROW_FRAMES = 360-180;  // 成長にかける時間(0〜359f)
static const int FERN_WIND_START = 360-180;  // 風の開始
static const int FERN_RELEASE_START = 480-180;  // 胞子散布の開始
static const int FERN_RELEASE_SPAN = 180;  // 葉先→根元へ放出が進む時間
static const int FERN_RELEASE_JITTER = 15;   // 放出タイミングのばらつき(0〜この値)
static const int FERN_CYCLE_FRAMES = 780-180;  // 敵本体が弾幕を撃ち直す間隔(散布終了後の余韻込み)

// ---- シダの形状 ----
static const int    FERN_LEAF_NUM = 800+100;   // 葉の弾数
static const int    FERN_WARMUP = 10;    // 反復の最初に捨てる点数
static const int    FERN_STEM_ITER = 20;    // 茎(f2の反復軌道)を追う回数
static const double FERN_STEM_SPACING = 9.0;   // 茎の弾の間隔(px) 中玉の直径14より小さくして連続した線にする
static const double FERN_SCALE_X = 28.0;  // IFS座標1あたりのpx(横)
static const double FERN_SCALE_Y = 28.0;  // IFS座標1あたりのpx(縦) 高さ約280px
static const double FERN_X_OFFSET = 0.25;  // シダの横幅(-2.2〜2.7)の中心を敵の真下に合わせる補正
static const int    FERN_POINT_MAX = 1200;  // 点テーブルの容量

// ---- 動き ----
static const double FERN_SWAY_AMP = 16.0;  // 風による最大の揺れ幅(px) 葉先で最大
static const double FERN_SWAY_SPEED = 0.10;  // 揺れの角速度(rad/f)
static const double FERN_SWAY_RAMP = 60.0;  // 揺れが最大になるまでのフレーム数
static const int    FERN_ACCEL_FRAMES = 30;    // 放出された弾が最高速になるまでのフレーム数

// ---- 茎から出る胞子弾 ----
static const int    FERN_SPORE_INTERVAL = 20;    // 発射間隔
static const int    FERN_EMITTER_NUM = 3;     // 1回で撃つ発射元の数
static const int    FERN_SPORE_WAY = 5;     // 扇状弾のway数
static const double FERN_SPORE_SPREAD = 0.20;  // way間の角度(rad)
static const int    FERN_EMITTER_CAND_MAX = 256; // 発射元候補の最大数

// ---- 弾の役割と param の割り当て ----
enum { FERN_TYPE_LEAF = 0, FERN_TYPE_STEM = 1, FERN_TYPE_SPORE = 2 };
// sEnemyShot::param_i
enum { FERN_PI_TYPE = 0, FERN_PI_RELEASE = 1, FERN_PI_RELEASED = 2 };
// sEnemyShot::param_d  アンカー=風で揺れる前の定位置 / WEIGHT=揺れやすさ(葉先ほど1に近い) / SPEED_MAX=放出後の最高速
enum { FERN_PD_ANCHOR_X = 0, FERN_PD_ANCHOR_Y = 1, FERN_PD_WEIGHT = 2, FERN_PD_SPEED_MAX = 3 };
// sEnemyShotSet::param_i  成長フェーズで既に配置した点の数
enum { FERN_SET_REVEALED = 0 };

// ---- シダの点テーブル(弾幕セット生成時に作り直す) ----
namespace {
    struct FernPoint {
        double ix, iy;   // IFS座標(元のシダの座標系)
        double dist;     // 根元(0,0)からの距離(IFS座標系)
        bool   isStem;   // 茎の点か
    };
}
static FernPoint fernPoints[FERN_POINT_MAX];
static int       fernPointNum = 0;
static double    fernMaxDist = 1.0;

// 効果音(連続再生で音が重ならないよう止めてから鳴らす)
static void FernPlaySE(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 弾を1つ生成して弾幕セットのリスト末尾に追加する
static sEnemyShot* FernAddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind, int type)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[FERN_PI_TYPE] = type;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// バーンズリーのシダ：確率的に4つの変換から1つ選んで点(x, y)を1回進める
static void FernStep(double& x, double& y)
{
    // GetRand(99) は 0〜99 の100通り
    const int r = GetRand(99);
    double nx, ny;
    if (r == 0) {            // f1 (1%)  茎
        nx = 0.0;
        ny = 0.16 * y;
    }
    else if (r <= 85) {      // f2 (85%) 次の小葉へ
        nx = 0.85 * x + 0.04 * y;
        ny = -0.04 * x + 0.85 * y + 1.6;
    }
    else if (r <= 92) {      // f3 (7%)  左の枝
        nx = 0.2 * x - 0.26 * y;
        ny = 0.23 * x + 0.22 * y + 1.6;
    }
    else {                   // f4 (7%)  右の枝
        nx = -0.15 * x + 0.28 * y;
        ny = 0.26 * x + 0.24 * y + 0.44;
    }
    x = nx;
    y = ny;
}

static void FernAddPoint(double ix, double iy, bool isStem)
{
    if (fernPointNum >= FERN_POINT_MAX) return;
    FernPoint& fp = fernPoints[fernPointNum++];
    fp.ix = ix;
    fp.iy = iy;
    fp.dist = sqrt(ix * ix + iy * iy);
    fp.isStem = isStem;
}

// シダの点テーブルを作る(葉の点＋茎の点を、根元に近い順に並べる)
static void FernBuild()
{
    fernPointNum = 0;

    // 葉：IFSを反復して点を打つ
    double x = 0.0, y = 0.0;
    for (int i = 0; i < FERN_WARMUP + FERN_LEAF_NUM; i++) {
        FernStep(x, y);
        if (i >= FERN_WARMUP) FernAddPoint(x, y, false);
    }

    // 茎：(0,0)からf2だけを反復した軌道がシダの主軸になる。
    // 反復点の間隔は広いので、弾が連続した線になるよう線形補間で埋める
    x = 0.0;
    y = 0.0;
    for (int i = 0; i < FERN_STEM_ITER; i++) {
        const double nx = 0.85 * x + 0.04 * y;
        const double ny = -0.04 * x + 0.85 * y + 1.6;
        const double dx = (nx - x) * FERN_SCALE_X;
        const double dy = (ny - y) * FERN_SCALE_Y;
        const int n = (int)(sqrt(dx * dx + dy * dy) / FERN_STEM_SPACING) + 1;
        for (int k = 0; k < n; k++) {
            FernAddPoint(x + (nx - x) * k / n, y + (ny - y) * k / n, true);
        }
        x = nx;
        y = ny;
    }
    FernAddPoint(x, y, true);

    // 根元(y小)から葉先(y大)の順に並べる → 成長フェーズで下へ伸びるように見える
    std::sort(fernPoints, fernPoints + fernPointNum, [](const FernPoint& a, const FernPoint& b) {
        if (a.iy != b.iy) return a.iy < b.iy;
        return a.ix < b.ix;
    });

    fernMaxDist = 0.0;
    for (int i = 0; i < fernPointNum; i++) {
        if (fernPoints[i].dist > fernMaxDist) fernMaxDist = fernPoints[i].dist;
    }
    if (fernMaxDist <= 0.0) fernMaxDist = 1.0;
}

// フェーズ1：時間に比例して、点テーブルの先頭から順に弾を置いていく
static void FernGrow(sEnemyShotSet* pEnemyShotSet, int t)
{
    const int target = fernPointNum * (t + 1) / FERN_GROW_FRAMES;
    while (pEnemyShotSet->param_i[FERN_SET_REVEALED] < target) {
        const FernPoint& fp = fernPoints[pEnemyShotSet->param_i[FERN_SET_REVEALED]];
        pEnemyShotSet->param_i[FERN_SET_REVEALED]++;

        const double x = pEnemyShotSet->x + (fp.ix - FERN_X_OFFSET) * FERN_SCALE_X;
        const double y = pEnemyShotSet->y + fp.iy * FERN_SCALE_Y;
        // 静止中の弾の向きは「根元から見た成長方向(葉先側)」にしておく
        const double muki = (fp.dist > 0.0) ? atan2(fp.iy * FERN_SCALE_Y, fp.ix * FERN_SCALE_X) : DX_PI / 2.0;

        sEnemyShot* pEnemyShot;
        if (fp.isStem) {
            pEnemyShot = FernAddShot(pEnemyShotSet, x, y, muki, 0.0, img_enemyShotMediumBall[8], FERN_TYPE_STEM);
        }
        else {
            pEnemyShot = FernAddShot(pEnemyShotSet, x, y, muki, 0.0, img_enemyShotSmallBall[2], FERN_TYPE_LEAF);
        }

        // 葉先に近い弾ほど早く放出される
        const double ratio = fp.dist / fernMaxDist;   // 0:根元 〜 1:葉先
        pEnemyShot->param_i[FERN_PI_RELEASE] = FERN_RELEASE_START + (int)((1.0 - ratio) * FERN_RELEASE_SPAN) + GetRand(FERN_RELEASE_JITTER);
        pEnemyShot->param_d[FERN_PD_ANCHOR_X] = x;
        pEnemyShot->param_d[FERN_PD_ANCHOR_Y] = y;
        pEnemyShot->param_d[FERN_PD_WEIGHT] = ratio * ratio;
    }
}

// フェーズ3：まだ残っている茎の弾から、自機狙いの扇状弾(胞子)を撃つ
static void FernFireSpores(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* candidates[FERN_EMITTER_CAND_MAX];
    int candidateNum = 0;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead && candidateNum < FERN_EMITTER_CAND_MAX) {
        if (pShot->param_i[FERN_PI_TYPE] == FERN_TYPE_STEM && pShot->param_i[FERN_PI_RELEASED] == 0) {
            candidates[candidateNum++] = pShot;
        }
        pShot = pShot->next;
    }
    if (candidateNum == 0) return;

    FernPlaySE(sound_enemyShot_light);
    for (int e = 0; e < FERN_EMITTER_NUM; e++) {
        const sEnemyShot* pSrc = candidates[GetRand(candidateNum - 1)];
        const double base = atan2(player.y - pSrc->y, player.x - pSrc->x);
        const double speed = 1.7 + e * 0.4;   // 発射元ごとに速さをずらして奥行きを出す
        for (int k = 0; k < FERN_SPORE_WAY; k++) {
            const double muki = base + (k - (FERN_SPORE_WAY - 1) / 2.0) * FERN_SPORE_SPREAD;
            FernAddShot(pEnemyShotSet, pSrc->x, pSrc->y, muki, speed, img_enemyShotMediumBall[6], FERN_TYPE_SPORE);
        }
    }
}

// 弾幕：シダの胞子
static void ShotFern(sEnemyShotSet* pEnemyShotSet)
{
    const int t = pEnemyShotSet->count;

    if (t == 0) {
        FernBuild();
        pEnemyShotSet->param_i[FERN_SET_REVEALED] = 0;
    }

    // フェーズ1：成長
    if (t < FERN_GROW_FRAMES) {
        FernGrow(pEnemyShotSet, t);
        if (t % 24 == 0) FernPlaySE(sound_enemyShot_light);
    }
    // フェーズ2：風(予告音)
    if (t == FERN_WIND_START) FernPlaySE(sound_enemyCharge);
    // フェーズ3：胞子散布
    if (t == FERN_RELEASE_START) FernPlaySE(sound_enemyShot_medium);
    if (t >= FERN_RELEASE_START && t < FERN_RELEASE_START + FERN_RELEASE_SPAN
        && (t - FERN_RELEASE_START) % FERN_SPORE_INTERVAL == 0) {
        FernFireSpores(pEnemyShotSet);
    }

    // 全弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[FERN_PI_TYPE] == FERN_TYPE_SPORE) {
            // 胞子弾は等速直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 放出時刻になったら、定位置から離れて動き出す
            if (pShot->param_i[FERN_PI_RELEASED] == 0 && t >= pShot->param_i[FERN_PI_RELEASE]) {
                pShot->param_i[FERN_PI_RELEASED] = 1;
                if (GetRand(3) == 0) {
                    // 4発に1発は自機狙い(±15度)
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x) + (GetRand(30) - 15) / 180.0 * DX_PI;
                }
                else {
                    // 残りは下向きにばらける(真下を中心に±70度)
                    pShot->muki = DX_PI / 2.0 + (GetRand(140) - 70) / 180.0 * DX_PI*3;
                }
                pShot->param_d[FERN_PD_SPEED_MAX] = 1.4 + GetRand(80) / 100.0;
                pShot->speed = 0.0;
            }

            if (pShot->param_i[FERN_PI_RELEASED] == 1) {
                // 放出後：加速しつつ直進
                const int age = t - pShot->param_i[FERN_PI_RELEASE];
                const double rate = (age + 1 >= FERN_ACCEL_FRAMES) ? 1.0 : (age + 1) / (double)FERN_ACCEL_FRAMES;
                pShot->speed = pShot->param_d[FERN_PD_SPEED_MAX] * rate;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (t >= FERN_WIND_START) {
                // 風：定位置を中心に左右へ揺れる(葉先ほど大きく)
                double ramp = (t - FERN_WIND_START) / FERN_SWAY_RAMP;
                if (ramp > 1.0) ramp = 1.0;
                const double offset = ramp * FERN_SWAY_AMP * pShot->param_d[FERN_PD_WEIGHT] * sin((t - FERN_WIND_START) * FERN_SWAY_SPEED);
                pShot->x = pShot->param_d[FERN_PD_ANCHOR_X] + offset;
            }
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_BarnsleyFern_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    // 敵の真下を根元にして、吊り下がるシダを一定周期で生やす
    if (count % FERN_CYCLE_FRAMES == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}