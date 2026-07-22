// enemyPat_daimonjiOkuribi.cpp
//
// 「大文字送り火」モチーフの弾幕パターン
// 京都五山送り火の「大」の字をイメージし、
//   フェーズ1：点火     … 親火から3画（横画・左払い・右払い）が根本から先端へ向かって出現
//   フェーズ2：静止燃焼   … 「大」の字の形で静止し、プレイヤーは隙間を通り抜けて避ける
//   フェーズ3：飛散(1段目)… 親火の位置を中心に、全弾が放射状かつ重力を伴って飛散
//   フェーズ4：飛散(2段目)… 1段目の飛散中に、各弾がランダムに向きを変えて再加速する
//              「連続波状飛散」で、避けたはずの弾道が途中で変化する
// という4段階で構成される。
//
// ---- 難易度アップ要素 ----
// ・多重発生：位置・出現タイミングの異なる「大」を同時に2つ発生させ、
//             それぞれの安全地帯（字の隙間）がずれることで避けにくくする。
// ・連続波状飛散：1段目の飛散から一定時間後、各弾がランダムな角度に向きを変えて
//                 再加速する2段目の飛散を発生させる。
// ・周期短縮：次の弾幕発生までの間隔を詰め、休憩時間を減らす。
//
// ---- 使用素材の選定メモ ----
// ・弾種：img_enemyShotMediumBall（中玉 7.0x7.0）を「大」の字本体に使用。
//         炎の粒らしい丸みと、視認しやすい大きさのバランスが良いため採用。
//   img_enemyShotLargeBall（大玉 20.0x20.0）を親火（送り火の起点）にのみ使用し、
//         他の弾と差別化して「特別な一発」であることを視覚的に強調。
//   img_enemyShotBullet（銃弾 5.0x2.0）を締めの自機狙い弾に使用。
//         細長い形状は「飛んでくる一発」として視認・避け判断がしやすいため採用。
//   img_enemyShotLaser（短レーザー）は指示通り不使用（判定が大きすぎるため）。
// ・色：赤(0)・黄(1)・橙(8)の暖色系のみを使用し、送り火の炎らしさを統一。
//       横画・左右払いの各先端（一番外側の弾）だけ黄色にして「先端が最も熱い」
//       炎らしい印象を演出。
// ・効果音：sound_enemyCharge（予告音）を、弾幕開始＝点火の合図として使用。
//
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  パラメータ定義
// ============================================================

// 「大」の画の種別（sEnemyShot::param_i[0] に格納）
static const int STROKE_OYABI = 0; // 親火（送り火の起点）
static const int STROKE_YOKO_L = 1; // 横画・左側
static const int STROKE_YOKO_R = 2; // 横画・右側
static const int STROKE_ASHI_L = 3; // 左払い
static const int STROKE_ASHI_R = 4; // 右払い
static const int STROKE_FINAL = 5; // 親火が最後に放つ自機狙いの一発

// 点火タイミング関連
static const int IGNITE_INTERVAL = 3;  // 弾が1つ出現する間隔（フレーム）
static const int N_YOKO = 6;           // 横画　片側あたりの弾数
static const int N_ASHI = 8;           // 左右払い　片側あたりの弾数
static const int LAST_IGNITE_FRAME = N_ASHI * IGNITE_INTERVAL; // 点火完了フレーム(24)

// 静止燃焼・飛散タイミング関連
static const int HOLD_DURATION = 60;                              // 静止燃焼の長さ
static const int BURST_START = LAST_IGNITE_FRAME + HOLD_DURATION; // 1段目飛散開始フレーム(84)
static const int OYABI_BURST_DELAY = 20; // 親火だけ飛散開始を遅らせる量
static const int OYABI_FINAL_DELAY = 20; // 親火が飛び始めてから自機狙い弾を撃つまでの遅延

