// enemyPat_typhoon.cpp
// 弾幕「渦巻く暴風圏 ― 台風の目」
//
// 仮想の「台風の中心」(s_typhoonX, s_typhoonY) を画面内に置き、
// 自機のほうへゆっくり進ませながら、既存の敵弾だけで台風を表現する。
//
//   ① 予告      : 白い中玉のリングが3回広がる(気圧の変化)
//   ② 目の壁    : 中玉(白)+中楕円弾(マゼンタ)+小玉(シアン)の3本の弧が中心を回る。弧の間が入口
//                 壁の内側は「台風の目」で、雨が入ってこない安全地帯
//   ③ 雨雲の渦  : 壁の外周から5本の螺旋腕(鱗弾+小玉、時々大玉)を、曲がりながら撃ち出す
//   ④ 暴風雨    : 斜めに強く降る銃弾。風向きは全体の風(s_wind)で決まる
//   ⑤ 吹き返し  : 風向きが反転(飛行中の弾も曲がる向きが変わる)し、目の壁が中心へ崩れ落ちる。
//                 菱形弾の輪が外へ破裂し、短レーザーの落雷がジグザグに落ちる
//
// 1サイクル 1200フレーム(60fpsで20秒)でループする。
//
// 注意:
//  - count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと、
//    画面外の弾の消去はメインルーチン側で行う仕様のため、ここでは行わない。
//  - 雨の弾だけは「台風の目に入った弾を消す」ため、この関数内でリストから外して delete している。
//    (画面外消去ではなく、目の内側だけを安全地帯にするための処理)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数(ここを調整すれば難易度・見た目を変えられる)
// ============================================================
static const double TY_PI2 = DX_PI * 2.0;
static const double TY_DEG = DX_PI / 180.0;

static const int TY_CYCLE = 1200; // 1サイクルのフレーム数

static const int TY_WALL_START = 30;   // 目の壁の形成開始
static const int TY_ARMS_START = 90;   // 螺旋腕の発射開始
static const int TY_RAIN_START = 150;  // 暴風雨の開始
static const int TY_WARN_COLLAPSE = 780;  // 崩壊の予告音
static const int TY_COLLAPSE = 810;  // 吹き返し(風向き反転・壁の崩壊)
static const int TY_LIGHTNING_START = 830;  // 落雷の開始
static const int TY_LIGHTNING_END = 1030; // 落雷の終了
static const int TY_ARMS_END = 1050; // 螺旋腕の終了
static const int TY_RAIN_END = 1080; // 暴風雨の終了

static const double TY_WALL_RADIUS = 64.0; // 壁の基準半径
static const double TY_ARM_SPAWN_RADIUS = 96.0; // 螺旋腕の発射半径(壁の外側)
static const double TY_EYE_RADIUS = 42.0; // 雨が入れない「目」の半径

// 弾の種類
enum {
    TY_SMALL_BALL,   // 小玉
    TY_MEDIUM_BALL,  // 中玉
    TY_LARGE_BALL,   // 大玉
    TY_BULLET,       // 銃弾
    TY_SCALE,        // 鱗弾
    TY_DIAMOND,      // 菱形弾
    TY_MEDIUM_OVAL,  // 中楕円弾
    TY_LASER         // 短レーザー
};

// ============================================================
//  パターン全体で共有する状態
// ============================================================
static double s_typhoonX = 240.0; // 台風の中心
static double s_typhoonY = 110.0;
static double s_armAngle = 0.0;   // 螺旋腕の発射角
static double s_wallAngle = 0.0;  // 目の壁の回転角
static int    s_wind = 1;         // +1: 画面上で反時計回り / -1: 時計回り(吹き返し)
static int    s_time = 0;         // サイクル内の経過フレーム
static int    s_cycle = 0;        // 何サイクル目か(古いShotSetが再び発射しないための識別)

// ============================================================
//  共通ヘルパー
// ============================================================

// 種類と色(0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙)から画像を返す
static int TyGetShotImage(int type, int color)
{
    switch (type) {
    case TY_SMALL_BALL:  return img_enemyShotSmallBall[color];
    case TY_MEDIUM_BALL: return img_enemyShotMediumBall[color];
    case TY_LARGE_BALL:  return img_enemyShotLargeBall[color];
    case TY_BULLET:      return img_enemyShotBullet[color];
    case TY_SCALE:       return img_enemyShotScale[color];
    case TY_DIAMOND:     return img_enemyShotDiamond[color];
    case TY_MEDIUM_OVAL: return img_enemyShotMediumOval[color];
    case TY_LASER:       return img_enemyShotLaser[color];
    }
    return img_enemyShotSmallBall[0];
}

