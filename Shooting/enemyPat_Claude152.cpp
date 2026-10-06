#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：等長の罠（ミュラーリヤー錯視）
//
//  同じ長さのシャフトを持つ2本の「棒」が降りてくる。
//    棒A(id 0): 矢羽が内向き  <――>  … 実際より短く見える
//    棒B(id 1): 矢羽が外向き  >――<  … 実際より長く見える
//  最初は左右にずれて降下し、途中でスライドして端点がそろう。
//  そろった瞬間に両端点から縦の補助線(短レーザー)が伸び、
//  「実は同じ長さだった」ことを弾の配置で示す。
//
//  1つのShotSetが1組(棒A+棒B)を担当し、ShotSetのcountを時刻として
//  各弾の位置を閉じた式で求める（棒を構成する弾は互いの相対位置が崩れない）。
// ============================================================

// ---- 調整用パラメータ ----
static const double kScreenCenterX = 240.0;
static const double kBarHalf = 130.0;   // シャフト長の半分（全長260）
static const double kBarStagger = 30.0;    // 最初に中央から左右へずらす量
static const double kBarSpeedY = 1.2;     // 降下速度
static const double kBarUpperY = 90.0-60;    // 上側の棒の初期y
static const double kBarLowerY = 170.0-60;   // 下側の棒の初期y
static const double kFinLength = 64.0;    // 矢羽の長さ
static const double kFinAngle = 30.0 * DX_PI / 180.0; // 矢羽とシャフトの角度
static const double kFinStrokeGap = 2.0;     // 矢羽の2本線の片側ずれ量
static const double kShaftRowGap = 3.5;     // シャフトの列間隔
static const int    kShaftCount = 44;      // シャフトの1列あたりの弾数
static const int    kShaftRows = 3;       // シャフトの列数
static const int    kFinCount = 10;      // 矢羽1本線あたりの分割数（弾数は+1）

// ---- 時間割（ShotSetのcount基準・フレーム） ----
static const int kFormEnd = 40;   // 0〜40: ボスから各位置へ展開
static const int kSlideStart = 90;   // 90〜160: 左右にスライドして端点をそろえる
static const int kSlideEnd = 160;
static const int kReleaseTime = 160;  // 端点マーカー射出・補助線開始
static const int kVolleyStart = 60;   // 矢羽先端からの自機狙い
static const int kVolleyEnd = 135;
static const int kVolleyInterval = 25;
static const int kGuideInterval = 10;   // 補助線の発射間隔
static const int kGuideCount = 9;    // 補助線の発射回数

// ---- 弾のロール／部位 ----
enum { kRoleFree = 0, kRoleBar = 1 };                 // param_i[0]（0は通常の直進弾）
enum { kPartShaft = 0, kPartWing = 1, kPartMarker = 2 }; // param_i[3]
// param_i[1]: 棒のid  param_i[2]: 矢羽の先端フラグ
// param_d[0],[1]: 棒の中心からの相対位置  param_d[2]: 矢羽の向き

// 効果音（連続再生で途切れないよう一度止めてから鳴らす）
static void PlaySe(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 弾を1発追加して返す
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, int kind, double x, double y, double muki, double speed)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// 時刻tにおける棒の中心位置
static void BarAnchor(const sEnemyShotSet* pEnemyShotSet, int barId, int t, double* ax, double* ay)
{
    const double cx0 = pEnemyShotSet->param_d[barId * 2];
    const double cy0 = pEnemyShotSet->param_d[barId * 2 + 1];
    double u = (double)(t - kSlideStart) / (double)(kSlideEnd - kSlideStart);
    if (u < 0.0) u = 0.0;
    if (u > 1.0) u = 1.0;
    const double s = u * u * (3.0 - 2.0 * u); // smoothstep
    int td = t - kFormEnd;
    if (td < 0) td = 0;
    *ax = cx0 + (kScreenCenterX - cx0) * s;
    *ay = cy0 + kBarSpeedY * (double)td;
}

// 棒を構成する弾を1発追加
static void AddBarShot(sEnemyShotSet* pEnemyShotSet, int barId, int part, int kind,
    double dx, double dy, double wingMuki, bool tip)
{
    // 展開の最終目標位置への向きを初期のmukiにする
    double ax, ay;
    BarAnchor(pEnemyShotSet, barId, 0, &ax, &ay);
    const double tx = ax + dx;
    const double ty = ay + dy;
    sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet, kind, pEnemyShotSet->x, pEnemyShotSet->y,
        atan2(ty - pEnemyShotSet->y, tx - pEnemyShotSet->x), 0.0);
    pEnemyShot->param_i[0] = kRoleBar;
    pEnemyShot->param_i[1] = barId;
    pEnemyShot->param_i[2] = tip ? 1 : 0;
    pEnemyShot->param_i[3] = part;
    pEnemyShot->param_d[0] = dx;
    pEnemyShot->param_d[1] = dy;
    pEnemyShot->param_d[2] = wingMuki;
    if (part == kPartWing) pEnemyShot->muki = wingMuki;
}

