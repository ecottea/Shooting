// enemyPat_irairaBou.cpp
//
// ============================================================
//  弾幕パターン：電壁「ビリビリ回廊」
//  ― イライラ棒をモチーフにした弾幕 ―
// ============================================================
//
//  概要：
//    弾の壁2列でできた細い「回廊」が画面上部から降りてくる。
//    自機は壁に触れないよう、回廊の中を低速で慎重に進む。
//    イライラ棒の「コースをなぞる緊張感」を弾幕で再現したパターン。
//    ボス自身も回廊の中を進んでいく（＝ボスが「棒」役）。
//
//  周期構成（1周期 1920フレーム = 約32秒 @60fps、以降ループ）：
//    [   0, 180) 序盤       ：まっすぐな回廊（幅60px）
//    [ 180, 720) 蛇行区間   ：回廊が振幅20→100pxで蛇行
//    [ 720,1200) 狭所区間   ：定期的に幅が60→30pxへ収縮
//                              （1秒前に壁が赤くなって予告＋予告音）
//    [1200,1680) 障害物区間 ：回廊内を赤い障害弾が上下に往復
//    [1680,1920) ゴール     ：回廊が大きく開き、黄色に変わる
//    [1740,1920) 壁の発射を停止（息抜き区間）
//
//  素材の選定（enemyPat_sampleForAI.cpp 記載の一覧より）：
//    ・壁弾   ：大玉(20x20)。隙間なく連続する「線路」の壁に最適なため。
//                色は 通常=3:シアン(感電イメージ)、警告/狭所=0:赤、
//                ゴール=1:黄 で使い分け。
//    ・障害弾 ：大玉(0:赤)。壁（シアン）と別色にし、一目で危険物と
//                分かるようにするため。
//    ・SE     ：sound_enemyShot_heavy    …周期開始
//                sound_enemyCharge(予告音)…狭所の1秒前予告
//                sound_enemyShot_medium   …障害物出現
//                sound_enemyShot_light    …ゴール
//    ※ GetRand は意図的に未使用。イライラ棒と同じく「固定コースを
//       覚えて通る」設計にするため、コース形状は完全に決定論的にしている。
//
//  実装メモ：
//    ・壁は10フレームごとに画面上端(y=-10)へ一列発射し、全弾を毎フレーム
//      1px降下させる。同時生存数は最大でも約1200発で、プール(4096)に
//      十分収まる。
//    ・蛇行の傾きは最大約1.33px/フレームに抑えてある。これにより隣接する
//      ウェーブ同士が必ず重なり合い、壁に斜めの抜け穴ができない。
//    ・障害弾は「今いる深さの壁が発射された時刻」を逆算して回廊中心に
//      追従するため、蛇行する回廊からはみ出さない。
//    ・count, pEnemyShotSet->count, pEnemyShot->count のインクリメントと
//      画面外の弾の消去はメインルーチン側の仕様のため、ここでは行わない。
// ============================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---- 調整用パラメータ ----------------------------------------
static const int    CYCLE_FRAMES = 1920; // 1周期の長さ[フレーム]（約32秒@60fps）
static const int    WALL_WAVE_PERIOD = 10;   // 壁ウェーブの発射間隔[フレーム]
static const double WALL_SPEED = 1.0;  // 壁の降下速度[px/フレーム]
static const double WALL_BALL_R = 10.0; // 壁弾（大玉）の半径[px]
static const double GAP_NORMAL = 60.0; // 回廊の通常幅[px]
static const double GAP_NARROW = 30.0; // 狭所での回廊幅[px]

// ---- フェーズ境界 --------------------------------------------
static const int PHASE2_START = 180;  // 蛇行区間の開始
static const int PHASE3_START = 720;  // 狭所区間の開始
static const int PHASE4_START = 1200; // 障害物区間の開始
static const int PHASE5_START = 1680; // ゴールの開始
static const int WALL_STOP_START = 1740; // ここから周期末までは壁を発射しない

// ---- 色（素材一覧より） ---------------------------------------
static const int COLOR_WALL = 3; // シアン：通常の壁（感電イメージ）
static const int COLOR_WARN = 0; // 赤    ：狭所の予告＆狭所中の壁
static const int COLOR_GOAL = 1; // 黄    ：ゴール区間の壁
static const int COLOR_OBST = 0; // 赤    ：障害弾

