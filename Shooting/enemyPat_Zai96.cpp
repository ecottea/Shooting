// enemyPat_tomatina.cpp
// 弾幕「トマティナ・カオス」
//
// 【仕様上の前提】
//  - count / pEnemyShotSet->count / pEnemyShot->count のインクリメントはメインルーチンが行う(ここでは加算しない)
//  - 画面外に出た弾の消去もメインルーチンが行う(ここでは消去しない)
//  - トマトの「着弾」は弾を消すのではなく、弾自身を飛沫(小玉)へ変化させることで表現
//  - GetRand(x) は 0〜x の整数を返す(x+1 種類)ことに注意
//
// 【フェーズ構成】(1サイクル = 1800フレーム、以後ループ)
//   フェーズ1「投擲開始」    : 放物線を描いて飛ぶトマト(大玉・赤)をまとめて投げる
//   フェーズ2「潰れた飛沫」  : 上空から落ちたトマトが着弾地点で飛沫(小玉・橙/黄)に変わる
//                             + 敵本体からの自機狙いばら撒き
//   フェーズ3「トマトの洪水」: 画面上部からトマトが降り注ぐ + 紙吹雪(白小玉)で視認性を攪乱
//                             + 敵が横断しながら自機狙いのトマトを直接投げる

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const int PHASE1_FRAMES = 480;                            // フェーズ1: 約8秒
static const int PHASE2_FRAMES = 600;                            // フェーズ2: 約10秒
static const int PHASE3_FRAMES = 720;                            // フェーズ3: 約12秒
static const int CYCLE_FRAMES = PHASE1_FRAMES + PHASE2_FRAMES + PHASE3_FRAMES; // 1800

// ============================================================
//  共通ヘルパー
// ============================================================

// セットのリスト末尾へ弾を1発追加
static sEnemyShot* AddEnemyShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 弾幕セットを新規作成してグローバルリストへ登録
static sEnemyShotSet* AddShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
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

// ============================================================
//  フェーズ1用: トマト投擲(放物線)
//    セットの param_i[0] = 飛翔フレーム数 T
//    セットの param_d[0] = 重力 g (呼び出し側で設定)
//    弾の param_i[0] == 0 は放物線モード
// ============================================================
static void Shot_TomatoThrow(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int    T = pEnemyShotSet->param_i[0];
        double g = pEnemyShotSet->param_d[0];
        int    n = 3 + GetRand(2); // 1度に3〜5個投げる

        for (int i = 0; i < n; i++) {
            // 目標地点: 自機付近(±60のぶれ)。画面上部のときは軌道が上に抜けないよう下限でクランプ
            double ty = player.y - 30.0;
            if (ty < 200.0) ty = 200.0;
            double tx = player.x + (GetRand(120) - 60) * 2;
            double dx = tx - pEnemyShotSet->x;
            double dy = ty - pEnemyShotSet->y;

            sEnemyShot* pShot = AddEnemyShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                0.0, 0.0, img_enemyShotLargeBall[0]); // トマト = 大玉・赤
            pShot->param_i[0] = 0;                      // 放物線モード
            pShot->param_d[0] = dx / T + (GetRand(40) - 20) / 100.0; // vx(投擲のぶれ)
            pShot->param_d[1] = (dy - 0.5 * g * T * T) / T;          // vy0
            pShot->param_d[2] = g;                                    // 重力
        }
    }

    // 毎フレーム更新: 放物線運動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_d[1] += pShot->param_d[2]; // vy += g
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];
        pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]); // 進行方向を向く(回転系素材でも通用)

        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ2用: 上空から落ちるトマトと着弾飛沫
