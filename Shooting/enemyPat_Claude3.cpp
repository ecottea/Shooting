// enemyPat_Claude5.cpp
// パターン18：万華鏡ボス
//
//  ① 薔薇展開         ─ 8花弁薔薇曲線上に配置した Scale弾 96発を放射状に発射
//  ② 螺旋網           ─ 4腕螺旋（SmallBall 72発）で銀河アームを描く
//  ③ 万華鏡           ─ 6回対称3重リング（BigBall/MediumBall/Diamond 54発）
//  ④ エピサイクロイド  ─ 5尖星形に並んだ Bullet弾 80発
//
//  ※ stageData.cpp の関数テーブルに Pattern18_KaleidoscopeBoss を追加してください
// ─────────────────────────────────────────────────────────────────────────────
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

// ════════════════════════════════════════════════════════════════════════════
// ファイルスコープ ユーティリティ
// ════════════════════════════════════════════════════════════════════════════

static void addShot(sEnemyShotSet* p,
                    double ox, double oy, double muki, double speed, int kind)
{
    sEnemyShot* s = new sEnemyShot;
    s->x = p->x + ox;  s->y = p->y + oy;
    s->muki = muki;    s->speed = speed;  s->kind = kind;
    s->prev = p->pEnemyShotHead->prev;
    s->next = p->pEnemyShotHead;
    p->pEnemyShotHead->prev->next = s;
    p->pEnemyShotHead->prev       = s;
}

static void moveShots(sEnemyShotSet* p)
{
    sEnemyShot* s = p->pEnemyShotHead->next;
    while (s != p->pEnemyShotHead) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
        s = s->next;
    }
}

static void spawnSet(sEnemyShotSet::PatternFunc func,
                     double x, double y, double muki)
{
    sEnemyShotSet* p  = new sEnemyShotSet;
    p->count          = 0;
    p->x = x;  p->y = y;  p->muki = muki;
    p->patternFunc    = func;
    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead->next = p->pEnemyShotHead;
    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev       = p;
}

// ════════════════════════════════════════════════════════════════════════════
// ① 薔薇展開（8花弁薔薇曲線 96発）
//
//  r = 45·cos(4θ) を [0, 2π) でサンプリング。
//  r < 0 の点は自然に逆側の花弁を形成し、合計8枚が展開する。
//  花弁4枚ごとに4色が繰り返し、外縁の弾ほど速い。
//  弾種：Scale（鱗弾）── 移動方向に回転するため花弁が"流れる"表現に
// ════════════════════════════════════════════════════════════════════════════
static void ShotRoseBurst(sEnemyShotSet* p)
{
    if (p->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int    N       = 96;
        const double a       = 45.0;
        const double phi     = p->muki;
        const int    cols[4] = { 0, 2, 3, 5 }; // 赤・緑・シアン・マゼンタ

        for (int i = 0; i < N; i++) {
            double t  = 2.0 * PI * i / N + phi;
            double r  = a * cos(4.0 * t);       // 8花弁薔薇（r<0も有効）
            double sx = r * cos(t);
            double sy = r * sin(t);
            if (sx == 0.0 && sy == 0.0) continue;
            addShot(p, sx, sy,
                    atan2(sy, sx),
                    1.6 + fabs(r) / 38.0,        // 外縁ほど速い（最大≈2.78）
                    img_enemyShotScale[cols[(i * 4 / N) % 4]]);
        }
    }
    moveShots(p);
}

// ════════════════════════════════════════════════════════════════════════════
// ② 螺旋網（4腕螺旋 72発）
//
//  4本の腕が外向きに螺旋し、ピンホイール（風車）状に広がる。
//  各腕 18発、ねじれ角 twist=0.12rad/発 → 腕全体で約117°弧を描く。
//  腕ごとに色変わり、外側の弾ほど高速。
//  弾種：SmallBall（小玉）── 密なラインで腕を描く
// ════════════════════════════════════════════════════════════════════════════
static void ShotSpiralWeb(sEnemyShotSet* p)
{
    if (p->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int    arms    = 4;
        const int    perArm  = 18;              // 合計 72発
        const double twist   = 0.12;            // 螺旋ねじれ量 [rad/発]
        const int    cols[4] = { 0, 1, 3, 5 }; // 赤・黄・シアン・マゼンタ

        for (int a = 0; a < arms; a++) {
            double base = a * (2.0 * PI / arms) + p->muki;
            for (int j = 0; j < perArm; j++) {
                double dist  = 10.0 + j * 4.5;  // 10 ～ 85.5
                double angle = base + j * twist;
                double sx    = dist * cos(angle);
                double sy    = dist * sin(angle);
                addShot(p, sx, sy,
                        atan2(sy, sx),
                        1.3 + j * 0.06,          // 1.3 ～ 2.32
                        img_enemyShotSmallBall[cols[a]]);
            }
        }
    }
    moveShots(p);
}

