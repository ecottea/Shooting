// ============================================================
//  弾幕パターン：「ターフウォーズ・スプラッシュ」
//  スプラトゥーン風（インクで床を塗り、逃げ場を奪う）
// ------------------------------------------------------------
//  ■使用素材（enemyPat_sampleForAI.cpp で確認できるもののみ）
//    大玉      img_enemyShotLargeBall[]    … インク塊（シューター弾）
//    中楕円弾  img_enemyShotMediumOval[]   … 塗り跡（停滞するインク）
//    小玉      img_enemyShotSmallBall[]    … インクの飛沫
//    短レーザー img_enemyShotLaser[]        … チャージャー（予告→本発射）
//    中玉      img_enemyShotMediumBall[]   … スプラッシュボム
//  ■使用効果音
//    sound_enemyShot_heavy   : インク塊の発射／ボム起爆
//    sound_enemyCharge       : チャージャーの予告音
//    sound_enemyShot_extreme : チャージャー本発射
//    sound_enemyShot_light   : ボム投擲
// ------------------------------------------------------------
//  count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//  画面外の弾の消去はメインルーチンが行う。ここでは一切行わない。
//  弾は「削除」せず、状態を書き換えて役割を変える（大玉→塗り跡など）。
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---- 弾の状態（sEnemyShot::param_i[0]）----------------------
enum {
    TW_SPRAY = 0,  // 通常移動（飛沫など）
    TW_BLOB = 1,  // インク塊（一定の高さで分裂）
    TW_SPLAT = 2,  // 塗り跡（ゆっくり落下して画面外へ）
    TW_CHARGE = 3,  // チャージャー：チャージ中（予告）
    TW_BEAM = 4,  // チャージャー：発射後
    TW_BOMB = 5,  // スプラッシュボム：起爆待ち
};

// ---- 弾の色（0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙）
static const int COL_INK = 5; // 敵インク（マゼンタ）
static const int COL_CHARGE = 6; // 予告（白）
static const int COL_BOMB = 8; // ボム（橙）

// ---- 共通処理 ----------------------------------------------
// 弾を1個生成してセットの末尾に繋ぐ
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 弾幕セットを1個生成してリストに繋ぐ
static sEnemyShotSet* AddSet(double x, double y, double muki,
    sEnemyShotSet::PatternFunc func, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->patternFunc = func;
    pSet->kind = kind;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// 等速直進
static void MoveStraight(sEnemyShot* p)
{
    p->x += p->speed * cos(p->muki);
    p->y += p->speed * sin(p->muki);
}

// ============================================================
//  1) インク扇状射撃 ＋ 着弾スプラッシュ
//     大玉が扇状に飛び、一定の高さで弾けて小玉と塗り跡になる
// ============================================================
static void ShotInkSpread(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = -4; i <= 4; i++) {
            // 自機方向を中心に ±50° の扇状
            double a = pSet->muki + i * (25.0 / 180.0 * DX_PI/2);
            sEnemyShot* p = AddShot(pSet, pSet->x, pSet->y, a, 2.2,
                img_enemyShotLargeBall[COL_INK]);
            p->param_i[0] = TW_BLOB;
            // 着弾する高さ（画面中段）
            p->param_d[0] = 140.0 + GetRand(160);
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next;

        if (p->param_i[0] == TW_BLOB) {
            MoveStraight(p);
            if (p->y >= p->param_d[0]) {
                // 着弾：その場を塗り跡に変え、飛沫を撒く
                p->param_i[0] = TW_SPLAT;
                p->kind = img_enemyShotMediumOval[COL_INK];
                p->speed = 0.9;
                p->muki = DX_PI / 2.0; // 真下へ（画面外に出ればメインが消す）

                for (int k = 0; k < 6*3; k++) {
                    double a = k * (DX_PI * 2.0 / 6.0/3)
                        + (GetRand(20) - 10) / 180.0 * DX_PI;
                    sEnemyShot* s = AddShot(pSet, p->x, p->y, a,
                        2.0 + GetRand(80) / 100.0,
                        img_enemyShotSmallBall[COL_INK]);
                    s->param_i[0] = TW_SPRAY;
                }
            }
        }
        else {
            // 塗り跡・飛沫はそのまま直進
            MoveStraight(p);
        }
        p = pNext;
    }
}

