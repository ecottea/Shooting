// ============================================================
//  弾幕「ナワバリバトル」(スプラトゥーン風)
//
//  橙(8)と青(4)のインクで戦場を塗り合う弾幕。1サイクル1320フレーム(22秒)でループする。
//    ① 0〜300     ショット連射   : 銃弾のストリーム(橙は時計回り、青は反時計回り)+小玉の飛沫
//    ② 300〜780   ボム投擲と着弾 : 大玉を投げ、着弾でスプラッシュ(星形のスパイク)とインクだまりを作る
//    ③ 780〜1080  ローラー振り   : 中楕円弾の壁(隙間あり)が左右に振れながら迫り、後ろにインクが滴る
//    ④ 1080〜1320 スーパーチャクチ: 敵が跳び上がり、予告→黒い大玉が着弾→全方位にレーザーとリング
//
//  設計メモ:
//   ・弾の削除はメインルーチンに任せるため、全ての弾が最終的に「直進」か「落下」で画面外へ出ていく
//   ・1つのShotSetが空になる瞬間が無いように、ボム/フィナーレ/ローラーは1つのShotSetが全ての弾を管理する
//   ・弾の役割は param_i[0] に入れる(0は直進なので、何も設定しない弾は直進になる)
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  色(img_enemyShotXxx[色] の添字)
//  0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
// ------------------------------------------------------------
static const int NB_INK_BLUE = 4;
static const int NB_INK_WHITE = 6;
static const int NB_INK_BLACK = 7;
static const int NB_INK_ORANGE = 8;

// ------------------------------------------------------------
//  タイムライン(サイクル内フレーム t)
// ------------------------------------------------------------
static const int NB_CYCLE = 1320+60;           // 1サイクルの長さ
static const int NB_STREAM_BEGIN = 30;      // ① ストリーム開始(それまでは予告)
static const int NB_PHASE2_BEGIN = 300;     // ② 開始
static const int NB_BOMB_BEGIN = 330;       // 最初のボムを投げるt
static const int NB_BOMB_INTERVAL = 30;     // ボムを投げる間隔
static const int NB_BOMB_NUM = 12-1;          // ボムの数(橙青交互)
static const int NB_PHASE3_BEGIN = 780+60;     // ③ 開始
static const int NB_ROLLER_BEGIN = 800+60;     // 最初のローラー振りのt
static const int NB_ROLLER_INTERVAL = 50;   // ローラー振りの間隔
static const int NB_ROLLER_NUM = 6;         // ローラー振りの回数
static const int NB_PHASE4_BEGIN = 1080+60;    // ④ 開始(敵が跳び上がる)
static const int NB_FINALE_BEGIN = 1110+60;    // フィナーレの弾幕を生成するt
static const int NB_RETURN_BEGIN = 1230+60;    // 敵が画面内へ戻り始めるt

// ------------------------------------------------------------
//  各種パラメータ
// ------------------------------------------------------------
static const int    NB_STREAM_NUM = 4;          // 1色あたりのストリーム本数
static const int    NB_BOMB_FLIGHT = 50;        // ボムの飛行時間(フレーム)
static const int    NB_PUDDLE_LIFE = 180;       // インクだまりの寿命(フレーム)
static const int    NB_SPIKE_NUM = 14-6;          // スプラッシュのスパイク本数
static const int    NB_ORBIT_NUM = 8;           // インクだまりを周回する小玉の数
static const double NB_ORBIT_R = 36.0;          // 周回半径
static const double NB_ORBIT_W = 0.05;          // 周回の角速度(rad/フレーム)
static const int    NB_WALL_NUM = 28;           // ローラーの壁を構成する弾の本数(1列)
static const int    NB_WALL_GAP_HALF = 2;       // 隙間の半幅(片側の弾数)。隙間は5本分
static const double NB_WALL_PITCH = 12.0;       // 壁の弾の間隔
static const double NB_WALL_SPEED = 3.0;        // 壁の速さ
static const double NB_GRAVITY = 0.05;          // 滴り落ちるインクの重力加速度
static const double NB_FINALE_X = 240.0;        // スーパーチャクチの着弾点
static const double NB_FINALE_Y = 210.0;
static const int    NB_FINALE_DROP_BEGIN = 50;  // フィナーレ内: 黒い大玉が落ち始める時刻
static const int    NB_FINALE_LAND = 90;        // フィナーレ内: 着弾時刻(予告の収束完了と同時)
static const int    NB_FINALE_DROP_TIME = NB_FINALE_LAND - NB_FINALE_DROP_BEGIN;

