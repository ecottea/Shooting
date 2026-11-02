// ============================================================
//  弾幕「杉綾織り（ヘリンボーン・ウィーブ）」
//
//  斜めに傾いた「レンガ状の弾列」を、左傾き(＼)・右傾き(／)を交互に並べて画面上部から降らせる。
//  奇数列は偶数列より半周期ぶん遅れて出現し、織り目がジグザグに噛み合う。
//
//  フェーズ:
//    1. 織り始め (count 〜240)  : 静的な杉綾がまっすぐ落ちる
//    2. 綾振り   (count 240〜)  : 行ごとに横揺れの向きを交互にし、織り目がずれていく
//                                 （行どうしの相対ずれが最大1列になる瞬間、V字/Λ字の杉綾が完成する）
//    3. 綻び     (count 600〜)  : 一部のレンガが赤く染まって予告→自機狙いの槍になって飛んでくる
//
//  使用素材:
//    銃弾(レンガ・青/シアン) / 小玉(背骨・白) / 菱形弾(アクセント・黄) / 短レーザー(槍・赤)
//
//  構造:
//    1行 = 1つの sEnemyShotSet。行ごとに偶数列(count==0)と奇数列＋背骨(count==15)を生成する。
//    レンガは「中心座標 + 軸上オフセット」で位置を決める剛体として扱う（回転して槍になれる）。
//
//  sEnemyShotSet:
//    param_i[0] = 綾振りの向き(+1/-1)  param_i[1] = アクセント行フラグ
//    param_d[0] = この行の横方向の累積移動量（奇数列・背骨を行に揃えて生成するために使う）
//  sEnemyShot:
//    param_i[0] = 種別(HB_TYPE_*)  param_i[2] = ほどけ開始時のShotSet側count(-1ならほどけない)
//    param_i[3] = 1ならレンガの代表弾（効果音・槍の生成を担当）
//    param_d[0],[1] = レンガ中心x,y  param_d[2] = 現在の軸角  param_d[3] = 軸上オフセット
//    param_d[4] = ほどけ時の狙い角  param_d[5] = 初期の軸角
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  調整用定数
// ============================================================
// --- 配置 ---
static constexpr int    HB_COLS = 15-6;      // 列数（ゲーム画面480 / 列間隔32 = 15列）
static constexpr double HB_COL_PITCH = 32.0*2;    // 列の間隔
static constexpr double HB_COL_X0 = 16.0*2-8;    // 0列目の中心x
static constexpr double HB_SPAWN_Y = 4.0;     // 生成y（画面外消去のmargin内に収めるため、上端ぎりぎりから出す）
static constexpr double HB_FALL_SPEED = 2.0;     // 落下速度
static constexpr int    HB_ROW_INTERVAL = 30*2;      // 行の生成間隔(フレーム)
static constexpr int    HB_STAGGER_FRAME = HB_ROW_INTERVAL / 2;  // 奇数列を遅らせるフレーム数（半周期）

// --- レンガ ---
static constexpr int    HB_BRICK_BULLETS = 6*2;       // 1レンガあたりの銃弾の数
static constexpr double HB_BRICK_STEP = 6.0;     // 銃弾の間隔（軸方向）
static constexpr int    HB_ACCENT_PERIOD = 4;       // アクセント行の周期（行番号 % 4 == 3 の行）

// --- 背骨（列の境目の小玉） ---
static constexpr int    HB_SPINE_BALLS = 3*2;       // 1か所あたりの小玉の数
static constexpr double HB_SPINE_STEP = 6.0;     // 小玉の縦間隔
static constexpr double HB_SPINE_Y = HB_SPAWN_Y + HB_FALL_SPEED * HB_STAGGER_FRAME / 2.0;  // 偶数・奇数レンガの中間の高さ

// --- フェーズ（グローバルcount基準） ---
static constexpr int    HB_ROW_START = 30;      // 最初の行を出す前の猶予
static constexpr int    HB_WEAVE_START = 240;     // 綾振り開始
static constexpr int    HB_UNRAVEL_START = 600;     // 綻び開始

