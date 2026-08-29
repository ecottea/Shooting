// enemyPat_Tmp.cpp
// 隕石モチーフの弾幕「メテオ・シャワー（流星群）」
// ----------------------------------------------------------------------------
// 【使用素材（enemyPat_sampleForAI.cpp から把握して抜粋）】
//   ・大玉(20.0x20.0)  : img_enemyShotLargeBall[色]   -> 隕石本体
//   ・中玉(7.0x7.0)    : img_enemyShotMediumBall[色]  -> 本体を包む炎（周回）
//   ・小玉(2.5x2.5)    : img_enemyShotSmallBall[色]   -> 燃える尾（トレイル）／破片
//   色番号 : 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
//   効果音 : sound_enemyShot_light / _medium / _heavy / _extreme / sound_enemyCharge(予告音)
//
// 【仕様メモ】
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//     および画面外弾の消去はメインルーチンが行うので、ここでは行わない。
//   ・GetRand(x) は 0〜x の (x+1) 通りを返す。
//   ・ゲーム画面は 480x480。
//   ・弾の役割は param_i[0] で管理する（下記 ROLE_*）。
// ----------------------------------------------------------------------------

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の役割（param_i[0] に格納）
enum {
    ROLE_BODY = 0,   // 隕石本体（大玉）
    ROLE_ORBIT,      // 本体を包む炎（中玉：本体を周回）
    ROLE_TRAIL,      // 燃える尾の火の粉（小玉：短寿命）
    ROLE_FRAGMENT    // 破砕後の破片（小玉：重力で落下）
};

// パラメータの意味
//  ＜pEnemyShotSet＞
//    muki       : 隕石本体の落下方向
//    param_d[0] : 落下速度
//    param_d[1] : 破砕するYライン
//    param_i[0] : 破砕済みフラグ（0:落下中 1:破砕済み）
//  ＜ROLE_ORBIT の sEnemyShot＞
//    param_d[0] : 現在の周回角
//  ＜ROLE_FRAGMENT の sEnemyShot＞
//    param_d[0] : vx, param_d[1] : vy （速度ベクトル。重力を加える）

static const int    TRAIL_LIFE = 22;    // 尾の火の粉の寿命（フレーム）
static const double ORBIT_RADIUS = 22.0;  // 炎の周回半径
static const double GRAVITY = 0.035; // 破片にかける重力加速度
static const int    BODY_MAXLIFE = 300;   // 本体が画面外へ抜けた場合の保険で破砕する時間

