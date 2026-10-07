// enemyPat_Tmp.cpp
// 弾幕：相転移「凝結・蒸発」
// ボスの周りに放出された"気体"(白色小弾)が、
// 　凝結 → "液体"(シアン中弾)が波となって降り注ぐ
// 　凍結 → "氷"(青中弾)として画面に滞留し、螺旋弾の障害物になる
// 　融解・蒸発 → 再び雨として降り、最後は"気体"(白小弾)となって立ち昇る
// という一連の状態変化を、同じ弾オブジェクトのパラメータ書き換えだけで表現する。
//
// ※ count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//    画面外の弾の削除はメインルーチン側で行われる前提。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------------
// 共通ヘルパ
// ------------------------------------------------------------------

// 弾をセットのリストに追加して返す
static sEnemyShot* AddShot(sEnemyShotSet* pSet)
{
    sEnemyShot* p = new sEnemyShot;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 弾セットを生成してリストに登録して返す
static sEnemyShotSet* AddShotSet(void (*func)(sEnemyShotSet*), double x, double y, double muki)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;
    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
    return p;
}

// 角度差を -PI ～ PI に正規化
static double AngleDiff(double a, double b)
{
    double d = a - b;
    while (d > DX_PI) d -= 2.0 * DX_PI;
    while (d < -DX_PI) d += 2.0 * DX_PI;
    return d;
}

// ------------------------------------------------------------------
// 相転移サイクル(雲)の弾幕
//  set->count をタイムラインとして使う(ボス側のフェーズと同期させて生成する)
//   0-119   : 気体(白小弾)をボスから噴出。ブラウン運動しながら減速(冷却)
//   120-199 : 凝結。各弾が核(グリッド点)へ向かい、到達したら液体(シアン中弾)化
//   200-319 : 液体。波状に揺れながら降り注ぐ(弱い追尾付き)
//   320     : 凍結。青中弾となりその場に滞留(障害物弾)
//   480     : 融解。再びシアン中弾としてゆっくり落下
//   540     : 蒸発。白小弾となって立ち昇る(画面外でメインルーチンが削除)
// ------------------------------------------------------------------
static void ToLiquid(sEnemyShot* s)
{
    s->param_i[0] = 2;                              // 状態 = 液体
    s->kind = img_enemyShotMediumBall[3];           // シアン中弾
    s->param_d[3] = GetRand(628) / 100.0;           // 揺れの位相
    s->param_d[4] = 0.7 + GetRand(80) / 100.0;      // 落下速度 0.7～1.5
    s->param_d[5] = 8 + GetRand(17);                // 揺れの振幅
    s->param_d[6] = s->x;                           // 振動の基準X
    s->speed = 0.0;
}