// 棒1本分（シャフト・矢羽・端点マーカー）を生成
static void SpawnBar(sEnemyShotSet* pEnemyShotSet, int barId)
{
    const bool inward = (barId == 0); // 棒Aは矢羽内向き、棒Bは外向き
    const int shaftKind = inward ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[4]; // 赤 / 青
    const int wingKind = inward ? img_enemyShotDiamond[8] : img_enemyShotDiamond[3];   // 橙 / シアン
    const int markerKind = img_enemyShotMediumBall[1];                                      // 黄

    // シャフト
    for (int row = 0; row < kShaftRows; row++) {
        for (int i = 0; i < kShaftCount; i++) {
            const double dx = -kBarHalf + 2.0 * kBarHalf * (double)i / (double)(kShaftCount - 1);
            const double dy = ((double)row - (double)(kShaftRows - 1) / 2.0) * kShaftRowGap;
            AddBarShot(pEnemyShotSet, barId, kPartShaft, shaftKind, dx, dy, 0.0, false);
        }
    }

    for (int end = 0; end < 2; end++) { // 0:左端 1:右端
        const double vx = (end == 0) ? -kBarHalf : kBarHalf;

        // 矢羽（端点を頂点とするV字。2本線で太さを出す）
        const double dirX = ((end == 0) ? 1.0 : -1.0) * (inward ? 1.0 : -1.0);
        for (int side = 0; side < 2; side++) {
            const double dirY = (side == 0) ? -1.0 : 1.0;
            const double ux = cos(kFinAngle) * dirX;
            const double uy = sin(kFinAngle) * dirY;
            const double px = -uy; // 矢羽に垂直な方向
            const double py = ux;
            const double wingMuki = atan2(uy, ux);
            for (int stroke = 0; stroke < 2; stroke++) {
                const double off = (stroke == 0) ? -kFinStrokeGap : kFinStrokeGap;
                for (int k = 0; k <= kFinCount; k++) {
                    const double s = kFinLength * (double)k / (double)kFinCount;
                    const double dx = vx + ux * s + px * off;
                    const double dy = uy * s + py * off;
                    AddBarShot(pEnemyShotSet, barId, kPartWing, wingKind, dx, dy, wingMuki,
                        (k == kFinCount && stroke == 0));
                }
            }
        }

        // 端点マーカー
        AddBarShot(pEnemyShotSet, barId, kPartMarker, markerKind, vx, 0.0, 0.0, false);
    }
}

// 棒を構成する弾の位置・向きを更新
static void UpdateBarShot(const sEnemyShotSet* pEnemyShotSet, sEnemyShot* pShot, int t)
{
    const int barId = pShot->param_i[1];
    const double dx = pShot->param_d[0];
    const double dy = pShot->param_d[1];
    double nx, ny;

    if (t < kFormEnd) {
        // ボスの位置から目標位置へイーズアウトで展開
        double ax, ay;
        BarAnchor(pEnemyShotSet, barId, 0, &ax, &ay);
        const double u = (double)t / (double)kFormEnd;
        const double e = 1.0 - (1.0 - u) * (1.0 - u) * (1.0 - u);
        nx = pEnemyShotSet->x + (ax + dx - pEnemyShotSet->x) * e;
        ny = pEnemyShotSet->y + (ay + dy - pEnemyShotSet->y) * e;
    }
    else {
        double ax, ay;
        BarAnchor(pEnemyShotSet, barId, t, &ax, &ay);
        nx = ax + dx;
        ny = ay + dy;
    }

    const double mx = nx - pShot->x;
    const double my = ny - pShot->y;
    const double d2 = mx * mx + my * my;
    pShot->speed = sqrt(d2);
    if (pShot->param_i[3] == kPartWing) {
        pShot->muki = pShot->param_d[2];  // 矢羽は常に矢羽の線の向き
    }
    else if (d2 > 1.0e-6) {
        pShot->muki = atan2(my, mx);      // それ以外は実際の進行方向
    }
    pShot->x = nx;
    pShot->y = ny;
}

