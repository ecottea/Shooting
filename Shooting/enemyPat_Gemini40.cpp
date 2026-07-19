// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#ifndef DX_PI
#define DX_PI 3.14159265358979323846
#endif

// 二重振り子弾幕の更新および弾の制御ルーチン
static void ShotChaosPendulum(sEnemyShotSet* pEnemyShotSet)
{
    // =========================================================================
    // param_d および param_i の用途定義
    // [param_d[0]]: 第1関節の角度 theta1
    // [param_d[1]]: 第1関節の角速度 omega1
    // [param_d[2]]: 第2関節の角度 theta2
    // [param_d[3]]: 第2関節の角速度 omega2
    // [param_i[0]]: 射撃のカラーインデックスや内部タイマー用
    // =========================================================================

    // --- 1. 二重振り子の擬似物理シミュレーション (毎フレーム更新) ---
    // ゲームのフレームカウントなどを利用し、カオスな複数周期を合成
    double t = (double)(pEnemyShotSet->count);

    // 第1アーム：大きなゆったりとした揺れに、細かい高調波をブレンドして不規則性を出す
    double target_omega1 = 0.04 * sin(t * 0.015) + 0.02 * cos(t * 0.037);
    pEnemyShotSet->param_d[1] = pEnemyShotSet->param_d[1] * 0.95 + target_omega1 * 0.05; // 慣性補間
    pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[1];

    // 第2アーム：第1アームの動きに強く影響を受けつつ、独自の鋭い振り子運動を行う（カオス源）
    // 互いの位相が噛み合った瞬間に一気に一回転するようなトルクをシミュレート
    double gravity_effect = 0.05 * sin(pEnemyShotSet->param_d[0] - pEnemyShotSet->param_d[2]);
    double target_omega2 = 0.08 * sin(t * 0.023) + 0.05 * cos(t * 0.051 + 1.5) + gravity_effect;
    pEnemyShotSet->param_d[3] = pEnemyShotSet->param_d[3] * 0.93 + target_omega2 * 0.07;
    pEnemyShotSet->param_d[2] += pEnemyShotSet->param_d[3];

    // 各アームの長さ（ドット単位）
    double L1 = 90.0;
    double L2 = 80.0;

    // 関節（第1アーム先端）の座標計算
    double jointX = pEnemyShotSet->x + L1 * sin(pEnemyShotSet->param_d[0]);
    double jointY = pEnemyShotSet->y + L1 * cos(pEnemyShotSet->param_d[0]);

    // 最先端（第2アーム先端）の座標計算
    double tipX = jointX + L2 * sin(pEnemyShotSet->param_d[2]);
    double tipY = jointY + L2 * cos(pEnemyShotSet->param_d[2]);


    // --- 2. 振り子アーム（実体）の配置 (count == 0 の初期化時または特定の周期でアームを視覚化) ---
    // ※今回はアーム自体が常に動いて見えるよう、3フレームに1回アーム構成弾を「その場に静止する弾」として配置します。
    // メイン側で自動移動されるため、速度0の弾としてその瞬間のアームの形に配置し、すぐ消去されるか流れるようにします。
    if (pEnemyShotSet->count % 3 == 0) {
        // 素材選定：アームは形が見えやすいように「青色の小玉 (img_enemyShotSmallBall[4])」を使用
        int armColorIdx = 4; // 4: 青

        // 第1アーム上の配置 (等間隔に7個)
        for (int i = 1; i <= 7; i++) {
            double ratio = (double)i / 7.0;
            sEnemyShot* pNewShot = new sEnemyShot;
            pNewShot->x = pEnemyShotSet->x + (jointX - pEnemyShotSet->x) * ratio;
            pNewShot->y = pEnemyShotSet->y + (jointY - pEnemyShotSet->y) * ratio;
            pNewShot->muki = 0.0;
            pNewShot->speed = 0.0; // 静止弾（その場に置き去られ、プレイヤーへの障害物となる）
            pNewShot->kind = img_enemyShotSmallBall[armColorIdx];

            // リストへ挿入
            pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNewShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
            pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
        }

        // 第2アーム上の配置 (等間隔に6個)
        for (int i = 1; i <= 6; i++) {
            double ratio = (double)i / 6.0;
            sEnemyShot* pNewShot = new sEnemyShot;
            pNewShot->x = jointX + (tipX - jointX) * ratio;
            pNewShot->y = jointY + (tipY - jointY) * ratio;
            pNewShot->muki = 0.0;
            pNewShot->speed = 0.0;
            pNewShot->kind = img_enemyShotSmallBall[armColorIdx];

            // リストへ挿入
            pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNewShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
            pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
        }
    }


    // --- 3. カオス回転による軌跡の物質化・弾の放出 ---
    // 振り子の先端が激しく動いたとき（角速度が一定以上のとき）、または一定の周期で、
    // 先端(tip)と関節(joint)から、外周へ向けて鋭い「マゼンタ色の菱形弾 (img_enemyShotDiamond[5])」を解き放ちます。
    bool isFastRotation = (fabs(pEnemyShotSet->param_d[3]) > 0.06);

    // 5フレームに1回、または高速回転中に弾を発生
    if ((pEnemyShotSet->count % 5 == 0) || isFastRotation) {

        // 高速回転中の激しい放出のときのみ、効果音を鳴らす
        if (isFastRotation && (pEnemyShotSet->count % 5 == 0)) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 素材選定：危険度が高くスタイリッシュに見える「マゼンタ色の菱形弾」を使用
        int bulletColorIdx = 5; // 5: マゼンタ

        // 先端から放出される弾 (2つずつ、外側へ慣性＋ランダムな方向へ飛ぶ)
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pNewShot = new sEnemyShot;
            pNewShot->x = tipX;
            pNewShot->y = tipY;

            // 振り子の先端が動いている方向（接線方向）に、GetRandを使った揺らぎを加える
            // GetRand(360) - 180 で -180度〜+180度の範囲をラジアンに変換
            double angleOffset = ((double)(GetRand(60)) - 30.0) / 180.0 * DX_PI;
            pNewShot->muki = pEnemyShotSet->param_d[2] + DX_PI / 2.0 + angleOffset;

            // 速度も振り子の角速度に比例させつつランダム要素を付与
            pNewShot->speed = 1.5 + (fabs(pEnemyShotSet->param_d[3]) * 15.0) + ((double)GetRand(100) / 100.0);
            pNewShot->kind = img_enemyShotDiamond[bulletColorIdx];

            // リストへ挿入
            pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNewShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
            pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
        }

        // 関節部分からも少し遅い弾をポロポロと落とす（内側へ飛び込もうとするプレイヤーへの牽制）
        if (pEnemyShotSet->count % 10 == 0) {
            sEnemyShot* pNewShot = new sEnemyShot;
            pNewShot->x = jointX;
            pNewShot->y = jointY;
            // 全方位ランダムな向き
            pNewShot->muki = ((double)GetRand(360) / 180.0) * DX_PI;
            pNewShot->speed = 1.0 + ((double)GetRand(50) / 100.0);
            pNewShot->kind = img_enemyShotDiamond[bulletColorIdx];

            // リストへ挿入
            pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNewShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
            pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
        }
    }


    // --- 4. 既存の弾の移動処理 ---
    // すでに発射されてリストに存在する弾をそれぞれの向き・速度に従って前進させる
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // speed が 0 の弾（アームを構成する弾）はその場に留まり、スピードがある弾は飛んでいく
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        if (pShot->count >= 3 && pShot->kind == img_enemyShotSmallBall[4]) pShot->x = 9999;

        pShot = pShot->next;
    }
}