// ------------------------------------------------------------
//  弾の役割(param_i[0])。0を直進にしておくことで、未設定の弾は直進になる
// ------------------------------------------------------------
enum {
    NB_ROLE_STRAIGHT = 0,   // 等速直進
    NB_ROLE_BOMB,           // 投擲中のボム(減速して着弾点で止まる)
    NB_ROLE_PUDDLE,         // インクだまり(その場で停止、寿命が来たら沈んで落ちる)
    NB_ROLE_ORBIT,          // インクだまりの周りを回る小玉
    NB_ROLE_FALL,           // 重力で滴り落ちる弾
    NB_ROLE_WALL,           // ローラーの壁を構成する弾(直進+後ろにインクを垂らす)
    NB_ROLE_CONVERGE,       // 予告: 着弾点へ収束する白い小玉
    NB_ROLE_DROP            // 上空から落ちてくる黒い大玉
};
//  param_i[1]: 経過フレーム(BOMB/PUDDLE/ORBIT) / 壁の位置 k+列*100 (WALL) / リング番号 (CONVERGE)
//  param_i[2]: チーム 0=橙 1=青 (BOMB/WALL) / 寿命 (PUDDLE/ORBIT)
//  param_d[0]〜[3]: BOMB=減速量,-,着弾x,着弾y / ORBIT=中心x,中心y,現在角度,- / CONVERGE=角度,初期半径,放射速度
//  param_d[4]: ORBITの角速度
//  param_d[6],[7]: FALLの速度ベクトル(vx,vy)

// ------------------------------------------------------------
//  ユーティリティ
// ------------------------------------------------------------
static int InkColor(int team)
{
    return (team == 0) ? NB_INK_ORANGE : NB_INK_BLUE;
}

// -w 以上 +w 以下の実数(決定論的RNG使用)
static double RandSigned(double w)
{
    return (GetRand(2000) - 1000) / 1000.0 * w;
}

