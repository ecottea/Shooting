// PopcornDanmaku.cpp
// 弾幕「爆裂ポップコーン・カーニバル」実装

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  内部で使う弾種別コード（pEnemyShot->param_i[0] に格納）
// ------------------------------------------------------------
enum BulletType {
    TYPE_SEED = 0,   // トウモロコシの種（弾ける前）
    TYPE_POPCORN = 1,   // ポップコーン本体（大玉、ふわふわ減速）
    TYPE_FRAG = 2,   // 破裂破片（小玉、高速直進）
    TYPE_CARAMEL = 3,   // キャラメルソース（液滴、蛇行落下）
    TYPE_SALT = 4    // 塩粒（極小白弾、一直線）
};

// ------------------------------------------------------------
//  前方宣言
// ------------------------------------------------------------
static void PopcornMachinePattern(sEnemyShotSet* pSet);

// ------------------------------------------------------------
//  敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Popcorn_DeepSeek()
{
    // 最初のフレームで初期化
    if (count == 1) {
        // 画面下部中央にポップコーン製造機を配置
        enemy.x = 240.0;
        enemy.y = 440.0;
        enemy.maxHp = enemy.hp = 200;

        // 攻撃を管理する ShotSet を作成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = PopcornMachinePattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // パラメータ初期化
        pSet->param_i[0] = 0;    // 現在のアクティブな種の数
        pSet->param_i[1] = 60;   // 次の種を発射するまでのタイマー
        pSet->param_i[2] = 0;    // キャラメルソース発射タイマー（0=停止中）
        pSet->param_i[3] = 300;  // 塩粒タイマー（15秒）

        // 自機狙い種弾の到達目標Y座標（画面上部1/3付近）
        pSet->param_d[0] = 160.0;

        // 弾リストのヘッダを作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバル ShotSet リストに接続
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // 敵本体は移動しない（固定砲台）
}

// ------------------------------------------------------------
//  ポップコーンマシンの弾幕パターン
// ------------------------------------------------------------
static void PopcornMachinePattern(sEnemyShotSet* pSet)
{
    // パラメータのエイリアス
    int& activeSeeds = pSet->param_i[0];   // 現在フィールド上の種の数
    int& seedTimer = pSet->param_i[1];   // 種発射タイマー
    int& caramelTimer = pSet->param_i[2];   // キャラメル出現タイマー
    int& saltTimer = pSet->param_i[3];   // 塩粒タイマー
    double seedTargetY = pSet->param_d[0];   // 種の停止目標Y

    // 各種タイマーをカウントダウン
    if (seedTimer > 0) seedTimer--;
    if (caramelTimer > 0) caramelTimer--;
    if (saltTimer > 0) saltTimer--;

    // --------------------------------------------------------
    //  1. 種（シード）弾の発射
    // --------------------------------------------------------
    if (seedTimer == 0 && activeSeeds < 5) {   // 同時に最大5個まで
        // 次回の発射間隔を40～60フレームに設定
        seedTimer = 40 + GetRand(20);

        sEnemyShot* pSeed = new sEnemyShot;
        // 発射位置（製造機の少し上、左右ランダム）
        pSeed->x = pSet->x + GetRand(60) - 30;
        pSeed->y = pSet->y + 10.0;

        // 自機のX座標に向かいつつ、y=seedTargetY を目指す向き
        pSeed->muki = atan2(seedTargetY - pSeed->y, player.x - pSeed->x);
        pSeed->speed = 2.0;   // 中速

        // 外見：黄色（1）の中玉
        pSeed->kind = img_enemyShotMediumBall[1];

        // 弾のパラメータ
        pSeed->param_i[0] = TYPE_SEED;   // 種類＝種
        pSeed->param_i[1] = 0;           // 状態：0=上昇中
        pSeed->param_i[2] = 0;           // 停止後の待機フレーム数

        // リストに追加
        pSeed->prev = pSet->pEnemyShotHead->prev;
        pSeed->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pSeed;
        pSet->pEnemyShotHead->prev = pSeed;

        activeSeeds++;

        // 発射音（軽い音）
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    //  2. キャラメルソースの発生（タイマーが0になった時）
    // --------------------------------------------------------
    if (caramelTimer == 0 && pSet->count > 100) {
        // 6本のカーテン状に配置
        for (int i = 0; i < 6; i++) {
            sEnemyShot* pCaramel = new sEnemyShot;
            pCaramel->x = 40.0 + i * 80.0;   // 等間隔
            pCaramel->y = -20.0;             // 画面上部から
            pCaramel->muki = DX_PI / 2.0;   // 下向き
            pCaramel->speed = 1.5;

            // 外見：橙色（8）の菱形弾（液滴を表現）
            pCaramel->kind = img_enemyShotDiamond[8];

            pCaramel->param_i[0] = TYPE_CARAMEL;
            pCaramel->param_d[0] = pCaramel->speed;               // 元の速度を保存
            pCaramel->param_d[1] = GetRand(628) / 100.0;          // 蛇行用位相（ランダム）

            // リストに追加
            pCaramel->prev = pSet->pEnemyShotHead->prev;
            pCaramel->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pCaramel;
            pSet->pEnemyShotHead->prev = pCaramel;
        }
        // タイマーを無効化（次の破裂まで休止）
        caramelTimer = 9999;
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    //  3. 塩粒の発生（15秒ごと）
    // --------------------------------------------------------
    if (saltTimer == 0) {
        // 画面全体に25個の白い粒をばらまく
        for (int i = 0; i < 25; i++) {
            sEnemyShot* pSalt = new sEnemyShot;
            //pSalt->x = GetRand(480);
            //pSalt->y = GetRand(480);
            pSalt->x = pSet->x;
            pSalt->y = pSet->y;
            pSalt->muki = GetRand(360) / 180.0 * DX_PI;   // 全方位ランダム
            //pSalt->speed = 4.0;
            pSalt->speed = 3.0 + GetRand(200) / 100;

            // 外見：白色（6）の小玉
            pSalt->kind = img_enemyShotSmallBall[6];

            pSalt->param_i[0] = TYPE_SALT;   // 種類

            // リストに追加
            pSalt->prev = pSet->pEnemyShotHead->prev;
            pSalt->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pSalt;
            pSet->pEnemyShotHead->prev = pSalt;
        }
        saltTimer = 300;   // 15秒後に再発射
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    //  4. 全弾の移動処理
    // --------------------------------------------------------
    sEnemyShot* pBullet = pSet->pEnemyShotHead->next;
    while (pBullet != pSet->pEnemyShotHead) {
        sEnemyShot* nextBullet = pBullet->next;   // 削除されても大丈夫なように次を保存

        int type = pBullet->param_i[0];

        switch (type) {
            // ---- 種弾 ----
        case TYPE_SEED:
        {
            int& phase = pBullet->param_i[1];
            if (phase == 0) {
                // 上昇中：設定された向きと速度で移動
                pBullet->x += pBullet->speed * cos(pBullet->muki);
                pBullet->y += pBullet->speed * sin(pBullet->muki);
                // 目標Y座標に達したら停止
                if (pBullet->y <= seedTargetY) {
                    phase = 1;               // 待機状態へ
                    pBullet->param_i[2] = 0; // 待機カウンタリセット
                    pBullet->y = seedTargetY;
                }
            }
            else if (phase == 1) {
                // 待機中：その場でカウント
                pBullet->param_i[2]++;
                if (pBullet->param_i[2] >= 45) {   // 約0.75秒待つ
                    // ---- 破裂（ポップコーン化） ----
                    phase = 2;   // 破裂済みマーク

                    // 8方向ポップコーン弾（大玉、白、減速）
                    for (int i = 0; i < 8; i++) {
                        double baseAngle = i * DX_PI / 4.0
                            + (GetRand(30) - 15) / 180.0 * DX_PI; // ±15°ランダム
                        double spd = 2.5 + GetRand(100) / 100.0; // 2.5～3.5

                        sEnemyShot* pPop = new sEnemyShot;
                        pPop->x = pBullet->x;
                        pPop->y = pBullet->y;
                        pPop->muki = baseAngle;
                        pPop->speed = spd;

                        pPop->kind = img_enemyShotLargeBall[6];  // 白大玉

                        pPop->param_i[0] = TYPE_POPCORN;
                        pPop->param_d[0] = spd;                  // 初期速度保存（減速計算用）

                        pPop->prev = pSet->pEnemyShotHead->prev;
                        pPop->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = pPop;
                        pSet->pEnemyShotHead->prev = pPop;
                    }

                    // 破片（小玉、黄、高速直進）12個
                    for (int i = 0; i < 12; i++) {
                        double angle = GetRand(360) / 180.0 * DX_PI;
                        double spd = 3.5 + GetRand(100) / 100.0; // 3.5～4.5

                        sEnemyShot* pFrag = new sEnemyShot;
                        pFrag->x = pBullet->x;
                        pFrag->y = pBullet->y;
                        pFrag->muki = angle;
                        pFrag->speed = spd;

                        pFrag->kind = img_enemyShotSmallBall[1]; // 黄小玉

                        pFrag->param_i[0] = TYPE_FRAG;

                        pFrag->prev = pSet->pEnemyShotHead->prev;
                        pFrag->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = pFrag;
                        pSet->pEnemyShotHead->prev = pFrag;
                    }

                    // キャラメルソースの予約（破裂から2秒後）
                    if (caramelTimer == 9999 || caramelTimer > 120) {
                        caramelTimer = 120;
                    }

                    // 破裂した種を画面外へ飛ばして削除予約（メインルーチンが消去）
                    pBullet->y = -1000.0;
                    pBullet->speed = 0.0;
                    activeSeeds--;

                    // 破裂音（中程度）
                    if (CheckSoundMem(sound_enemyShot_medium) == 1)
                        StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }
            }
            else {
                // 既に破裂済みならそのまま画面外へ
                pBullet->y = -1000.0;
                pBullet->speed = 0.0;
            }
            break;
        }

        // ---- ポップコーン（減速しながら漂う） ----
        case TYPE_POPCORN:
            pBullet->speed *= 0.98;   // 毎フレーム2%減速
            pBullet->x += pBullet->speed * cos(pBullet->muki);
            pBullet->y += pBullet->speed * sin(pBullet->muki);
            break;

            // ---- 破片（一定速直進） ----
        case TYPE_FRAG:
            pBullet->x += pBullet->speed * cos(pBullet->muki);
            pBullet->y += pBullet->speed * sin(pBullet->muki);
            break;

            // ---- キャラメルソース（蛇行しながら落下） ----
        case TYPE_CARAMEL:
        {
            // 横方向の揺れ（サイン波）
            double sway = 3.0 * sin(pBullet->count * 0.05 + pBullet->param_d[1]);
            pBullet->x += sway;
            pBullet->y += pBullet->speed;   // 等速落下
            break;
        }

        // ---- 塩粒（一直線高速） ----
        case TYPE_SALT:
            pBullet->x += pBullet->speed * cos(pBullet->muki);
            pBullet->y += pBullet->speed * sin(pBullet->muki);
            break;

        default:
            break;
        }

        pBullet = nextBullet;
    }
}