// 効果音 0:なし 1:light 2:medium 3:heavy 4:extreme 5:charge(予告音)
static void TyPlaySE(int se)
{
    int handle;
    switch (se) {
    case 1: handle = sound_enemyShot_light;   break;
    case 2: handle = sound_enemyShot_medium;  break;
    case 3: handle = sound_enemyShot_heavy;   break;
    case 4: handle = sound_enemyShot_extreme; break;
    case 5: handle = sound_enemyCharge;       break;
    default: return;
    }
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ShotSetを作ってリストに繋ぐ
static sEnemyShotSet* TyAddShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
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

// 弾を1発作ってShotSetのリストに繋ぐ
static sEnemyShot* TyAddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int type, int color)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = TyGetShotImage(type, color);
    pShot->margin = 120;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 全弾を muki 方向へ speed だけ直進させる
static void TyMoveStraight(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  リング(予告の輪・破裂する菱形の輪)
//   param_i[0]:発射数 [1]:弾の種類 [2],[3]:交互に使う2色 [4]:効果音
//   param_d[0]:速さ  [1]:発射半径 [2]:角度オフセット
// ============================================================
static void TyShotRing(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        TyPlaySE(pSet->param_i[4]);
        const int num = pSet->param_i[0];
        for (int i = 0; i < num; i++) {
            const double a = pSet->param_d[2] + TY_PI2 * i / num;
            const int color = (i % 2 == 0) ? pSet->param_i[2] : pSet->param_i[3];
            TyAddShot(pSet,
                pSet->x + pSet->param_d[1] * cos(a),
                pSet->y + pSet->param_d[1] * sin(a),
                a, pSet->param_d[0], pSet->param_i[1], color);
        }
    }
    TyMoveStraight(pSet);
}

static void TySpawnRing(int num, int type, int color1, int color2,
    double speed, double startRadius, double angleOffset, int se)
{
    sEnemyShotSet* pSet = TyAddShotSet(TyShotRing, s_typhoonX, s_typhoonY);
    pSet->param_i[0] = num;
    pSet->param_i[1] = type;
    pSet->param_i[2] = color1;
    pSet->param_i[3] = color2;
    pSet->param_i[4] = se;
    pSet->param_d[0] = speed;
    pSet->param_d[1] = startRadius;
    pSet->param_d[2] = angleOffset;
}

// ============================================================
//  目の壁
//   3本の弧が台風の中心を回る。弧と弧の間が入口。
//   param_i[0]: 0=周回中 / 1=崩壊(中心へ直進)
//   param_d[0]: 弧上の角度  param_d[1]: 半径方向のオフセット
// ============================================================
static void TyAddWallShot(sEnemyShotSet* pSet, double angle, double radiusOffset, int type, int color)
{
    sEnemyShot* pShot = TyAddShot(pSet, s_typhoonX, s_typhoonY, angle - DX_PI / 2.0, 0.0, type, color);
    pShot->param_d[0] = angle;
    pShot->param_d[1] = radiusOffset;
    pShot->param_i[0] = 0;
}

