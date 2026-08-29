// enemyPat_tmp.cpp
//
// 弾幕パターン:流星群(メテオシャワー)
// ----------------------------------------------------------------
//  ・隕石(核=大玉橙 + 岩体=中玉赤)が画面上端から内側へ傾いて降下
//  ・進行方向の後方に小玉(黄/橙)をばら撒いて尾を引く(減速して約1秒で消滅)
//  ・高度y160~233で破砕。約0.5秒前から白く点滅して予告
//      - 核(大玉)は重力で加速しながらそのまま画面下へ落下
//      - 岩体(中玉)はランダム方向(プレイヤー方向を25%混ぜる)へ放物線で飛散
//  ・飛散した岩体は30~48F後に小弾3発(大隕石の岩塊は6発)に二次破砕
//  ・大隕石は大玉2個構成・低速・破砕時に菱形弾を16方向へ高速放射
//  ・1波8~10個(うち大隕石1~2個)、出現間隔1.0~1.5秒、波の合間に3秒の静寂
// ----------------------------------------------------------------
// 仕様上の注意:
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//     画面外に出た弾の消去はメインルーチン側で行われる。
//     このファイル内での削除は「尾の寿命切れ」「岩体の二次破砕」の2ヶ所のみ。
//   ・隕石は y=25 から出現(margin=20 の画面外自動消去で即消えないため)。
//   ・弾の拡大縮小はできないため、破砕予告は色の点滅(白⇔通常)で代替。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ==================== 定数 ====================
static const double METEOR_DEG2RAD = DX_PI / 180.0; // 度→ラジアン
static const double METEOR_GRAV = 0.06;          // 破片の重力加速度(px/f^2)

// ==================== 弾の役割(sEnemyShot::param_i[0]) ====================
static const int ROLE_CORE = 0; // 核(大玉)        : 落下中は中心 / 破砕後は重力落下
static const int ROLE_ROCK = 1; // 岩体(中玉・大玉) : 落下中は周回 / 破砕後は飛散→二次破砕
static const int ROLE_TAIL = 2; // 尾(小玉)        : 減速して漂い、寿命で消滅
static const int ROLE_SPARK = 3; // 破片(小玉・菱形弾): 直進して画面外へ

// ---------- sEnemyShotSet のパラメータ使い分け ----------
//   param_i[0] : フェーズ(0=落下中 1=破砕済み) / param_i[1] : 大隕石フラグ
//   param_d[0] : 落下速度 / param_d[1] : 破砕高度 / param_d[2] : 岩体の周回角速度
// ---------- sEnemyShot のパラメータ使い分け ----------
//   param_i[0] : 役割 / param_i[1] : 二次破砕時刻・寿命(絶対count値)
//   param_i[2] : 通常色 / param_i[3] : 点滅色 / param_i[4] : 二次破砕の弾数
//   param_d[0] : vx / param_d[1] : vy(尾の減速移動・破砕後の弾道)
//   param_d[2] : 周回初期角度 / param_d[3] : 周回半径(岩体)

// ==================== 弾の追加 ====================
static sEnemyShot* AddMeteorShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, int role)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = role;
    p->param_i[2] = kind;  // 通常色(点滅から戻す用)
    p->param_i[3] = kind;  // 点滅色(核・岩体は後から白に上書き)
    p->margin = 240;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// ==================== 弾の削除(寿命切れ・分裂時のみ) ====================
static void RemoveMeteorShot(sEnemyShot* p)
{
    p->prev->next = p->next;
    p->next->prev = p->prev;
    delete p;
}

