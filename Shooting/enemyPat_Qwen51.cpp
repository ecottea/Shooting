// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：電撃！スパーク・イライラトンネル
static void ShotIrairaTunnel(sEnemyShotSet* pSet)
{
    // パラメータの参照と更新
    double base_x = pSet->param_d[0]; // ボスの現在X座標
    double width = pSet->param_d[1]; // トンネルの幅(半径)
    double phase = pSet->param_d[2]; // うねりの位相

    // うねり計算 (サイン波で壁をくねらせる)
    double wave = sin((pSet->count + phase) * 0.05) * 60.0;
    double center_x = base_x + wave;

    // 計算した中心座標をスパーク生成時にも使えるように保存
    pSet->param_d[5] = center_x;

    // ギミック①：急な絞り込み (count 300~600 で幅を狭くする)
    if (pSet->count > 300 && pSet->count < 600) {
        width = 120.0 - (pSet->count - 300) * (60.0 / 300.0); // 120.0 -> 40.0 へ線形減少
    }

    // ギミック②：Y字分岐的な圧迫 (count 600~750 で中心を右に急速にずらし、左側を封鎖)
    if (pSet->count > 600 && pSet->count < 750) {
//        center_x += (pSet->count - 600) * 1.2;
    }

    // ギミック③：電圧上昇 (count 800 以降)
    int color = 3; // 通常はシアン(3)
    if (pSet->count > 800) {
        // 白(6)と赤(0)を高速で点滅させて「危険」を演出
        color = ((pSet->count / 8) % 2 == 0) ? 6 : 0;
        width -= 0.3; // さらに狭くなる
        if (width < 35.0) width = 35.0; // 限界値
    }

    // 現在の幅を保存
    pSet->param_d[1] = width;

    // 壁の生成 (4フレームに1回、高密度な壁を形成)
    if (pSet->count % 2 == 0) {
        // 左壁の弾
        sEnemyShot* pLeft = new sEnemyShot;
        pLeft->x = center_x - width;
        pLeft->y = pSet->y;
        pLeft->muki = DX_PI / 2.0; // 下向き
        pLeft->speed = 3.5;
        pLeft->kind = img_enemyShotMediumOval[color]; // 中楕円弾(横向きに見えるように回転等はメイン側で対応、またはこれで壁として機能する)
        pLeft->param_d[0] = 1.0; // 壁フラグ
        pLeft->margin = 50.0;

        pLeft->prev = pSet->pEnemyShotHead->prev;
        pLeft->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pLeft;
        pSet->pEnemyShotHead->prev = pLeft;

        // 右壁の弾
        sEnemyShot* pRight = new sEnemyShot;
        pRight->x = center_x + width;
        pRight->y = pSet->y;
        pRight->muki = DX_PI / 2.0;
        pRight->speed = 3.5;
        pRight->kind = img_enemyShotMediumOval[color];
        pRight->param_d[0] = 1.0; // 壁フラグ
        pRight->margin = 50.0;

        pRight->prev = pSet->pEnemyShotHead->prev;
        pRight->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pRight;
        pSet->pEnemyShotHead->prev = pRight;

        // 生成時の効果音 (間引いて鳴らす)
        if (pSet->count % 60 == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }

    // 既存の弾の更新と漏電スパークの生成
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 壁の弾はY方向に移動
        pShot->y += pShot->speed;

        // 壁の弾の場合 (param_d[0] == 1.0)
        if (pShot->param_d[0] == 1.0 && pShot->kind == img_enemyShotMediumOval[3]) {
            // 漏電スパーク：一定確率で内側(中心)に向かって小玉を発射
            // GetRand(19) は 0~19 の 20 種類を返すので、==0 で 5% の確率になる
            if (GetRand(800) == 0) {
                sEnemyShot* pSpark = new sEnemyShot;

                // 左壁なら右向き(0)、右壁なら左向き(PI)
                double current_center = pSet->param_d[5];
                double spark_muki = (pShot->x < current_center) ? 0.0 : DX_PI;

                // 少しばらつきを持たせる (GetRand(40) は 0~40, -20 で -20~20)
                spark_muki += (GetRand(40) - 20) / 180.0 * DX_PI;

                pSpark->x = pShot->x;
                pSpark->y = pShot->y;
                pSpark->muki = spark_muki;
                pSpark->speed = 1.0 + GetRand(20) / 20.0; // 2.0 ~ 4.0
                pSpark->kind = img_enemyShotSmallBall[6]; // 白(6)
                pSpark->param_d[0] = 0.0; // スパークフラグ
                pSpark->margin = 50.0;

                pSpark->prev = pSet->pEnemyShotHead->prev;
                pSpark->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pSpark;
                pSet->pEnemyShotHead->prev = pSpark;
            }
        }
        else {
            // スパークの更新 (斜め移動)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン：イライラ棒モチーフ
void EnemyPat_Irairabou_Qwen()
{
    static int muki;
    static sEnemyShotSet* pTunnelSet = nullptr;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // やや長めの耐久でトンネルを生き延びる演出
        muki = 1;

        // トンネルセットの初期化
        pTunnelSet = new sEnemyShotSet;
        pTunnelSet->count = 0;
        pTunnelSet->patternFunc = ShotIrairaTunnel;
        pTunnelSet->x = enemy.x;
        pTunnelSet->y = 60.0; // 壁が生成される起点Y座標
        pTunnelSet->muki = DX_PI / 2.0;
        pTunnelSet->kind = 0;

        // パラメータ初期化
        pTunnelSet->param_d[0] = enemy.x; // base_x (ボスのX座標)
        pTunnelSet->param_d[1] = 120.0;   // width (トンネルの幅の半径)
        pTunnelSet->param_d[2] = 0.0;     // phase (うねりの位相)
        pTunnelSet->param_d[5] = enemy.x; // current_center_x

        pTunnelSet->pEnemyShotHead = new sEnemyShot;
        pTunnelSet->pEnemyShotHead->prev = pTunnelSet->pEnemyShotHead;
        pTunnelSet->pEnemyShotHead->next = pTunnelSet->pEnemyShotHead;

        pTunnelSet->prev = enemyShotSetHead.prev;
        pTunnelSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pTunnelSet;
        enemyShotSetHead.prev = pTunnelSet;

        // 予告音で緊張感を高める
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // ボスの移動 (左右に往復)
        enemy.x += 1.5 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }

        // トンネルセットの基準X座標をボスに追従させる (リアルタイム変形ギミック)
        if (pTunnelSet) {
            pTunnelSet->param_d[0] = enemy.x;
        }
    }
}