// 連続波状飛散（2段目）関連
static const int    SECOND_WAVE_TIME = 40;  // 1段目開始から2段目へ切り替わるまでの時間
static const double BURST_SPEED2 = 2.4; // 2段目の飛散速度（1段目より速い）
static const int    WAVE_ANGLE_RANGE = 80;  // 2段目でランダムに向きを変える角度の幅(度)

// 「大」の字の形状パラメータ
static const double DAI_W = 140.0; // 横画：中心から左右それぞれへの長さ
static const double DAI_LX = 100.0; // 払い：中心からの横方向の長さ
static const double DAI_LY = 180.0; // 払い：中心からの縦方向の長さ

// 1段目飛散フェーズの物理パラメータ
static const double BURST_SPEED = 1.6;   // 1段目の飛散速度
static const double GRAVITY = 0.015; // 落下（火の粉が舞い落ちる表現）

// 多重発生：同時に発生させる「大」の数と、それぞれの中心位置・出現タイミングのずれ
static const int    N_MULTI = 3;
static const double MULTI_CX[N_MULTI] = { 260.0, 170.0, 310.0 }; // 各インスタンスの中心X
static const double MULTI_CY[N_MULTI] = { 50.0, 170.0, 230.0 }; // 各インスタンスの中心Y
static const int    MULTI_STAGGER[N_MULTI] = { 0, 15, 30 };         // 出現タイミングのずれ(フレーム)

// このパターンを再発生させる間隔（周期短縮：以前より詰めている）
static const int LOOP_INTERVAL = 200;

// ============================================================
//  「大」の字の各弾の静止目標座標を計算する
//  strokeType : STROKE_YOKO_L / STROKE_YOKO_R / STROKE_ASHI_L / STROKE_ASHI_R
//  i          : 中心(交点)から何番目の弾か（1始まり）
//  cx, cy0    : 「大」の交点（中心）座標
// ============================================================
static void GetDaimonjiTargetPos(int strokeType, int i, double cx, double cy0,
    double& outX, double& outY)
{
    switch (strokeType) {
    case STROKE_YOKO_L:
        outX = cx - (double)i / N_YOKO * DAI_W;
        outY = cy0;
        break;
    case STROKE_YOKO_R:
        outX = cx + (double)i / N_YOKO * DAI_W;
        outY = cy0;
        break;
    case STROKE_ASHI_L:
        outX = cx - (double)i / N_ASHI * DAI_LX;
        outY = cy0 + (double)i / N_ASHI * DAI_LY;
        break;
    case STROKE_ASHI_R:
        outX = cx + (double)i / N_ASHI * DAI_LX;
        outY = cy0 + (double)i / N_ASHI * DAI_LY;
        break;
    default:
        outX = cx;
        outY = cy0;
        break;
    }
}