static double ClampD(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// 効果音を頭から鳴らし直す
static void PlaySe(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ShotSetを生成してリストに繋ぐ
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->kind = 0;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// ShotSetの弾リスト末尾に弾を1つ追加する
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
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

// 弾を「滴り落ちる弾」にする。向きと速さは速度ベクトルから決める
static void SetFall(sEnemyShot* pShot, double vx, double vy)
{
    pShot->param_i[0] = NB_ROLE_FALL;
    pShot->param_d[6] = vx;
    pShot->param_d[7] = vy;
    pShot->speed = sqrt(vx * vx + vy * vy);
    pShot->muki = atan2(vy, vx);
}

// ------------------------------------------------------------
//  ボムの着弾: スプラッシュ(星形スパイク+飛沫+白いハイライト)を出し、ボム本体をインクだまりにする
// ------------------------------------------------------------
static void BombLand(sEnemyShotSet* pSet, sEnemyShot* pBomb)
{
    const int team = pBomb->param_i[2];
    const int col = InkColor(team);
    const double cx = pBomb->param_d[2];
    const double cy = pBomb->param_d[3];

    // ボム本体 → インクだまり
    pBomb->x = cx;
    pBomb->y = cy;
    pBomb->speed = 0.0;
    pBomb->muki = DX_PI / 2.0;
    pBomb->param_i[0] = NB_ROLE_PUDDLE;
    pBomb->param_i[1] = 0;
    pBomb->param_i[2] = NB_PUDDLE_LIFE;

    PlaySe(sound_enemyShot_medium);

    const double base = GetRand(359) * DX_PI / 180.0;
    const double step = 2.0 * DX_PI / NB_SPIKE_NUM;

    // スパイク: 速度の違う中楕円弾を同じ方向に並べると、時間とともに放射状の線(スパイク)に伸びる
    // 偶数番は長いスパイク(5発、先端は白)、奇数番は短いスパイク(3発)
    for (int k = 0; k < NB_SPIKE_NUM; k++) {
        double jitter = RandSigned(0.06);
        double ang = base + step * k + jitter;
        bool isLong = (k % 2 == 0);
        int num = isLong ? 5 : 3;
        for (int j = 0; j < num; j++) {
            double speed = isLong ? (1.3 + 0.65 * j) : (1.1 + 0.5 * j);
            speed += RandSigned(0.1);
            int c = col;
            if (isLong && j == num - 1) c = NB_INK_WHITE;
            AddShot(pSet, cx, cy, ang, speed, img_enemyShotMediumOval[c]);
        }
    }

    // 飛沫: スパイクの隙間に小玉を3発ずつ
    //for (int k = 0; k < NB_SPIKE_NUM; k++) {
    //    double dir = base + step * (k + 0.5);
    //    for (int j = 0; j < 3; j++) {
    //        double ang = dir + RandSigned(0.12);
    //        double speed = 1.4 + GetRand(150) / 100.0;
    //        AddShot(pSet, cx, cy, ang, speed, img_enemyShotSmallBall[col]);
    //    }
    //}

    // ハイライト: 白い小玉を6発
    for (int i = 0; i < 6; i++) {
        double ang = base + i * DX_PI / 3.0;
        AddShot(pSet, cx, cy, ang, 2.2, img_enemyShotSmallBall[NB_INK_WHITE]);
    }

    // インクだまりを周回する小玉(橙は時計回り、青は反時計回り)
    const double w = (team == 0) ? NB_ORBIT_W : -NB_ORBIT_W;
    for (int i = 0; i < NB_ORBIT_NUM; i++) {
        double ang = i * 2.0 * DX_PI / NB_ORBIT_NUM;
        sEnemyShot* p = AddShot(pSet, cx, cy, ang, 0.0, img_enemyShotSmallBall[col]);
        p->param_i[0] = NB_ROLE_ORBIT;
        p->param_i[1] = 0;
        p->param_i[2] = NB_PUDDLE_LIFE;
        p->param_d[0] = cx;
        p->param_d[1] = cy;
        p->param_d[2] = ang;
        p->param_d[4] = w;
    }
}

// ------------------------------------------------------------
//  ShotSet内の全ての弾を役割に応じて動かす(各パターン関数の最後で呼ぶ)
// ------------------------------------------------------------
static void UpdateShots(sEnemyShotSet* pSet)
{
    // ローラーの壁から垂らすインクの発生位置(ループ中にリストへ追加しないため一旦ためる)
    double dropX[64];
    double dropY[64];
    int dropTeam[64];
    int dropNum = 0;

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case NB_ROLE_STRAIGHT:
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case NB_ROLE_WALL: {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            // 2列目(後列)の1つおきの弾から、12フレームごとにインクを垂らす
            int pos = pShot->param_i[1];
            if (pSet->count % 12 == 6 && pos >= 100 && (pos - 100) % 2 == 0 && dropNum < 64
                && pShot->x > 0.0 && pShot->x < 480.0 && pShot->y > 0.0 && pShot->y < 480.0) {
                dropX[dropNum] = pShot->x;
                dropY[dropNum] = pShot->y;
                dropTeam[dropNum] = pShot->param_i[2];
                dropNum++;
            }
            break;
        }

        case NB_ROLE_BOMB: {
            // 線形に減速し、NB_BOMB_FLIGHTフレーム目にちょうど着弾点で止まる
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed -= pShot->param_d[0];
            pShot->param_i[1]++;
            if (pShot->param_i[1] >= NB_BOMB_FLIGHT) {
                BombLand(pSet, pShot);
            }
            break;
        }

        case NB_ROLE_PUDDLE:
            pShot->param_i[1]++;
            if (pShot->param_i[1] >= pShot->param_i[2]) {
                SetFall(pShot, 0.0, 0.4);   // 寿命が来たら下へ沈んでいく
            }
            break;

        case NB_ROLE_ORBIT: {
            pShot->param_i[1]++;
            double r = pShot->param_i[1] * 1.8;     // 中心から徐々に広がる
            if (r > NB_ORBIT_R) r = NB_ORBIT_R;
            const double w = pShot->param_d[4];
            pShot->param_d[2] += w;
            const double a = pShot->param_d[2];
            pShot->x = pShot->param_d[0] + r * cos(a);
            pShot->y = pShot->param_d[1] + r * sin(a);
            pShot->muki = a + ((w >= 0.0) ? DX_PI / 2.0 : -DX_PI / 2.0);   // 進行方向(接線)
            pShot->speed = fabs(w) * r;
            if (pShot->param_i[1] >= pShot->param_i[2]) {
                // 寿命が来たら接線方向へ飛び出し、重力で落ちる
                SetFall(pShot, -sin(a) * w * r, cos(a) * w * r);
            }
            break;
        }

        case NB_ROLE_FALL:
            pShot->param_d[7] += NB_GRAVITY;
            pShot->x += pShot->param_d[6];
            pShot->y += pShot->param_d[7];
            pShot->speed = sqrt(pShot->param_d[6] * pShot->param_d[6] + pShot->param_d[7] * pShot->param_d[7]);
            pShot->muki = atan2(pShot->param_d[7], pShot->param_d[6]);
            break;

        case NB_ROLE_CONVERGE:
            if (pSet->count < NB_FINALE_LAND) {
                // 着弾点へ向かって収束(最初は速く、最後はゆっくり)
                double u = pSet->count / (double)NB_FINALE_LAND;
                double r = pShot->param_d[1] * (1.0 - u) * (1.0 - u);
                double a = pShot->param_d[0];
                pShot->x = pSet->x + r * cos(a);
                pShot->y = pSet->y + r * sin(a);
                pShot->muki = a + DX_PI;
                pShot->speed = 0.0;
            }
            else {
                // 着弾の瞬間に、元の角度で外向きへ放射する
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = pShot->param_d[0];
                pShot->speed = pShot->param_d[2];
                pShot->param_i[0] = NB_ROLE_STRAIGHT;
            }
            break;

        case NB_ROLE_DROP: {
            int a = pSet->count - NB_FINALE_DROP_BEGIN;
            if (a >= NB_FINALE_DROP_TIME) {
                // 着弾: インクだまりになってしばらく居座る
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->speed = 0.0;
                pShot->param_i[0] = NB_ROLE_PUDDLE;
                pShot->param_i[1] = 0;
                pShot->param_i[2] = 120;
            }
            else {
                double u = a / (double)NB_FINALE_DROP_TIME;
                pShot->x = pSet->x;
                pShot->y = pSet->y - 220.0 * (1.0 - u * u);     // 加速しながら落下
                pShot->muki = DX_PI / 2.0;
            }
            break;
        }
        }
        pShot = pShot->next;
    }

    // ローラーの壁の後ろに垂らすインク(小玉)
    for (int i = 0; i < dropNum; i++) {
        double vx = RandSigned(0.25);
        double vy = GetRand(60) / 100.0;
        sEnemyShot* p = AddShot(pSet, dropX[i], dropY[i], DX_PI / 2.0, 0.0, img_enemyShotSmallBall[InkColor(dropTeam[i])]);
        SetFall(p, vx, vy);
    }
}

