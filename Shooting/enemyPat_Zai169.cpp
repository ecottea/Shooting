//=============================================================
//  弾幕パターン「Turf ウォー・スプラッシュ」
//  スプラトゥーンモチーフ：既存弾だけで「フィールド塗り」を表現する
//
//  【アイデア】
//   ・減速して止まった弾を「ペンキ溜まり（静止弾）」としてフィールドに残す
//   ・ボスは残したペンキ溜まりへ瞬間移動する（イカ泳ぎ）
//   ・周期の終わりに全ペンキ溜まりが一斉射撃（スプラッシュダウン）
//
//  【周期のスケジュール】(t = pEnemyShotSet->count % 1080)
//    t 0-287   : シューター散布（3-way×2連射 → 減速してペンキ化）
//    t 300-539 : スプラッシュボム（60f毎に2発 → 着地で8-way炸裂＋大型ペンキ）
//    t 560-600 : ローラー1（チャージ → 水平12連のインクの壁）
//    t 686-726 : ローラー2（反対側から）
//    t 930-989 : 全ペンキ溜まり点滅予告
//    t 990     : 全ペンキ溜まり一斉6-way射撃＆消滅（フィールドリセット）
//=============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//-------------------------------------------------------------
// 定数（難易度調整はここをいじる）
//-------------------------------------------------------------
static const int    TURF_CYCLE = 1080;  // 1周期のフレーム数（約18秒）
static const int    MAX_PUDDLES = 32*10;    // ペンキ溜まりの同時存在上限
static const int    PUDDLE_LIFE = 360*10;   // ペンキ溜まりの寿命（6秒）
static const int    PUDDLE_LIFE_L = 480*10;   // 大型ペンキ溜まりの寿命（8秒）
static const double STOP_SPEED = 0.9;   // これより遅くなった弾はペンキ化する
static const int    BOMB_FLIGHT = 60;    // ボムの飛行フレーム数
static const double BOMB_G = 0.25;  // ボムの重力加速度

// 周期内スケジュール
static const int SPRAY_END = 288;
static const int BOMB_START = 300;
static const int BOMB_END = 540;
static const int ROLL1_CHARGE = 560;
static const int ROLL1_FIRE = 600;
static const int ROLL2_CHARGE = 686;
static const int ROLL2_FIRE = 726;
static const int WARN_START = 930;
static const int FIRE_T = 990;

// 弾の役割（sEnemyShot::param_i[0] に格納）
enum eShotRole {
    ROLE_DEAD = -1,  // 画面外へ退避済み（消去はメインルーチンに任せる）
    ROLE_SHOT = 0,   // 減速弾：停止するとその場でペンキ溜まり化
    ROLE_RING = 1,   // 等速直線弾：炸裂とフィナーレ用
    ROLE_BOMB = 2,   // 放物線ボム：着地で8-way炸裂＋大型ペンキ化
    ROLE_PUDDLE = 3    // ペンキ溜まり：静止弾
};

// 色インデックス（0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙）
static const int COL_ORANGE = 8;
static const int COL_MAGENTA = 5;
static const int COL_YELLOW = 1;

// イカ泳ぎの基準位置（ここを基点に浮遊する）
static double g_swimX = 240.0;
static double g_swimY = 40.0;

//-------------------------------------------------------------
// ヘルパー
//-------------------------------------------------------------
static void PlayShotSound(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 弾を生成してリスト末尾に追加
static sEnemyShot* SpawnShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, int role)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;  p->y = y;
    p->muki = muki;  p->speed = speed;
    p->kind = kind;
    p->param_i[0] = role;
    p->param_d[0] = 1.0;  // 減速率（1.0＝減速なし）
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 弾を画面外へ退避させる（実際の消去はメインルーチンに委ねる）
static void VanishShot(sEnemyShot* p)
{
    p->x = -9999.0;
    p->y = -9999.0;
    p->speed = 0.0;
    p->param_i[0] = ROLE_DEAD;
}

// 弾をその場でペンキ溜まり化する（上限に達していたらそのまま消える）
static void MakePuddle(sEnemyShotSet* pSet, sEnemyShot* p, bool large)
{
    if (pSet->param_i[0] >= MAX_PUDDLES) {
        VanishShot(p);
        return;
    }
    // ペンキ溜まりは必ず画面内にクランプする。
    // （静止弾が画面外に残るとメインルーチンに勝手に消され、
    //   生存数カウンタ param_i[0] が減らないまま漏れるため）
    if (p->x < 16.0)  p->x = 16.0;
    if (p->x > 464.0) p->x = 464.0;
    if (p->y < 16.0)  p->y = 16.0;
    if (p->y > 464.0) p->y = 464.0;

    int base = large ? img_enemyShotLargeBall[COL_ORANGE] : img_enemyShotMediumBall[COL_ORANGE];
    int alt = large ? img_enemyShotLargeBall[COL_MAGENTA] : img_enemyShotMediumBall[COL_MAGENTA];
    p->param_i[0] = ROLE_PUDDLE;
    p->speed = 0.0;
    p->muki = 0.0;
    p->kind = base;
    p->param_i[1] = p->count + (large ? PUDDLE_LIFE_L : PUDDLE_LIFE); // 寿命
    p->param_i[2] = pSet->param_i[1]++;  // ペンキID（一斉射撃の角度オフセット用）
    p->param_i[3] = base;                // 通常時の画像ハンドル
    p->param_i[4] = alt;                 // 予告点滅用の画像ハンドル
    pSet->param_i[0]++;                  // 生存ペンキ数 +1
}