// ----- リンクリスト補助 -----
static void LinkShot(sEnemyShotSet* pSet, sEnemyShot* p)
{
    p->margin = 240;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

static void UnlinkAndDelete(sEnemyShot* p)
{
    p->prev->next = p->next;
    p->next->prev = p->prev;
    delete p;
}

// ============================================================================
// 弾幕：隕石1個の生成・落下・破砕を管理
// ============================================================================
static void ShotMeteor(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    // ---- 初期化：隕石本体＋炎（周回）を生成 ----
    if (pEnemyShotSet->count == 0) {
        // 落下（予告）音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 隕石本体（大玉・橙）
        pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki;        // 落下方向
        pShot->speed = pEnemyShotSet->param_d[0];  // 落下速度
        pShot->kind = img_enemyShotLargeBall[8];  // 橙の大玉
        pShot->param_i[0] = ROLE_BODY;
        LinkShot(pEnemyShotSet, pShot);

        // 本体を包む炎（中玉・赤）を3つ、周回させる
        for (int k = 0; k < 3; k++) {
            pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->kind = img_enemyShotMediumBall[0];      // 赤の中玉
            pShot->param_i[0] = ROLE_ORBIT;
            pShot->param_d[0] = 2.0 * DX_PI * k / 3.0;     // 初期周回角
            LinkShot(pEnemyShotSet, pShot);
        }
        return;
    }

    // ---- 本体を探し、破砕タイミングを判定 ----
    sEnemyShot* body = nullptr;
    for (sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        p != pEnemyShotSet->pEnemyShotHead; p = p->next) {
        if (p->param_i[0] == ROLE_BODY) { body = p; break; }
    }

    bool   doBreak = false;
    double bx = 0.0, by = 0.0, baseAng = DX_PI / 2.0;
    if (body != nullptr && pEnemyShotSet->param_i[0] == 0) {
        if (body->y >= pEnemyShotSet->param_d[1] || body->count >= BODY_MAXLIFE) {
            doBreak = true;
            bx = body->x; by = body->y; baseAng = body->muki;
        }
    }

    // ---- 破砕：本体・炎を消し、破片を全方位へ拡散 ----
    if (doBreak) {
        pEnemyShotSet->param_i[0] = 1; // 破砕済み

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 破片：内周(遅い)＋外周(速い)の2リング。合計 N*2 発
        const int N = 12;
        for (int ring = 0; ring < 2; ring++) {
            double spd = (ring == 0) ? 1.5 : 2.6;
            double ofs = (ring == 0) ? 0.0 : DX_PI / N; // リングごとに角度をずらす
            for (int i = 0; i < N; i++) {
                double ang = baseAng + 2.0 * DX_PI * i / N + ofs;
                pShot = new sEnemyShot;
                pShot->x = bx;
                pShot->y = by;
                pShot->kind = img_enemyShotSmallBall[(ring == 0) ? 8 : 0]; // 内:橙 外:赤
                pShot->param_i[0] = ROLE_FRAGMENT;
                pShot->param_d[0] = spd * cos(ang); // vx
                pShot->param_d[1] = spd * sin(ang); // vy
                LinkShot(pEnemyShotSet, pShot);
            }
        }

        // 本体と炎（周回）を除去
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* nx = p->next;
            if (p->param_i[0] == ROLE_BODY || p->param_i[0] == ROLE_ORBIT) {
                UnlinkAndDelete(p);
            }
            p = nx;
        }
        body = nullptr;
    }

    // ---- 全弾の更新 ----
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nx = p->next;

        switch (p->param_i[0]) {
        case ROLE_BODY:
            // 斜めに落下
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            // 2フレームごとに燃える尾（火の粉）を残す
            if (p->count % 2 == 0) {
                sEnemyShot* t = new sEnemyShot;
                t->x = p->x + (GetRand(10) - 5);
                t->y = p->y + (GetRand(10) - 5);
                t->muki = p->muki + DX_PI + (GetRand(40) - 20) / 180.0 * DX_PI; // 進行の逆向きへ散る
                t->speed = (20 + GetRand(40)) / 100.0;   // 0.2〜0.6 とゆっくり
                t->kind = img_enemyShotSmallBall[(GetRand(1) == 0) ? 8 : 0]; // 橙 or 赤
                t->param_i[0] = ROLE_TRAIL;
                LinkShot(pEnemyShotSet, t);
            }
            break;

        case ROLE_ORBIT:
            // 本体を周回して炎に見せる
            if (body != nullptr) {
                p->param_d[0] += 0.18; // 周回角を進める
                p->x = body->x + ORBIT_RADIUS * cos(p->param_d[0]);
                p->y = body->y + ORBIT_RADIUS * sin(p->param_d[0]);
            }
            break;

        case ROLE_TRAIL:
            // ゆっくり流れて短寿命で消える（燃えかす）
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            if (p->count > TRAIL_LIFE) UnlinkAndDelete(p);
            break;

        case ROLE_FRAGMENT:
            // 破片は重力で舞い落ちる
            p->param_d[1] += GRAVITY;   // vy に重力
            p->x += p->param_d[0];      // += vx
            p->y += p->param_d[1];      // += vy
            break;
        }

        p = nx;
    }
}

// ============================================================================
// 敵本体のパターン：上空から隕石を降らせ続ける
// ============================================================================
void EnemyPat_Meteor_Sakana()
{
    static int muki;        // 敵の左右移動方向
    static int side;        // 隕石を落とす側（左右交互のための種）

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200 で固定
        muki = 1;
        side = 0;
    }
    else {
        // ゆるやかに左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 隕石を投下（30フレームごと）。左右交互＋乱数で流星群らしくばらつかせる
    if (count % 30 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotMeteor;

        // 出現位置：画面上端。左右の側を交互に寄せつつ乱数を足す
        double baseX = (side % 2 == 0) ? 80.0+100 : 400.0-100;
        pSet->x = baseX + (GetRand(160) - 80);   // baseX ±80
        if (pSet->x < 0)   pSet->x = 0;
        if (pSet->x > 480) pSet->x = 480;
        pSet->y = -10.0;                          // 画面上端の少し外から

        // 落下方向：真下(PI/2)を基準に、外側から中央へ向かう斜め成分を付与
        double tilt = (side % 2 == 0) ? 1.0 : -1.0; // 左からは右下へ、右からは左下へ
        pSet->muki = DX_PI / 2.0 + tilt * (10 + GetRand(25)) / 180.0 * DX_PI;

        pSet->param_d[0] = (170 + GetRand(120)) / 100.0; // 落下速度 1.7〜2.9
        pSet->param_d[1] = 280 + GetRand(140)-150;           // 破砕Yライン 280〜420
        pSet->param_i[0] = 0;                            // 未破砕

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        side++;
    }
}
