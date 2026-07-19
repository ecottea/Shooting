// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：反転螺旋の往復弾
// ============================================================
static void ShotReverseSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 設定値
    const int PHASE_OUT_FRAMES = 60;  // 行き（生成）フェーズの期間
    const double SPIRAL_SPEED_OUT = 0.08; // 行き：角速度
    const double RADIAL_SPEED_OUT = 2.5;  // 行き：半径増加速度
    const double SPIRAL_SPEED_IN = -0.12; // 帰り：角速度（逆向き）
    const double RADIAL_SPEED_IN = -4.0;  // 帰り：半径減少速度（速い）

    // --------------------------------------------------------
    // 1. 弾の生成フェーズ (行き)
    // --------------------------------------------------------
    if (pEnemyShotSet->count < PHASE_OUT_FRAMES) {
        // 開始時に効果音
        if (pEnemyShotSet->count == 0) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 毎フレーム1発ずつ生成して螺旋を形成
        pEnemyShot = new sEnemyShot;

        // 画像と色の設定
        // pEnemyShotSet->kind をオフセットにして、セットごとに色の出だしを変える
        // GetRand(8) は 0～8 を返す
        int color_idx = (pEnemyShotSet->kind) % 9;
        pEnemyShot->kind = img_enemyShotMediumBall[color_idx];

        // パラメータ初期化 (極座標管理用)
        pEnemyShot->param_i[0] = 0; // Phase: 0=Out, 1=In
        pEnemyShot->param_d[0] = pEnemyShotSet->x; // 中心X (生成時のボス位置で固定)
        pEnemyShot->param_d[1] = pEnemyShotSet->y; // 中心Y
        pEnemyShot->param_d[2] = pEnemyShotSet->muki + pEnemyShotSet->count * 0.2; // 初期角度 (時間ずらし)
        pEnemyShot->param_d[3] = 20.0; // 初期半径 (ボスにめり込まないよう少し離す)

        // 初期位置決定 (描画用)
        pEnemyShot->x = pEnemyShot->param_d[0] + pEnemyShot->param_d[3] * cos(pEnemyShot->param_d[2]);
        pEnemyShot->y = pEnemyShot->param_d[1] + pEnemyShot->param_d[3] * sin(pEnemyShot->param_d[2]);
        pEnemyShot->muki = pEnemyShot->param_d[2] + DX_PI / 2.0; // 接線方向を向く

        pEnemyShot->margin = 480;

        // リストに登録
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // --------------------------------------------------------
    // 2. 折り返し判定とエフェクト
    // --------------------------------------------------------
    // 指定フレーム数経過後、一度だけ全弾のフェーズを「帰り」へ変更
    if (pEnemyShotSet->count == PHASE_OUT_FRAMES) {
        // 反転用の重たい効果音
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 配下の全弾を走査してフェーズ変更
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            pEnemyShot->param_i[0] = 1; // Phase: In
            pEnemyShot = pEnemyShot->next;
        }
    }

    // --------------------------------------------------------
    // 3. 弾の移動処理 (毎フレーム)
    // --------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double cx = pEnemyShot->param_d[0];
        double cy = pEnemyShot->param_d[1];
        double angle = pEnemyShot->param_d[2];
        double radius = pEnemyShot->param_d[3];

        if (pEnemyShot->param_i[0] == 0) {
            // 【行き】時計回りに回転しながら外へ広がる
            angle += SPIRAL_SPEED_OUT;
            radius += RADIAL_SPEED_OUT;
            pEnemyShot->muki = angle + DX_PI / 2.0; // 進行方向（接線）
        }
        else {
            // 【帰り】反時計回りに回転しながら中心へ縮む
            angle += SPIRAL_SPEED_IN; // 負の値なので減る
            radius += RADIAL_SPEED_IN; // 負の値なので減る
            pEnemyShot->muki = angle - DX_PI / 2.0; // 逆向きの接線
        }

        // 極座標からデカルト座標へ変換
        pEnemyShot->x = cx + radius * cos(angle);
        pEnemyShot->y = cy + radius * sin(angle);

        // パラメータ更新
        pEnemyShot->param_d[2] = angle;
        pEnemyShot->param_d[3] = radius;

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Reverse_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期位置・状態
        enemy.x = 240.0;
        enemy.y = 60.0; // 少し下に配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の移動 (左右移動)
        enemy.x += 1.2 * (double)muki;
        if (count % 150 == 75) muki *= -1; // 画面端で反転
    }

    // 弾幕生成 (120フレームごと)
    if (count % 120 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotReverseSpiral;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // 螺旋の基準角度 (プレイヤー狙い、あるいは真下など)
        // ここでは少しずらしながら撃って網目を複雑にする
        pEnemyShotSet->muki = DX_PI / 2.0 + (shot_count % 5) * 0.3;

        // kind を色シードとして利用
        pEnemyShotSet->kind = shot_count++;

        // ダミーのヘッドノード作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}