//-------------------------------------------------------------
// 弾幕本体：周期スケジューラ＋全弾の挙動更新
//-------------------------------------------------------------
static void ShotTurfWar(sEnemyShotSet* pSet)
{
    int t = pSet->count % TURF_CYCLE;

    //--- 1. 発射スケジューラ ----------------------------------
    // [Wave1] シューター散布：3-way×2連射、減速してペンキ化
    if (t < SPRAY_END && (t % 48 == 0 || t % 48 == 6)) {
        if (t % 48 == 0) PlayShotSound(sound_enemyShot_medium);
        double aim = atan2(player.y - enemy.y, player.x - enemy.x);
        for (int i = 0; i < 3; i++) {
            double mk = aim + (i - 1) * 0.24 + (GetRand(8) - 4) / 100.0;
            double sp = 6.0 + GetRand(80) / 10.0;  // 6.0～14.0 → 飛距離約120～280px
            sEnemyShot* p = SpawnShot(pSet, enemy.x, enemy.y + 10.0, mk, sp,
                img_enemyShotSmallBall[COL_ORANGE], ROLE_SHOT);
            p->param_d[0] = 0.95;
        }
    }
    // [Wave2] スプラッシュボム：プレイヤー付近へ放物線で2発
    if (t >= BOMB_START && t < BOMB_END && t % 60 == 0) {
        PlayShotSound(sound_enemyShot_light);
        for (int i = 0; i < 2; i++) {
            double tx = player.x + (GetRand(120) - 60);
            double ty = player.y + (GetRand(80) - 40);
            if (tx < 40.0)  tx = 40.0;
            if (tx > 440.0) tx = 440.0;
            if (ty < 240.0) ty = 240.0;
            if (ty > 434.0) ty = 434.0;
            sEnemyShot* p = SpawnShot(pSet, enemy.x, enemy.y, 0.0, 0.0,
                img_enemyShotMediumOval[COL_YELLOW], ROLE_BOMB);
            // BOMB_FLIGHTフレーム後にちょうど(tx,ty)へ着地する放物線の初速
            p->param_d[0] = (tx - enemy.x) / BOMB_FLIGHT;
            p->param_d[1] = (ty - enemy.y - 0.5 * BOMB_G * BOMB_FLIGHT * BOMB_FLIGHT) / BOMB_FLIGHT;
        }
    }
    // [Wave3] ローラー：水平一列のインクの壁 → 減速してペンキ縞に
    if (t == ROLL1_FIRE || t == ROLL2_FIRE) {
        PlayShotSound(sound_enemyShot_heavy);
        for (int i = 0; i < 12; i++) {
            double x = 40.0 + i * (400.0 / 11.0);
            sEnemyShot* p = SpawnShot(pSet, x, 100.0, DX_PI / 2.0, 2.4,
                img_enemyShotMediumBall[COL_ORANGE], ROLE_SHOT);
            p->param_d[0] = 0.985;
        }
    }
    // [Finale] 予告音と一斉射撃の音
    if (t == WARN_START) PlayShotSound(sound_enemyCharge);
    if (t == FIRE_T)     PlayShotSound(sound_enemyShot_extreme);

    //--- 2. 弾の挙動更新 --------------------------------------
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case ROLE_SHOT:  // 減速弾：止まったらペンキ溜まり化
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed *= pShot->param_d[0];
            if (pShot->speed < STOP_SPEED) MakePuddle(pSet, pShot, false);
            break;

        case ROLE_RING:  // 等速直線弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case ROLE_BOMB:  // 放物線ボム：着地で8-way炸裂＋大型ペンキ化
            pShot->param_d[1] += BOMB_G;
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            if (pShot->count >= BOMB_FLIGHT) {
                PlayShotSound(sound_enemyShot_heavy);
                for (int k = 0; k < 8; k++) {
                    double mk = DX_PI / 8.0 + k * (DX_PI / 4.0);
                    SpawnShot(pSet, pShot->x, pShot->y, mk, 3.4,
                        img_enemyShotSmallBall[COL_ORANGE], ROLE_RING);
                }
                MakePuddle(pSet, pShot, true);  // ボム自身が大型ペンキになる
            }
            break;

        case ROLE_PUDDLE:  // ペンキ溜まり（静止弾）
            if (pShot->count >= pShot->param_i[1]) {  // 寿命で消滅
                pSet->param_i[0]--;
                VanishShot(pShot);
                break;
            }
            if (t == FIRE_T) {  // フィナーレ：6-way一斉射撃して消滅
                for (int k = 0; k < 6; k++) {
                    double mk = k * (DX_PI / 3.0) + pShot->param_i[2] * (DX_PI / 12.0);
                    SpawnShot(pSet, pShot->x, pShot->y, mk, 2.8,
                        img_enemyShotSmallBall[COL_ORANGE], ROLE_RING);
                }
                pSet->param_i[0]--;
                VanishShot(pShot);
                break;
            }
            // 予告中はマゼンタに点滅、それ以外は通常色
            pShot->kind = (t >= WARN_START)
                ? (((pShot->count >> 2) & 1) ? pShot->param_i[4] : pShot->param_i[3])
                : pShot->param_i[3];
            break;

        default:  // ROLE_DEAD：何もしない
            break;
        }
        pShot = pShot->next;
    }
}

