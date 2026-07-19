// 金閣寺の一枚天井.cpp
// 射命丸文のスペルカード：赤い小玉で回転する正方形の檻を作り、
// 四隅から定期的に自機狙い弾を発射し、本体が高速突進を繰り返す。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------- 補助関数：正方形の外周に弾を1つ追加 ----------
static void AddSquareBullet(sEnemyShotSet* pSet, double rx, double ry)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = 0.0;          // 位置は毎フレーム SquareCeiling で更新
    pShot->y = 0.0;
    pShot->muki = 0.0;
    pShot->speed = 0.0;          // 自動移動させず、手動で座標を制御
    pShot->kind = img_enemyShotSmallBall[0];   // 赤色小玉
    pShot->param_d[0] = rx;        // 正方形中心からの相対 X 座標
    pShot->param_d[1] = ry;        // 正方形中心からの相対 Y 座標
    pShot->margin = 200;

    // 弾幕セットのリンクリストに挿入
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ---------- 弾幕関数：正方形の檻（修正版）---------- //※バグっていたので直させた
static void SquareCeiling(sEnemyShotSet* pSet)
{
    const double HALF_SIDE = 110.0;
    const double STEP = 13.0;
    const double ANG_VEL = 0.018;

    if (pSet->count == 0) {
        // 初回生成（変更なし）
        for (double x = -HALF_SIDE; x <= HALF_SIDE; x += STEP)
            AddSquareBullet(pSet, x, -HALF_SIDE);
        for (double x = -HALF_SIDE; x <= HALF_SIDE; x += STEP)
            AddSquareBullet(pSet, x, HALF_SIDE);
        for (double y = -HALF_SIDE + STEP; y <= HALF_SIDE - STEP; y += STEP)
            AddSquareBullet(pSet, -HALF_SIDE, y);
        for (double y = -HALF_SIDE + STEP; y <= HALF_SIDE - STEP; y += STEP)
            AddSquareBullet(pSet, HALF_SIDE, y);

        pSet->param_d[0] = 0.0;
        pSet->param_d[1] = ANG_VEL;
        pSet->param_i[0] = 0;
    }
    else {
        double angle = pSet->param_d[0];
        double cx = pSet->x;
        double cy = pSet->y;

        // ★修正1：枠弾だけを回転させる（speed == 0 の弾が枠弾）
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->speed == 0.0) { // 枠弾の判定
                double rx = pShot->param_d[0];
                double ry = pShot->param_d[1];
                pShot->x = cx + rx * cos(angle) - ry * sin(angle);
                pShot->y = cy + rx * sin(angle) + ry * cos(angle);
            }
            pShot = pShot->next;
        }

        // ★修正2：枠弾以外の全ての弾（四隅弾など）を自力で移動させる
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->speed != 0.0) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }

        pSet->param_d[0] += pSet->param_d[1];

        // 四隅からの発射処理（変更なし）
        pSet->param_i[0]++;
        if (pSet->param_i[0] >= 80) { // ※ナーフ
            pSet->param_i[0] = 0;

            double corners[4][2] = {
                {-HALF_SIDE, -HALF_SIDE},
                { HALF_SIDE, -HALF_SIDE},
                {-HALF_SIDE,  HALF_SIDE},
                { HALF_SIDE,  HALF_SIDE}
            };

            for (int i = 0; i < 4; i++) {
                double wx = cx + corners[i][0] * cos(angle) - corners[i][1] * sin(angle);
                double wy = cy + corners[i][0] * sin(angle) + corners[i][1] * cos(angle);

                for (int j = 0; j < 4; j++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = wx;
                    pNew->y = wy;
                    double baseDir = atan2(player.y - wy, player.x - wx);
                    double spread = (GetRand(60) - 30) / 180.0 * DX_PI;
                    pNew->muki = baseDir + spread;
                    pNew->speed = (180 + GetRand(120)) / 100.0;
                    pNew->kind = img_enemyShotSmallBall[5]; // マゼンタ小玉

                    // リンクリストに追加
                    pNew->prev = pSet->pEnemyShotHead->prev;
                    pNew->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pNew;
                    pSet->pEnemyShotHead->prev = pNew;
                }
            }
        }
    }
}

// ========== 敵本体 ==========
void EnemyPat_Kinkakuji_DeepSeek()
{
    // 静的変数：状態、タイマー、正方形弾幕セットへのポインタ
    static int state = 0;       // 0:移動中 1:スペル展開 2:高速突進
    static int timer = 0;
    static sEnemyShotSet* pSquareSet = nullptr;

    // --- 初回初期化 ---
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        state = 0;
        timer = 0;
        pSquareSet = nullptr;
        return;
    }

    // --- 正方形が生成されていれば、敵本体に追従させる ---
    if (pSquareSet != nullptr) {
        pSquareSet->x = enemy.x;
        pSquareSet->y = enemy.y;
    }

    // --- 状態遷移 ---
    switch (state) {
    case 0: // 画面中央やや下へ移動
    {
        double targetX = 240.0;
        double targetY = 310.0;
        double dx = targetX - enemy.x;
        double dy = targetY - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < 1.5) {
            enemy.x = targetX;
            enemy.y = targetY;
            state = 1;
            timer = 0;
        }
        else {
            double speed = 2.5;
            enemy.x += speed * dx / dist;
            enemy.y += speed * dy / dist;
        }
    }
    break;

    case 1: // スペル展開中
    {
        // 正方形弾幕セットが未作成ならここで生成
        if (pSquareSet == nullptr) {
            // スペル宣言用の重めの効果音を再生
            if (CheckSoundMem(sound_enemyShot_heavy) == 1)
                StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            pSquareSet = new sEnemyShotSet;
            pSquareSet->count = 0;
            pSquareSet->patternFunc = SquareCeiling;
            pSquareSet->x = enemy.x;
            pSquareSet->y = enemy.y;
            pSquareSet->muki = 0.0;
            pSquareSet->kind = 0;

            // ダミーヘッダの作成
            pSquareSet->pEnemyShotHead = new sEnemyShot;
            pSquareSet->pEnemyShotHead->prev = pSquareSet->pEnemyShotHead;
            pSquareSet->pEnemyShotHead->next = pSquareSet->pEnemyShotHead;

            // 全体の弾幕セットリストに追加
            pSquareSet->prev = enemyShotSetHead.prev;
            pSquareSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSquareSet;
            enemyShotSetHead.prev = pSquareSet;
        }

        // 本体は中央付近をゆっくり左右に漂う
        enemy.x += 0.7 * cos(count * 0.03);

        // 約5秒（300フレーム）後に突進状態へ
        timer++;
        if (timer == 240) PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // ※追加
        if (timer > 300) {
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK); // ※追加
            state = 2;
            timer = 0;
        }
    }
    break;

    case 2: // 高速突進（プレイヤーめがけて）
    {
        double dx = player.x - enemy.x;
        double dy = player.y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);
        double speed = 5.5;

        if (dist < 130.0) { // ※ナーフ
            // プレイヤー近傍に到達したら展開状態に戻る
            state = 1;
            timer = 0;
        }
        else {
            enemy.x += speed * dx / dist;
            enemy.y += speed * dy / dist;
        }

        // 画面外に出過ぎないように軽く制限
        if (enemy.x < 30)  enemy.x = 30;
        if (enemy.x > 450) enemy.x = 450;
        if (enemy.y < 30)  enemy.y = 30;
        if (enemy.y > 450) enemy.y = 450;
    }
    break;
    }
}