//    弾の param_i[0] == 2 は落下トマト、それ以外は通常弾
//    弾の param_d[0] = 着弾y、param_d[1] = 落下速度
// ============================================================
static void Shot_TomatoFall(sEnemyShotSet* pEnemyShotSet)
{
    // 周期的にトマト(大玉・赤)を画面上端からランダムな位置に落とす
    if (pEnemyShotSet->count % 14 == 0 && pEnemyShotSet->count < 560) {
        double x = 20.0 + GetRand(440); // 20..460
        sEnemyShot* pShot = AddEnemyShot(pEnemyShotSet, x, -10.0, 0.0, 0.0, img_enemyShotLargeBall[0]);
        pShot->param_i[0] = 2;                       // 落下トマトモード
        pShot->param_d[0] = 110.0 + GetRand(190);   // 着弾y: 110..300
        pShot->param_d[1] = 2.0 + GetRand(10) / 10.0; // 落下速度 2.0..3.0
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 2) {
            pShot->y += pShot->param_d[1];
            // 着弾: トマト自身を飛沫(小玉・橙)へ変化させ、周囲に粒(黄/橙)を撒く
            if (pShot->y >= pShot->param_d[0]) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                pShot->param_i[0] = 1; // 通常弾モードへ
                pShot->kind = img_enemyShotSmallBall[8]; // 潰れたトマト = 小玉・橙
                pShot->muki = GetRand(360) / 180.0 * DX_PI;
                pShot->speed = 1.0 + GetRand(10) / 10.0;

                int n = 4 + GetRand(3); // 飛沫4〜6粒
                for (int i = 0; i < n; i++) {
                    double muki = GetRand(360) / 180.0 * DX_PI;
                    double sp = 0.8 + GetRand(14) / 10.0;
                    int    col = (i % 2 == 0) ? 1 : 8; // 黄と橙を交互に
                    AddEnemyShot(pEnemyShotSet, pShot->x, pShot->y, muki, sp, img_enemyShotSmallBall[col]);
                }
            }
        }
        else {
            // 通常弾(飛沫)は直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ2用: 敵本体からの自機狙いばら撒き(参加者の直接投擲)
//    セットの muki は作成時に自機方向へ設定しておく
// ============================================================
static void Shot_AimedScatter(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int n = 5 + GetRand(3); // 5〜7発
        for (int i = 0; i < n; i++) {
            double muki = pEnemyShotSet->muki + (GetRand(60) - 30) / 180.0 * DX_PI;
            double sp = (180 + GetRand(80)) / 100.0;
            int    col = (i % 2 == 0) ? 8 : 1; // 橙と黄
            AddEnemyShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, muki, sp, img_enemyShotSmallBall[col]);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ3用: トマトの洪水 + 紙吹雪
// ============================================================
static void Shot_TomatoRain(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK); // 洪水の予告兼用
    }

    // トマト(大玉・赤)を画面上部からまんべんなく降らせる
    if (pEnemyShotSet->count % 9 == 0 && pEnemyShotSet->count < 660) {
        double x = 15.0 + GetRand(450); // 15..465
        AddEnemyShot(pEnemyShotSet, x, -10.0, DX_PI / 2.0, 2.2 + GetRand(8) / 10.0, img_enemyShotLargeBall[0]);
    }

    // 紙吹雪(小玉・白): ゆっくり斜めに流れ、視認性を攪乱する
    if (pEnemyShotSet->count % 5 == 0 && pEnemyShotSet->count < 660) {
        double x = GetRand(480); // 0..480
        double muki = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
        AddEnemyShot(pEnemyShotSet, x, -10.0, muki, 0.7 + GetRand(6) / 10.0, img_enemyShotSmallBall[6]);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ3用: 敵が直接投げる自機狙いトマト
//    セットの muki は作成時に自機方向へ設定しておく
// ============================================================
static void Shot_AimedTomato(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int n = 1 + GetRand(2); // 1〜2個
        for (int i = 0; i < n; i++) {
            double muki = pEnemyShotSet->muki + (GetRand(30) - 15) / 180.0 * DX_PI;
            double sp = 3.2 + GetRand(8) / 10.0;
            AddEnemyShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, muki, sp, img_enemyShotLargeBall[0]);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Tomatina_Zai()
{
    static int muki; // 敵の横移動方向

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }

    // サイクル内の経過フレーム(初回フレームで c=0)
    int c = (count - 1) % CYCLE_FRAMES;

    if (c < PHASE1_FRAMES) {
        // ---------------- フェーズ1: 投擲開始 ----------------
        // 左右にゆっくり移動しながら投げる(60フレームは導入の余裕)
        if (c > 60) {
            enemy.x += 0.98 * (double)muki;
            if (enemy.x < 60.0)  muki = 1;
            if (enemy.x > 420.0) muki = -1;
        }
        // フェーズ2でずれた y をゆっくり戻す
        enemy.y += (40.0 - enemy.y) * 0.05;

        // 40フレームごとにまとめて投擲
        if (c % 40 == 20) {
            sEnemyShotSet* pSet = AddShotSet(Shot_TomatoThrow, enemy.x, enemy.y + 10.0);
            pSet->param_i[0] = 80;   // 飛翔フレーム数 T
            pSet->param_d[0] = 0.10; // 重力 g
        }
    }
    else if (c < PHASE1_FRAMES + PHASE2_FRAMES) {
        // ---------------- フェーズ2: 潰れたトマトの飛沫 ----------------
        int c2 = c - PHASE1_FRAMES;

        // 画面中央付近でふらふら揺れる(祭りの高揚感)
        enemy.x = 240.0 + 70.0 * sin(c2 * 0.012);
        enemy.y = 40.0 + 15.0 * sin(c2 * 0.020);

        // 落下トマトセット(フェーズ2の最初に1回だけ生成)
        if (c2 == 0) {
            AddShotSet(Shot_TomatoFall, 0.0, 0.0);
        }
        // 34フレームごとに自機狙いばら撒き
        if (c2 % 34 == 10) {
            sEnemyShotSet* pSet = AddShotSet(Shot_AimedScatter, enemy.x, enemy.y + 10.0);
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        }
    }
    else {
        // ---------------- フェーズ3: トマトの洪水 ----------------
        int c3 = c - PHASE1_FRAMES - PHASE2_FRAMES;

        if (c3 == 0) {
            muki = (enemy.x < 240.0) ? 1 : -1;
            AddShotSet(Shot_TomatoRain, 0.0, 0.0); // 洪水セット(1回だけ生成)
        }
        // 高速横断しながら
        enemy.x += 2.2 * (double)muki;
        if (enemy.x < 40.0)  muki = 1;
        if (enemy.x > 440.0) muki = -1;

        // 48フレームごとに自機狙いのトマトを直接投げる
        if (c3 % 48 == 24) {
            sEnemyShotSet* pSet = AddShotSet(Shot_AimedTomato, enemy.x, enemy.y + 10.0);
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        }
    }
}