// --- 綾振り ---
static constexpr double HB_WEAVE_AMP = 16.0;    // 片側の振れ幅（隣の行との相対で最大32＝1列分）
static constexpr double HB_WEAVE_PERIOD = 150.0;   // 往復の周期(フレーム)
static constexpr double HB_WEAVE_RAMP = 60.0;    // 綾振りが全振幅になるまでのフレーム数

// --- 綻び ---
static constexpr int    HB_UNRAVEL_PERCENT = 12;  // 各レンガがほどける確率(%)
static constexpr int    HB_UNRAVEL_DELAY_MIN = 45;  // 生成からほどけ始めるまでの最短フレーム
static constexpr int    HB_UNRAVEL_DELAY_RND = 50;  // 上に加算する乱数幅（0〜この値）
static constexpr int    HB_PREP_FRAME = 24;  // 予告（減速しながら自機の方向へ回転）のフレーム数
static constexpr int    HB_LAUNCH_RAMP = 20;  // 発射後に最高速へ達するまでのフレーム数
static constexpr double HB_LAUNCH_SPEED = 3.8; // ほどけたレンガの飛翔速度
static constexpr double HB_SPEAR_SPEED = 4.8; // 先頭の短レーザーの速度

// --- 色 (0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙) ---
static constexpr int HB_COLOR_RED = 0;
static constexpr int HB_COLOR_YELLOW = 1;
static constexpr int HB_COLOR_CYAN = 3;
static constexpr int HB_COLOR_BLUE = 4;
static constexpr int HB_COLOR_WHITE = 6;

// 弾の種別（sEnemyShot::param_i[0]）
enum {
    HB_TYPE_BRICK = 0,   // レンガ本体の銃弾
    HB_TYPE_ACCENT = 1,   // レンガ端のアクセント（菱形弾）
    HB_TYPE_SPINE = 2,   // 背骨の小玉
    HB_TYPE_SPEAR = 3,   // ほどけたレンガの先頭を飛ぶ短レーザー
};

// ============================================================
//  補助関数
// ============================================================
// 角度を (-π, π] に正規化
static double HB_NormalizeAngle(double a)
{
    while (a > DX_PI) a -= 2.0 * DX_PI;
    while (a <= -DX_PI) a += 2.0 * DX_PI;
    return a;
}

// 綾振りによる行の横方向速度。dirは行ごとの向き(+1/-1)、tはグローバルcount
// 往復運動の速度式。全振幅になるまでrampで滑らかに立ち上げる
static double HB_WeaveVx(int dir, int t)
{
    if (t < HB_WEAVE_START) return 0.0;
    const double u = (double)(t - HB_WEAVE_START);
    double ramp = u / HB_WEAVE_RAMP;
    if (ramp > 1.0) ramp = 1.0;
    return (double)dir * ramp * HB_WEAVE_AMP * (2.0 * DX_PI / HB_WEAVE_PERIOD)
        * cos(2.0 * DX_PI * u / HB_WEAVE_PERIOD);
}