// ------------------------------------------------------------
// 回廊の中心X座標を返す
//   t : ローカル時間[フレーム]（その時刻に発射されたウェーブの中心）
//   ※ 各区間の終端は必ず中心240へ連続に戻るよう定数を調整済み。
//     （区間の継ぎ目で回廊が瞬間移動し、回避不能な重なりが
//       発生しないようにするため）
// ------------------------------------------------------------
static double CorridorCenter(double t)
{
    if (t < (double)PHASE2_START) {
        return 240.0; // 序盤はまっすぐ
    }
    if (t < (double)PHASE3_START) {
        // 蛇行区間：振幅を20→100pxに育てつつ、周期も少しずつ速くする
        // （傾きは最大でも約1.33px/フレーム。10フレーム間隔の隣接
        //   ウェーブとの中心距離が大玉の直径20pxを超えないため、
        //   壁に斜めの抜け穴はできない）
        double dt = t - (double)PHASE2_START;
        double amp = 20.0 + 80.0 * (dt / 540.0);
        double theta = 0.010 * dt + 0.00000303 * dt * dt; // dt=540でちょうど2π
        return 240.0 + amp * sin(theta);
    }
    if (t < (double)PHASE4_START) {
        // 狭所区間：振幅40pxの緩い蛇行（480fでちょうど1.5周期→終端は中心）
        return 240.0 + 40.0 * sin((t - (double)PHASE3_START) * (3.0 * DX_PI / 480.0));
    }
    if (t < (double)PHASE5_START) {
        // 障害物区間：振幅30pxのさらに緩い蛇行（480fでちょうど1周期→終端は中心）
        return 240.0 + 30.0 * sin((t - (double)PHASE4_START) * (2.0 * DX_PI / 480.0));
    }
    return 240.0; // ゴール〜周期末はまっすぐ
}

// ------------------------------------------------------------
// 回廊の幅[px]を返す
//   local : ローカル時間[フレーム]（0〜CYCLE_FRAMES-1）
// ------------------------------------------------------------
static double CorridorGap(int local)
{
    if (local >= PHASE3_START && local < PHASE4_START) {
        // 狭所区間：240フレーム周期で 60→30→60 と収縮する
        int s = (local - PHASE3_START) % 240;
        if (s >= 60 && s < 90)   return GAP_NORMAL - (GAP_NORMAL - GAP_NARROW) * (s - 60) / 30.0;
        if (s >= 90 && s < 180)  return GAP_NARROW;
        if (s >= 180 && s < 210) return GAP_NARROW + (GAP_NORMAL - GAP_NARROW) * (s - 180) / 30.0;
        return GAP_NORMAL;
    }
    if (local >= PHASE5_START) {
        // ゴール：60→160pxへ開く
        int e = local - PHASE5_START;
        if (e < 60) return GAP_NORMAL + 100.0 * e / 60.0;
        return 160.0;
    }
    return GAP_NORMAL;
}

// ------------------------------------------------------------
// 警告表示区間（壁を赤くする区間）かどうかを返す
//   狭所の1秒前〜狭所終了まで true になる
// ------------------------------------------------------------
static bool IsWarnSection(int local)
{
    if (local >= PHASE3_START && local < PHASE4_START) {
        return ((local - PHASE3_START) % 240) < 210;
    }
    return false;
}