// ==================== 隕石の破砕 ====================
static void BreakMeteor(sEnemyShotSet* pSet)
{
    pSet->param_i[0] = 1;  // フェーズ:破砕済み

    // 破砕音(大隕石は重い音)
    int snd = (pSet->param_i[1] == 1) ? sound_enemyShot_heavy : sound_enemyShot_medium;
    if (CheckSoundMem(snd)) StopSoundMem(snd);
    PlaySoundMem(snd, DX_PLAYTYPE_BACK);

    // 落下速度(破片への引き継ぎ用)
    double cvx = cos(pSet->muki) * pSet->param_d[0];
    double cvy = sin(pSet->muki) * pSet->param_d[0];

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == ROLE_CORE) {
            // 核:点滅を戻して、そのまま重力で落下し続ける
            p->kind = p->param_i[2];
            p->param_d[0] = cvx;
            p->param_d[1] = cvy;
        }
        else if (p->param_i[0] == ROLE_ROCK) {
            // 岩体:ランダム方向にプレイヤー方向を25%混ぜた方向へ飛散
            p->kind = p->param_i[2];
            double rndA = GetRand(360) * METEOR_DEG2RAD;
            double aimA = atan2(player.y - p->y, player.x - p->x);
            double scat = atan2(sin(rndA) * 0.75 + sin(aimA) * 0.25,
                cos(rndA) * 0.75 + cos(aimA) * 0.25);
            double spd = 1.4 + GetRand(16) / 10.0;
            p->param_d[0] = cos(scat) * spd + cvx * 0.25;  // 落下速度を25%引き継ぐ
            p->param_d[1] = sin(scat) * spd + cvy * 0.25;
            p->param_i[1] = p->count + 30 + GetRand(19);   // 30~48F後に二次破砕
        }
        p = p->next;
    }

    // 大隕石のみ:全方位へ菱形弾(橙)を高速放射
    if (pSet->param_i[1] == 1) {
        for (int i = 0; i < 16 * 3; i++) {
            AddMeteorShot(pSet, pSet->x, pSet->y,
                (i * 22.5 / 3 + GetRand(14) - 7) * METEOR_DEG2RAD,
                3.6 + GetRand(9) / 10.0,
                img_enemyShotDiamond[8], ROLE_SPARK);
        }
    }
}