// ------------------------------------------------------------
//  ① ショット連射: 橙と青の銃弾ストリーム
//    param_d[0]: 橙ストリームの基準角 / param_d[1]: 青ストリームの基準角
// ------------------------------------------------------------
static void ShotInkStream(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const double armStep = 2.0 * DX_PI / NB_STREAM_NUM;

        // 銃弾: 各色4本(角度±6°、速さ3.6〜5.2のブレ)
        for (int team = 0; team < 2; team++) {
            double th = (team == 0) ? pSet->param_d[0] : pSet->param_d[1];
            for (int k = 0; k < NB_STREAM_NUM; k++) {
                double ang = th + armStep * k + RandSigned(0.105);
                double speed = 3.6 + GetRand(160) / 100.0;
                double mx = pSet->x + 14.0 * cos(ang);
                double my = pSet->y + 14.0 * sin(ang);
                AddShot(pSet, mx, my, ang, speed, img_enemyShotBullet[InkColor(team)]);
            }
        }

        // 飛沫: いずれかのストリーム付近へ、遅めの小玉を3発
        for (int i = 0; i < 3-1; i++) {
            int team = GetRand(1);
            int k = GetRand(NB_STREAM_NUM - 1);
            double th = (team == 0) ? pSet->param_d[0] : pSet->param_d[1];
            double ang = th + armStep * k + RandSigned(0.45);
            double speed = 1.6 + GetRand(150) / 100.0;
            double mx = pSet->x + 14.0 * cos(ang);
            double my = pSet->y + 14.0 * sin(ang);
            AddShot(pSet, mx, my, ang, speed, img_enemyShotSmallBall[InkColor(team)]);
        }
    }
    UpdateShots(pSet);
}