// ------------------------------------------------------------
// 壁弾ウェーブを周期的に生み出し続ける「回廊」マネージャ
//   ・WALL_WAVE_PERIOD フレームごとに画面上端へ壁弾を一列発射する
//     （回廊部分は空け、警告/狭所/ゴール区間では色を変える）
//   ・全ての壁弾を毎フレーム WALL_SPEED だけ降下させる
// ------------------------------------------------------------
static void ShotWallCorridor(sEnemyShotSet* pEnemyShotSet)
{
    int local = pEnemyShotSet->count % CYCLE_FRAMES;

    // ---- 周期イベントのSE ----
    if (local == 0) {
        // 周期開始
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    if (local == PHASE5_START) {
        // ゴール
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    if (local >= PHASE3_START && local < PHASE4_START) {
        if ((local - PHASE3_START) % 240 == 0) {
            // 狭所の1秒前予告（予告音）
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }

    // ---- 壁弾ウェーブの発射 ----
    if (local < WALL_STOP_START && pEnemyShotSet->count % WALL_WAVE_PERIOD == 0) {
        double cx = CorridorCenter((double)local);
        double gap = CorridorGap(local);

        int color = COLOR_WALL;
        if (IsWarnSection(local))  color = COLOR_WARN;
        if (local >= PHASE5_START) color = COLOR_GOAL;

        // 中心に合わせてグリッドをずらし、壁の輪郭を滑らかにする
        double x0 = fmod(cx, 20.0);
        for (double x = x0 - 20.0; x < 500.0; x += 20.0) {
            // 回廊部分は空ける（有効な開き幅が gap になるように弾の半径分を考慮）
            if (fabs(x - cx) < gap * 0.5 + WALL_BALL_R) continue;

            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = x;
            pEnemyShot->y = -10.0;         // 画面上端の少し外から流れ始める
            pEnemyShot->muki = DX_PI * 0.5;   // 直下
            pEnemyShot->speed = WALL_SPEED;
            pEnemyShot->kind = img_enemyShotLargeBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- 全壁弾を等速で降下させる ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->y += WALL_SPEED;
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 障害弾セット：イライラ棒の「動く障害物」に相当
//   回廊内をゆっくり上下に往復する赤い大玉3つ。
//   壁と同じ速度で降下する基準Yに、位相のずれたsinを重ねて往復させる。
//   X座標は「今いる深さの壁が発射された時刻」を逆算して回廊中心に
//   追従させるため、蛇行する回廊からはみ出すことはない。
// ------------------------------------------------------------
static void ShotObstacle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = 240.0;
            pEnemyShot->y = 60.0 + 60.0 * i;
            pEnemyShot->kind = img_enemyShotLaser[COLOR_OBST];
            pEnemyShot->param_d[0] = i * 2.094;       // 上下往復の位相（120度ずつずらす）
            pEnemyShot->param_d[1] = i * 1.5;         // 左右の揺れの位相
            pEnemyShot->param_d[2] = 60.0 + 60.0 * i; // 基準Y（壁と同速で降下）
            pEnemyShot->param_d[3] = i % 2 ? -30.0 : 30.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 基準Yを壁と同じ速度で降下させ、sinで上下に往復
        pShot->param_d[2] += WALL_SPEED;
        pShot->y = pShot->param_d[2] + 55.0 * sin(pShot->count * 0.03 + pShot->param_d[0]);

        // 今いる深さの壁が発射された時刻を逆算し、その時点の回廊中心に追従
        double emitT = (double)PHASE4_START + pEnemyShotSet->count
            - (pShot->y + 10.0) / WALL_SPEED;
        double cx = CorridorCenter(emitT);

        // 左右の揺れ幅は、その深さの回廊幅に収まるよう自動で絞る
        // （狭所の壁の上を通るときに壁へめり込まないようにする）
        int eLocal = (int)emitT;
        if (eLocal < 0) eLocal = 0;
        if (eLocal >= CYCLE_FRAMES) eLocal = CYCLE_FRAMES - 1;
        double swayMax = CorridorGap(eLocal) * 0.5 - WALL_BALL_R - 2.0;
        if (swayMax < 0.0) swayMax = 0.0;
        double sway = (8.0 < swayMax) ? 8.0 : swayMax;

        pShot->x = cx + sway * sin(pShot->count * 0.02 + pShot->param_d[1]) + pShot->param_d[3];

        // 見た目の回転（描画側が向きに応じて画像を回転する場合に効く）
        pShot->muki = pShot->count * 0.008;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
//   ・ボス自身も自分の作った回廊の中を進む（イライラ棒の「棒」役）
//   ・障害物区間の先頭で障害弾セットを投げる
// ------------------------------------------------------------
void EnemyPat_Irairabou_Kimi()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 300; // 200で固定

        // 回廊マネージャを1つだけ生成。
        // 以降はこのセットが自力で壁弾ウェーブを生み出し続ける。
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWallCorridor;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 0.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // ボスは回廊に沿って進む。
        // y=50 を今ちょうど通過している壁は約60フレーム前に発射された
        // ものなので、その時刻の回廊中心に合わせる。
        int local = (count - 1) % CYCLE_FRAMES;
        enemy.x = CorridorCenter((double)local - 60.0);
        enemy.y = 50.0;
    }

    // 障害物区間の開始フレームで障害弾セットを投げる
    if ((count - 1) % CYCLE_FRAMES == PHASE4_START) {
        sEnemyShotSet* pObstacleSet = new sEnemyShotSet;
        pObstacleSet->count = 0;
        pObstacleSet->patternFunc = ShotObstacle;
        pObstacleSet->x = 240.0;
        pObstacleSet->y = 0.0;
        pObstacleSet->muki = 0.0;
        pObstacleSet->kind = 0;

        pObstacleSet->pEnemyShotHead = new sEnemyShot;
        pObstacleSet->pEnemyShotHead->prev = pObstacleSet->pEnemyShotHead;
        pObstacleSet->pEnemyShotHead->next = pObstacleSet->pEnemyShotHead;

        pObstacleSet->prev = enemyShotSetHead.prev;
        pObstacleSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pObstacleSet;
        enemyShotSetHead.prev = pObstacleSet;
    }
}