// ════════════════════════════════════════════════════════════════════════════
// ③ 万華鏡（6回対称 3重リング 54発）
//
//  内・中・外リングで弾種・速度・色・角度オフセットを変え、
//  曼荼羅状の対称文様を形成する。
//  各リングは少しずつ角度をずらして「重なりの美」を出す。
//
//  内輪 12発: 大玉  距離 20/32      速度 1.00/1.35
//  中輪 18発: 中玉  距離 52/61/70   速度 1.70/1.95/2.20
//  外輪 24発: 菱形  距離 80/88/96/104 速度 2.20～2.80
// ════════════════════════════════════════════════════════════════════════════
static void ShotKaleidoscope(sEnemyShotSet* p)
{
    if (p->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int    sym = 6;
        const double phi = p->muki;

        // 内輪 sym×2 = 12発  大玉
        for (int i = 0; i < sym * 2; i++) {
            double ang  = 2.0 * PI * i / (sym * 2) + phi;
            double dist = 20.0 + (i % 2) * 12.0;
            double sx   = dist * cos(ang);
            double sy   = dist * sin(ang);
            addShot(p, sx, sy, atan2(sy, sx),
                    1.0 + (i % 2) * 0.35,
                    img_enemyShotLargeBall[(i / 2) % 4]);
        }
        // 中輪 sym×3 = 18発  中玉（角度を +Δ ずらしてリングを分離）
        for (int i = 0; i < sym * 3; i++) {
            double ang  = 2.0 * PI * i / (sym * 3) + phi + PI / (sym * 3);
            double dist = 52.0 + (i % 3) * 9.0;
            double sx   = dist * cos(ang);
            double sy   = dist * sin(ang);
            addShot(p, sx, sy, atan2(sy, sx),
                    1.7 + (i % 3) * 0.25,
                    img_enemyShotMediumBall[((i / 3) + 1) % 4]);
        }
        // 外輪 sym×4 = 24発  菱形弾（さらに +Δ ずらす）
        for (int i = 0; i < sym * 4; i++) {
            double ang  = 2.0 * PI * i / (sym * 4) + phi + PI / (sym * 4);
            double dist = 80.0 + (i % 4) * 8.0;
            double sx   = dist * cos(ang);
            double sy   = dist * sin(ang);
            addShot(p, sx, sy, atan2(sy, sx),
                    2.2 + (i % 4) * 0.2,
                    img_enemyShotDiamond[((i / 4) + 2) % 4]);
        }
    }
    moveShots(p);
}

// ════════════════════════════════════════════════════════════════════════════
// ④ エピサイクロイド（5尖星形 80発）
//
//  x = 6a·cos(t) − a·cos(6t)
//  y = 6a·sin(t) − a·sin(6t)   （a = 7.5）
//  → 内径≈37.5、外径≈52.5 の 5尖星形
//  5色の銃弾（各色 16発）が星形の各腕で色分けされ、定速で展開。
//  定速にすることで展開後も星形シルエットが鮮明に保たれる。
// ════════════════════════════════════════════════════════════════════════════
static void ShotEpicycloid(sEnemyShotSet* p)
{
    const int colorTable[] = { 0, 1, 2, 3, 5 };
    if (p->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int    N   = 80;
        const double a   = 7.5;
        const double phi = p->muki;

        for (int i = 0; i < N; i++) {
            double t  = 2.0 * PI * i / N + phi;
            double sx = 6.0 * a * cos(t) - a * cos(6.0 * t);
            double sy = 6.0 * a * sin(t) - a * sin(6.0 * t);
            if (sx == 0.0 && sy == 0.0) continue;
            addShot(p, sx, sy,
                    atan2(sy, sx),
                    2.1,                                     // 定速で星形を鮮明に保つ
                    img_enemyShotBullet[colorTable[(i * 5 / N) % 5]]);  // 5色×16発ずつ
        }
    }
    moveShots(p);
}