//-------------------------------------------------------------
// イカ泳ぎ：自分が塗ったペンキ溜まりの位置へ瞬間移動する
//-------------------------------------------------------------
static void SquidSwim()
{
    sEnemyShotSet* pSet = nullptr;
    for (sEnemyShotSet* s = enemyShotSetHead.next; s != &enemyShotSetHead; s = s->next) {
        if (s->patternFunc == ShotTurfWar) { pSet = s; break; }
    }
    if (pSet == nullptr) return;

    // 移動候補：フィールド上半分に残っているペンキ溜まり
    static const int SWIM_CAND_MAX = 48;
    sEnemyShot* cand[SWIM_CAND_MAX];
    int n = 0;
    for (sEnemyShot* p = pSet->pEnemyShotHead->next; p != pSet->pEnemyShotHead; p = p->next) {
        if (p->param_i[0] == ROLE_PUDDLE && p->y >= 60.0 && p->y <= 220.0) {
            cand[n++] = p;
            if (n >= SWIM_CAND_MAX) break;
        }
    }
    if (n == 0) return;

    sEnemyShot* p = cand[GetRand(n - 1)];  // GetRand(n-1) は 0～n-1 を返す
    g_swimX = p->x;  if (g_swimX < 40.0)  g_swimX = 40.0;
    if (g_swimX > 440.0) g_swimX = 440.0;
    g_swimY = p->y;  if (g_swimY < 40.0)  g_swimY = 40.0;
    if (g_swimY > 200.0) g_swimY = 200.0;
    enemy.x = g_swimX;
    enemy.y = g_swimY;
    PlayShotSound(sound_enemyShot_light);
}

//-------------------------------------------------------------
// 敵本体のパターン
//-------------------------------------------------------------
void EnemyPat_Splatoon_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        g_swimX = enemy.x;
        g_swimY = enemy.y;

        // マスター弾セットを1つだけ生成（全弾管理は周期スケジューラが行う）
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTurfWar;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->param_i[0] = 0;  // 生存ペンキ溜まり数
        pSet->param_i[1] = 0;  // ペンキID採番用
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        int t = count % TURF_CYCLE;
        bool charge = (t >= ROLL1_CHARGE && t <= ROLL1_FIRE) ||
            (t >= ROLL2_CHARGE && t <= ROLL2_FIRE);

        if (charge) {
            // ローラーの構え：フィールド端に陣取りチャージ（無防備＝攻撃チャンス）
            int side = (count / TURF_CYCLE) % 2;  // 周期ごとに左右交代
            if (t <= ROLL1_FIRE) enemy.x = (side == 0) ? 80.0 : 400.0;
            else                 enemy.x = (side == 0) ? 400.0 : 80.0;
            enemy.y = 100.0;
            g_swimX = enemy.x;  // 構え後はその位置を基点に浮遊
            g_swimY = enemy.y;
            if (t == ROLL1_CHARGE || t == ROLL2_CHARGE) PlayShotSound(sound_enemyCharge);
        }
        else {
            // 通常浮遊：基準位置の周りをゆったり漂う
            enemy.x = g_swimX + sin(count * 0.016) * 40.0;
            enemy.y = g_swimY + sin(count * 0.011) * 10.0;
            if (enemy.x < 40.0)  enemy.x = 40.0;
            if (enemy.x > 440.0) enemy.x = 440.0;
            if (enemy.y < 24.0)  enemy.y = 24.0;
            if (enemy.y > 210.0) enemy.y = 210.0;

            // イカ泳ぎ：4秒ごとにペンキ溜まりへ瞬間移動
            if (count % 240 == 120) SquidSwim();
        }
    }
}