// ------------------------------------------------------------
//  ② ボム投擲: 1つのShotSetが1個のボム(着弾→スプラッシュ→インクだまり)を管理する
//    param_i[0]: チーム / param_d[0],[1]: 着弾点
// ------------------------------------------------------------
static void ShotInkBomb(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int team = pSet->param_i[0];
        const double dx = pSet->param_d[0] - pSet->x;
        const double dy = pSet->param_d[1] - pSet->y;
        const double dist = sqrt(dx * dx + dy * dy);
        const double v0 = 2.0 * dist / (NB_BOMB_FLIGHT + 1);    // 線形減速でちょうど着弾点に止まる初速

        sEnemyShot* p = AddShot(pSet, pSet->x, pSet->y, atan2(dy, dx), v0, img_enemyShotLargeBall[InkColor(team)]);
        p->param_i[0] = NB_ROLE_BOMB;
        p->param_i[1] = 0;
        p->param_i[2] = team;
        p->param_d[0] = v0 / NB_BOMB_FLIGHT;
        p->param_d[2] = pSet->param_d[0];
        p->param_d[3] = pSet->param_d[1];

        PlaySe(sound_enemyShot_light);
    }
    UpdateShots(pSet);
}

// ------------------------------------------------------------
//  ③ ローラー振り: 中楕円弾の2列の壁(隙間5本分)
//    param_i[0]: チーム / param_i[1]: 隙間の中心(k) / param_d[0]: 進行方向
// ------------------------------------------------------------
static void ShotInkRoller(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int team = pSet->param_i[0];
        const int gap = pSet->param_i[1];
        const double dir = pSet->param_d[0];
        const double dirX = cos(dir);
        const double dirY = sin(dir);
        const double nX = -dirY;    // 壁の延びる方向(進行方向に垂直)
        const double nY = dirX;
        const int col = InkColor(team);

        for (int row = 0; row < 2; row++) {
            for (int k = 0; k < NB_WALL_NUM; k++) {
                int d = k - gap;
                if (d < 0) d = -d;
                if (d <= NB_WALL_GAP_HALF) continue;    // 隙間

                double off = (k - (NB_WALL_NUM - 1) / 2.0 + row * 0.5) * NB_WALL_PITCH;    // 2列目は半ピッチずらす
                double back = 14.0 - row * 16.0;                                           // 2列目は16px後ろ
                double px = pSet->x + dirX * back + nX * off;
                double py = pSet->y + dirY * back + nY * off;
                sEnemyShot* p = AddShot(pSet, px, py, dir, NB_WALL_SPEED, img_enemyShotMediumOval[col]);
                p->param_i[0] = NB_ROLE_WALL;
                p->param_i[1] = k + row * 100;
                p->param_i[2] = team;
            }
        }
        PlaySe(sound_enemyShot_heavy);
    }
    UpdateShots(pSet);
}