// ============================================================
//  「大」の字を構成する弾を1つ生成し、リストに繋ぐ
//  x, y      : 静止時の目標座標（親火は交点そのもの）
//  imgKind   : 弾の絵柄（形状＋色）
// ============================================================
static void SpawnDaimonjiShot(sEnemyShotSet* pEnemyShotSet, int strokeType,
    double x, double y, int imgKind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->kind = imgKind;
    pEnemyShot->param_i[0] = strokeType;
    pEnemyShot->param_i[1] = 0; // 自機狙い弾の発射済みフラグ（親火専用）
    pEnemyShot->param_i[2] = 0; // 2段目飛散への突入フラグ

    // 1段目の飛散方向＝親火（中心）から見た方向を正規化して保存しておく
    double dx = x - pEnemyShotSet->x;
    double dy = y - pEnemyShotSet->y;
    double len = sqrt(dx * dx + dy * dy);
    if (len < 0.001) {
        // 親火自身（中心そのもの）は、飛散時にまっすぐ下へ落ちるものとする
        dx = 0.0;
        dy = 1.0;
        len = 1.0;
    }
    pEnemyShot->param_d[0] = x;        // 静止フェーズの目標X
    pEnemyShot->param_d[1] = y;        // 静止フェーズの目標Y
    pEnemyShot->param_d[2] = dx / len; // 1段目飛散方向X（正規化済み）
    pEnemyShot->param_d[3] = dy / len; // 1段目飛散方向Y（正規化済み）
    // param_d[4..7] は2段目突入時に計算する（新しい基準点と方向）

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ============================================================
//  弾幕：大文字送り火
//  点火 → 静止燃焼 → 飛散(1段目) → 飛散(2段目・連続波状) の4フェーズ
// ============================================================
static void ShotDaimonji(sEnemyShotSet* pEnemyShotSet)
{
    // 多重発生：インスタンスごとの出現タイミングのずれを kind に保存しているので差し引く
    int c = pEnemyShotSet->count - pEnemyShotSet->kind;
    double cx = pEnemyShotSet->x;      // 「大」の交点（中心）X
    double cy0 = pEnemyShotSet->y;      // 「大」の交点（中心）Y

    // ---- フェーズ1：点火（根本＝中心から先端へ向かって弾を出現させる） ----
    if (c == 0) {
        // 親火（送り火の起点となる一発）
        SpawnDaimonjiShot(pEnemyShotSet, STROKE_OYABI, cx, cy0,
            img_enemyShotLargeBall[1]); // 黄・大玉
    }

    for (int i = 1; i <= N_ASHI; i++) {
        if (c != i * IGNITE_INTERVAL) continue;

        // 横画（中心から左右へ）：N_YOKO 本ぶんだけ点火する
        if (i <= N_YOKO) {
            double tx, ty;
            int yokoColor = (i == N_YOKO) ? img_enemyShotMediumBall[1]  // 先端は黄
                : img_enemyShotMediumBall[8]; // それ以外は橙

            GetDaimonjiTargetPos(STROKE_YOKO_L, i, cx, cy0, tx, ty);
            SpawnDaimonjiShot(pEnemyShotSet, STROKE_YOKO_L, tx, ty, yokoColor);

            GetDaimonjiTargetPos(STROKE_YOKO_R, i, cx, cy0, tx, ty);
            SpawnDaimonjiShot(pEnemyShotSet, STROKE_YOKO_R, tx, ty, yokoColor);
        }

        // 左右払い（中心から斜め下へ）：N_ASHI 本ぶん点火する
        {
            double tx, ty;
            int ashiColor = (i == N_ASHI) ? img_enemyShotMediumBall[1]  // 先端は黄
                : img_enemyShotMediumBall[0]; // それ以外は赤

            GetDaimonjiTargetPos(STROKE_ASHI_L, i, cx, cy0, tx, ty);
            SpawnDaimonjiShot(pEnemyShotSet, STROKE_ASHI_L, tx, ty, ashiColor);

            GetDaimonjiTargetPos(STROKE_ASHI_R, i, cx, cy0, tx, ty);
            SpawnDaimonjiShot(pEnemyShotSet, STROKE_ASHI_R, tx, ty, ashiColor);
        }
    }

    // ---- フェーズ2〜4：静止燃焼 → 飛散(1段目) → 飛散(2段目・連続波状) ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 途中で弾を追加するので先に確保しておく

        if (pShot->param_i[0] == STROKE_FINAL) {
            // 親火が最後に放つ自機狙い弾：通常の速度直進で移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 親火だけ飛散開始を少し遅らせ、最後まで残るように見せる
            int burstStart = BURST_START;
            if (pShot->param_i[0] == STROKE_OYABI) burstStart += OYABI_BURST_DELAY;

            if (c < burstStart) {
                // 静止燃焼フェーズ：目標座標に留まり続ける
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
            }
            else {
                double t = (double)(c - burstStart);

                // 連続波状飛散：1段目から一定時間経過したら、向きを変えて2段目に切り替える
                if (pShot->param_i[2] == 0 && t >= (double)SECOND_WAVE_TIME) {
                    double tw = (double)SECOND_WAVE_TIME;
                    double curX = pShot->param_d[0] + pShot->param_d[2] * BURST_SPEED * tw;
                    double curY = pShot->param_d[1] + pShot->param_d[3] * BURST_SPEED * tw
                        + GRAVITY * tw * tw;

                    // 元の飛散方向を、ランダムな角度だけ回転させて2段目の方向とする
                    double angleRad = (double)(GetRand(WAVE_ANGLE_RANGE) - WAVE_ANGLE_RANGE / 2)
                        / 180.0 * DX_PI;
                    double baseDirX = pShot->param_d[2];
                    double baseDirY = pShot->param_d[3];

                    pShot->param_d[4] = curX;
                    pShot->param_d[5] = curY;
                    pShot->param_d[6] = baseDirX * cos(angleRad) - baseDirY * sin(angleRad);
                    pShot->param_d[7] = baseDirX * sin(angleRad) + baseDirY * cos(angleRad);
                    pShot->param_i[2] = 1; // 2段目突入フラグ
                }

                if (pShot->param_i[2] == 0) {
                    // 1段目：中心から見た方向へ直進しつつ落下
                    pShot->x = pShot->param_d[0] + pShot->param_d[2] * BURST_SPEED * t;
                    pShot->y = pShot->param_d[1] + pShot->param_d[3] * BURST_SPEED * t
                        + GRAVITY * t * t;
                }
                else {
                    // 2段目：向きを変えて再加速した波状の飛散
                    double t2 = t - (double)SECOND_WAVE_TIME;
                    pShot->x = pShot->param_d[4] + pShot->param_d[6] * BURST_SPEED2 * t2;
                    pShot->y = pShot->param_d[5] + pShot->param_d[7] * BURST_SPEED2 * t2
                        + GRAVITY * t2 * t2;
                }

                // 親火は飛び始めてから少し経ったところで、締めに自機狙いの一発を放つ
                if (pShot->param_i[0] == STROKE_OYABI &&
                    pShot->param_i[1] == 0 &&
                    t >= (double)OYABI_FINAL_DELAY) {

                    sEnemyShot* pFinal = new sEnemyShot;
                    pFinal->x = pShot->x;
                    pFinal->y = pShot->y;
                    pFinal->kind = img_enemyShotBullet[6]; // 白・銃弾（視認性重視）
                    pFinal->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pFinal->speed = 2.2;
                    pFinal->param_i[0] = STROKE_FINAL;

                    pFinal->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pFinal->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pFinal;
                    pEnemyShotSet->pEnemyShotHead->prev = pFinal;

                    pShot->param_i[1] = 1; // 発射済みフラグを立てる
                }
            }
        }

        pShot = pNext;
    }
}

// ============================================================
//  敵本体のパターン：大文字送り火
// ============================================================
void EnemyPat_Daimonji_Claude()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // 画面上部でゆったりと左右に揺れる
        enemy.x += 0.5 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // LOOP_INTERVAL フレームごとに「大文字送り火」を多重発生させる
    if (count % LOOP_INTERVAL == 1) {
        // 使える効果音一覧: sound_enemyShot_light, medium, heavy, extreme, sound_enemyCharge(予告音)
        // 点火の合図として予告音を使用
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 多重発生：位置と出現タイミングの異なる「大」を同時にN_MULTI個発生させる
        for (int m = 0; m < N_MULTI; m++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotDaimonji;
            pEnemyShotSet->x = MULTI_CX[m];       // このインスタンスの「大」の中心X
            pEnemyShotSet->y = MULTI_CY[m];       // このインスタンスの「大」の中心Y
            pEnemyShotSet->kind = MULTI_STAGGER[m]; // 出現タイミングのずれ(フレーム)

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}