// 弾幕：棒A+棒B 1組
static void ShotBars(sEnemyShotSet* pEnemyShotSet)
{
    const int t = pEnemyShotSet->count;
    sEnemyShot* const pHead = pEnemyShotSet->pEnemyShotHead;

    if (t == 0) {
        PlaySe(sound_enemyCharge);
        SpawnBar(pEnemyShotSet, 0);
        SpawnBar(pEnemyShotSet, 1);
    }
    if (t == kFormEnd) PlaySe(sound_enemyShot_medium);
    if (t == kReleaseTime) PlaySe(sound_enemyShot_heavy);

    // 全弾の更新
    sEnemyShot* pShot = pHead->next;
    while (pShot != pHead) {
        // 端点マーカーは棒から離れて自機狙いの直進弾になる
        if (pShot->param_i[0] == kRoleBar && pShot->param_i[3] == kPartMarker && t == kReleaseTime) {
            pShot->param_i[0] = kRoleFree;
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 3.2;
        }
        if (pShot->param_i[0] == kRoleBar) {
            UpdateBarShot(pEnemyShotSet, pShot, t);
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // 矢羽の先端から自機狙い3way
    if (t >= kVolleyStart && t <= kVolleyEnd && (t - kVolleyStart) % kVolleyInterval == 0) {
        PlaySe(sound_enemyShot_light);
        sEnemyShot* pTip = pHead->next;
        while (pTip != pHead) {
            if (pTip->param_i[0] == kRoleBar && pTip->param_i[2] == 1) {
                const double aim = atan2(player.y - pTip->y, player.x - pTip->x);
                const int kind = (pTip->param_i[1] == 0) ? img_enemyShotBullet[5] : img_enemyShotBullet[2]; // マゼンタ / 緑
                for (int j = -1; j <= 1; j++) {
                    AddShot(pEnemyShotSet, kind, pTip->x, pTip->y, aim + 0.14 * (double)j, 2.6);
                }
            }
            pTip = pTip->next;
        }
    }

    // 端点がそろった後、両端点から縦の補助線（上下方向）
    if (t >= kReleaseTime && t < kReleaseTime + kGuideInterval * kGuideCount
        && (t - kReleaseTime) % kGuideInterval == 0) {
        for (int barId = 0; barId < 2; barId++) {
            double ax, ay;
            BarAnchor(pEnemyShotSet, barId, t, &ax, &ay);
            for (int end = 0; end < 2; end++) {
                const double gx = ax + ((end == 0) ? -kBarHalf : kBarHalf);
                AddShot(pEnemyShotSet, img_enemyShotLaser[6], gx, ay, DX_PI / 2.0, 6.0); // 下向き（白）
                AddShot(pEnemyShotSet, img_enemyShotLaser[6], gx, ay, -DX_PI / 2.0, 6.0); // 上向き（白）
            }
        }
    }
}

// 1組分のShotSetを生成
//   swap  : 0なら棒Aが上、1なら棒Bが上
//   mirror: 0なら棒Aが左にずれる、1なら棒Aが右にずれる
static void SpawnWave(int swap, int mirror)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotBars;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;
    pEnemyShotSet->muki = DX_PI / 2.0;
    pEnemyShotSet->kind = 0;
    pEnemyShotSet->param_i[0] = swap;
    pEnemyShotSet->param_i[1] = mirror;

    const double offset = mirror ? kBarStagger : -kBarStagger; // 棒Aのずれ。棒Bは逆方向
    pEnemyShotSet->param_d[0] = kScreenCenterX + offset;                // 棒Aの初期x
    pEnemyShotSet->param_d[1] = swap ? kBarLowerY : kBarUpperY;        // 棒Aの初期y
    pEnemyShotSet->param_d[2] = kScreenCenterX - offset;                // 棒Bの初期x
    pEnemyShotSet->param_d[3] = swap ? kBarUpperY : kBarLowerY;        // 棒Bの初期y

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 出現スケジュール（1サイクル内のフレーム、上下入れ替え、左右反転）
struct sWaveEvent {
    int frame;
    int swap;
    int mirror;
};
static const sWaveEvent kWaveSchedule[] = {
    {   0, 0, 0 },
    { 240, 1, 1 },
    { 480, 0, 1 },  // ここから3組を60フレーム間隔でたたみかける
    { 540, 1, 0 },
    { 600, 0, 0 },
};
static const int kCycleLength = 960;

// 敵本体のパターン
void EnemyPat_MullerLyer_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    enemy.x = kScreenCenterX + 70.0 * sin((double)(count - 1) * 0.02);

    const int f = (count - 1) % kCycleLength;
    const int n = (int)(sizeof(kWaveSchedule) / sizeof(kWaveSchedule[0]));
    for (int i = 0; i < n; i++) {
        if (f == kWaveSchedule[i].frame) {
            SpawnWave(kWaveSchedule[i].swap, kWaveSchedule[i].mirror);
        }
    }
}