// ==================== 隕石1個分の弾幕 ====================
static void ShotMeteor(sEnemyShotSet* pSet)
{
    // ----- 出現:核と岩体を生成 -----
    if (pSet->count == 0) {
        int isBig = pSet->param_i[1];

        // 核(大玉・橙)
        sEnemyShot* core = AddMeteorShot(pSet, pSet->x, pSet->y,
            pSet->muki, pSet->param_d[0], img_enemyShotLargeBall[8], ROLE_CORE);
        core->param_i[3] = img_enemyShotLargeBall[6];  // 予告点滅色:白

        // 岩体(中玉・赤)を等間隔+ランダムずらしで不規則に取り囲む
        int rockN = isBig ? 8 : 5 + GetRand(2);   // 通常5~6個、大隕石は8個
        double baseAng = GetRand(360) * METEOR_DEG2RAD;
        for (int i = 0; i < rockN; i++) {
            sEnemyShot* rock = AddMeteorShot(pSet, pSet->x, pSet->y,
                pSet->muki, pSet->param_d[0], img_enemyShotMediumBall[0], ROLE_ROCK);
            rock->param_d[2] = baseAng + i * (2.0 * DX_PI / rockN)
                + (GetRand(30) - 15) * METEOR_DEG2RAD;
            rock->param_d[3] = 8.0 + GetRand(11);           // 周回半径 8~19
            rock->param_i[3] = img_enemyShotMediumBall[6];  // 点滅色:白
            rock->param_i[4] = 3;                           // 二次破砕で出す弾数
        }

        // 大隕石のみ:2つ目の岩塊(大玉・赤)
        if (isBig) {
            sEnemyShot* bld = AddMeteorShot(pSet, pSet->x, pSet->y,
                pSet->muki, pSet->param_d[0], img_enemyShotLargeBall[0], ROLE_ROCK);
            bld->param_d[2] = GetRand(360) * METEOR_DEG2RAD;
            bld->param_d[3] = 15.0;
            bld->param_i[3] = img_enemyShotLargeBall[6];
            bld->param_i[4] = 6;  // 岩塊は大きいので破片も多め
        }
    }

    // ----- 落下フェーズ -----
    if (pSet->param_i[0] == 0) {
        pSet->x += cos(pSet->muki) * pSet->param_d[0];
        pSet->y += sin(pSet->muki) * pSet->param_d[0];

        // 尾の生成:本体の後方へ小玉(黄/橙)を2発ずつ
        if (pSet->count % 3 == 0) {
            double dist = (pSet->param_i[1] == 1) ? 26.0 : 14.0;  // 大隕石は本体が大きい分離す
            for (int i = 0; i < 2; i++) {
                double back = pSet->muki + DX_PI + (GetRand(61) - 30) * METEOR_DEG2RAD;
                double d = dist + GetRand(10);
                double spd = pSet->param_d[0] * (0.2 + GetRand(11) / 100.0); // 落下速度の20~31%
                sEnemyShot* t = AddMeteorShot(pSet,
                    pSet->x + cos(back) * d,
                    pSet->y + sin(back) * d,
                    back, spd,
                    (GetRand(1) == 0) ? img_enemyShotSmallBall[1] : img_enemyShotSmallBall[8],
                    ROLE_TAIL);
                t->param_d[0] = cos(back) * spd;  // 減速移動用の速度ベクトル
                t->param_d[1] = sin(back) * spd;
                t->param_i[1] = 45 + GetRand(20); // 寿命 45~64F(約1秒)
            }
        }

        // 破砕高度に達したら砕ける
        if (pSet->y >= pSet->param_d[1]) {
            BreakMeteor(pSet);
        }
    }

    // ----- 全弾の更新 -----
    double rotPhase = pSet->count * pSet->param_d[2];  // 岩体の周回位相
    bool   blink = (pSet->param_i[0] == 0) && (pSet->y > pSet->param_d[1] - 80.0); // 破砕予告中か
    int    blinkOn = (pSet->count / 4) % 2;           // 4Fごとに点滅

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next;  // 削除されても辿れるよう先に退避

        switch (p->param_i[0]) {
        case ROLE_TAIL:   // 尾:減速して漂い、寿命で消滅
            p->x += p->param_d[0];
            p->y += p->param_d[1];
            p->param_d[0] *= 0.93;
            p->param_d[1] *= 0.93;
            if (p->count >= p->param_i[1]) {
                RemoveMeteorShot(p);
            }
            break;

        case ROLE_SPARK:  // 破片:直進(画面外で自動消去)
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            break;

        default:          // 核・岩体
            if (pSet->param_i[0] == 0) {
                // 落下中:セット位置に追従(岩体は中心の周りをゆっくり周回=回転表現)
                if (p->param_i[0] == ROLE_CORE) {
                    p->x = pSet->x;
                    p->y = pSet->y;
                }
                else {
                    double a = p->param_d[2] + rotPhase;
                    p->x = pSet->x + cos(a) * p->param_d[3];
                    p->y = pSet->y + sin(a) * p->param_d[3];
                }
                // 破砕直前は白く点滅して予告
                if (blink) {
                    p->kind = blinkOn ? p->param_i[2] : p->param_i[3];
                }
            }
            else {
                // 破砕後:慣性+重力で放物線を描いて飛ぶ
                p->x += p->param_d[0];
                p->y += p->param_d[1];
                p->param_d[1] += METEOR_GRAV;

                // 岩体は時間差で小弾に二次破砕(進行方向へ扇状に飛ぶ)
                if (p->param_i[0] == ROLE_ROCK && p->count >= p->param_i[1]) {
                    double baseA = atan2(p->param_d[1], p->param_d[0]);
                    int n = p->param_i[4];
                    for (int k = 0; k < n; k++) {
                        AddMeteorShot(pSet, p->x, p->y,
                            baseA + (k - (n - 1) / 2.0) * 0.26 + (GetRand(10) - 5) * METEOR_DEG2RAD,
                            2.0 + GetRand(10) / 10.0,
                            img_enemyShotSmallBall[1], ROLE_SPARK);
                    }
                    RemoveMeteorShot(p);
                }
            }
            break;
        }

        p = pNext;
    }
}

