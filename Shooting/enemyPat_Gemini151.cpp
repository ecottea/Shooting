#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：サブタレイニアンローズ
static void ShotSubterraneanRose(sEnemyShotSet* pEnemyShotSet)
{
    // 弾の発射（初回のみ）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int N = 60; // 60 way
        // 全方位弾の基準角を毎回ランダムに決める
        double base_angle = (GetRand(3600) / 3600.0) * 2.0 * DX_PI;
        double V = pEnemyShotSet->param_d[0];
        int color = pEnemyShotSet->param_i[0]; // 0: 青, 1: 赤

        for (int i = 0; i < N; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_angle + (2.0 * DX_PI * i) / N;
            pEnemyShot->speed = V;

            // 初期の弾画像設定
            if (color == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青中玉
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤中玉
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの移動と大玉・中玉の判定
    int color = pEnemyShotSet->param_i[0];
    double A = pEnemyShotSet->param_d[1];
    double B = pEnemyShotSet->param_d[2];

    // 現在の count に基づく偏角の範囲を計算
    double start_angle, end_angle;
    if (color == 0) {
        // 青：(count*A) から (count*A+B) の範囲
        start_angle = ::count * A;
        end_angle = ::count * A + B;
    }
    else {
        // 赤：-(count*A) から -(count*A+B) の範囲
        // 判定しやすいように小さい方を start_angle とする
        start_angle = -(::count * A) - B;
        end_angle = -(::count * A);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 弾の移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 中心（ボス座標）から見た偏角を計算
        double angle = atan2(pShot->y - enemy.y, pShot->x - enemy.x);

        // 判定用の角度 a を start_angle 以上 start_angle + 2PI 未満に正規化する
        double a = angle;
        while (a < start_angle) a += 2.0 * DX_PI;
        while (a >= start_angle + 2.0 * DX_PI) a -= 2.0 * DX_PI;

        // 範囲内にいるかチェック
        if (a <= end_angle) {
            // 範囲内なら大玉
            if (color == 0) pShot->kind = img_enemyShotLargeBall[4]; // 青大玉
            else            pShot->kind = img_enemyShotLargeBall[0]; // 赤大玉
        }
        else {
            // 範囲外なら中玉
            if (color == 0) pShot->kind = img_enemyShotMediumBall[4]; // 青中玉
            else            pShot->kind = img_enemyShotMediumBall[0]; // 赤中玉
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SubterraneanRose_Gemini()
{
    static int phase;
    static int wait_timer;
    static int phase_threshold;
    static int color_flag; // 0: 青, 1: 赤
    static int local_timer;

    // 初期化
    if (::count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0; // 画面中央上部で固定
        enemy.maxHp = enemy.hp = 200; // 400から変更しないこと

        phase = 0;
        wait_timer = 0;
        phase_threshold = 150; // HP 1/4 (100) 削れるごとの閾値
        color_flag = 0;
        local_timer = 0;
    }

    // HPが1/4削れる度にフェーズを移行し、一時的に発射を止める
    if (phase < 3 && enemy.hp <= phase_threshold) {
        phase++;
        phase_threshold -= 50;
        wait_timer = 60; // 60フレーム（約1秒）だけ弾を撃つのを休む
    }

    // フェーズごとのパラメータ T, V, A, B
    int T;
    double V, A, B;

    switch (phase) {
    case 0: T = 35; V = 1.6; A = 0.015; B = DX_PI / 3.0; break; // 第1段階
    case 1: T = 30; V = 1.9; A = 0.020; B = DX_PI / 2.5; break; // 第2段階
    case 2: T = 25; V = 2.2; A = 0.025; B = DX_PI / 2.0; break; // 第3段階
    case 3: T = 20; V = 2.5; A = 0.030; B = DX_PI / 1.5; break; // 第4段階 (発狂)
    default:T = 20; V = 2.5; A = 0.030; B = DX_PI / 1.5; break;
    }

    if (wait_timer > 0) {
        wait_timer--;
    }
    else {
        // Tフレーム間隔で発射
        if (local_timer % T == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSubterraneanRose;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;

            // sEnemyShotSet の汎用パラメータに変数を渡す
            pEnemyShotSet->param_i[0] = color_flag;
            pEnemyShotSet->param_d[0] = V;
            pEnemyShotSet->param_d[1] = A;
            pEnemyShotSet->param_d[2] = B;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            color_flag = 1 - color_flag; // 次回は違う色(青と赤を交互に)
        }
        local_timer++;
    }
}