// 効果音を鳴らし直す
static void HB_PlaySound(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 弾を1発生成してShotSetの連結リスト末尾に追加する
static sEnemyShot* HB_AddShot(sEnemyShotSet* pSet, int kind, double x, double y, double muki, int type)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = 0.0;
    pShot->kind = kind;
    pShot->param_i[0] = type;
    pShot->param_i[2] = -1;   // ほどけ無し
    pShot->margin = 120;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// レンガを構成する弾を1発生成する（中心(cx,cy)から軸角thの方向へaだけ離れた位置）
static void HB_AddBrickShot(sEnemyShotSet* pSet, int type, int kind,
    double cx, double cy, double th, double a, int unravelAt, bool leader)
{
    sEnemyShot* pShot = HB_AddShot(pSet, kind, cx + a * cos(th), cy + a * sin(th), th, type);
    pShot->param_i[2] = unravelAt;
    pShot->param_i[3] = leader ? 1 : 0;
    pShot->param_d[0] = cx;
    pShot->param_d[1] = cy;
    pShot->param_d[2] = th;
    pShot->param_d[3] = a;
    pShot->param_d[4] = 0.0;
    pShot->param_d[5] = th;
}

// 指定した偶奇の列のレンガを生成する (parity: 0=偶数列, 1=奇数列)
static void HB_SpawnColumns(sEnemyShotSet* pSet, int parity)
{
    const bool accentRow = (pSet->param_i[1] != 0);
    const double halfSpan = (HB_BRICK_BULLETS - 1) / 2.0 * HB_BRICK_STEP;   // 端の弾の軸上オフセット

    for (int i = parity; i < HB_COLS; i += 2) {
        // 傾きの符号: +1 = 「＼」（画面座標で右下がり）、-1 = 「／」
        const int s = (i % 2 == 0) ? 1 : -1;
        const double th = (double)s * DX_PI / 4.0;
        const double cx = HB_COL_X0 + HB_COL_PITCH * i + pSet->param_d[0];   // 行の累積移動量を足して行全体を揃える
        const double cy = HB_SPAWN_Y;
        const int color = (s > 0) ? HB_COLOR_BLUE : HB_COLOR_CYAN;

        // 綻びフェーズでは、このレンガがほどけるかを抽選する（リプレイ再現のため必ず決まった順序で乱数を使う）
        int unravelAt = -1;
        if (::count >= HB_UNRAVEL_START && GetRand(99) < HB_UNRAVEL_PERCENT) {
            unravelAt = pSet->count + HB_UNRAVEL_DELAY_MIN + GetRand(HB_UNRAVEL_DELAY_RND);
        }

        for (int k = 0; k < HB_BRICK_BULLETS; k++) {
            const double a = (k - (HB_BRICK_BULLETS - 1) / 2.0) * HB_BRICK_STEP;
            HB_AddBrickShot(pSet, HB_TYPE_BRICK, img_enemyShotBullet[color],
                cx, cy, th, a, unravelAt, k == HB_BRICK_BULLETS / 2);
        }

        // アクセント: レンガの下側の端に菱形弾を1発足す
        if (accentRow) {
            const double a = (double)s * (halfSpan + HB_BRICK_STEP);
            HB_AddBrickShot(pSet, HB_TYPE_ACCENT, img_enemyShotDiamond[HB_COLOR_YELLOW],
                cx, cy, th, a, unravelAt, false);
        }
    }
}

// 背骨（列の境目に小玉の縦の破線）を生成する
static void HB_SpawnSpine(sEnemyShotSet* pSet)
{
    for (int j = 1; j < HB_COLS; j++) {
        const double bx = HB_COL_X0 + HB_COL_PITCH * (j - 0.5) + pSet->param_d[0];
        for (int m = 0; m < HB_SPINE_BALLS; m++) {
            const double by = HB_SPINE_Y + (m - (HB_SPINE_BALLS - 1) / 2.0) * HB_SPINE_STEP;
            HB_AddShot(pSet, img_enemyShotSmallBall[HB_COLOR_WHITE], bx, by, DX_PI / 2.0, HB_TYPE_SPINE);
        }
    }
}

// ============================================================
//  弾の更新
// ============================================================
// レンガ・アクセントの更新（中心の移動 → 軸角から各弾の位置を再計算）
static void HB_UpdateBrickShot(sEnemyShotSet* pSet, sEnemyShot* pShot, double rowVx)
{
    double cx = pShot->param_d[0];
    double cy = pShot->param_d[1];
    double th = pShot->param_d[2];
    const double a = pShot->param_d[3];
    const int unravelAt = pShot->param_i[2];
    const int u = (unravelAt >= 0) ? (pSet->count - unravelAt) : -1;   // ほどけ開始からの経過フレーム

    double vx = rowVx;
    double vy = HB_FALL_SPEED;

    if (u >= 0) {
        if (u == 0) {
            // ほどけ開始: 自機の方向を記録し、赤く染めて予告音を鳴らす
            pShot->param_d[4] = atan2(player.y - cy, player.x - cx);
            pShot->kind = (pShot->param_i[0] == HB_TYPE_ACCENT)
                ? img_enemyShotDiamond[HB_COLOR_RED]
                : img_enemyShotBullet[HB_COLOR_RED];
            if (pShot->param_i[3] == 1) HB_PlaySound(sound_enemyCharge);
        }
        const double aim = pShot->param_d[4];
        const double th0 = pShot->param_d[5];

        if (u < HB_PREP_FRAME) {
            // 予告: 減速しながら、レンガの軸を自機の方向へ回していく
            const double s = (double)u / HB_PREP_FRAME;
            const double e = s * s * (3.0 - 2.0 * s);
            th = th0 + HB_NormalizeAngle(aim - th0) * e;
            vx = rowVx * (1.0 - s);
            vy = HB_FALL_SPEED * (1.0 - s);
        }
        else {
            // 発射: 軸を進行方向に揃え、加速しながら自機狙いで飛ぶ「槍」になる
            if (u == HB_PREP_FRAME && pShot->param_i[3] == 1) {
                // 代表弾が先頭に短レーザーを1本添える
                sEnemyShot* pSpear = HB_AddShot(pSet, img_enemyShotLaser[HB_COLOR_RED], cx, cy, aim, HB_TYPE_SPEAR);
                pSpear->speed = HB_SPEAR_SPEED;
                HB_PlaySound(sound_enemyShot_heavy);
            }
            th = aim;
            double ramp = (double)(u - HB_PREP_FRAME + 1) / HB_LAUNCH_RAMP;
            if (ramp > 1.0) ramp = 1.0;
            const double sp = HB_LAUNCH_SPEED * ramp;
            vx = cos(aim) * sp;
            vy = sin(aim) * sp;
        }
    }

    cx += vx;
    cy += vy;
    pShot->param_d[0] = cx;
    pShot->param_d[1] = cy;
    pShot->param_d[2] = th;
    pShot->x = cx + a * cos(th);
    pShot->y = cy + a * sin(th);
    pShot->muki = th;   // 向きはレンガの軸方向（細長い弾が棒状に見える）
    pShot->speed = sqrt(vx * vx + vy * vy);
}

// 弾幕：杉綾織り（1行ぶん）
static void HB_ShotRow(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 使える効果音: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        HB_PlaySound(sound_enemyShot_light);
        HB_SpawnColumns(pEnemyShotSet, 0);   // 偶数列
    }
    else if (pEnemyShotSet->count == HB_STAGGER_FRAME) {
        HB_SpawnColumns(pEnemyShotSet, 1);   // 奇数列（半周期遅れ）
        HB_SpawnSpine(pEnemyShotSet);        // 背骨
    }

    const double rowVx = HB_WeaveVx(pEnemyShotSet->param_i[0], ::count);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case HB_TYPE_BRICK:
        case HB_TYPE_ACCENT:
            HB_UpdateBrickShot(pEnemyShotSet, pShot, rowVx);
            break;
        case HB_TYPE_SPINE:
            pShot->x += rowVx;
            pShot->y += HB_FALL_SPEED;
            break;
        case HB_TYPE_SPEAR:
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
        pShot = pShot->next;
    }
    // 行全体の累積移動量（以降に生成する奇数列・背骨の位置合わせ用）
    pEnemyShotSet->param_d[0] += rowVx;
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Herringbone_Claude()
{
    static int rowCount;   // これまでに生成した行の数

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        rowCount = 0;
    }
    else {
        // 画面上部をゆっくり左右に揺れる
        enemy.x = 240.0 + 100.0 * sin((double)count * 2.0 * DX_PI / 360.0);
    }

    if (count > HB_ROW_START && count % HB_ROW_INTERVAL == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = HB_ShotRow;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = (rowCount % 2 == 0) ? 1 : -1;                    // 綾振りの向き（行ごとに交互）
        pEnemyShotSet->param_i[1] = (rowCount % HB_ACCENT_PERIOD == HB_ACCENT_PERIOD - 1) ? 1 : 0;   // アクセント行
        pEnemyShotSet->param_d[0] = 0.0;                                             // 行の累積移動量
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
        rowCount++;
    }
}