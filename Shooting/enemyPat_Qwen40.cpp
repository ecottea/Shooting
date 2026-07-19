#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 二重振り子弾幕パターン
// ============================================================
// 概要:
// 画面中央のボスから伸びる2本の仮想アーム（二重振り子）が物理演算に基づいて運動します。
// その先端（第2アーム）が描く軌跡（リサジュー図形）に沿って、速度ベクトル方向へ弾を射出します。
// 物理演算の特性上、同じ軌道を繰り返さないため、プレイヤーには「予測不能な虹色のリボン」のように見えます。
//
// 使用素材:
// - 弾: img_enemyShotSmallBall (色配列を使用し、時間経過で虹色に変化させて軌跡を可視化)
// - 音: sound_enemyShot_light (先端速度が速い＝遠心力が強い時のみ発音)
//
static void ShotDoublePendulum(sEnemyShotSet* pSet)
{
    // --- 物理定数 ---
    const double L1 = 110.0; // 第1アーム長さ
    const double L2 = 110.0; // 第2アーム長さ
    const double M1 = 1.0;   // 質量1
    const double M2 = 1.0;   // 質量2
    const double G = 0.6;    // 重力加速度 (ゲームバランス調整値)
    const double DT = 1.0;   // タイムステップ

    // --- 状態変数の取得 (param_dを使用) ---
    double t1 = pSet->param_d[0]; // 角度1 (鉛直下を0)
    double t2 = pSet->param_d[1]; // 角度2
    double w1 = pSet->param_d[2]; // 角速度1
    double w2 = pSet->param_d[3]; // 角速度2

    // --- 物理演算 (運動方程式) ---
    // 数値計算の安定化のため、わずかな減衰を入れる
//    w1 *= 0.998;
//    w2 *= 0.998;

    // ラグランジュの運動方程式から導出された角加速度
    double num1 = -G * (2 * M1 + M2) * sin(t1) - M2 * G * sin(t1 - 2 * t2)
        - 2 * sin(t1 - t2) * M2 * (w2 * w2 * L2 + w1 * w1 * L1 * cos(t1 - t2));
    double den1 = L1 * (2 * M1 + M2 - M2 * cos(2 * t1 - 2 * t2));
    double a1 = num1 / den1;

    double num2 = 2 * sin(t1 - t2) * (w1 * w1 * L1 * (M1 + M2) + G * (M1 + M2) * cos(t1)
        + w2 * w2 * L2 * M2 * cos(t1 - t2));
    double den2 = L2 * (2 * M1 + M2 - M2 * cos(2 * t1 - 2 * t2));
    double a2 = num2 / den2;

    // --- 積分 (シンプレクティックオイラー法) ---
    // エネルギー保存則の破綻（発散）を防ぐため、速度を先に更新する
    w1 += a1 * DT;
    w2 += a2 * DT;
    t1 += w1 * DT;
    t2 += w2 * DT;

    // --- 状態の保存 ---
    pSet->param_d[0] = t1;
    pSet->param_d[1] = t2;
    pSet->param_d[2] = w1;
    pSet->param_d[3] = w2;

    // --- ギミック: カオスの注入 ---
    // 一定時間ごとにエネルギーを注入し、運動をカオス状態に保つ
    if (pSet->count % 240 == 0) {
        w1 += (GetRand(100) - 50) / 20.0;
        w2 += (GetRand(100) - 50) / 20.0;
    }

    // --- 位置と速度の計算 ---
    // 支点 (pSet->x, pSet->y) からの相対座標を計算
    double x1 = pSet->x + L1 * sin(t1);
    double y1 = pSet->y + L1 * cos(t1);
    double x2 = x1 + L2 * sin(t2);
    double y2 = y1 + L2 * cos(t2);

    // 先端の速度ベクトル (vx, vy)
    // y = L*cos(t) の微分は -L*sin(t)*w となる点に注意
    double vx = L1 * w1 * cos(t1) + L2 * w2 * cos(t2);
    double vy = -L1 * w1 * sin(t1) - L2 * w2 * sin(t2);

    double speed_mag = sqrt(vx * vx + vy * vy);

    // --- 弾の生成 ---
    // 先端の速度ベクトル方向へ、速度の大きさに比例した弾速で弾を放つ
    // これにより、振り子が速く動く場所（遠心力が強い場所）ほど弾が遠くへ飛び、
    // 折り返し点では弾が足元に落ちるような挙動になる
    if (speed_mag > 0.5) { // 閾値未満では弾を出さない（ノイズ対策）
        sEnemyShot* pShot = new sEnemyShot;

        pShot->x = x2;
        pShot->y = y2;
        pShot->muki = atan2(vy, vx);

        // 弾速: 物理シミュレーションの速度をそのまま反映
        pShot->speed = speed_mag * 0.6;

        // 弾の種類: 小玉を使用
        // 色は時間経過(count)で変化させ、リサジュー図形が虹色の線として描画されるようにする
        pShot->kind = img_enemyShotSmallBall[(pSet->count / 2) % 8];

        // 当たり判定用のマージンはデフォルト(20.0)でOK

        // リンクリストへの登録
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;

        // 効果音: 速度が速い（エネルギーが高い）時のみ発音
        if (speed_mag > 5.0) {
            if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_DoublePendulum_Qwen()
{
    // ボスの移動制御用変数
    static int move_muki;

    if (count == 1) {
        // --- 初期化 ---
        enemy.x = 240.0; // 画面中央
        enemy.y = 80.0;  // 上寄り（振り子をぶら下げるため）
        enemy.maxHp = enemy.hp = 200;
        move_muki = 1;

        // --- 弾幕セットの生成 ---
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDoublePendulum;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0; // ボス本体より少し下を支点にする
        pSet->muki = 0.0;
        pSet->kind = 0;

        // 物理演算の初期状態 (カオスになりやすい角度を設定)
        pSet->param_d[0] = 3.14 / 2.0; // t1: 90度
        pSet->param_d[1] = -1.0;       // t2: ランダムな初期値
        pSet->param_d[2] = 0.0;        // w1
        pSet->param_d[3] = 0.0;        // w2

        // 弾リストの初期化
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // セットをリストに登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // --- ボスの移動 (左右にゆっくり移動し、支点を動かすことでさらにカオスを誘発) ---
        enemy.x += 0.5 * move_muki;
        if (enemy.x > 400 || enemy.x < 80) move_muki *= -1;

        // 支点の位置も更新する（重要: 物理演算の原点が動く）
        // ※本来は運動方程式に慣性力が加わるが、ゲーム的には「支点が勝手に動く」として処理する
        sEnemyShotSet* pSet = enemyShotSetHead.prev; // リストの末尾（今回生成したセット）を取得
        if (pSet != &enemyShotSetHead) {
            pSet->x = enemy.x;
            pSet->y = enemy.y + 20.0;
        }
    }
}