static void ShotPhaseCloud(sEnemyShotSet* pSet)
{
    int c = pSet->count;

    // 気体の噴出(毎フレーム2発)
    if (c < 120) {
        for (int k = 0; k < 2; k++) {
            sEnemyShot* s = AddShot(pSet);
            s->x = enemy.x + GetRand(30) - 15;
            s->y = enemy.y + 10.0 + GetRand(20) - 10;
            s->muki = GetRand(628) / 100.0;         // 全方向
            s->speed = 3.0 + GetRand(200) / 100.0;  // 3.0～5.0
            s->kind = img_enemyShotSmallBall[6];    // 白小弾 = 気体
            s->param_i[0] = 0;                      // 状態 = 気体
        }
    }

    // 凝結開始：核となる目標点(グリッド)を割り当てる
    if (c == 120) {
        sEnemyShot* s = pSet->pEnemyShotHead->next;
        while (s != pSet->pEnemyShotHead) {
            if (s->param_i[0] == 0) {
                s->param_i[0] = 1;                          // 状態 = 凝結中
                s->param_d[1] = 60.0 + GetRand(6) * 60.0;   // 核X: 60～420
                s->param_d[2] = 80.0 + GetRand(4) * 60.0;   // 核Y: 80～320
            }
            s = s->next;
        }
    }

    // 凍結 / 融解 / 蒸発の状態変化(潜熱的な一斉遷移)
    if (c == 320 || c == 480 || c == 540) {
        sEnemyShot* s = pSet->pEnemyShotHead->next;
        while (s != pSet->pEnemyShotHead) {
            if (c == 320 && s->param_i[0] == 2) {
                s->param_i[0] = 3;                          // 状態 = 氷
                s->kind = img_enemyShotMediumBall[4];       // 青中弾 = 氷
                s->speed = 0.0;
            }
            if (c == 480 && s->param_i[0] == 3) {
                s->param_i[0] = 4;                          // 状態 = 融解
                s->kind = img_enemyShotMediumBall[3];       // シアン中弾 = 液体
                s->param_d[4] = 1.0 + GetRand(50) / 100.0;  // 融けた分だけ速く落ちる
            }
            if (c == 540 && (s->param_i[0] == 4 || s->param_i[0] == 2)) {
                s->param_i[0] = 5;                          // 状態 = 蒸気
                s->kind = img_enemyShotSmallBall[6];        // 白小弾 = 気体へ戻る
                s->muki = -DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI; // 上向き±60°
                s->speed = 2.0 + GetRand(150) / 100.0;
            }
            s = s->next;
        }
    }

    // 状態ごとの移動
    sEnemyShot* s = pSet->pEnemyShotHead->next;
    while (s != pSet->pEnemyShotHead) {
        switch (s->param_i[0]) {
        case 0: // 気体：ブラウン運動しつつ冷却で減速
            s->muki += (GetRand(21) - 10) * 0.008;
            s->speed *= 0.995;
            s->x += s->speed * cos(s->muki);
            s->y += s->speed * sin(s->muki);
            break;
        case 1: // 凝結中：核へ誘導
        {
            double desired = atan2(s->param_d[2] - s->y, s->param_d[1] - s->x);
            s->muki += AngleDiff(desired, s->muki) * 0.1;
            s->speed = 3.0;
            s->x += s->speed * cos(s->muki);
            s->y += s->speed * sin(s->muki);
            double dx = s->param_d[1] - s->x;
            double dy = s->param_d[2] - s->y;
            if (dx * dx + dy * dy < 100.0 || c >= 190) ToLiquid(s); // 核に到達 or 時間切れ
            break;
        }
        case 2: // 液体：波状に降る(ごく緩い横追尾)
        case 4: // 融解：同様に降る
            s->param_d[6] += (player.x > s->param_d[6]) ? 0.12 : -0.12;
            s->y += s->param_d[4];
            s->x = s->param_d[6] + sin(c * 0.04 + s->param_d[3]) * s->param_d[5];
            break;
        case 3: // 氷：静止(障害物として滞留)
            break;
        case 5: // 蒸気：揺らぎながら立ち昇る
            s->muki += sin(c * 0.3 + s->param_d[3]) * 0.05;
            s->x += s->speed * cos(s->muki);
            s->y += s->speed * sin(s->muki);
            break;
        }
        s = s->next;
    }
}

// ------------------------------------------------------------------
// 単発バースト(気体・液体フェーズの攻撃)
//  param_i[0]=弾数, param_i[1]=弾画像, param_d[0]=速度, param_d[1]=拡がり(rad)
// ------------------------------------------------------------------
static void ShotBurst(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int n = pSet->param_i[0];
        for (int i = 0; i < n; i++) {
            sEnemyShot* s = AddShot(pSet);
            s->x = pSet->x;
            s->y = pSet->y;
            double t = (n > 1) ? (i / (double)(n - 1) - 0.5) : 0.0;
            s->muki = pSet->muki + t * pSet->param_d[1];
            s->speed = pSet->param_d[0];
            s->kind = pSet->param_i[1];
        }
    }
    // 直進
    sEnemyShot* s = pSet->pEnemyShotHead->next;
    while (s != pSet->pEnemyShotHead) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
        s = s->next;
    }
}