static void TyShotWall(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        TyPlaySE(2);
        for (int arc = 0; arc < 3; arc++) {
            const double arcStart = TY_PI2 * arc / 3.0;
            // 内側: 中玉(白)
            for (int i = 0; i < 12; i++) {
                TyAddWallShot(pSet, arcStart + i * 7.0 * TY_DEG, -8.0, TY_MEDIUM_BALL, 6);
            }
            // 外側: 中楕円弾(マゼンタ)。長軸が接線方向を向く
            for (int i = 0; i < 6; i++) {
                TyAddWallShot(pSet, arcStart + (3.5 + i * 14.0) * TY_DEG, 8.0, TY_MEDIUM_OVAL, 5);
            }
            // 縁: 小玉(シアン)でちぎれ雲のように
            for (int i = 0; i < 16; i++) {
                TyAddWallShot(pSet, arcStart + i * 5.0 * TY_DEG, 18.0, TY_SMALL_BALL, 3);
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 吹き返し: 壁が中心へ向かって崩れ落ちる
        if (pShot->param_i[0] == 0 && s_time >= TY_COLLAPSE) {
            pShot->param_i[0] = 1;
            pShot->muki = atan2(s_typhoonY - pShot->y, s_typhoonX - pShot->x);
            pShot->speed = 2.2;
        }

        if (pShot->param_i[0] == 0) {
            // 中心から広がって壁になる(60フレームかけて形成)
            double f = pShot->count / 60.0;
            if (f > 1.0) f = 1.0;
            f = f * f * (3.0 - 2.0 * f);
            const double ang = pShot->param_d[0] + s_wallAngle;
            const double radius = (TY_WALL_RADIUS + 6.0 * sin(s_time * 0.03) + pShot->param_d[1]) * f;
            pShot->x = s_typhoonX + radius * cos(ang);
            pShot->y = s_typhoonY + radius * sin(ang);
            pShot->muki = ang - DX_PI / 2.0; // 周回方向(接線)
            pShot->speed = fabs(radius * 0.012);
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  雨雲の渦(螺旋腕)
//   param_d[0]: 進行方向が曲がる速さ(rad/フレーム)。曲がる向きは s_wind で決まる
// ============================================================
static void TyAddArmShot(sEnemyShotSet* pSet, double angle, double speed, double turn, int type, int color)
{
    sEnemyShot* pShot = TyAddShot(pSet,
        s_typhoonX + TY_ARM_SPAWN_RADIUS * cos(angle),
        s_typhoonY + TY_ARM_SPAWN_RADIUS * sin(angle),
        angle, speed, type, color);
    pShot->param_d[0] = turn;
}

static void TyShotArms(sEnemyShotSet* pSet)
{
    const bool active = (pSet->param_i[0] == s_cycle) && s_time >= TY_ARMS_START && s_time < TY_ARMS_END;

    // 5本の腕を2フレームごとに発射(鱗弾を中心に、両脇を小玉で厚くする)
    if (active && pSet->count % 2 == 0) {
        if (pSet->count % 16 == 0) TyPlaySE(1);
        for (int k = 0; k < 5; k++) {
            const double a = s_armAngle + TY_PI2 * k / 5.0;
            const double speed = 2.0 + 0.15 * k;    // 腕ごとに速さを変えて渦を乱す
            const double turn = 0.0045 + 0.0003 * k;
            const int color = (k % 2 == 0) ? 3 : 4; // シアン/青
            TyAddArmShot(pSet, a, speed, turn, TY_SCALE, color);
            TyAddArmShot(pSet, a - 0.10, speed, turn, TY_SMALL_BALL, 6);
            TyAddArmShot(pSet, a + 0.10, speed, turn, TY_SMALL_BALL, 6);
        }
    }
    // 積乱雲: ゆっくり進む大玉を、腕を順番に選んで15フレームごとに
    if (active && pSet->count % 15 == 0) {
        const int k = (pSet->count / 15) % 5;
        TyAddArmShot(pSet, s_armAngle + TY_PI2 * k / 5.0, 1.4, 0.0065, TY_LARGE_BALL, 6);
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 風向きが反転すると、飛行中の弾も曲がる向きが変わる
        pShot->muki -= pShot->param_d[0] * s_wind;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  暴風雨
//   param_d[0]: 横風の強さ  param_d[1]: 落下速度
//   台風の目に入った雨は消す(目の内側は無風・無雨の安全地帯)
// ============================================================
static void TyShotRain(sEnemyShotSet* pSet)
{
    const bool active = (pSet->param_i[0] == s_cycle) && s_time >= TY_RAIN_START && s_time < TY_RAIN_END;

    if (active) {
        int n = 3 + (s_time - TY_RAIN_START) / 250; // だんだん強くなる
        if (n > 5) n = 5;
        for (int i = 0; i < n; i++) {
            double x, y;
            if (GetRand(99) < 70) {
                // 上端から
                x = (double)GetRand(480);
                y = -8.0;
            }
            else {
                // 風上の側面から(画面の下側にも雨が届くように)
                x = (s_wind > 0) ? -8.0 : 488.0;
                y = (double)GetRand(400);
            }
            const double vx = 1.6 + GetRand(10) / 10.0;
            const double vy = 4.2 + GetRand(20) / 10.0;
            sEnemyShot* pShot = TyAddShot(pSet, x, y, atan2(vy, vx * s_wind),
                sqrt(vx * vx + vy * vy), TY_BULLET, 3 + GetRand(1)); // シアン/青
            pShot->param_d[0] = vx;
            pShot->param_d[1] = vy;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 風向きが反転すると、飛行中の雨も逆向きに流れる
        const double vx = pShot->param_d[0] * s_wind;
        const double vy = pShot->param_d[1];
        pShot->x += vx;
        pShot->y += vy;
        pShot->muki = atan2(vy, vx); // 落下方向
        pShot->speed = sqrt(vx * vx + vy * vy);

        // 台風の目に入った雨は消す
        const double dx = pShot->x - s_typhoonX;
        const double dy = pShot->y - s_typhoonY;
        if (dx * dx + dy * dy < TY_EYE_RADIUS * TY_EYE_RADIUS) {
            sEnemyShot* pNext = pShot->next;
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = pNext;
            continue;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  落雷(短レーザーがジグザグに落ちる)
//   param_i[0]: ジグザグの初期の向き(+1/-1)
// ============================================================
static void TyShotLightning(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        TyPlaySE(3);
        sEnemyShot* pShot = TyAddShot(pSet, pSet->x, -5.0, DX_PI / 2.0, 8.0, TY_LASER, 1); // 黄
        pShot->param_i[0] = (GetRand(1) == 0) ? 1 : -1;
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        const int phase = (pShot->count / 10) % 2;
        pShot->muki = DX_PI / 2.0 + ((phase == 0) ? 1.0 : -1.0) * pShot->param_i[0] * 0.4;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Typhoon_Claude()
{
    if (count < 1) return;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        s_typhoonX = 240.0;
        s_typhoonY = 110.0;
        s_armAngle = 0.0;
        s_wallAngle = 0.0;
        s_wind = 1;
        s_time = 0;
        s_cycle = 0;
    }

    // 敵本体は画面上部でゆるやかに揺れる
    enemy.x = 240.0 + 70.0 * sin(count * 0.012);
    enemy.y = 40.0 + 6.0 * sin(count * 0.021);

    const int t = (count - 1) % TY_CYCLE;
    s_time = t;

    // ---- 新しい台風の発生 ----
    if (t == 0) {
        s_cycle++;
        s_typhoonX = 120.0 + GetRand(240);
        s_typhoonY = 110.0;
        s_wind = 1;
        s_armAngle = GetRand(627) / 100.0;
        s_wallAngle = 0.0;
    }

    // ---- 台風の中心の移動(自機のx座標へゆっくり寄り、少しずつ南下) ----
    if (t >= 20) {
        s_typhoonX += (player.x - s_typhoonX) * 0.004;
        if (s_typhoonX < 90.0)  s_typhoonX = 90.0;
        if (s_typhoonX > 390.0) s_typhoonX = 390.0;
        if (s_typhoonY < 300.0) s_typhoonY += 0.18;
    }

    // ---- 回転角の更新(s_wind=+1 で画面上の反時計回り) ----
    s_armAngle -= 0.045 * s_wind;
    s_wallAngle -= 0.012;

    // ---- ① 予告: 白/シアンの中玉リングが3回広がる ----
    if (t == 0 || t == 20 || t == 40) {
        const int i = t / 20;
        TySpawnRing(48/2, TY_MEDIUM_BALL, 6, 3, 2.0 + 0.4 * i, 8.0, 0.03 * i, (i == 0) ? 5 : 1);
    }

    // ---- ② 目の壁の形成 ----
    if (t == TY_WALL_START) {
        TyAddShotSet(TyShotWall, s_typhoonX, s_typhoonY);
    }

    // ---- ③ 雨雲の渦 ----
    if (t == TY_ARMS_START) {
        sEnemyShotSet* pSet = TyAddShotSet(TyShotArms, s_typhoonX, s_typhoonY);
        pSet->param_i[0] = s_cycle;
    }

    // ---- ④ 暴風雨 ----
    if (t == TY_RAIN_START) {
        sEnemyShotSet* pSet = TyAddShotSet(TyShotRain, 0.0, 0.0);
        pSet->param_i[0] = s_cycle;
    }

    // ---- ⑤ 吹き返し ----
    if (t == TY_WARN_COLLAPSE) {
        TyPlaySE(5); // 予告音
    }
    if (t == TY_COLLAPSE) {
        s_wind = -1; // 風向き反転(飛行中の腕・雨も向きが変わる)
        TySpawnRing(72, TY_DIAMOND, 1, 8, 3.4, 100.0, 0.0, 4);
    }
    if (t == TY_COLLAPSE + 16) {
        TySpawnRing(72, TY_DIAMOND, 8, 1, 2.6, 100.0, DX_PI / 72.0, 3);
    }

    // 落雷(半分は自機の近く、半分は画面のどこか)
    if (t >= TY_LIGHTNING_START && t < TY_LIGHTNING_END && (t - TY_LIGHTNING_START) % 20 == 0) {
        double lx;
        if (GetRand(1) == 0) {
            lx = (double)GetRand(480);
        }
        else {
            lx = player.x + GetRand(120) - 60;
            if (lx < 10.0)  lx = 10.0;
            if (lx > 470.0) lx = 470.0;
        }
        TyAddShotSet(TyShotLightning, lx, 0.0);
    }
}