// ==================== 隕石の出現 ====================
static void SpawnMeteor(int isBig)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotMeteor;
    pSet->kind = isBig;

    // 画面上端ぎりぎりから出現(これより上だと画面外自動消去の対象になる)
    pSet->x = 40.0 + GetRand(400);   // 40~440
    pSet->y = 25.0;

    // 落下方向:鉛直から画面の内側へ0~30度ランダムに傾ける(横に逃げないように)
    double inward = (pSet->x < 240.0) ? 1.0 : -1.0;
    pSet->muki = DX_PI / 2.0 + inward * GetRand(31) * METEOR_DEG2RAD;

    pSet->param_i[0] = 0;                                // フェーズ:落下中
    pSet->param_i[1] = isBig;                            // 大隕石フラグ
    pSet->param_d[0] = isBig ? 2.0 + GetRand(4) / 10.0   // 落下速度(大隕石は遅い)
        : 2.6 + GetRand(8) / 10.0;
    pSet->param_d[1] = 160.0 + GetRand(73);              // 破砕高度 160~233
    pSet->param_d[2] = (GetRand(40) - 20) / 1000.0;      // 周回角速度 ±0.02rad/F

    // 弾リストの初期化
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    // グローバルな弾幕セットリストへ繋ぐ
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    // 出現音(大隕石は専用の予告音)
    int snd = isBig ? sound_enemyCharge : sound_enemyShot_light;
    if (CheckSoundMem(snd)) StopSoundMem(snd);
    PlaySoundMem(snd, DX_PLAYTYPE_BACK);
}

// ==================== 敵本体のパターン:流星群 ====================
void EnemyPat_Meteor_Zai()
{
    static int restTimer;       // 波と波の間の静寂カウント
    static int spawnTimer;      // 次の隕石までのカウント
    static int spawned;         // 今の波で出した隕石の数
    static int waveTotal;       // 今の波の隕石総数
    static int bigNo1, bigNo2;  // 大隕石を出す番号(-1=出さない)

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 42.0;
        enemy.maxHp = enemy.hp = 200;
        restTimer = 0;
        spawnTimer = 60-50;               // 最初の隕石は1秒後
        spawned = 0;
        waveTotal = 8 + GetRand(3);   // 1波に8~10個
        bigNo1 = 2 + GetRand(2);   // 序盤2個は通常隕石にして学習させる
        bigNo2 = (GetRand(1) == 0) ? 4 + GetRand(4) : -1;  // 2個目の大隕石は50%
    }
    else {
        // 敵本体は上空をゆっくり漂う(攻撃はすべて隕石が担う)
        enemy.x = 240.0 + 110.0 * sin((count - 1) * 0.011);
        enemy.y = 42.0 + 10.0 * sin((count - 1) * 0.005);
    }

    // ----- 隕石の出現管理 -----
    if (restTimer > 0) {
        // 静寂:出現を止め、残った弾が消えるのを待つ
        if (--restTimer == 0) {
            spawned = 0;
            waveTotal = 8 + GetRand(3);
            spawnTimer = 60-50;
            bigNo1 = 2 + GetRand(2);
            bigNo2 = (GetRand(1) == 0) ? 4 + GetRand(4) : -1;
        }
    }
    else if (spawned < waveTotal) {
        if (spawnTimer > 0) spawnTimer--;
        if (spawnTimer == 0) {
            SpawnMeteor((spawned == bigNo1 || spawned == bigNo2) ? 1 : 0);
            spawned++;
            if (spawned >= waveTotal) {
                restTimer = 180-60;               // 出現し終えたら3秒の静寂
            }
            else {
                spawnTimer = 60 + GetRand(31)-50; // 次は1.0~1.5秒後
            }
        }
    }
}