// ------------------------------------------------------------------
// 螺旋弾(凍結フェーズの攻撃)：氷の間を縫って回避させる
// ------------------------------------------------------------------
static void ShotSpiral(sEnemyShotSet* pSet)
{
    if (pSet->count < 160 && pSet->count % 2 == 0) {
        for (int k = 0; k < 2; k++) {   // 2本の腕(反対方向)
            sEnemyShot* s = AddShot(pSet);
            s->x = enemy.x;
            s->y = enemy.y + 10.0;
            s->muki = pSet->param_d[0] + k * DX_PI;
            s->speed = 3.0;
            s->kind = img_enemyShotSmallBall[4];    // 青小弾 = 冷気
        }
        pSet->param_d[0] += 0.21;       // 回転
    }
    // 直進
    sEnemyShot* s = pSet->pEnemyShotHead->next;
    while (s != pSet->pEnemyShotHead) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
        s = s->next;
    }
}

// ------------------------------------------------------------------
// 敵本体のパターン
//  1サイクル720フレームで温度が「高温→低温→高温」と循環する
//   phase   0-239 : 気体 … 白弾の拡散バースト
//   phase 240-319 : 液体 … シアン弾の狙い撃ち(凍結の予告音付き)
//   phase 320-479 : 凍結 … 氷が滞留し、螺旋弾が氷の間を抜けてくる
//   phase 480-719 : 融解・休息 … 融けた雨と蒸発の演出、攻撃はまばら
// ------------------------------------------------------------------
void EnemyPat_Tmp()
{
    static int muki;
    static double spd;
    const int CYCLE = 720;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        spd = 1.2;
    }
    else {
        // フェーズによって移動速度が変わる(冷えると動きが鈍る)
        enemy.x += spd * muki;
        if (enemy.x < 80.0) { enemy.x = 80.0;  muki = 1; }
        if (enemy.x > 400.0) { enemy.x = 400.0; muki = -1; }
        enemy.y = 40.0 + sin((count - 1) * 0.02) * 12.0;
    }

    int phase = (count - 1) % CYCLE;
    double aim = atan2(player.y - enemy.y, player.x - enemy.x);

    // 相転移サイクル(雲)の生成
    if (phase == 0) {
        // 使える効果音: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        AddShotSet(ShotPhaseCloud, enemy.x, enemy.y, 0.0);
    }

    if (phase < 240) {
        // 気体フェーズ：拡散する白弾のバースト
        spd = 1.4;
        if (phase % 45 == 0) {
            sEnemyShotSet* p = AddShotSet(ShotBurst, enemy.x, enemy.y + 10.0, aim);
            p->param_i[0] = 5;                          // 5-way
            p->param_i[1] = img_enemyShotSmallBall[6];  // 白小弾
            p->param_d[0] = 4.5;
            p->param_d[1] = 1.0;                        // 拡がり 約57°
        }
    }
    else if (phase < 320) {
        // 液体フェーズ：シアン弾の狙い撃ち + 凍結の予告
        spd = 1.0;
        if (phase == 240 || phase == 285) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);  // 凍結の予告音
        }
        if (phase == 240 || phase == 270) {
            sEnemyShotSet* p = AddShotSet(ShotBurst, enemy.x, enemy.y + 10.0, aim);
            p->param_i[0] = 3;                              // 3-way
            p->param_i[1] = img_enemyShotMediumBall[3];     // シアン中弾 = 液滴
            p->param_d[0] = 2.8;
            p->param_d[1] = 0.5;
        }
    }
    else if (phase < 480) {
        // 凍結フェーズ：氷が障害物として残り、螺旋弾が迫る
        spd = 0.6;
        if (phase == 320) {
            AddShotSet(ShotSpiral, enemy.x, enemy.y, 0.0);
        }
    }
    else {
        // 融解・休息フェーズ：まばらな狙い撃ちのみ
        spd = 1.0;
        if (phase % 90 == 0) {
            sEnemyShotSet* p = AddShotSet(ShotBurst, enemy.x, enemy.y + 10.0, aim);
            p->param_i[0] = 1;
            p->param_i[1] = img_enemyShotMediumBall[3];
            p->param_d[0] = 2.2;
            p->param_d[1] = 0.0;
        }
    }
}