// ============================================================
//  2) チャージャー
//     白いレーザーが自機方向へゆっくり伸びて予告 → 高速で本発射
// ============================================================
static void ShotCharger(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12; i++) {
            sEnemyShot* p = AddShot(pSet, pSet->x, pSet->y, pSet->muki, 1.5,
                img_enemyShotLaser[COL_CHARGE]);
            p->param_i[0] = TW_CHARGE;
            p->param_i[1] = 60; // チャージ時間（フレーム）
            p->param_d[0] = 13.0 - i * 1.15;
            p->margin = 80;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next;

        MoveStraight(p);

        if (p->param_i[0] == TW_CHARGE && p->count >= p->param_i[1]) {
            // 本発射
            p->param_i[0] = TW_BEAM;
            p->kind = img_enemyShotLaser[COL_INK];
            p->speed = p->param_d[0];
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        p = pNext;
    }
}

// ============================================================
//  3) スプラッシュボム
//     自機を狙って中玉が飛び、自機の高さ付近で起爆。
//     12方向の飛沫と、周囲4か所の塗り跡を残す。
// ============================================================
static void ShotSplashBomb(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* p = AddShot(pSet, pSet->x, pSet->y, pSet->muki, 3.5,
            img_enemyShotMediumBall[COL_BOMB]);
        p->param_i[0] = TW_BOMB;
        p->param_i[1] = 90;             // 保険のタイムアウト（フレーム）
        p->param_d[0] = pSet->param_d[0]; // 起爆する高さ（生成時に自機のYを渡す）
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next;

        if (p->param_i[0] == TW_BOMB) {
            MoveStraight(p);
            if (p->y >= p->param_d[0] || p->count >= p->param_i[1]) {
                // 起爆：ボム自身は塗り跡に変化
                p->param_i[0] = TW_SPLAT;
                p->kind = img_enemyShotMediumOval[COL_BOMB];
                p->speed = 0.9;
                p->muki = DX_PI / 2.0;

                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                // リング状の飛沫
                for (int k = 0; k < 12*3; k++) {
                    double a = k * (DX_PI * 2.0 / 12.0/3);
                    sEnemyShot* s = AddShot(pSet, p->x, p->y, a, 3.2,
                        img_enemyShotSmallBall[COL_BOMB]);
                    s->param_i[0] = TW_SPRAY;
                }
                // 周囲に塗り跡を残す
                for (int k = 0; k < 4*2; k++) {
                    double a = k * (DX_PI * 2.0 / 4.0) + DX_PI / 4.0/2;
                    sEnemyShot* s = AddShot(pSet,
                        p->x + 30.0 * cos(a),
                        p->y + 30.0 * sin(a),
                        DX_PI / 2.0, 0.9,
                        img_enemyShotMediumOval[COL_BOMB]);
                    s->param_i[0] = TW_SPLAT;
                }
            }
        }
        else {
            // 塗り跡・飛沫はそのまま直進
            MoveStraight(p);
        }
        p = pNext;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Splatoon_DeepSeek()
{
    static int moveDir;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        moveDir = 1;
    }
    else {
        // 左右に往復（塗り直しループ）
        enemy.x += 1.2 * moveDir;
        if (enemy.x >= 380.0) { enemy.x = 380.0; moveDir = -1; }
        if (enemy.x <= 100.0) { enemy.x = 100.0; moveDir = 1; }
    }

    // 720フレーム（約12秒）で1周期
    int t = (count - 1) % 720;

    double sx = enemy.x;
    double sy = enemy.y + 10.0;
    double aim = atan2(player.y - sy, player.x - sx);

    // 1) インク扇状射撃
    if (t == 0 || t == 360) {
        AddSet(sx, sy, aim, ShotInkSpread, 0);
    }
    // 2) チャージャー線
    if (t == 150 || t == 510) {
        AddSet(sx, sy, aim, ShotCharger, 0);
    }
    // 3) スプラッシュボム（起爆高さ＝撃った瞬間の自機Yを渡す）
    if (t == 270 || t == 630) {
        sEnemyShotSet* pSet = AddSet(sx, sy, aim, ShotSplashBomb, 0);
        pSet->param_d[0] = player.y;
    }
}