// ════════════════════════════════════════════════════════════════════════════
// パターン本体：万華鏡ボス（Pattern18）
//
//  ■ 敵の動き
//    入場（count≤75）    : 上空 y=-30 から y=62 へ緩降下
//    フェーズ1（76-380） : 水平揺れ（振幅±105、周期≈273f）
//    フェーズ2（381-720）: 8の字（水平±118、垂直±36）
//    フェーズ3（721+）   : 楕円（水平±128、垂直±40、高速）
//
//  ■ 発射スケジュール（timer = 入場後カウンタ）
//    ①薔薇  :  85f ごと（timer=85, 170, …）  毎回 PI/8 ずつ回転
//    ②螺旋  :  68f ごと（timer=46, 114, …）  プレイヤー方向+36°
//    ③万華鏡: 112f ごと（timer=56, 168, …）  timer*0.07 で徐々に回転
//    ④星形  :  92f ごと（timer=82, 174, …）  プレイヤー正面を尖頭に向ける
// ════════════════════════════════════════════════════════════════════════════
void EnemyPat_Beautiful_Claude()
{
    static int timer;    // 入場後の発射タイマー
    static int roseIdx;  // 薔薇の回転インデックス

    // ── 初期化 ──────────────────────────────────────────────────────────
    if (count == 1) {
        enemy.x    = 240.0;
        enemy.y    = -30.0;
        enemy.maxHp = enemy.hp = 200;
        timer      = 0;
        roseIdx    = 0;
    }

    // ── 敵の移動 ────────────────────────────────────────────────────────
    const int t = count - 1;

    // 目標座標を計算
    double targetX, targetY;

    if (t < 75) {
        // 入場：緩やかな指数的降下
        enemy.y += (62.0 - enemy.y) * 0.07;
        targetX = enemy.x;
        targetY = enemy.y;
    }
    else if (t < 380) {
        // フェーズ1：ゆっくり横揺れ
        targetX = 240.0 + 105.0 * sin((t -  75) * 0.023);
        targetY =  62.0 +  14.0 * sin((t -  75) * 0.040);
    }
    else if (t < 720) {
        // フェーズ2：8の字（縦にも揺れ、密度が増す）
        targetX = 240.0 + 118.0 * sin((t - 380) * 0.021);
        targetY =  70.0 +  36.0 * sin((t - 380) * 0.042);
    }
    else {
        // フェーズ3：高速楕円軌道
        double ang = (t - 720) * 0.031;
        targetX = 240.0 + 128.0 * cos(ang);
        targetY =  76.0 +  40.0 * sin(ang);
    }

    // 速度制限つきで目標へ移動
    const double maxSpeed = 4.0;   // 適切な値に調整（軌道の最大速度より少し大きめ）
    double dx = targetX - enemy.x;
    double dy = targetY - enemy.y;
    double dist = sqrt(dx * dx + dy * dy);

    if (dist <= maxSpeed) {
        // 近ければ目標に直接到達
        enemy.x = targetX;
        enemy.y = targetY;
    }
    else {
        // 遠ければ最大速度で近づく
        enemy.x += (dx / dist) * maxSpeed;
        enemy.y += (dy / dist) * maxSpeed;
    }

    // ── 弾幕スケジュール（入場完了後のみ発射） ──────────────────────────
    if (t < 75) return;
    timer++;

    // ① 薔薇展開：85フレームごと・PI/8ずつ回転して毎回異なる向きに
    if (timer % 85 == 0) {
        spawnSet(ShotRoseBurst, enemy.x, enemy.y,
                 roseIdx++ * (PI / 8.0));
    }
    // ② 螺旋網：68フレームごと・プレイヤー方向から36°ずらす
    if ((timer + 22) % 68 == 0) {
        double aim = atan2(player.y - enemy.y, player.x - enemy.x);
        spawnSet(ShotSpiralWeb, enemy.x, enemy.y, aim + PI / 5.0);
    }
    // ③ 万華鏡：112フレームごと・タイマーで徐々に回転
    if ((timer + 56) % 112 == 0) {
        spawnSet(ShotKaleidoscope, enemy.x, enemy.y, timer * 0.07);
    }
    // ④ エピサイクロイド：92フレームごと・プレイヤー正面を星の尖頭に向ける
    if ((timer + 10) % 92 == 0) {
        double aim = atan2(player.y - enemy.y, player.x - enemy.x);
        spawnSet(ShotEpicycloid, enemy.x, enemy.y, aim);
    }
}