// ------------------------------------------------------------
//  ④ スーパーチャクチ: 予告(白い小玉が収束)→黒い大玉が着弾→レーザーとリングが全方位へ
//    pSet->x, y が着弾点
// ------------------------------------------------------------
static void ShotInkFinale(sEnemyShotSet* pSet)
{
    const int c = pSet->count;
    const double cx = pSet->x;
    const double cy = pSet->y;

    // 予告: 白い小玉を3重のリングで配置し、着弾点へ収束させる
    if (c == 0) {
        for (int ring = 0; ring < 3; ring++) {
            const double r0 = 150.0 - 35.0 * ring;
            const double burstSpeed = 2.4 + 0.8 * ring;
            for (int i = 0; i < 24; i++) {
                double ang = (i + ring * 0.5) * 2.0 * DX_PI / 24.0;
                sEnemyShot* p = AddShot(pSet, cx + r0 * cos(ang), cy + r0 * sin(ang), ang + DX_PI, 0.0,
                    img_enemyShotSmallBall[NB_INK_WHITE]);
                p->param_i[0] = NB_ROLE_CONVERGE;
                p->param_i[1] = ring;
                p->param_d[0] = ang;
                p->param_d[1] = r0;
                p->param_d[2] = burstSpeed;
            }
        }
        PlaySe(sound_enemyCharge);
    }

    // 黒い大玉が上空から落ち始める
    if (c == NB_FINALE_DROP_BEGIN) {
        sEnemyShot* p = AddShot(pSet, cx, cy - 220.0, DX_PI / 2.0, 0.0, img_enemyShotLargeBall[NB_INK_BLACK]);
        p->param_i[0] = NB_ROLE_DROP;
    }

    // 着弾: 橙青交互の短レーザーを2層で全方位へ+インクのしぶき
    if (c == NB_FINALE_LAND) {
        for (int layer = 0; layer < 2; layer++) {
            const double speed = (layer == 0) ? 4.2 : 3.0;
            for (int i = 0; i < 36; i++) {
                double ang = (i + layer * 0.5) * 2.0 * DX_PI / 36.0;
                int team = (i + layer) % 2;
                AddShot(pSet, cx, cy, ang, speed, img_enemyShotLaser[InkColor(team)]);
            }
        }
        // しぶき: 上向きに飛び散って重力で落ちる小玉
        for (int i = 0; i < 60; i++) {
            double vx = RandSigned(2.5);
            double vy = -(2.5 + GetRand(200) / 100.0);
            sEnemyShot* p = AddShot(pSet, cx, cy, atan2(vy, vx), 0.0, img_enemyShotSmallBall[InkColor(i % 2)]);
            SetFall(p, vx, vy);
        }
        PlaySe(sound_enemyShot_extreme);
    }

    // 衝撃波: 橙青交互の小玉リング(48発)を14フレームごとに4つ
    {
        int rel = c - (NB_FINALE_LAND + 8);
        if (rel >= 0 && rel % 14 == 0 && rel / 14 < 4) {
            int k = rel / 14;
            for (int i = 0; i < 48; i++) {
                double ang = (i + k * 0.5) * 2.0 * DX_PI / 48.0;
                int team = (i + k) % 2;
                AddShot(pSet, cx, cy, ang, 3.0, img_enemyShotSmallBall[InkColor(team)]);
            }
            if (k % 2 == 0) PlaySe(sound_enemyShot_medium);
        }
    }

    // 締め: 白い中玉のリング(24発)を10フレームごとに3つ、外周へ
    {
        int rel = c - (NB_FINALE_LAND + 60);
        if (rel >= 0 && rel % 10 == 0 && rel / 10 < 3) {
            int k = rel / 10;
            for (int i = 0; i < 24; i++) {
                double ang = (i + k * 0.33) * 2.0 * DX_PI / 24.0;
                AddShot(pSet, cx, cy, ang, 2.2, img_enemyShotMediumBall[NB_INK_WHITE]);
            }
        }
    }

    UpdateShots(pSet);
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Splatoon_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    // 1サイクル(NB_CYCLEフレーム)ごとにループする。状態はcountから全て決まる
    const int t = (count - 1) % NB_CYCLE;

    // サイクルの頭で予告音
    if (t == 0) PlaySe(sound_enemyCharge);

    // ---- 敵の移動(目標位置へ滑らかに近づく) ----
    {
        double tx = 240.0;
        double ty = 70.0;
        if (t < NB_PHASE2_BEGIN) {
            // ① 中央で構える
        }
        else if (t < NB_PHASE3_BEGIN) {
            // ② 左右に動きながらボムを投げる
            tx = 240.0 + 120.0 * sin((t - NB_PHASE2_BEGIN) * 0.025);
            ty = 60.0;
        }
        else if (t < NB_PHASE4_BEGIN) {
            // ③ ローラーを振る
            tx = 240.0 + 80.0 * sin((t - NB_PHASE3_BEGIN) * 0.04);
            ty = 80.0;
        }
        else if (t < NB_RETURN_BEGIN) {
            // ④ 跳び上がって画面外へ
            ty = -40.0;
        }
        enemy.x += (tx - enemy.x) * 0.08;
        enemy.y += (ty - enemy.y) * 0.08;
    }

    // ---- ① ショット連射(3フレームごとにストリーム用ShotSetを生成) ----
    if (t >= NB_STREAM_BEGIN && t < NB_PHASE2_BEGIN && t % 3 == 0) {
        sEnemyShotSet* pSet = CreateShotSet(ShotInkStream, enemy.x, enemy.y + 10.0);
        // 橙は時計回り、青は反時計回りに回転。±の揺れを加えて「振り」を出す
        pSet->param_d[0] = 0.03 * t + 0.6 * sin(0.045 * t);
        pSet->param_d[1] = DX_PI / 4.0 - 0.03 * t - 0.6 * sin(0.045 * t + 1.0);
        if (t % 9 == 0) PlaySe(sound_enemyShot_light);
    }

    // ---- ② ボム投擲(橙青交互) ----
    if (t >= NB_BOMB_BEGIN && t < NB_BOMB_BEGIN + NB_BOMB_INTERVAL * NB_BOMB_NUM
        && (t - NB_BOMB_BEGIN) % NB_BOMB_INTERVAL == 0) {
        const int idx = (t - NB_BOMB_BEGIN) / NB_BOMB_INTERVAL;
        // 着弾点: 自機から60〜150px離れたランダムな位置(自機の真上には落とさない)
        double ang = GetRand(359) * DX_PI / 180.0;
        double dist = 60.0-50 + GetRand(90-80);
        double bombX = ClampD(player.x + dist * cos(ang), 40.0-40, 440.0+40);
        double bombY = ClampD(player.y + dist * sin(ang), 110.0-110, 430.0+50);

        sEnemyShotSet* pSet = CreateShotSet(ShotInkBomb, enemy.x, enemy.y + 10.0);
        pSet->param_i[0] = idx % 2;
        pSet->param_d[0] = bombX;
        pSet->param_d[1] = bombY;
    }

    // ---- ③ ローラー振り(橙青交互、左右に傾きを変えながら) ----
    if (t >= NB_ROLLER_BEGIN && t < NB_ROLLER_BEGIN + NB_ROLLER_INTERVAL * NB_ROLLER_NUM
        && (t - NB_ROLLER_BEGIN) % NB_ROLLER_INTERVAL == 0) {
        static const double tiltTable[NB_ROLLER_NUM] = { 0.5, -0.5, 0.35, -0.35, 0.2, -0.2 };
        const int idx = (t - NB_ROLLER_BEGIN) / NB_ROLLER_INTERVAL;
        const double dir = DX_PI / 2.0 + tiltTable[idx];    // 下向き+左右の傾き
        const double wallX = enemy.x;
        const double wallY = enemy.y + 10.0;

        // 隙間の位置: 自機の横位置から5〜9本分(約60〜110px)ずらす → 自機は動いて隙間へ入る必要がある
        const double nX = -sin(dir);
        const double nY = cos(dir);
        const double off = (player.x - wallX) * nX + (player.y - wallY) * nY;
        const int kPlayer = (int)floor(off / NB_WALL_PITCH + (NB_WALL_NUM - 1) / 2.0 + 0.5);
        const int sign = (GetRand(1) == 0) ? -1 : 1;
        const int shift = 5 + GetRand(4);
        int gap = kPlayer + sign * shift;
        if (gap < 3) gap = 3;
        if (gap > NB_WALL_NUM - 4) gap = NB_WALL_NUM - 4;

        sEnemyShotSet* pSet = CreateShotSet(ShotInkRoller, wallX, wallY);
        pSet->param_i[0] = idx % 2;
        pSet->param_i[1] = gap;
        pSet->param_d[0] = dir;
    }

    // ---- ④ スーパーチャクチ ----
    if (t == NB_FINALE_BEGIN) {
        CreateShotSet(ShotInkFinale, NB_FINALE_X, NB_FINALE_Y);
    }
}