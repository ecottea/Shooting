// enemyPat_sierpinski.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ギャスケットの頂点座標を再帰的に計算して弾を配置するヘルパー関数
static void CreateSierpinskiGasket(sEnemyShotSet* pEnemyShotSet, double cx, double cy, double size, int depth, double base_angle, int color_base)
{
    // 深さが0になったら、その位置に弾を配置（最小単位の頂点）
    if (depth == 0) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = cx;
        pShot->y = cy;
        pShot->count = 0;

        // 結晶感を出すために菱形弾を使用。再帰のルートに応じて色を散らす
        pShot->kind = img_enemyShotDiamond[color_base % 6];

        // 弾自体のパラメータに、セットの中心からの「初期相対座標」を記憶させる
        pShot->param_d[0] = cx - pEnemyShotSet->x;
        pShot->param_d[1] = cy - pEnemyShotSet->y;

        pShot->margin = 480;

        // 双方向リストへ接続
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
        return;
    }

    // 次の世代のサイズは半分
    double next_size = size / 2.0;

    // 正三角形の3頂点方向（120度ずつ）に子となるギャスケットを配置
    for (int i = 0; i < 3; i++) {
        double angle = base_angle + i * (2.0 * DX_PI / 3.0);
        double nx = cx + cos(angle) * next_size;
        double ny = cy + sin(angle) * next_size;

        // 再帰呼び出し（深さを1減らす）
        CreateSierpinskiGasket(pEnemyShotSet, nx, ny, next_size, depth - 1, base_angle, color_base + i);
    }
}

// 弾幕：回転するシェルピンスキーのギャスケット
static void ShotSierpinski(sEnemyShotSet* pEnemyShotSet)
{
    // [1] 初期化：最初のフレームでギャスケットを一斉に生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 初期回転角度をランダムに設定 (0 ~ 359度)
        double start_angle = (GetRand(359) / 360.0) * 2.0 * DX_PI;
        int random_color = GetRand(8);

        // 外枠サイズ160、深さ4で生成（3の4乗 = 81発の弾で構成された綺麗なギャスケットになります）
        CreateSierpinskiGasket(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, 260.0, 4, start_angle, random_color);
    }

    // [2] 移動と変形処理
    // ショットセット（中心点）自体を自機方向へゆっくり前進させる
    double set_speed = 1.0;
    pEnemyShotSet->x += set_speed * cos(pEnemyShotSet->muki);
    pEnemyShotSet->y += set_speed * sin(pEnemyShotSet->muki);

    // 経過フレーム数から「現在の回転角」と「拡大率」を計算
    double rot_angle = pEnemyShotSet->count * 0.012;               // 毎フレーム約0.7度回転
    double scale = 0.2 + pEnemyShotSet->count * 0.006;            // 最初は小さく出現し、徐々に巨大化
    if (scale > 1.3) scale = 1.3;                                 // 最大サイズ制限

    // 全ての構成弾の座標を、形状を維持したまま再計算
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 記憶しておいた初期の相対座標を取得
        double rx = pShot->param_d[0];
        double ry = pShot->param_d[1];

        // 回転行列とスケールを適用
        double rotX = (rx * cos(rot_angle) - ry * sin(rot_angle)) * scale;
        double rotY = (rx * sin(rot_angle) + ry * cos(rot_angle)) * scale;

        // 移動するセットの中心点に足し合わせる
        pShot->x = pEnemyShotSet->x + rotX;
        pShot->y = pEnemyShotSet->y + rotY;

        // 弾のグラフィック向きも回転に合わせる
        pShot->muki = rot_angle;

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Sierpinski_Gemini()
{
    if (count == 1) {
        // 初期位置の設定
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 画面上部をゆったりと左右にホバー移動
        enemy.x = 240.0 + 80.0 * sin(count / 120.0 * DX_PI);
        enemy.y = 100.0 + 20.0 * cos(count / 60.0 * DX_PI);
    }

    // 180フレーム（約3秒）ごとに1セット、巨大なギャスケットを放つ
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSierpinski;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // 狙いを自機方向にする（この方向に図形全体が迫ります）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}