// 敵本体のパターン関数（二重振り子弾幕フェーズ）
void EnemyPat_DoublePendulum_Gemini()
{
    // 初期化フェーズ (敵が出現した最初のフレーム)
    if (count == 1) {
        // ゲーム画面中央上部に配置
        enemy.x = 240.0;
        enemy.y = 120.0; // 振り子が画面内に綺麗に収まりやすいよう、少し下げて配置
        enemy.maxHp = enemy.hp = 200; // 200固定仕様

        // 弾幕データセット（Set）の構築
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotChaosPendulum;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 二重振り子の初期角度・角速度のセットアップ (ランダムさを持たせる)
        // GetRand(100) を使って初期角度を微小に変える
        pEnemyShotSet->param_d[0] = DX_PI + ((double)GetRand(40) - 20.0) / 180.0 * DX_PI; // theta1 (ほぼ下向きスタート)
        pEnemyShotSet->param_d[1] = 0.0;                                                 // omega1
        pEnemyShotSet->param_d[2] = DX_PI + ((double)GetRand(80) - 40.0) / 180.0 * DX_PI; // theta2
        pEnemyShotSet->param_d[3] = 0.0;                                                 // omega2

        // 双方向リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体管理リストへ接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵本体は激しく動かず、ボスの威厳を持って中央上部で微小にホバー移動
        // 振り子の根本（x, y）が少し揺れることで、カオス挙動にさらなる複雑なブレを与える
        enemy.x = 240.0 + 15.0 * sin((double)count * 0.02);
        enemy.y = 120.0 + 5.0 * cos((double)count * 0.03);

        // 稼働中の弾幕セット側の基準座標も、敵本体の座標に毎フレーム追従させる
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotChaosPendulum) {
                pSet->x = enemy.x;
                pSet->y = enemy.y;
            }
            pSet = pSet->next;
        }
    }
}